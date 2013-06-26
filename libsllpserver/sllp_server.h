/* 
 * Sirius Low Level Control Protocol Server Library API
 * Version 0.1
 * Bruno Martins
 * CON - Controls Group
 * LNLS - Brazilian Synchrotron Light Laboratory 
 */

#ifndef SLLP_SERVER_H
#define	SLLP_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#define SLLP_MAX_MESSAGE 16386

enum sllp_operation
{
    SLLP_OP_READ,                   // Read command arrived
    SLLP_OP_WRITE,                  // Write command arrived
};

enum sllp_err
{
    SLLP_SUCCESS,                   // Operation executed successfully
    SLLP_ERR_PARAM_INVALID,         // An invalid parameter was passed
    SLLP_ERR_PARAM_OUT_OF_RANGE,    // A param not in the acceptable range was
                                    // passed
    SLLP_ERR_OUT_OF_MEMORY,         // Not enough memory to complete operation
    
    SLLP_ERR_MAX
};

typedef struct sllp_instance sllp_instance_t;   // Type of the sllp handle

struct sllp_var
{
    uint8_t id;                     // ID of the variable, used in the protocol.
    bool    writable;               // Determine if the variable is writable.
    uint8_t size;                   // Indicates how many bytes 'data' contains.
    uint8_t *data;                  // Pointer to the value of the variable.

    void    *user;                  // The user can make use of this variable as
                                    // he wishes. It is not touched by SLLP.
};

struct sllp_curve
{
    uint8_t id;                     // ID of the curve, used in the protocol.
    bool    writable;               // Determine if the curve is writable.
    uint8_t nblocks;                // How many 16kB blocks the curve contains.
    uint8_t checksum[16];           // MD5 checksum of the curve

    // Read a 16384 bytes block into data
    void (*read_block) (struct sllp_curve *curve, uint8_t block, uint8_t *data);

    // Write a 16384 bytes block from data
    void (*write_block)(struct sllp_curve *curve, uint8_t block, uint8_t *data);

    void    *user;                  // The user can make use of this variable as
                                    // he wishes. It is not touched by SLLP.
};

struct sllp_raw_packet
{
    uint8_t *data;
    uint16_t len;
};

typedef void (*sllp_hook_t) (enum sllp_operation op, struct sllp_var **list);

/**
 * Allocate a new SLLP instance, returning a handle to it. This instance should
 * be deallocated with sllp_destroy after its use.
 *
 * @param role [input] The role that this instance will perform.
 *
 * @return A handle to an instance of the SLLP lib or NULL if there wasn't
 *         enough memory to do the allocation.
 */
sllp_instance_t *sllp_new (void);

/**
 * Deallocate a SLLP instance
 * 
 * @param sllp [input] Handle to the instance to be deallocated.
 * 
 * @return SLLP_SUCCESS or one of the following errors:
 * <ul>
 *   <li>SLLP_ERR_PARAM_INVALID: sllp is a NULL pointer. </li>
 * </ul>
 */
enum sllp_err sllp_destroy (sllp_instance_t *sllp);

/**
 * Register a variable with a SLLP instance. The memory pointed by the var
 * parameter must remain valid throughout the entire lifespan of the sllp
 * instance. The id field of the var parameter will be written by the SLLP lib.
 *
 * The fields writable, size and data of the var parameter must be filled
 * correctly.
 *
 * The user field is untouched.
 *
 * @param sllp [input] Handle to the instance.
 * @param var [input] Structure describing the variable to be registered.
 *
 * @return SLLP_SUCCESS or one of the following errors:
 * <ul>
 *   <li> SLLP_ERR_PARAM_INVALID: sllp, data or id is a NULL pointer. </li>
 *   <li> SLLP_PARAM_OUT_OF_RANGE: size is less than SLLP_VAR_SIZE_MIN or
 *                                 greater than SLLP_VAR_SIZE_MAX. </li>
 * </ul>
 */
enum sllp_err sllp_register_variable (sllp_instance_t *sllp,
                                      struct sllp_var *var);

/**
 * Register a curve with a SLLP instance. The memory pointed by te curve
 * parameter must remain valid throughout the entire lifespan of the sllp
 * instance. The id field of the curve parameter will be written by the SLLP
 * lib.
 *
 * The fields writable, nblocks and read_block must be filled correctly.
 * If writable is true, the field write_block must also be filled correctly.
 * Otherwise, write_block must be NULL.
 *
 * The user field is untouched.
 *
 * @param sllp [input] Handle to the SLLP instance.
 * @param curve [input] Poiner to the curve to be registered.
 *
 * @return SLLP_SUCCESS or one of the following errors:
 * <ul>
 *   <li>SLLP_ERR_PARAM_INVALID: either sllp or curve is a NULL pointer.</li>
 *   <li>SLLP_ERR_PARAM_INVALID: curve->read_block is NULL.</li>
 *   <li>SLLP_ERR_PARAM_INVALID: curve->writable is true and curve->write_block
 *                               is NULL.</li>
 *   <li>SLLP_ERR_PARAM_INVALID: curve->writable is false and curve->write_block
 *                               is not NULL.</li>
 * </ul>
 */
enum sllp_err sllp_register_curve (sllp_instance_t *sllp,
                                   struct sllp_curve *curve);

/**
 * Register a function that will be called in two moments:
 *
 * 1. When a command that reads one or more variables arrives, the hook function
 * is called right BEFORE reading the variables and putting the read values in
 * the response message.
 *
 * 2. When a command that writes to one or more variables arrives, the hook
 * function is called AFTER writing the values of the variables.
 *
 * Other commands don't call the hook function. It's not required to register a
 * hook function. It's possible to deregister a previously registered hook
 * function by simply passing a NULL pointer in the hook parameter.
 *
 * A hook function receives two parameters: the first one indicating the type
 * of operation being performed (specified in enum sllp_operation) and the
 * second one containing the list of variables being affected by that operation.
 *
 * @param sllp [input] Handle to a SLLP instance.
 * @param hook [input] Hook function
 *
 * @return SLLP_SUCCESS or one of the following errors:
 * <ul>
 *   <li> SLLP_ERR_INVALID_PARAM: sllp is a NULL pointer.
 * </ul>
 */
enum sllp_err sllp_register_hook (sllp_instance_t *sllp, sllp_hook_t hook);

/**
 * Process a received message and prepare an answer.
 *
 * @param sllp [input] Handle to a SLLP instance.
 * @param request [input] The message to be processed.
 * @param response [output] The answer to be sent
 *
 * @return SLLP_SUCCESS or one of the following errors:
 * <ul>
 *   <li> SSLP_ERR_PARAM_INVALID: Either sllp, request, or response is
 *                                a NULL pointer.</li>
 * </ul>
 */
enum sllp_err sllp_process_packet (sllp_instance_t *sllp,
                                   struct sllp_raw_packet *request,
                                   struct sllp_raw_packet *response);

#endif

