#ifndef SLLP_LIST_H
#define	SLLP_LIST_H

#include <stdbool.h>

struct sllp_list_element
{
    void *value;
    struct sllp_list_element *next;
};

struct sllp_list
{
    struct sllp_list_element *head;
    struct sllp_list_element *tail;
    unsigned int count;
};

enum sllp_err sllp_list_init      (struct sllp_list *list);
enum sllp_err sllp_list_add       (struct sllp_list *list, void *value);
enum sllp_err sllp_list_clear     (struct sllp_list *list);
enum sllp_err sllp_list_contains  (struct sllp_list *list, void *value,
                                   bool *contains);
enum sllp_err sllp_list_value_at  (struct sllp_list *list, unsigned int pos,
                                   void **value);
enum sllp_err sllp_list_trim      (struct sllp_list *list, unsigned int first);
enum sllp_err sllp_list_copy_to_vector (struct sllp_list *list, void **vector);


#endif	/* LIST_H */

