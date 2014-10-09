#ifndef __H5_H__
#define __H5_H__

#include <hdf5.h>
#include "genotq.h"

struct hdf5_file
{
    hid_t file_id, plist_id, gt_dataspace_id, md_dataspace_id;
    uint32_t num_gt_ints;
};

struct hdf5_file init_hdf5_file(char *hdf5_file_name,
                                uint32_t num_vars);

int write_hdf5_gt(struct hdf5_file hdf5_f,
                  uint32_t id,
                  uint32_t *gt,
                  char *md);


int read_hdf5_gt(struct hdf5_file hdf5_f, uint32_t id, uint32_t *gt);

int read_hdf5_md(struct hdf5_file hdf5_f, uint32_t id, char **md);

int close_hdf5_file(struct hdf5_file hdf5_f);

#endif