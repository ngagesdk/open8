/*
	p8_compress.c
	
	(c) Copyright 2014-2016 Lexaloffle Games LLP
	author: joseph@lexaloffle.com

	compression used in code section of .p8.png format
	
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef MAX
	#define MAX(x, y) (((x) > (y)) ? (x) : (y))
	#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

typedef unsigned char           uint8;

#define HIST_LEN 4096
#define LITERALS 60
#define PICO8_CODE_ALLOC_SIZE (0x10000+1)

#define codo_malloc malloc
#define codo_free free
#define codo_memset memset

// removed from end of decompressed if it exists
// (injected to maintain 0.1.7 forwards compatibility)
#define FUTURE_CODE "if(_update60)_update=function()_update60()_update60()end"
#define FUTURE_CODE2 "if(_update60)_update=function()_update60()_update_buttons()_update60()end"

// ^ is dummy -- not a literal. forgot '-', but nevermind! (gets encoded as rare literal)
char *literal = "^\n 0123456789abcdefghijklmnopqrstuvwxyz!#%(){}[]<>+=/*:;.,~_";
int literal_index[256]; // map literals to 0..LITERALS-1. 0 is reserved (not listed in literals string)

int find_repeatable_block(uint8 *dat, int pos, int len, int *block_offset)
{
	// block len starts from 2, so no need to record 0, 1 --> max is (15 + 2)
	int max_block_len = 17; // any more doesn't have much effect for code. more important to look back further.
	int max_hist_len = (255-LITERALS)*16; // less than HIST_LEN
	int i, j;
	int best_len = 0;
	int best_i = -100000;
	int max_len;

	// brute force search

	// block length can't be longer than remaining
	max_len = MIN(max_block_len, len - pos);
	
	// can't be longer than preceeding data
	max_hist_len = MIN(max_hist_len, pos); 
	
	for (i = pos - max_hist_len; i < pos; i++)
	{
		// find length starting at i
		
		j = i;
		while ((j-i) < max_len && j < pos && dat[j] == dat[pos+j-i]) j++;
		
		if ((j-i) > best_len)
		{
			best_len = (j-i);
			best_i = i;
		}
	}
	
	*block_offset = (pos-best_i);
	
	return best_len;
}


#define WRITE_VAL(x) {*p_8 = (x); p_8++;}

// returns compressed length
int num_blocks, num_blocks_large, num_literals;
int freq[256];

int compress_mini(uint8 *in_p, uint8 *out, int len)
{
	uint8 *p_8 = out;
	int pos = 0;
	int block_offset;
	int block_len;
	int i, j, best_i;
	uint8 *in;
	char *modified_code;
	
	// init literals search
	memset(literal_index, 0, 256);
	for (i = 1; i < LITERALS; i++)
	{
		literal_index[literal[i]] = i;
	}
	
	// 0.1.8 : inject future api implementation if _update60 found in in_p
	// note: doesn't apply to plain .p8 format
	
	modified_code = codo_malloc(strlen(in_p) + 1024);
	strcpy(modified_code, in_p);
	
	if (strstr(in_p, "_update60"))
	if (len < PICO8_CODE_ALLOC_SIZE - (strlen(FUTURE_CODE2)+1)) // skip if won't fit when decompressing
	{
		// 0.1.9: make sure there is some whitespace before future_code (0.1.8 bug)
		if (modified_code[strlen(modified_code)-1] != ' ' && modified_code[strlen(modified_code)-1] != '\n')
		{
			strcat(modified_code, "\n");
		}
		strcat(modified_code, FUTURE_CODE2);
		len += strlen(FUTURE_CODE2)+1;
	}
	
	in = modified_code;
	
	// header tag: ":c:"
	// will show up in code section of old versions of pico-8
	WRITE_VAL(':');
	WRITE_VAL('c');
	WRITE_VAL(':');
	WRITE_VAL(0);
	
	// write uncompressed size
	WRITE_VAL(len/256);
	WRITE_VAL(len%256);
	
	// compressed size (fill in later). used for robust/safe decompression
	WRITE_VAL(0);
	WRITE_VAL(0);
	
	num_blocks = 0;
	num_literals = 0;
	
	memset(freq, 0, sizeof(freq));
	#if 0
	// generate histogram
	for (i = 0; i < len; i++)
		freq[in[i]]++;
		
	// show highest
	for (i = 0; i < 256; i++)
		if (freq[i] > len / 64)
			printf("[%c] : %d\n", i, freq[i]);
	#endif
	
	while (pos < len)
	{
		// either copy or literal
		
		//printf("pos: %d\n", pos);
		
		block_len = find_repeatable_block(in, pos, len, &block_offset);
		
		// use block when 3 or more long. performs better than 2, because after
		// writing first literal, second one might be part of a block.
		if (block_len >= 3) 
		{
			// block: 2 bytes
			
			// printf(":: block. offset: %d len: %d\n", block_offset, block_len);
			
			WRITE_VAL((block_offset / 16) + LITERALS);
			WRITE_VAL((block_offset % 16) + (block_len-2) * 16);
			pos += block_len;
			
			// stats
			num_blocks ++;
			
			if (block_len > 17) num_blocks_large++;
		}
		else
		{
			// literal: 0 means read next byte
			// printf(":: literal: %d [%c]\n", in[pos], in[pos]);
			
			WRITE_VAL(literal_index[in[pos]]);
			
			if (literal_index[in[pos]] == 0)
				WRITE_VAL(in[pos]);
				
			pos ++;
			
			// stats
			
			//printf("%c",in[pos]);
			
			num_literals ++;
			freq[in[pos]]++;
		}
	}
	
	// compressed is larger than input -> just return input
	if ((p_8 - out) >= strlen(in))
	{
		memcpy(out, in, strlen(in));
		return strlen(in);
	}
	
	//printf("size: %d  blocks: %d (%d large)  literals: %d\n", (p_8 - out), num_blocks, num_blocks_large, num_literals);

	codo_free(modified_code);
	
	return p_8 - out;
}

#define READ_VAL(val) {val = *in; in++;}
int decompress_mini(uint8 *in_p, uint8 *out_p, int max_len)
{
	int block_offset;
	int block_length;
	int val;
	uint8 *in = in_p;
	uint8 *out = out_p;
	int len;
	
	// header tag ":c:"
	READ_VAL(val);
	READ_VAL(val);
	READ_VAL(val);
	READ_VAL(val);
	
	// uncompressed length
	READ_VAL(val);
	len = val * 256;
	READ_VAL(val);
	len += val;
	
	// compressed length (to do: use to check)
	READ_VAL(val);
	READ_VAL(val);
	
	codo_memset(out_p, 0, max_len);
	
	if (len > max_len) return 1; // corrupt data
	
	while (out < out_p + len)
	{
		READ_VAL(val);
		
		if (val < LITERALS)
		{
			// literal
			if (val == 0)
			{
				READ_VAL(val);
				//printf("rare literal: %d\n", val);
				*out = val;
			}
			else
			{
				// printf("common literal: %d (%c)\n", literal[val], literal[val]);
				*out = literal[val];
			}
			out++;
		}
		else
		{
			// block
			block_offset = val - LITERALS;
			block_offset *= 16;
			READ_VAL(val);
			block_offset += val % 16;
			block_length = (val / 16) + 2;
			
			memcpy(out, out - block_offset, block_length);
			out += block_length;
		}
	}
	
	
	// remove injected code (needed to be future compatible with PICO-8 C 0.1.7 / FILE_VERSION 8)
	// older versions will leave this code intact, allowing it to implement fallback 60fps support
	
	if (strstr(out_p, FUTURE_CODE))
	if (strlen(out_p)-((char *)strstr(out_p, FUTURE_CODE) - (char *)out_p) == strlen(FUTURE_CODE)) // at end
	{
		out = out_p + strlen(out_p) - strlen(FUTURE_CODE);
		*out = 0;
	}
	
	// queue circus music
	if (strstr(out_p, FUTURE_CODE2))
	if (strlen(out_p)-((char *)strstr(out_p, FUTURE_CODE2) - (char *)out_p) == strlen(FUTURE_CODE2)) // at end
	{
		out = out_p + strlen(out_p) - strlen(FUTURE_CODE2);
		*out = 0;
	}
	
	
	return out - out_p;
}


void compress_test(char *fn)
{
	FILE *f;
	uint8 *dat;
	uint8 *out;
	int len;
	int comp_len;
	int decomp_len;
	int i;
	
	dat = malloc(65536);
	out = malloc(65536);
	
	f = fopen(fn, "r");
	
	len = fread(dat, 1, 65536, f);
	fclose(f);
	
	//comp_len = codo_compress_lz4_hc(dat, out, len); // not as good as compress_mini()
	comp_len = compress_mini(dat, out, len);
	
	memset(dat, 0, 65536);
	
	decomp_len = decompress_mini(out, dat, 65536);

	// show highest freq of literals
	#if 0
	for (i = 0; i < 256; i++)
		if (freq[i] > 50)
			printf("[%c] : %d\n", i, freq[i]);
	#endif
	printf("len %d --> comp_len %d\n", len, comp_len);
	printf("decomp_len: %d\n", decomp_len);
	
	printf("blocks: %d literals %d\n", num_blocks, num_literals);
	printf("block len: %3.3f\n", (float)(len - num_literals) / (float)num_blocks);
	
	//printf("output: %s\n", dat);
	f = fopen("out.txt", "wb");
	fwrite(dat, 1, strlen(dat), f);
	fclose(f);
	
	free(dat);
	free(out);
}

int main(int argc,  char *argv[])
{
	if (argc > 1)
		compress_test(argv[1]);
}

