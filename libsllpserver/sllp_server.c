#include "sllp_server.h"
#include "sllp_list.h"
#include "common.h"
#include "message.h"

#include <stdlib.h>
#include <string.h>

sllp_instance_t *sllp_new (void)
{
    struct sllp_instance *sllp = malloc(sizeof(*sllp));

    if(!sllp)
        return NULL;
    
    sllp_list_init (&sllp->vars_list);
    sllp_list_init (&sllp->groups_list);
    sllp_list_init (&sllp->curves_list);

    group_init(&sllp->group_all, GROUP_ALL_ID, false);
    group_init(&sllp->group_read, GROUP_READ_ID, false);
    group_init(&sllp->group_write, GROUP_WRITE_ID, true);

    sllp_list_add (&sllp->groups_list, (void*) &sllp->group_all);
    sllp_list_add (&sllp->groups_list, (void*) &sllp->group_read);
    sllp_list_add (&sllp->groups_list, (void*) &sllp->group_write);

    memset(sllp->modified_list, 0, sizeof(sllp->modified_list));

    return sllp;
}

enum sllp_err sllp_destroy (sllp_instance_t* sllp)
{
    if(!sllp)
        return SLLP_ERR_PARAM_INVALID;

    struct sllp_list_element *g;
    for(g = sllp->groups_list.head; g; g = g->next)
        sllp_list_clear(&((struct sllp_group *) g->value)->vars_list);
    
    sllp_list_clear (&sllp->vars_list);
    sllp_list_clear (&sllp->groups_list);
    sllp_list_clear (&sllp->curves_list);

    free(sllp);

    return SLLP_SUCCESS;
}


enum sllp_err sllp_register_variable (sllp_instance_t *sllp,
                                      struct sllp_var *var)
{
    if(!sllp || !var)
        return SLLP_ERR_PARAM_INVALID;

    // Check variable fields
    if(var->size < VARIABLE_MIN_SIZE || var->size > VARIABLE_MAX_SIZE)
        return SLLP_ERR_PARAM_OUT_OF_RANGE;

    if(!var->data)
        return SLLP_ERR_PARAM_INVALID;

    // Check vars limit
    if(sllp->vars_list.count == MAX_VARIABLES)
        return SLLP_ERR_OUT_OF_MEMORY; // TODO: improve error code?

    // Add to the variables list
    if(sllp_list_add(&sllp->vars_list, (void*) var))
        return SLLP_ERR_OUT_OF_MEMORY;

    // Adjust var id
    var->id = sllp->vars_list.count - 1;

    // Add to the group containing all variables
    if(sllp_list_add(&sllp->group_all.vars_list, (void*) var))
        return SLLP_ERR_OUT_OF_MEMORY;

    sllp->group_all.data_size += var->size;

    // Add either to the WRITABLE or to the READ_ONLY group
    struct sllp_group *g;
    g = var->writable ? &sllp->group_write : &sllp->group_read;

    if(sllp_list_add(&g->vars_list, (void*) var))
        return SLLP_ERR_OUT_OF_MEMORY;

    g->data_size += var->size;

    return SLLP_SUCCESS;
}

enum sllp_err sllp_register_curve (sllp_instance_t *sllp,
                                   struct sllp_curve *curve)
{
    if(!sllp || !curve)
        return SLLP_ERR_PARAM_INVALID;

    // Check variable fields
    if(!curve->read_block)
        return SLLP_ERR_PARAM_INVALID;

    if(curve->writable && !curve->write_block)
        return SLLP_ERR_PARAM_INVALID;
    else if(!curve->writable && curve->write_block)
        return SLLP_ERR_PARAM_INVALID;

    // Check vars limit
    if(sllp->curves_list.count == MAX_CURVES)
        return SLLP_ERR_OUT_OF_MEMORY;

    // Add to the curves list
    if(sllp_list_add(&sllp->curves_list, (void*) curve))
        return SLLP_ERR_OUT_OF_MEMORY;

    // Adjust var id
    curve->id = sllp->curves_list.count - 1;

    return SLLP_SUCCESS;
}

enum sllp_err sllp_process_packet (sllp_instance_t *sllp,
                                    struct sllp_raw_packet *request,
                                    struct sllp_raw_packet *response)
{
    if(!sllp || !request || !response)
        return SLLP_ERR_PARAM_INVALID;

    return packet_process(sllp, request, response);
}

enum sllp_err sllp_register_hook(sllp_instance_t* sllp, sllp_hook_t hook)
{
    if(!sllp || !hook)
        return SLLP_ERR_PARAM_INVALID;

    sllp->hook = hook;

    return SLLP_SUCCESS;
}
