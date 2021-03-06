#ifndef __GENOTQ_H__
#define __GENOTQ_H__

#include <stdint.h>
#include <limits.h>
#include "parse_q.h"
#include "pq.h"
#include "ped.h"
#include "bcf.h"
#include "plt.h"
#include "ubin.h"
#include "wah.h"
#include "wahbm.h"
#include "wahbm_in_place.h"
#include "wahbm_compressed_in_place.h"

#define CHUNK 16384

struct uint_ll {
        uint32_t value;
            struct uint_ll *next;
};

uint32_t bin_char_to_int(char *bin);

int *unpack_2_bit_ints(uint32_t packed_ints);

int *unpack_1_bit_ints(uint32_t packed_ints);


int popcount(uint32_t x);

/**
 * @brief   Compress an array of integers encoded binary digits using
 *          run-length encoding.
 *
 * @param I     Array of integers
 * @param I_len Number of elements in I
 * @param O     Array containing the run-length encoding of I
 *
 * @retval number of elements in O
 *
 * Example Usage:
 * @code
 * @endcode
 */
uint32_t ints_to_rle(uint32_t *I, int I_len, uint32_t **O);


void parse_cmd_line_int_csv(uint32_t *I,
                            int num_I,
                            char *cmd_line_arg);
const char *int_to_binary(uint32_t x);
#endif
