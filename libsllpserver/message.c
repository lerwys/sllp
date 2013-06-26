#include "common.h"
#include "message.h"
#include "md5/md5.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define HEADER_LEN 2

#define WRITABLE 0x80
#define READ_ONLY 0x00

enum command_code
{
    CMD_QUERY_STATUS = 0x00,
    CMD_STATUS,
    CMD_QUERY_VARS_LIST,
    CMD_VARS_LIST,
    CMD_QUERY_GROUPS_LIST,
    CMD_GROUPS_LIST,
    CMD_QUERY_GROUP,
    CMD_GROUP,
    CMD_QUERY_CURVES_LIST,
    CMD_CURVES_LIST,

    CMD_READ_VAR = 0x10,
    CMD_VAR_READING,
    CMD_READ_GROUP,
    CMD_GROUP_READING,

    CMD_WRITE_VAR = 0x20,
    CMD_WRITE_GROUP = 0x22,

    CMD_CREATE_GROUP = 0x30,
    CMD_GROUP_CREATED,
    CMD_REMOVE_ALL_GROUPS,

    CMD_CURVE_TRANSMIT = 0x40,
    CMD_CURVE_BLOCK,
    CMD_CURVE_RECALC_CSUM,

    CMD_OK = 0xE0,
    CMD_ERR_MALFORMED_MESSAGE,
    CMD_ERR_OP_NOT_SUPPORTED,
    CMD_ERR_INVALID_ID,
    CMD_ERR_INVALID_VALUE,
    CMD_ERR_INVALID_PAYLOAD_SIZE,
    CMD_ERR_READ_ONLY,
    CMD_ERR_INSUFFICIENT_MEMORY,
    CMD_ERR_INTERNAL,

    CMD_MAX
};

struct raw_message
{
    uint8_t command_code;
    uint8_t encoded_size;
    uint8_t payload[];
}__attribute__((packed));

struct message
{
    enum command_code command_code;
    uint16_t payload_size;
    uint8_t *payload;
};

// <editor-fold defaultstate="collapsed" desc="Auxiliary functions">
static enum sllp_err message_process(sllp_instance_t *sllp,
                                     struct message *recv_msg,
                                     struct message *send_msg);

static enum sllp_err message_set_answer(struct message *msg,
                                        enum command_code code);

static bool is_payload_size_equal_to(struct message *msg,struct message *answer,
                                     uint16_t size, bool greater_or_equal);

static uint16_t decode_size(uint8_t size);
static uint8_t encode_size(uint16_t size);
static bool is_size_ok(uint16_t packet_size, uint16_t payload_size);
// </editor-fold>

enum sllp_err packet_process (sllp_instance_t *sllp,
                              struct sllp_raw_packet *recv_pkt,
                              struct sllp_raw_packet *send_pkt)
{
    if(!sllp || !recv_pkt || !send_pkt)
        return SLLP_ERR_PARAM_INVALID;

    // Interpret packet payload as a message
    struct raw_message *recv_raw_msg = (struct raw_message *) recv_pkt->data;
    struct raw_message *send_raw_msg = (struct raw_message *) send_pkt->data;

    // Create proper messages from the raw messages
    struct message recv_msg, send_msg;
   
    recv_msg.command_code = (enum command_code) recv_raw_msg->command_code;
    recv_msg.payload_size = decode_size(recv_raw_msg->encoded_size);
    recv_msg.payload      = recv_raw_msg->payload;    

    send_msg.payload      = send_raw_msg->payload;

    // Check inconsistency between the size of the received data and the size
    // specified in the message header
    if(!is_size_ok(recv_pkt->len, recv_msg.payload_size))
        message_set_answer(&send_msg, CMD_ERR_MALFORMED_MESSAGE);
    else
        message_process(sllp, &recv_msg, &send_msg);

    send_raw_msg->command_code = send_msg.command_code;
    send_raw_msg->encoded_size = encode_size(send_msg.payload_size);
    send_pkt->len = send_msg.payload_size + 2;

    return SLLP_SUCCESS;
}

static enum sllp_err message_process(sllp_instance_t *sllp,
                                     struct message *recv_msg,
                                     struct message *send_msg)
{
    if(!recv_msg || !send_msg)
        return SLLP_ERR_PARAM_INVALID;

    sllp->modified_list[0] = NULL;

