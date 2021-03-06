/**
 * @file genotq.c
 * @Author Ryan Layer (ryan.layer@gmail.com)
 * @date May, 2014
 * @brief Functions for converting and opperation on various encoding
 * strategies
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <math.h>
#include <limits.h>
#include "genotq.h"


//{{{ struct ubin_file init_ubin_file(char *file_name)
struct ubin_file init_ubin_file(char *file_name)
{
    struct ubin_file uf;

    uf.file = fopen(file_name, "rb");

    if (!uf.file) {
        fprintf(stderr, "Unable to open %s\n", file_name);
        return uf;
    }

    // Jump to the begining of the file to grab the record size
    fseek(uf.file, 0, SEEK_SET);
    int r = fread(&uf.num_fields,sizeof(uint32_t),1,uf.file);
    r = fread(&uf.num_records,sizeof(uint32_t),1,uf.file);
    uf.header_offset = ftell(uf.file);

    return uf;

}
//}}}

//{{{ uint32_t convert_file_by_name_ubin_to_wahbm16(char *ubin_in, 
uint32_t convert_file_by_name_ubin_to_wahbm16(char *ubin_in,
                                                  char *wah_out)
{
    FILE *wf = fopen(wah_out,"wb");

    if (!wf) {
        printf("Unable to open %s\n", wah_out);
        return 1;
    }

    struct ubin_file uf = init_ubin_file(ubin_in);

    //write header for WAH bitmap index file
    fwrite(&(uf.num_fields), sizeof(uint32_t), 1, wf);
    fwrite(&(uf.num_records), sizeof(uint32_t), 1, wf);
    int zero = 0;
    int k;
    for (k = 0; k < uf.num_records*4; ++k)
        fwrite(&zero, sizeof(int), 1, wf);

    int num_ints_per_record = 1 + ((uf.num_fields - 1) / 16);
    int num_bytes_per_record = num_ints_per_record * 4;

    uint32_t *c = (uint32_t *)
        malloc(num_ints_per_record*sizeof(uint32_t));

    int i,j,wah_i = 0, offset_total  = 0;

    // skip to the target record and read in the full record
    fseek(uf.file, uf.header_offset, SEEK_SET);

    uint32_t tenth_num_records = uf.num_records / 10;
    fprintf(stderr,"Compressing genotypes");
    
    for (i = 0; i < uf.num_records; ++i) {
        if ( (tenth_num_records == 0) || (i % tenth_num_records == 0))
            fprintf(stderr, ".");

        int r =fread(c,sizeof(uint32_t),num_ints_per_record,uf.file);
         
        uint16_t *wah;
        uint32_t *wah_sizes;
        uint32_t wah_len = ubin_to_bitmap_wah16(c,
                                                    num_ints_per_record,
                                                    uf.num_fields,
                                                    &wah,
                                                    &wah_sizes);

        fseek(wf,sizeof(uint32_t)* (2+4* wah_i),  SEEK_SET);
        for (j = 0; j < 4; ++j) {
            offset_total += wah_sizes[j];
            fwrite(&offset_total, sizeof(uint32_t), 1, wf);
        }

        fseek(wf,0,SEEK_END);
        size_t ret = fwrite(wah, sizeof(uint16_t), wah_len, wf);
        if (ret != wah_len)
            fprintf(stderr, "ret:%zu != wah_len:%u\n", ret, wah_len);

        wah_i+=1;
        free(wah);
        free(wah_sizes);
    }

    fprintf(stderr, "Done");

    free(c);

    fclose(wf);
    fclose(uf.file);
    return 0;
}
//}}}

//{{{ uint32_t convert_file_by_name_ubin_to_wahbm(char *ubin_in, 
/*
 * Convert a file contained 16 genotypes packed into a 32-bit int into a WAH
 * endoded bitmap file.  The WAH BM file has a header that indicates the number
 * number of fields, the number of records, and then the relative offset of the
 * end of each bit map.  The encoded bitmaps follow after the header.
 *
 * INPUT
 *   ubin_in: a file containing 32-bit ints, wehre each int encodes
 *            up to 16 genotypes (0,1,2,3)
 * OUTPT
 *   wah_out: a WAH bitmap file that encodes the genotypes listed in
 *            ubin_in as compressed bit maps
 */
