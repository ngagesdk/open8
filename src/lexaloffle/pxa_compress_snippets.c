/*
	
	pxa compression snippets for PICO-8 cartridge format (as of 0.2.4c)

	author: joseph@lexaloffle.com

	Copyright (c) 2020-22  Lexaloffle Games LLP

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

*/

#include "pico8.h"



// 3 3 5 4  (gives balanced trees for typical data)

#define PXA_MIN_BLOCK_LEN 3
#define BLOCK_LEN_CHAIN_BITS 3
#define BLOCK_DIST_BITS 5
#define TINY_LITERAL_BITS 4


// has to be 3 (optimized in find_repeatable_block)
#define MIN_BLOCK_LEN 3
#define HASH_MAX 4096
#define MINI_HASH(pp, i) ((pp[i+0]*7 + pp[i+1]*1503 + pp[i+2]*51717) & (HASH_MAX-1))

typedef unsigned short int uint16;
typedef unsigned char uint8;

static uint16 *hash_list[HASH_MAX];
static uint16 *hash_heap = NULL;
static int found[HASH_MAX];

#define WRITE_VAL(x) {*p_8 = (x); p_8++;}


static int bit = 1;
static int byte = 0;
static int dest_pos = 0;
static int src_pos = 0;


//-------------------------------------------------
// pxa bit-level read/write help functions
//-------------------------------------------------

static uint8 *dest_buf = NULL;
static uint8 *src_buf = NULL;

// 0.2.0j
// encode / decode as an int
static int get_write_pos()
{
	int result = (dest_pos << 16) | (byte << 8) | bit;
	return result;
}
static void set_write_pos(int val)
{
	bit = val & 0xff;
	byte = (val >> 8) & 0xff;
	dest_pos = (val >> 16) & 0x7fff;
}



static int getbit()
{
	int ret;
	
	ret = (src_buf[src_pos] & bit) ? 1 : 0;
	bit <<= 1;
	if (bit == 256)
	{
		bit = 1;
		src_pos ++;
	}
	return ret;
}

void putbit(int bval)
{
	dest_buf[dest_pos] &= ~bit; // 0.2.0j: per-bit
	if (bval) dest_buf[dest_pos] |= bit;

	bit <<= 1;

	if (bit == 256)
	{
		bit = 1;
		dest_pos ++;
		byte = dest_buf[dest_pos]; // 0.2.0j: so that don't clobber existing bits (can overwrite at bit level)
	}
}

static int getval(int bits)
{
	int i;
	int val = 0;
	if (bits == 0) return 0;

	for (i = 0; i < bits; i++)
		if (getbit())
			val |= (1 << i);

	return val;
}


static int putval(int val, int bits)
{
	int i;
	if (bits <= 0) return 0;

	for (i = 0; i < bits; i++)
		putbit(val & (1 << i));

	return bits;
}

static void putbitlen(int val)
{
	int i;
	for (i = 0; i < val-1; i++)
		putbit(0);
	putbit(1);
}


static int putchain(int val, int link_bits, int max_bits)
{
	int i;
	int max_link_val = (1 << link_bits) - 1; // 3 bits means can write < 7 in a single link
	int bits_written = 0;
	int vv = max_link_val;

	while (vv == max_link_val)
	{
		vv = MIN(val, max_link_val);
		bits_written += putval(vv, link_bits);
		val -= vv;

		if (bits_written >= max_bits) return bits_written; // next val is implicitly 0
	}
	return bits_written;
}

static int getchain(int link_bits, int max_bits)
{
	int i;
	int max_link_val = (1 << link_bits) - 1;
	int val = 0;	
	int vv = max_link_val;
	int bits_read = 0;

	while (vv == max_link_val)
	{
		vv = getval(link_bits);
		bits_read += link_bits;
		val += vv;
		if (bits_read >= max_bits) return val; // next val is implicitly 0
	}
	
	return val;
}


