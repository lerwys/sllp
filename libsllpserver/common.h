#ifndef COMMON_H
#define	COMMON_H

#include <stdbool.h>
#include <stdint.h>

#include "sllp_server.h"
#include "sllp_list.h"

#define VARIABLE_MIN_SIZE 1u
#define VARIABLE_MAX_SIZE 127u

#define CURVE_INFO_SIZE 18u
#define CURVE_CSUM_SIZE 16
#define CURVE_BLOCK_DATA_SIZE 16384

#define MAX_VARIABLES 128u
#define MAX_GROUPS 128u
#define MAX_CURVES 128u

struct sllp_group
{
    uint8_t          id;            // ID of the group, used in the protocol.
    bool             writable;      // Determine if the group is writable.
    uint8_t          data_size;     // How many bytes all variable's values
                                    // amount to.
    struct sllp_list vars_list;     // List of the variables contained in the
                                    // group.
};

struct sllp_instance
{
    struct sllp_list vars_list, groups_list, curves_list;
    struct sllp_group group_all, group_read, group_write;
    struct sllp_var *modified_list[MAX_VARIABLES+1];
    sllp_hook_t hook;
};

enum group_id
{
    GROUP_ALL_ID,
    GROUP_READ_ID,
    GROUP_WRITE_ID,

    GROUP_STANDARD_COUNT,
};

enum sllp_err group_init (struct sllp_group *group, uint8_t id, bool writable);


#endif	/* COMMON_H */