uint32_t convert_file_by_name_ubin_to_wahbm(char *ubin_in, char *wah_out)
{
    FILE *wf = fopen(wah_out,"wb");

    if (!wf) {
        printf("Unable to open %s\n", wah_out);
        return 1;
    }

    struct ubin_file uf = init_ubin_file(ubin_in);

    //write header for WAH bitmap index file
    fwrite(&(uf.num_fields), sizeof(uint32_t), 1, wf);
    fwrite(&(uf.num_records), sizeof(uint32_t), 1, wf);
    int zero = 0;
    int k;
    //the header includes an index to each of BM, 4 per individual
    for (k = 0; k < uf.num_records*4; ++k)
        fwrite(&zero, sizeof(uint64_t), 1, wf);

    int num_ints_per_record = 1 + ((uf.num_fields - 1) / 16);
    int num_bytes_per_record = num_ints_per_record * 4;

    uint32_t *c = (uint32_t *) malloc(num_ints_per_record*sizeof(uint32_t));

    int i,j,wah_i = 0;
    uint64_t offset_total  = 0;

    // skip to the target record and read in the full record
    fseek(uf.file, uf.header_offset, SEEK_SET);

    uint32_t tenth_num_records = uf.num_records / 10;
    fprintf(stderr,"Compressing genotypes");

    for (i = 0; i < uf.num_records; ++i) {
        if ( (tenth_num_records == 0) || (i % tenth_num_records == 0))
            fprintf(stderr, ".");

        int r = fread(c, sizeof(uint32_t), num_ints_per_record, uf.file);
         
        uint32_t *wah;
        uint32_t *wah_sizes;
        uint32_t wah_len = ubin_to_bitmap_wah(c,
                                              num_ints_per_record,
                                              uf.num_fields,
                                              &wah,
                                              &wah_sizes);

        // Jump to the correct point in the WAH header
        fseek(wf, 
              2*sizeof(uint32_t) + // num fields and records
              sizeof(uint64_t)*(4*wah_i), SEEK_SET);
        // Write the end offset of all 4
        for (j = 0; j < 4; ++j) {
            offset_total += wah_sizes[j];
            fwrite(&offset_total, sizeof(uint64_t), 1, wf);
        }

        // Jump to the end of the file
        fseek(wf, 0, SEEK_END);
        // Write out the compressed WAH bitmap
        size_t ret = fwrite(wah, sizeof(uint32_t), wah_len, wf);
        if (ret != wah_len)
            fprintf(stderr, "ret:%zu != wah_len:%u\n", ret, wah_len);

        wah_i+=1;
        free(wah);
        free(wah_sizes);
    }

    fprintf(stderr, "Done\n");

    free(c);

    fclose(wf);
    fclose(uf.file);
    return 0;
}
//}}}

//{{{ uint32_t convert_file_by_name_ubin_to_wah(char *ubin_in,
uint32_t convert_file_by_name_ubin_to_wah(char *ubin_in, char *wah_out)
{
    FILE *wf = fopen(wah_out,"wb");

    if (!wf) {
        printf("Unable to open %s\n", wah_out);
        return 1;
    }

    struct ubin_file uf = init_ubin_file(ubin_in);

    //write header for WAH bitmap index file
    fwrite(&(uf.num_fields), sizeof(int), 1, wf);
    fwrite(&(uf.num_records), sizeof(int), 1, wf);
    int zero = 0;
    int k;
    for (k = 0; k < uf.num_records; ++k)
        fwrite(&zero, sizeof(int), 1, wf);

    int num_ints_per_record = 1 + ((uf.num_fields - 1) / 16);
    int num_bytes_per_record = num_ints_per_record * 4;

    uint32_t *c = (uint32_t *)
        malloc(num_ints_per_record*sizeof(uint32_t));

    int i,j,wah_i = 0, offset_total  = 0;

    // skip to the target record and read in the full record
    fseek(uf.file, uf.header_offset, SEEK_SET);

    for (i = 0; i < uf.num_records; ++i) {
        int r = fread(c,sizeof(uint32_t),num_ints_per_record,uf.file);
         
        uint32_t *wah;
        uint32_t wah_len = ints_to_wah(c,
                                           num_ints_per_record,
                                           uf.num_fields*2,
                                           &wah);

        fseek(wf,sizeof(uint32_t)* (2+wah_i),  SEEK_SET);

        offset_total += wah_len;
        fwrite(&offset_total, sizeof(uint32_t), 1, wf);

        fseek(wf,0,SEEK_END);
        size_t ret = fwrite(wah, sizeof(uint32_t), wah_len, wf);
        if (ret != wah_len)
            fprintf(stderr, "ret:%zu != wah_len:%u\n", ret, wah_len);

        wah_i+=1;
        free(wah);
    }

    free(c);

    fclose(wf);
    fclose(uf.file);
    return 0;
}
//}}}