/*
	// used for block distance. reasonably even distribution of values, but more frequently closer.

	// calc number of bits; write that first (steps of 2)
	// then write val
*/
static int putnum(int val)
{
	int jump = BLOCK_DIST_BITS;
	int bits = jump;
	int i;

	while ((1<<bits) <= val)
		bits += jump;
	// printf("writing num bitlen: %d  at %d %d\n", bits, dest_pos, bit);

	// 1  15 bits // more frequent so put first
	// 01 10 bits
	// 00  5 bits
	putchain(3-(bits/jump), 1, 2);

	putval(val, bits);
	return (bits/jump)+bits;
}

static int getnum()
{
	int jump = BLOCK_DIST_BITS;
	int bits = jump;
	int src_pos_0 = src_pos;
	int bit_0 = bit;
	int val;

	// 1  15 bits // more frequent so put first
	// 01 10 bits
	// 00  5 bits
	bits = (3 - getchain(1, 2)) * BLOCK_DIST_BITS;

	val = getval(bits);

	if (val == 0 && bits == 10)
		return -1; // raw block marker

	return val;
}

// ---------------------


#define PXA_WRITE_VAL(x) {literal_bits_written += putval(x,8);}
#define PXA_READ_VAL(x)  getval(8)
static int pxa_find_repeatable_block(uint8 *dat, int pos, int data_len, int *block_offset, int *score_out)
{
	int max_hist_len = 32767; // 15 bits -- super-dense carts are shorter
	int i, j;
	int best_len = 0;
	int best_pos0 = -100000;
	int max_len = data_len - pos;
	char *p;
	int skip;
	int hash;
	int last_pos;
	int score, dist, bit_cost, best_score = -1;
	int list_pos;

	p = &dat[pos];

	// block length can't be longer than remaining
	
	if (max_len < PXA_MIN_BLOCK_LEN) return 0;
	if (max_hist_len < PXA_MIN_BLOCK_LEN) return 0;
	
	hash = MINI_HASH(dat, pos);
	last_pos = found[hash]; // most recently found match. to do: could just calculate hash ranges at start. hash_first[] hash_last[].

	uint16 *list = hash_list[hash];

/*	
	for (list_pos = 0; 
			list && list_pos < list[1] && // for each item of list
			(list[2+list_pos] < pos 
				&& list[2+list_pos] >= pos - max_hist_len // where starting position in within range   0.2.0e: commented. **wroooong**. don't want to exit iter here.
			);
			list_pos++)
*/

	if (!list) return 0; // 0.2.0e: exit early
	for (list_pos = 0; list_pos < list[1] && list[2+list_pos] < pos; list_pos++) // 0.2.0e: can exit early if encounter future position (rest of list will also be)
	if (list[2+list_pos] >= pos - max_hist_len) // not out of range   0.2.0e: moved here -- still want to try rest of list
	{
		int pos0 = list[2 + list_pos];

		// test starting from pos0 + 0
		i = 0;

		// matches in history
		while (i < max_len && (pos0+i) < pos && dat[pos0 + i] == dat[pos + i])
			i ++;

		// matches in output of this repeated block
		while (i < max_len && (pos0+i) >= pos && dat[pos0 + (i % (pos-pos0))] == dat[pos + i])
			i ++;

		// distance cost

		{
			dist = pos - pos0; // distance

			bit_cost = 0;
			while (dist > 0){
				bit_cost ++;
				dist >>= BLOCK_DIST_BITS; // 5-bit steps
			}
			bit_cost = MIN(bit_cost,2) + bit_cost * BLOCK_DIST_BITS;   // bits to write len.bitlen   ends up being 6, 12, 17

			//printf("dist %d cost: %d\n", pos - pos0, bit_cost);

			// block length cost: number of chain links * chain bits
			// commented; don't need! (and expensive to calculate) always worth taking a block with larger number of bit chain nodes  
			// bit_cost += (1 + (i-PXA_MIN_BLOCK_LEN) / ((1 << BLOCK_LEN_CHAIN_BITS)-1)) * BLOCK_LEN_CHAIN_BITS;
			bit_cost += 3;

			bit_cost += 1; // is_block marker
		}


		score = i * 256 / bit_cost; // number of characters written / cost

		if (score > best_score)
		{
			best_score = score;
			best_pos0 = pos0;
			best_len = i;
		}
	
	}
	
	//printf("@@ pos: %d offset: %d len: %d\n", pos, (pos - best_pos0), best_len);

	if (best_pos0 >= 0)
		*block_offset = (pos - best_pos0);
	else
		*block_offset = 0;

	*score_out = best_score;
 
	return best_len;
}


