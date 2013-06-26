#include "sllp_list.h"
#include "sllp_server.h"

#include <stdlib.h>

static struct sllp_list_element *list_get_element (void* value)
{
    struct sllp_list_element *e = malloc(sizeof(*e));

    if(e)
    {
        e->next = NULL;
        e->value = value;
    }

    return e;
}

enum sllp_err sllp_list_init (struct sllp_list *list)
{
    if(!list)
        return SLLP_ERR_PARAM_INVALID;

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;

    return SLLP_SUCCESS;
}

enum sllp_err sllp_list_add (struct sllp_list *list, void *value)
{
    if(!list)
        return SLLP_ERR_PARAM_INVALID;

    bool contains;
    sllp_list_contains(list, value, &contains);

    if(contains)
        return SLLP_ERR_PARAM_OUT_OF_RANGE; // TODO: better error

    struct sllp_list_element *element = list_get_element(value);

    if(!element)
        return SLLP_ERR_OUT_OF_MEMORY;

    if(list->count)
        list->tail->next = element;
    else
        list->head = element;

    list->tail = element;
    ++list->count;

    return SLLP_SUCCESS;
}

enum sllp_err sllp_list_clear (struct sllp_list *list)
{
    if(!list)
        return SLLP_ERR_PARAM_INVALID;

    struct sllp_list_element *next, *current;
    
    current = next = list->head;

    while(current)
    {
        next = current->next;
        free(current);
        current = next;
    }

    sllp_list_init (list);

    return SLLP_SUCCESS;
}

enum sllp_err sllp_list_contains (struct sllp_list *list, void *value, bool *contains)
{
    if(!list || !contains)
        return SLLP_ERR_PARAM_INVALID;

    *contains = false;

    struct sllp_list_element *i;
    
    for(i = list->head; i; i=i->next)
        if(i->value == value)
        {
            *contains = true;
            break;
        }

    return SLLP_SUCCESS;
}

static enum sllp_err sllp_list_element_at(struct sllp_list *list,
                                          unsigned int pos,
                                          struct sllp_list_element **element)
{
    if(!list || !element)
        return SLLP_ERR_PARAM_INVALID;

    if(pos >= list->count)
        return SLLP_ERR_PARAM_OUT_OF_RANGE;

    *element = list->head;
    while(pos--)
        *element = (*element)->next;

    return SLLP_SUCCESS;
}

enum sllp_err sllp_list_value_at (struct sllp_list *list, unsigned int pos,
                                  void **value)
{
    enum sllp_err err;
    struct sllp_list_element *e;

    if((err = sllp_list_element_at(list, pos, &e)))
        return err;
    
    *value = e->value;

    return SLLP_SUCCESS;
}

enum sllp_err sllp_list_trim (struct sllp_list *list, unsigned int first)
{
    if(!list)
        return SLLP_ERR_PARAM_INVALID;

    // To trim from the first element is equivalent to clear the whole list
    if(!first)
        return sllp_list_clear(list);

    // Make a list of all the elements to be wiped
    struct sllp_list temp;
    sllp_list_init(&temp);

    if(sllp_list_element_at(list, first, &temp.head))
        return SLLP_ERR_PARAM_OUT_OF_RANGE;

    temp.tail = list->tail;

    // Trim the list : fix tail
    sllp_list_element_at(list, first - 1, &list->tail);
    // Trim the list : fix count
    list->count = first;

    // Wipe elements
    sllp_list_clear(&temp);

    return SLLP_SUCCESS;
}

enum sllp_err sllp_list_copy_to_vector (struct sllp_list *list, void **vector)
{
    if(!list || !vector)
        return SLLP_ERR_PARAM_INVALID;

    int count = list->count;
    struct sllp_list_element *e = list->head;
    
    while(count--)
    {
        *(vector++) = e->value;
        e = e->next;
    }

    vector[list->count] = NULL;

    return SLLP_SUCCESS;
}