//{{{ uint32_t get_ubin_record(struct ubin_file uf,
uint32_t get_ubin_record(struct ubin_file uf,
                             uint32_t record_id,
                             uint32_t **ubin_record)
{
    int num_ints_per_record = 1 + ((uf.num_fields - 1) / 16);

    uint32_t ubin_offset = uf.header_offset + 
            sizeof(uint32_t)*(record_id*num_ints_per_record);

    //fprintf(stderr, "ubin_offset:%u\n", ubin_offset);

    *ubin_record = (uint32_t *)
                   malloc(sizeof(uint32_t)*num_ints_per_record);

    fseek(uf.file, ubin_offset, SEEK_SET);
    int r = fread(*ubin_record,
                   sizeof(uint32_t),
                   num_ints_per_record,
                   uf.file);

    return num_ints_per_record;
}

//}}}

//{{{ uint32_t ubin_to_bitmap(uint32_t *U,
uint32_t ubin_to_bitmap(uint32_t *U,
                        uint32_t U_len,
                        uint32_t used_bits,
                        uint32_t **B)
{
    // Since U encodeds a series of two-bit values, and the bitmap uses one
    // bit per unique value in U, the bitmap for each value will require 1/2 
    // (rounded up) the number of ints used to encode U
    uint32_t value_index_size = (U_len + 2 - 1) / 2;
    // There are 4 unique values, so in total B will require 4x  
    uint32_t B_len = 4 * value_index_size;
    *B = (uint32_t *) calloc(B_len, sizeof(uint32_t));

    uint32_t two_bit, set_bit, bit_offset, B_int_i, B_two_bit_i, B_bit_i;

    B_int_i = 0;
    B_two_bit_i = 0;
    B_bit_i = 0;


    uint32_t i,j,k;
    for (i = 0; i < U_len; ++i) {
        for (j = 0; j < 16; ++j) {
            two_bit = ((U[i] >> (30 - (2*j)))& 3);
            for (k = 0; k < 4; ++k) {

                if (k == two_bit)
                    set_bit = 1;
                else 
                    set_bit = 0;

                /* 
                 * B consists of 4 bit arrays laid out consecutively.
                 * As we loop over the two bit values, a single bit must be
                 * set in all 4 bit arrays.  The first bit array is at
                 * B_int_i, and each bit array has value_index_size values
                 *
                 */

                bit_offset = (k * value_index_size) + B_int_i;

                (*B)[bit_offset] +=  set_bit << (31 - B_two_bit_i);
                

                /*
                fprintf(stderr, "k:%u\t"
                                "set_bit:%u\t"
                                "two_bit:%u\t" 
                                "B_int_i:%u\t" 
                                "B_two_bit_i:%u\n",
                                k,
                                set_bit,
                                two_bit,
                                B_int_i,
                                B_two_bit_i);
                */
            }
            B_two_bit_i += 1;
            B_bit_i += 2;

            if (B_two_bit_i == 32) {
                B_int_i += 1;
                B_two_bit_i = 0;
            }

            if (B_bit_i >= used_bits)
                break;
        }
        if (B_bit_i >= used_bits)
            break;
    }

    return B_len;
}
//}}}