// debug stats
static int block_bits_written = 0;
static int literal_bits_written = 0;
static int total_block_len = 0;
static int num_blocks, num_blocks_large, num_literals;


static void init_literals_state(int *literal, int *literal_pos)
{
	int i;

	// starting state makes little difference
	// using 255-i (which seems terrible) only costs 10 bytes more.

	for (i = 0; i < 256; i++)
		literal[i] = i;

	for (i = 0; i < 256; i++)
		literal_pos[literal[i]] = i;
}


// pxa_build_hash_lookup: lists of occurances of hashes
// maybe better to just do 2 passes (calculate lengths on first pass) but this works fine.
// re-allocate lists into a fixed pool as they grow
void pxa_build_hash_lookup(uint8 *in, int len)
{
	int i;
	int hash;
	uint16 *list;
	uint16 *new_list;

	// printf("building hash lookup\n");

	memset(hash_list, 0, sizeof(hash_list));

/*
	512k to build lookup:
		worst case is evenly allocated lists (most list overhead)
		-> list len 10 (16 allocated) * 8192 = ~80,000 to house 64k position indexes
		-> allocated at 4,8,16, so 8192 * 3 overhead + 8192 * 28 =   ~ 8192 * 32 uint16's = 262144
			// maximum oberved is white_ale_in_benin: 125478, so agrees.
*/

	int heap_size = 262144 * sizeof(uint16);

	// max hash size: 
	if (!hash_heap)
		hash_heap = malloc(heap_size);
	memset(hash_heap, 0, heap_size);

	int heap_pos = 0;
	
	for (i = 0; i < len-2; i++)
	{
		hash = MINI_HASH(in, i);

		list = hash_list[hash];

		// new list		
		if (!list){
			// printf("new list at %d\n", heap_pos);
			hash_list[hash] = &hash_heap[heap_pos];
			list = hash_list[hash];
			list[0] = 4; // allocated
			list[1] = 0; // items
			heap_pos += 2 + list[0];
		}

		// grow list if full
		if (list[0] == list[1])
		{
			// printf("grow list to %d  (size: %d)\n", heap_pos, list[0] * 2);
			hash_list[hash] = &hash_heap[heap_pos];
			new_list = hash_list[hash];

			new_list[0] = list[0] * 2; // double allocation. means can never exceed *2 memory consumption of final list in total
			new_list[1] = list[1];     // items
			memcpy(&new_list[2], &list[2], list[1] * sizeof(uint16)); // copy existing items
			list = new_list;
			heap_pos += 2 + list[0];
		}
		
		list[2 + list[1]] = i;
		list[1] ++;
	}
}


#define BACKUP_VLIST_STATE()  memcpy(literal_backup, literal, sizeof(literal));  memcpy(literal_pos_backup, literal_pos, sizeof(literal_pos));
#define RESTORE_VLIST_STATE() memcpy(literal, literal_backup, sizeof(literal));  memcpy(literal_pos, literal_pos_backup, sizeof(literal_pos));