    switch(recv_msg->command_code)
    {

    /*case CMD_QUERY_STATUS:          //Answer with CMD_STATUS
     
        // Set answer code
        message_set_answer(send_msg, CMD_ERR_OP_NOT_SUPPORTED);
        break;*/

    case CMD_QUERY_VARS_LIST:       // Answer with CMD_VARS_LIST
    {
        // Check payload size
        if(!is_payload_size_equal_to(recv_msg, send_msg, 0, false))
            break;

        // Set answer's command_code and payload_size
        message_set_answer(send_msg, CMD_VARS_LIST);

        // Variables are in order of their ID's
        struct sllp_var *var = NULL;

        int i;
        for(i = 0; i < sllp->vars_list.count; ++i)
        {
            sllp_list_value_at(&sllp->vars_list, i, (void**) &var);
            send_msg->payload[i] = (*var).writable ? WRITABLE : READ_ONLY;
            send_msg->payload[i] += var->size;
        }
        send_msg->payload_size = sllp->vars_list.count;
        break;
    }
    
    case CMD_QUERY_GROUPS_LIST:     // Answer with CMD_GROUPS_LIST        
    {
        // Check payload size
        if(!is_payload_size_equal_to(recv_msg, send_msg, 0, false))
            break;

        // Set answer's command_code and payload_size
        message_set_answer(send_msg, CMD_GROUPS_LIST);

        // Iterate
        struct sllp_group *grp;

        int i;
        for(i = 0; i < sllp->groups_list.count; ++i)
        {
            sllp_list_value_at(&sllp->groups_list, i, (void**) &grp);
            send_msg->payload[i] = grp->writable ? WRITABLE : READ_ONLY;
            send_msg->payload[i] += grp->vars_list.count;
        }
        send_msg->payload_size = sllp->groups_list.count;
        break;
    }
    
    case CMD_QUERY_GROUP:           // Answer with CMD_GROUP
    {
        // Check payload size
        if(!is_payload_size_equal_to(recv_msg, send_msg, 1, false))
            break;

        // Set answer code
        message_set_answer(send_msg, CMD_GROUP);

        // Get desired group
        struct sllp_group *grp;
        
        if(sllp_list_value_at(&sllp->groups_list, recv_msg->payload[0],
                              (void**) &grp))
        {
            message_set_answer(send_msg, CMD_ERR_INVALID_ID);
            break;
        }

        // Iterate over group's variables
        struct sllp_var *var;

        int i;
        for(i = 0; i < grp->vars_list.count; ++i)
        {
            sllp_list_value_at(&grp->vars_list, i, (void**) &var);
            send_msg->payload[i] = var->id;
        }
        
        send_msg->payload_size = grp->vars_list.count;
        break;
    }

    case CMD_QUERY_CURVES_LIST:
    {
        if(!is_payload_size_equal_to(recv_msg, send_msg, 0, false))
            break;

        message_set_answer(send_msg, CMD_CURVES_LIST);

        struct sllp_curve *curve;
        uint8_t *payloadp = send_msg->payload;
        int i;
        for(i = 0; i < sllp->curves_list.count; ++i)
        {
            sllp_list_value_at(&sllp->curves_list, i, (void** )&curve);

            (*payloadp++) = curve->writable;
            (*payloadp++) = curve->nblocks;
            memcpy(payloadp, curve->checksum, sizeof(curve->checksum));
            payloadp += sizeof(curve->checksum);
        }
        send_msg->payload_size = sllp->curves_list.count*CURVE_INFO_SIZE;

        break;
    }

    case CMD_READ_VAR:          // Answer with CMD_VAR_READING
    {
        // Check payload size
        if(!is_payload_size_equal_to(recv_msg, send_msg, 1, false))
            break;

        // Set answer code
        message_set_answer(send_msg, CMD_VAR_READING);

        // Get desired variable
        struct sllp_var *var;
        if(sllp_list_value_at(&sllp->vars_list, recv_msg->payload[0],
                             (void**) &var))
        {
            message_set_answer(send_msg, CMD_ERR_INVALID_ID);
            break;
        }

        if(sllp->hook)
        {
            sllp->modified_list[0] = var;
            sllp->modified_list[1] = NULL;
            sllp->hook(SLLP_OP_READ, sllp->modified_list);
        }

        send_msg->payload_size = var->size;
        memcpy(send_msg->payload, var->data, var->size);

        break;
    }

    case CMD_READ_GROUP:
    {
        // Check payload size
        if(!is_payload_size_equal_to(recv_msg, send_msg, 1, false))
            break;

        // Set answer code
        message_set_answer(send_msg, CMD_GROUP_READING);

        // Get desired group
        struct sllp_group *grp;

        if(sllp_list_value_at(&sllp->groups_list, recv_msg->payload[0],
                               (void**) &grp))
        {
            message_set_answer(send_msg, CMD_ERR_INVALID_ID);
            break;
        }

        // Call hook
        if(sllp->hook)
        {
            sllp_list_copy_to_vector(&grp->vars_list,
                                     (void**) sllp->modified_list);
            sllp->hook(SLLP_OP_READ, sllp->modified_list);
        }

        // Iterate over group's variables
        struct sllp_var *var;
        uint8_t *payloadp = send_msg->payload;
        int i = 0;        

        while(!sllp_list_value_at(&grp->vars_list, i, (void**) &var))
        {
            memcpy(payloadp, var->data, var->size);
            payloadp += var->size;
            ++i;
        }
        send_msg->payload_size = grp->data_size;

        break;
    }
    
    case CMD_WRITE_VAR:
    {
        // Check to see if body has at least two bytes (one for id, at least one
        // for the value)
        if(!is_payload_size_equal_to(recv_msg, send_msg, 2, true))
            break;

        // Set answer code
        message_set_answer(send_msg, CMD_OK);

        // Check ID
        struct sllp_var *var;
        if(sllp_list_value_at(&sllp->vars_list, recv_msg->payload[0],
                               (void**) &var))
        {
            message_set_answer(send_msg, CMD_ERR_INVALID_ID);
            break;
        }

        // Check payload size
        if(!is_payload_size_equal_to(recv_msg, send_msg, var->size + 1, false))
            break;

        // Check write permission
        if(!var->writable)
        {
            message_set_answer(send_msg, CMD_ERR_READ_ONLY);
            break;
        }

        // Everything is OK, perform the write operation
        memcpy(var->data, recv_msg->payload + 1, var->size);

        // Call hook
        if(sllp->hook)
        {
            sllp->modified_list[0] = var;
            sllp->modified_list[1] = NULL;
            sllp->hook(SLLP_OP_WRITE, sllp->modified_list);
        }

        break;
    }

    case CMD_WRITE_GROUP:
    {
        // Check to see if body has at least two bytes (one for id, at least one
        // for the values)
        if(!is_payload_size_equal_to(recv_msg, send_msg, 2, true))
            break;

        // Check ID
        struct sllp_group *grp;
        if(sllp_list_value_at(&sllp->groups_list,
                               (unsigned int) recv_msg->payload, (void**) &grp))
        {
            message_set_answer(send_msg, CMD_ERR_INVALID_ID);
            break;
        }

        // Check payload size
        if(!is_payload_size_equal_to(recv_msg, send_msg, grp->data_size + 1,
                                     false))
            break;

        // Check write permission
        if(!grp->writable)
        {
            message_set_answer(send_msg, CMD_ERR_READ_ONLY);
            break;
        }

        // Everything is OK, iterate
        int i = 0;
        struct sllp_var *var;
        uint8_t *payloadp = recv_msg->payload + 1;

        while(!sllp_list_value_at(&grp->vars_list, i, (void**) &var))
        {
            memcpy(var->data, payloadp, var->size);
            payloadp += var->size;
            ++i;
        }

        // Call hook
        if(sllp->hook)
        {
            sllp_list_copy_to_vector(&grp->vars_list,
                                     (void**) sllp->modified_list);
            sllp->hook(SLLP_OP_WRITE, sllp->modified_list);
        }

        break;
    }

    case CMD_CREATE_GROUP:
    {
        // Check if there's at least one variable to put on the group
        if(!is_payload_size_equal_to(recv_msg, send_msg, 1, true))
            break;

        if(is_payload_size_equal_to(recv_msg, send_msg, 
                                    sllp->vars_list.count + 1, true))
            break;

        if(sllp->groups_list.count == MAX_GROUPS)
        {
            message_set_answer(send_msg, CMD_ERR_INSUFFICIENT_MEMORY);
            break;
        }

        message_set_answer(send_msg, CMD_GROUP_CREATED);

        // Allocate group structure
        struct sllp_group *grp;
        grp = malloc(sizeof(*grp));

        if(!grp)
        {
            message_set_answer(send_msg, CMD_ERR_INSUFFICIENT_MEMORY);
            break;
        }        

        // Initialize group
        group_init(grp, sllp->groups_list.count, true);

        // Populate group
        int i;
        for(i = 0; i < recv_msg->payload_size; ++i)
        {
            struct sllp_var *var;
            if(!sllp_list_value_at(&sllp->vars_list, recv_msg->payload[i],
                                  (void**) &var ))
            {
                message_set_answer(send_msg, CMD_ERR_INVALID_ID);
                goto cmd_group_create_err;                
            }

            if(!sllp_list_add(&grp->vars_list, (void*) var))
            {
                message_set_answer(send_msg, CMD_ERR_INSUFFICIENT_MEMORY);
                goto cmd_group_create_err; 
            }

            grp->writable = grp->writable && var->writable;
            grp->data_size += var->size;
        }

        message_set_answer(send_msg, CMD_GROUP_CREATED);
        send_msg->payload_size = 1;
        send_msg->payload[0] = grp->writable ? WRITABLE : READ_ONLY;
        send_msg->payload[0] += grp->id;
        break;

cmd_group_create_err:
        sllp_list_clear(&grp->vars_list);
        free(grp);
        break;
    }

    case CMD_REMOVE_ALL_GROUPS:
    {
        if(!is_payload_size_equal_to(recv_msg, send_msg, 0, false))
            break;

        message_set_answer(send_msg, CMD_OK);
        
        sllp_list_trim(&sllp->groups_list, GROUP_STANDARD_COUNT);

        break;
    }

    case CMD_CURVE_TRANSMIT:
    {
        if(!is_payload_size_equal_to(recv_msg, send_msg, 2, false))
            break;

        struct sllp_curve *curve;
        if(sllp_list_value_at(&sllp->curves_list, recv_msg->payload[0],
                               (void**) &curve))
        {
            message_set_answer(send_msg, CMD_ERR_INVALID_ID);
            break;
        }

        uint8_t block_offset = recv_msg->payload[1];
        
        if(block_offset > curve->nblocks)
        {
            message_set_answer(send_msg, CMD_ERR_INVALID_VALUE);
            break;
        }
        
        message_set_answer(send_msg, CMD_CURVE_BLOCK);
        send_msg->payload[0] = curve->id;
        send_msg->payload[1] = block_offset;

        curve->read_block(curve, block_offset, send_msg->payload + 2);
        send_msg->payload_size = 2 + CURVE_BLOCK_DATA_SIZE;
        break;
    }

    case CMD_CURVE_BLOCK:
    {
        if(!is_payload_size_equal_to(recv_msg, send_msg,
                                     2 + CURVE_BLOCK_DATA_SIZE, false))
            break;

        uint8_t id = recv_msg->payload[0];
        struct sllp_curve *curve;
        if(sllp_list_value_at(&sllp->curves_list, id, (void**) &curve))
        {
            message_set_answer(send_msg, CMD_ERR_INVALID_ID);
            break;
        }

        uint8_t block_offset = recv_msg->payload[1];
        if(block_offset > curve->nblocks)
        {
            message_set_answer(send_msg, CMD_ERR_INVALID_VALUE);
            break;
        }

        curve->write_block(curve, block_offset, recv_msg->payload + 2);
        
        message_set_answer(send_msg, CMD_OK);
        break;
    }
    
    case CMD_CURVE_RECALC_CSUM:
    {
        if(!is_payload_size_equal_to(recv_msg, send_msg, 1, false))
            break;

        uint8_t id = recv_msg->payload[0];
        struct sllp_curve *curve;
        if(!sllp_list_value_at(&sllp->curves_list, id, (void**) &curve))
        {
            message_set_answer(send_msg, CMD_ERR_INVALID_ID);
            break;
        }

        unsigned int nblocks = curve->nblocks + 1;
        uint8_t block[CURVE_BLOCK_DATA_SIZE];
        MD5_CTX md5ctx;

        MD5Init(&md5ctx);

        unsigned int i;
        for(i = 0; i < nblocks; ++i)
        {
            curve->read_block(curve, (uint8_t)i, block);
            MD5Update(&md5ctx, block, CURVE_BLOCK_DATA_SIZE);
        }
        MD5Final(curve->checksum, &md5ctx);

        message_set_answer(send_msg, CMD_OK);
        break;
    }
        
    default:
        message_set_answer(send_msg, CMD_ERR_OP_NOT_SUPPORTED);
        break;

    }

    return SLLP_SUCCESS;
}

static enum sllp_err message_set_answer (struct message *msg, 
                                         enum command_code code)
{
    if(!msg)
        return SLLP_ERR_PARAM_INVALID;

    msg->command_code = code;
    msg->payload_size = 0;

    return SLLP_SUCCESS;
}

static bool is_payload_size_equal_to (struct message *msg,
                                      struct message *answer, uint16_t size,
                                      bool greater_or_equal)
{
    if(greater_or_equal)
    {
        if(msg->payload_size >= size)
            return true;
    }
    else
    {
        if(msg->payload_size == size)
            return true;
    }

    message_set_answer(answer, CMD_ERR_INVALID_PAYLOAD_SIZE);
    return false;
}

static uint16_t decode_size(uint8_t size)
{
    if(size < 0x80)
        return size;

    size &= 0x7F;

    return 128*size + 130;
}

static uint8_t encode_size(uint16_t size)
{
    if(size < 0x80)
        return size;

    size -= 130;

    return size/128 + (size%128 != 0);
}

static bool is_size_ok(uint16_t packet_size, uint16_t payload_size)
{
    if(packet_size < HEADER_LEN)
        return false;
    else if(packet_size - HEADER_LEN != payload_size)
        return false;
    return true;
}