//{{{ uint32_t ubin_to_bitmap_wah16(uint32_t *U,
uint32_t ubin_to_bitmap_wah16(uint32_t *U,
                                  uint32_t U_len,
                                  uint32_t num_fields,
                                  uint16_t **W,
                                  uint32_t **wah_sizes)
{
    uint32_t *B;
    // two bits per field
    uint32_t B_len = ubin_to_bitmap(U, U_len, num_fields*2, &B);
    uint32_t b_len = B_len / 4;  // size of each bitmap index

    *wah_sizes = (uint32_t *) malloc(4*sizeof(uint32_t));

    uint16_t *wahs[4];
    uint32_t wahs_size[4],
                 i,
                 j;

    uint32_t total_wah_size = 0;

    for (i = 0; i < 4; i++) {
        wahs_size[i] = ints_to_wah16( (B + (i*b_len)), 
                                      b_len, 
                                      num_fields, // 1 bit per field
                                      &(wahs[i]));
        (*wah_sizes)[i] = wahs_size[i];
        total_wah_size += wahs_size[i];
    }

    free(B);

    uint32_t W_i = 0;
    *W = (uint16_t *) malloc(total_wah_size*sizeof(uint16_t));
    for (i = 0; i < 4; i++) {
        for (j = 0; j < wahs_size[i]; j++) {
            (*W)[W_i] = wahs[i][j];
            W_i += 1;
        }
        free(wahs[i]);
    }

    return total_wah_size;
}
//}}}

//{{{ uint32_t ubin_to_bitmap_wah(uint32_t *U,
uint32_t ubin_to_bitmap_wah(uint32_t *U,
                            uint32_t U_len,
                            uint32_t num_fields,
                            uint32_t **W,
                            uint32_t **wah_sizes)
{
    uint32_t *B = NULL;
    // two bits per field
    uint32_t B_len = ubin_to_bitmap(U, U_len, num_fields*2, &B);
    uint32_t b_len = B_len / 4;  // size of each bitmap index

    *wah_sizes = (uint32_t *) malloc(4*sizeof(uint32_t));

    uint32_t *wahs[4],
                 wahs_size[4],
                 i,
                 j;

    uint32_t total_wah_size = 0;

    for (i = 0; i < 4; i++) {
        wahs_size[i] = ints_to_wah( (B + (i*b_len)), 
                                    b_len, 
                                    num_fields, // 1 bit per field
                                    &(wahs[i]));
        (*wah_sizes)[i] = wahs_size[i];
        total_wah_size += wahs_size[i];
    }

    free(B);

    uint32_t W_i = 0;
    *W = (uint32_t *) malloc(total_wah_size*sizeof(uint32_t));
    for (i = 0; i < 4; i++) {
        for (j = 0; j < wahs_size[i]; j++) {
            (*W)[W_i] = wahs[i][j];
            W_i += 1;
        }
        free(wahs[i]);
    }


    return total_wah_size;
}
//}}}

//{{{ uint32_t gt_records_ubin(struct ubin_file uf,
uint32_t gt_records_ubin(struct ubin_file uf,
                             uint32_t *record_ids,
                             uint32_t num_r,
                             uint32_t test_value,
                             uint32_t **R)
{
    return range_records_ubin(uf,
                              record_ids,
                              num_r,
                              test_value+1,
                              4,
                              R);

#if 0
    uint32_t num_output_ints = 1 + ((uf.num_fields - 1) / 32);

    uint32_t num_ints_per_record = 1 + ((uf.num_fields - 1) / 16);
    int num_bytes_per_record = num_ints_per_record * 4;

    *R = (uint32_t *) malloc(num_output_ints*sizeof(uint32_t));
    uint32_t i,j;
    for (i = 0; i < num_output_ints; ++i)
        (*R)[i] = -1;

    uint32_t *c = (uint32_t *)
        malloc(num_ints_per_record*sizeof(uint32_t));

    uint32_t R_int_i, R_bit_i;
    uint32_t c_int_i, c_two_bit_i;

    for (i = 0; i < num_r; ++i) {
        // skip to the target record and read in the full record
        fseek(uf.file, uf.header_offset + // skip the record & field size field
                    record_ids[i]*num_bytes_per_record, // skip to the reccord
                    SEEK_SET);
        fread(c,sizeof(uint32_t),num_ints_per_record,uf.file);

        R_int_i = 0;
        R_bit_i = 0;
        c_two_bit_i = 0;
        c_int_i = 0;

        for (j = 0; j < uf.num_fields; ++j) {
            // clear the bit

            if  ( !( test_value < ((c[c_int_i] >> (30 - c_two_bit_i*2)) & 3) ) )
                (*R)[R_int_i] = (*R)[R_int_i] & ~(1 << (31 - R_bit_i));

            R_bit_i += 1;
            if (R_bit_i == 32) {
                R_int_i += 1;
                R_bit_i = 0;
            }

            c_two_bit_i += 1;
            if (c_two_bit_i == 16) {
                c_two_bit_i = 0;
                c_int_i += 1;
            }
        }
    }

    free(c);

    return num_ints_per_record;
#endif
}
//}}}