int pxa_compress(uint8 *in_p, uint8 *out, int len)
{
	int pos = 0;
	int block_offset;
	int block_len;
	int i, j, best_i;
	uint8 *in;
	char *modified_code;
	int hash;
	int block_score, literal_score;
	int literal[256];
	int literal_pos[256];
	int literal_backup[256];
	int literal_pos_backup[256];

	// 0.2.0j
	int raw_pos_src0 = 0;
	int raw_header_write_pos = 0;
	int raw_block_write_pos = 0;
	int raw_pos_src = 0;
	int raw_pos_dest = 0;
	int stored_last_segment_as_raw = 0;
	int raw_block_size = 0;
	

	init_literals_state(literal, literal_pos);
	pxa_build_hash_lookup(in_p, len);

	bit = 1;
	byte = 0;
	dest_buf = out;
	dest_pos = 0;

	if (len == 0) return 0;
	
	for (i = 0; i < HASH_MAX; i++)
		found[i] = -1;
	
	modified_code = codo_malloc(len);
	memcpy(modified_code, in_p, len);
	in = modified_code;

	
	// appear empty in old versions of pico-8 (not relevant anymore)
	PXA_WRITE_VAL(0);
	PXA_WRITE_VAL('p');
	PXA_WRITE_VAL('x');
	PXA_WRITE_VAL('a');
	
	// write uncompressed size (plain uint32 so that easy to read & allocate dest before calling)
	PXA_WRITE_VAL(len/256);
	PXA_WRITE_VAL(len%256);

	// compressed size (fill in later). used for robust/safe decompression
	PXA_WRITE_VAL(0);
	PXA_WRITE_VAL(0);

	num_blocks = 0;
	num_literals = 0;
	num_blocks_large = 0;

	// start looking for raw blocks
	raw_pos_dest = dest_pos;
	raw_pos_src = raw_pos_src0 = pos;
	raw_header_write_pos = get_write_pos();
	raw_block_write_pos = get_write_pos();
	BACKUP_VLIST_STATE();


	while (pos < len)
	{
		// either copy or literal
		
		block_len = pxa_find_repeatable_block(in, pos, len, &block_offset, &block_score);

		
		int c = in[pos];
		int lpos = literal_pos[c];

		// score: start from 2+ for top-level literal marker + category marker (1,2,2 bits)

		int cat_bits = TINY_LITERAL_BITS;
		int cat_max_val = 1 << cat_bits;
		while (lpos >= cat_max_val)
		{
			cat_bits ++;
			cat_max_val += (1 << cat_bits);
			//printf(" cat_max_val %d   cat_bits: %d \n", cat_max_val, cat_bits);
		}

		// is correct
		//printf("lpos bit cost: %d %d (cat_max_val: %d)\n", lpos, (2 + ((MIN(8,cat_bits) - TINY_LITERAL_BITS) + cat_bits)), cat_max_val);

		literal_score = 1 * 256 / (2 + ((cat_bits - TINY_LITERAL_BITS) + cat_bits));
		
/*
		if (block_len >= PXA_MIN_BLOCK_LEN && block_score > literal_score)
			printf("block score: %04d   literal score: %04d %c\n", block_score, literal_score, block_score >= 128 ? '*' : ' ');
*/

		// If block score is good (>= 128), just take it. But otherwise, look for better block score in next 2 characters
		// before commiting to a block. Saves ~400 bytes for heavy carts (!)

		if (block_len >= PXA_MIN_BLOCK_LEN && block_score > literal_score)
		if (block_score < 128) // 25% faster, only slight drop in compression ratio (lost avg 3.6 bytes across 5 carts)
		{
			int ii;
			for (ii =1; ii < 3; ii++)
			{
				int block_offset2=0;
				int block_score2=0;
			
				pxa_find_repeatable_block(in, pos+ii, len, &block_offset2, &block_score2);
				if (block_score2 > block_score * 6/5) // 6/5
				{
					// printf("blocked! block_score2: %d block_score %d\n", block_score2, block_score);
					block_score = 0;
					break;
				}
			}
		}


		if (block_len >= PXA_MIN_BLOCK_LEN && block_score > literal_score)
		{
			// block
			//printf("*");


			// makes sense to mark with block because aim for ~ 50% blocks
			putbit(0); block_bits_written ++;


			// printf(" writing block offset:%d len:%d\n", block_offset, block_len);
			
			block_bits_written += putnum(block_offset - 1);
			block_bits_written += putchain(block_len-PXA_MIN_BLOCK_LEN, BLOCK_LEN_CHAIN_BITS, 100000);

			if (block_len-PXA_MIN_BLOCK_LEN >= 7){
				num_blocks_large ++;
			}

			pos += block_len;
			
			// stats
			num_blocks ++;
			total_block_len += block_len;
		}
		else
		{
			// literal

			putbit(1);

			// write category

			int cat_bits = TINY_LITERAL_BITS;
			int cat_max_val = 1 << cat_bits;
			int val = lpos;
			while (lpos >= cat_max_val)
			{
				val -= (1 << cat_bits);
				cat_bits ++;
				cat_max_val += (1 << cat_bits);
			}

			putchain(cat_bits - TINY_LITERAL_BITS, 1, 16); // 16: safety
			
			// write the index itself
			putval(val, cat_bits); // lpos
		
			// move c to start of vlist and update positions
			// only pay attention to value outside of blocks; compression ratio is fine (maybe better?) and faster to calculate
			
			for (i = lpos; i > 0; i--)
			{
				literal[i] = literal[i-1];
				literal_pos[literal[i]] ++;
			}
			literal[0] = c;
			literal_pos[c] = 0;

			pos ++;
			
			// stats
			
			num_literals ++;
			block_len = 1; // for writing hash
		}

		// add hash positions
		
		for (i = MAX(0, pos - block_len-2); i < pos-2; i++)
		{
			hash = MINI_HASH(in, i);
			found[hash] = i;
		}

		// 0.2.0j: if last 32 bytes (or remaining end of input) written have a ratio worse than ~1.0, rewrite as a raw block instead

		if (dest_pos - raw_pos_dest >= 32 || pos == len)
		{
			int compressed_size = dest_pos - raw_pos_dest;
			int raw_size = pos - raw_pos_src;
			int margin = raw_pos_src0 == raw_pos_src ? 3 : 0; // 3 for first section (header + null terminator), 0 for appended

			// rewrite as raw block?
			if (compressed_size > raw_size + margin)
			{
				if (stored_last_segment_as_raw == 0) // write header
				{
					// write header marker 010 00000 00000
					raw_block_size = raw_size;
					raw_header_write_pos = raw_block_write_pos;
					set_write_pos(raw_header_write_pos);
					putbit(0); putbit(1); putbit(0); putval(0, 10);
				}
				else
				{
					// append
					set_write_pos(raw_block_write_pos);
					dest_pos--; // overwrite previous null terminator
				}

				// write raw data (not aligned)
				int k = 0;
				for (k = 0; k < raw_size; k++)
					putval(in[raw_pos_src + k], 8);
				putval(0,8); // null terminator

				stored_last_segment_as_raw = 1;
				RESTORE_VLIST_STATE();
			}
			else{
				// leave as-is; reset start position of next possible raw block
				stored_last_segment_as_raw = 0;
				raw_pos_src0 = pos;
				BACKUP_VLIST_STATE();
			}

			raw_pos_dest = dest_pos;
			raw_pos_src = pos;
			raw_block_write_pos = get_write_pos();
		}

	}

	codo_free(modified_code);

	// advance to next byte (and zero any junk)
	while (bit != 1)
		putbit(0); 

	int bytes_written = dest_pos;
	
	dest_buf[6] = bytes_written / 256;
	dest_buf[7] = bytes_written % 256;


	// 0.2.0e: compressed is larger than input -> just return input (same as pxc)
	// for storing binary data -- perhaps cart is mostly data w/ tiny stub
	// otherwise, storing binary string compresses to around 1.25 (see /pxa/gen_rnd.p8)
	if (bytes_written > len)
	{
		// 0.2.0j: fixed: was in (which now points to deallocated memory. discovered because oversized-cart get_cart_hash was failing!)
		// would also cause small, or data-heavy .png file save/load to fail
		memcpy(out, in_p, len); 
		return len;
	}

	return bytes_written;
}


