#ifndef MESSAGE_H
#define	MESSAGE_H

#include <stdint.h>
#include <stdbool.h>

#include "sllp_server.h"

/**
 * Interprets a message and execute its command, preparing an answer for it.
 *
 * @param sllp [input] SLLP lib instance to be manipulated
 * @param recv_pkt [input] The received packet.
 * @param send_pkt [output] The packet to be sent back.
 * @param modified_list [output] NULL-terminated list of pointers to modified
 *                               variables.
 * 
 * @return SLLP_SUCCESS or one of the following errors:
 * <ul>
 *   <li>SLLP_ERR_PARAM_INVALID: either sllp, recv_pkt, send_pkt or 
 *                               modified_list is a NULL pointer.</li>
 * </ul>
 */
enum sllp_err packet_process (sllp_instance_t *sllp,
                              struct sllp_raw_packet *recv_pkt,
                              struct sllp_raw_packet *send_pkt);

#endif	/* COMMAND_H */