//{{{ uint32_t print_ubin(struct ubin_file uf,
uint32_t print_ubin(struct ubin_file uf,
                        uint32_t *record_ids,
                        uint32_t num_r,
                        uint32_t format)
{
    uint32_t num_ints_per_record = 1 + ((uf.num_fields - 1) / 16);
    int num_bytes_per_record = num_ints_per_record * 4;

    uint32_t *c = (uint32_t *)
        malloc(num_ints_per_record*sizeof(uint32_t));

    uint32_t i, j, k, num_printed = 0;

    if ( (num_r == 0) || (record_ids == NULL) ) {
        while (fread(c,sizeof(uint32_t),num_ints_per_record,uf.file)) {
            uint32_t printed_bits = 0;

            for (j = 0; j < num_ints_per_record; ++j) {
                if (j !=0)
                    printf(" ");

                if (format == 1) {
                    printf("%u", c[j]);
                } else if (format == 0) {
                    for (k = 0; k < 16; ++k) {
                        uint32_t bit = (c[j] >> (30 - 2*k)) & 3;
                        if (k !=0)
                            printf(" ");
                        printf("%u", bit);
                        printed_bits += 1;
                        if (printed_bits == uf.num_fields)
                            break;
                    }
                }
                num_printed += 1;
            }
            printf("\n");
        }
    } else {
        for (i = 0; i < num_r; ++i) {
            uint64_t o = record_ids[i];
            o*=num_bytes_per_record;
            o+=uf.header_offset;
            fseek(uf.file, o, SEEK_SET);
            int r = fread(c,sizeof(uint32_t),num_ints_per_record,uf.file);

            uint32_t printed_bits = 0;

            for (j = 0; j < num_ints_per_record; ++j) {
                if (j !=0)
                    printf(" ");

                if (format == 1) {
                    printf("%u", c[j]);
                } else if (format == 0) {
                    for (k = 0; k < 16; ++k) {
                        uint32_t bit = (c[j] >> (30 - 2*k)) & 3;
                        if (k !=0)
                            printf(" ");
                        printf("%u", bit);
                        printed_bits += 1;
                        if (printed_bits == uf.num_fields)
                            break;
                    }
                }
                num_printed += 1;
            }
            printf("\n");
        }
    }

    free(c);

    return num_printed;
}
//}}}

//{{{ uint32_t print_by_name_ubin(char *ubin_file_name,
uint32_t print_by_name_ubin(char *ubin_file_name,
                               uint32_t *record_ids,
                               uint32_t num_r,
                               uint32_t format)
{
    struct ubin_file uf = init_ubin_file(ubin_file_name);
    return print_ubin(uf, record_ids, num_r, format);
}
//}}} 