int pxa_decompress(uint8 *in_p, uint8 *out_p, int max_len)
{
	uint8 *dest;
	int i;
	int literal[256];
	int literal_pos[256];
	int dest_pos = 0;

	bit = 1;
	byte = 0;
	src_buf = in_p;
	src_pos = 0;

	init_literals_state(literal, literal_pos);

	// header

	int header[8];
	for (i = 0; i < 8; i++)
		header[i] = PXA_READ_VAL();

	int raw_len  = header[4] * 256 + header[5];
	int comp_len = header[6] * 256 + header[7];

	// printf(" read raw_len:  %d\n", raw_len);
	// printf(" read comp_len: %d\n", comp_len);

	while (src_pos < comp_len && dest_pos < raw_len && dest_pos < max_len)
	{
		int block_type = getbit();

		// printf("%d %d\n", src_pos, block_type); fflush(stdout);

		if (block_type == 0)
		{
			// block

			int block_offset = getnum() + 1;

			if (block_offset == 0)
			{
				// 0.2.0j: raw block
				while (dest_pos < raw_len)
				{
					out_p[dest_pos] = getval(8);
					if (out_p[dest_pos] == 0) // found end -- don't advance dest_pos
						break;
					dest_pos ++;
				}
			}
			else
			{
				int block_len = getchain(BLOCK_LEN_CHAIN_BITS, 100000) + PXA_MIN_BLOCK_LEN;

				// copy // don't just memcpy because might be copying self for repeating pattern
				while (block_len > 0){
					out_p[dest_pos] = out_p[dest_pos - block_offset];
					dest_pos++;
					block_len--;
				}

				// safety: null terminator. to do: just do at end
				if (dest_pos < max_len-1)
					out_p[dest_pos] = 0;
			}
		}else
		{
			// literal

			int lpos = 0;
			int bits = 0;

			int safety = 0;
			while (getbit() == 1 && safety++ < 16)
			{
				lpos += (1 << (TINY_LITERAL_BITS + bits));
				bits ++;
			}

			bits += TINY_LITERAL_BITS;
			lpos += getval(bits);

			if (lpos > 255) return 0; // something wrong

			// grab character and write
			int c = literal[lpos];

			out_p[dest_pos] = c;
			dest_pos++;
			out_p[dest_pos] = 0;
			
			int i;
			for (i = lpos; i > 0; i--)
			{
				literal[i] = literal[i-1];
				literal_pos[literal[i]] ++;
			}
			literal[0] = c;
			literal_pos[c] = 0;
		}
	}


	return 0;
}

int is_compressed_format_header(uint8 *dat)
{
	if (dat[0] == ':' && dat[1] == 'c' && dat[2] == ':' && dat[3] == 0) return 1;
	if (dat[0] == 0 && dat[1] == 'p' && dat[2] == 'x' && dat[3] == 'a') return 2;
	return 0;
}

// max_len should be 0x10000 (64k max code size)
// out_p should allocate 0x10001 (includes null terminator)
int pico8_code_section_decompress(uint8 *in_p, uint8 *out_p, int max_len)
{
	if (is_compressed_format_header(in_p) == 0) { memcpy(out_p, in_p, 0x3d00); out_p[0x3d00] = '\0'; return 0; } // legacy: no header -> is raw text
	if (is_compressed_format_header(in_p) == 1) return decompress_mini(in_p, out_p, max_len);
	if (is_compressed_format_header(in_p) == 2) return pxa_decompress (in_p, out_p, max_len);
	return 0;
}




