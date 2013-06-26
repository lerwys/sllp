#include "common.h"
#include "sllp_server.h"

enum sllp_err group_init (struct sllp_group *group, uint8_t id, bool writable)
{
    if(!group)
        return SLLP_ERR_PARAM_INVALID;

    group->id = id;
    group->writable = writable;
    group->data_size = 0;
    sllp_list_init(&group->vars_list);

    return SLLP_SUCCESS;
}