//{{{ uint32_t range_records_ubin(struct ubin_file uf,
uint32_t range_records_ubin(struct ubin_file uf,
                                uint32_t *record_ids,
                                uint32_t num_r,
                                uint32_t start_test_value,
                                uint32_t end_test_value,
                                uint32_t **R)
{
    uint32_t num_output_ints = 1 + ((uf.num_fields - 1) / 32);

    uint32_t num_ints_per_record = 1 + ((uf.num_fields - 1) / 16);
    int num_bytes_per_record = num_ints_per_record * 4;

    *R = (uint32_t *) malloc(num_output_ints*sizeof(uint32_t));
    uint32_t i,j;
    for (i = 0; i < num_output_ints; ++i)
        (*R)[i] = -1;

    uint32_t *c = (uint32_t *)
        malloc(num_ints_per_record*sizeof(uint32_t));

    uint32_t R_int_i, R_bit_i;
    uint32_t c_int_i, c_two_bit_i;

    for (i = 0; i < num_r; ++i) {
        // skip to the target record and read in the full record
        fseek(uf.file, uf.header_offset + // skip the record & field size field
                    record_ids[i]*num_bytes_per_record, // skip to the reccord
                    SEEK_SET);
        int r = fread(c,sizeof(uint32_t),num_ints_per_record,uf.file);

        R_int_i = 0;
        R_bit_i = 0;
        c_two_bit_i = 0;
        c_int_i = 0;

        for (j = 0; j < uf.num_fields; ++j) {
            // clear the bit
            uint32_t val = (c[c_int_i] >> (30 - c_two_bit_i*2)) & 3;

            if (!(val >= start_test_value && val < end_test_value))
                (*R)[R_int_i] = (*R)[R_int_i] & ~(1 << (31 - R_bit_i));

            R_bit_i += 1;
            if (R_bit_i == 32) {
                R_int_i += 1;
                R_bit_i = 0;
            }

            c_two_bit_i += 1;
            if (c_two_bit_i == 16) {
                c_two_bit_i = 0;
                c_int_i += 1;
            }
        }
    }

    free(c);

    return num_ints_per_record;
}
//}}}

//{{{ uint32_t count_range_records_plt(struct plt_file pf,
uint32_t count_range_records_ubin(struct ubin_file uf,
                                      uint32_t *record_ids,
                                      uint32_t num_r,
                                      uint32_t start_test_value,
                                      uint32_t end_test_value,
                                      uint32_t **R)
{
    *R = (uint32_t *) calloc(uf.num_fields,sizeof(uint32_t));

    uint32_t num_ints_per_record = 1 + ((uf.num_fields - 1) / 16);
    int num_bytes_per_record = num_ints_per_record * 4;

    uint32_t *c = (uint32_t *)
        malloc(num_ints_per_record*sizeof(uint32_t));

    uint32_t R_int_i, R_bit_i;
    uint32_t c_int_i, c_two_bit_i;

    uint32_t i,j;
    for (i = 0; i < num_r; ++i) {
        // skip to the target record and read in the full record
        fseek(uf.file, uf.header_offset + // skip the record & field size field
                    record_ids[i]*num_bytes_per_record, // skip to the reccord
                    SEEK_SET);
        int r = fread(c,sizeof(uint32_t),num_ints_per_record,uf.file);

        R_int_i = 0;
        R_bit_i = 0;
        c_two_bit_i = 0;
        c_int_i = 0;

        for (j = 0; j < uf.num_fields; ++j) {
            // clear the bit
            uint32_t val = (c[c_int_i] >> (30 - c_two_bit_i*2)) & 3;

            if ((val > start_test_value) && (val < end_test_value))
                (*R)[j] += 1;

            c_two_bit_i += 1;
            if (c_two_bit_i == 16) {
                c_two_bit_i = 0;
                c_int_i += 1;
            }
        }
    }

    free(c);

    return num_ints_per_record;
}
//}}}

//{{{ uint32_t gt_count_records_ubin(struct ubin_file uf,
uint32_t gt_count_records_ubin(struct ubin_file uf,
                                   uint32_t *record_ids,
                                   uint32_t num_r,
                                   uint32_t test_value,
                                   uint32_t **R)
{
    return count_range_records_ubin(uf,
                                   record_ids,
                                   num_r,
                                   test_value,
                                   4,
                                   R);
}
//}}}

