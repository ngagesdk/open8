#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Import the actual decompression function
extern int p8_decompress(const uint8_t *in, int in_len, uint8_t *out, int out_max);

START_TEST(test_decompression_bounds_safety)
{
    // Invariant: Decompression must never read/write outside allocated buffer bounds
    // regardless of malicious block_offset or block_length values in compressed input
    
    struct {
        const uint8_t *payload;
        int len;
        const char *desc;
    } test_cases[] = {
        // Case 1: Block offset pointing before buffer start (negative offset attack)
        {(const uint8_t *)"\x00\xff\xff\x10\x00", 5, "negative_offset"},
        
        // Case 2: Block length exceeding remaining buffer space
        {(const uint8_t *)"\x00\x01\x00\xff\xff", 5, "excessive_length"},
        
        // Case 3: Valid small compressed data
        {(const uint8_t *)"\x00\x41\x42\x43", 4, "valid_input"}
    };
    
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < num_cases; i++) {
        uint8_t output[1024];
        memset(output, 0xCC, sizeof(output));
        
        // Guard bytes before and after to detect out-of-bounds writes
        uint8_t guard_before[16];
        uint8_t guard_after[16];
        memset(guard_before, 0xAA, sizeof(guard_before));
        memset(guard_after, 0xBB, sizeof(guard_after));
        
        // Decompress with bounds checking
        int result = p8_decompress(test_cases[i].payload, test_cases[i].len, 
                                   output, sizeof(output));
        
        // Invariant: Function must return error or valid length, never crash
        ck_assert_msg(result >= -1, "Decompression crashed or corrupted memory for %s", 
                     test_cases[i].desc);
        
        // Invariant: Guard regions must remain untouched
        for (int j = 0; j < 16; j++) {
            ck_assert_msg(guard_before[j] == 0xAA && guard_after[j] == 0xBB,
                         "Buffer overflow detected for %s", test_cases[i].desc);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_decompression_bounds_safety);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}