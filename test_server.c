#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "libsllpserver/sllp_server.h"

uint8_t buf[SLLP_MAX_MESSAGE];
struct sllp_raw_packet response = { .data  = buf };

uint8_t query_status_buf[] = {0x00, 0x00};
struct sllp_raw_packet query_status = { .data = query_status_buf, .len = 2};

uint8_t query_vars_list_buf[] = {0x02, 0x00};
struct sllp_raw_packet query_vars_list = { .data = query_vars_list_buf, .len = 2};

uint8_t read_var_buf[] = {0x10, 0x01, 0x00};
struct sllp_raw_packet read_var = { .data = read_var_buf, .len = 3 };

void print_packet(struct sllp_raw_packet *packet)
{
	int i;
	printf("[");
	for(i = 0; i < packet->len; ++i)
		printf(" %2X", packet->data[i]);
	printf(" ]\n");
}

void execute_command(sllp_instance_t *sllp, struct sllp_raw_packet *request)
{
	printf(" Request: ");
	print_packet(request);
	sllp_process_packet(sllp, request, &response);
	printf("Response: ");
	print_packet(&response);
}

void hook(enum sllp_operation op, struct sllp_var **list);

int main(void)
{
	sllp_instance_t *sllp = sllp_new(SLLP_SLAVE);

	sllp_register_hook(sllp, hook);

	struct sllp_var digin, digout;

	digin.data = malloc(1);
	digin.size = 1;
	digin.user = "DIGIN";
	digin.writable = false;

	digout.data = malloc(1);
	digout.size = 1;
	digout.user = "DIGOUT";
	digout.writable = true;

	sllp_register_variable(sllp, &digin);
	sllp_register_variable(sllp, &digout);

	execute_command(sllp, &query_status);
	execute_command(sllp, &query_vars_list);

	execute_command(sllp, &read_var);

	return EXIT_SUCCESS;
}

void hook(enum sllp_operation op, struct sllp_var **list)
{
	while(*list)
	{
		uint8_t id = (*list)->id;
		char* type = (char*) (*list)->user;
		printf("Var type=%s id=%d\n", type, id);
		list++;
	}
}