//{{{ uint32_t convert_file_by_name_ubin_to_plt(char *ubin_in, char *plt_out)
uint32_t convert_file_by_name_ubin_to_plt(char *ubin_in, char *plt_out)
{
    struct ubin_file uf = init_ubin_file(ubin_in);

    FILE *pf = fopen(plt_out,"w");

    if (!pf) {
        printf("Unable to open %s\n", plt_out);
        return 1;
    }

    fprintf(pf,"%u\n", uf.num_fields);
    fprintf(pf,"%u\n", uf.num_records);


    uint32_t num_ints_per_record = 1 + ((uf.num_fields - 1) / 16);
    int num_bytes_per_record = num_ints_per_record * 4;

    uint32_t *c = (uint32_t *)
        malloc(num_ints_per_record*sizeof(uint32_t));

    uint32_t i, j, k, num_printed = 0;

    while (fread(c,sizeof(uint32_t),num_ints_per_record,uf.file)) {
        uint32_t printed_bits = 0;

        for (j = 0; j < num_ints_per_record; ++j) {
            if (j !=0)
                fprintf(pf," ");

            for (k = 0; k < 16; ++k) {
                uint32_t bit = (c[j] >> (30 - 2*k)) & 3;
                if (k !=0)
                    fprintf(pf," ");
                fprintf(pf,"%u", bit);
                printed_bits += 1;
                if (printed_bits == uf.num_fields)
                    break;
            }
            
           num_printed += 1;
        }
        fprintf(pf,"\n");
    }

    free(c);
    fclose(pf);
    fclose(uf.file);

    return num_printed;
}
//}}}

#if 0
//{{{ uint32_t convert_hdf5_ind_ubin_to_ind_wah(struct hdf5_file hdf5_f,
uint32_t convert_hdf5_ind_ubin_to_ind_wah(struct hdf5_file hdf5_f,
                                              char *wah_out)
{
    FILE *wf = fopen(wah_out,"wb");

    if (!wf) {
        printf("Unable to open %s\n", wah_out);
        return 1;
    }

    //struct ubin_file uf = init_ubin_file(ubin_in);

    //write header for WAH bitmap index file
    fwrite(&(hdf5_f.num_vars), sizeof(int), 1, wf);
    fwrite(&(hdf5_f.num_inds), sizeof(int), 1, wf);
    int zero = 0;
    int k;
    for (k = 0; k < hdf5_f.num_inds*4; ++k)
        fwrite(&zero, sizeof(int), 1, wf);

    int num_ints_per_record = 1 + ((hdf5_f.num_vars - 1) / 16);
    int num_bytes_per_record = num_ints_per_record * 4;

    uint32_t *c = (uint32_t *)
        malloc(num_ints_per_record*sizeof(uint32_t));

    int i,j,wah_i = 0, offset_total  = 0;

    // skip to the target record and read in the full record
    //fseek(uf.file, uf.header_offset, SEEK_SET);

    for (i = 0; i < hdf5_f.num_inds; ++i) {
        //fread(c,sizeof(uint32_t),num_ints_per_record,uf.file);
        //int r = read_hdf5_r_gt(hdf5_f, i, c);
        int r = read_hdf5_r_gts(hdf5_f, i, c);
         
        uint32_t *wah;
        uint32_t *wah_sizes;
        uint32_t wah_len = ubin_to_bitmap_wah(c,
                                                  num_ints_per_record,
                                                  hdf5_f.num_vars,
                                                  &wah,
                                                  &wah_sizes);

        fseek(wf,sizeof(uint32_t)* (2+4* wah_i),  SEEK_SET);
        for (j = 0; j < 4; ++j) {
            offset_total += wah_sizes[j];
            fwrite(&offset_total, sizeof(uint32_t), 1, wf);
            //fprintf(stderr,"%u\t%u\n", wah_sizes[j], offset_total);
        }

        fseek(wf,0,SEEK_END);
        size_t ret = fwrite(wah, sizeof(uint32_t), wah_len, wf);
        if (ret != wah_len)
            fprintf(stderr, "ret:%zu != wah_len:%u\n", ret, wah_len);

        wah_i+=1;
        free(wah);
        free(wah_sizes);
    }

    free(c);

    fclose(wf);
    return 0;
}
//}}}
#endif

