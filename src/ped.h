#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <sqlite3.h>
#include <sys/stat.h>

struct uint32_t_ll_node {
    uint32_t v;
    struct uint32_t_ll_node *next;
};

struct uint32_t_ll {
    struct uint32_t_ll_node *head, *tail;
    uint32_t len;
};

static int callback(void *ll_p,
                    int argc,
                    char **argv,
                    char **col_name);

uint32_t convert_file_by_name_ped_to_db(char *bcf_file_name,
                                        char *ped_file_name,
                                        uint32_t col,
                                        char *db_name);

uint32_t resolve_ind_query(uint32_t **R, char *query, char *ped_db_file);
