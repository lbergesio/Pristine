//
//  securechannel.h
//  SecureChannel
//
//  Created by Ondrej Rysavy on 21/12/14.
//  Copyright (c) 2014 Brno University of Technology. All rights reserved.
//

#ifndef SecureChannel_securechannel_h
#define SecureChannel_securechannel_h
#include <stdint.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/err.h>
#include <openssl/rand.h>


#define TRUE 1
#define FALSE 0
// ERROR CODES:
#define S_OK 0
#define E_FAIL 0xffffffff
#define E_FILE_NOT_FOUND -4
#define E_CANNOT_PARSE_PROFILE -1
#define E_CANNOT_FIND_CIPHER -2
#define E_CANNOT_FIND_MD -3
#define E_INVALID_CIPHER_CTX -1


// LIMITS:
#define CIPHER_NAME_MAXLEN 64
// Defines the max length of symmetric keys, which is actually set to 512 bits
#define KEY_MAXLEN 64


typedef struct {
    const EVP_CIPHER *enc_cipher;
    const EVP_MD* mac_digest;
    char key[KEY_MAXLEN];
    // Lifetime of write keys in seconds.
    uint32_t key_lifetime;
    // Maximum number of block to be encoded with a single write key.
    uint32_t block_limit;
} SC_PROFILE ;

typedef struct _SC_CTX
{
    // Id of the context
    int context_id;
    // Profile from which this context were generated.
    SC_PROFILE profile;
    // Encryption key.
    char enckey[KEY_MAXLEN];
    // Digest message key.
    char mackey[KEY_MAXLEN];
    // Sequence key.
    char seqkey[KEY_MAXLEN];
    // Cipher context.
    EVP_CIPHER_CTX *cipher_ctx;
    // Digest context.
    EVP_MD_CTX *digest_ctx;
    // Number of bytes processed by this context.
    uint64_t bytes;
    // Number of block processed by this context.
    uint64_t blocks;
    // time when this context was created
    time_t created;
} SC_CTX;


#define SC_SDU_TYPE_PLAIN 0
#define SC_SDU_TYPE_SECURED 1

#define SC_SDU_type_t uint16_t
#define SC_SDU_length_t uint16_t

#define SC_SDU_SECURED_context_epoch_t uint16_t
#define SC_SDU_SECURED_sequence_number_t uint32_t

#define E_INVALID_BLOCK_SIZE -1

typedef struct
{
    // Type of the record.
    SC_SDU_type_t type;
    // Length of the data part of record.
    SC_SDU_length_t length;
} SC_SDU_HEADER;

typedef struct
{
    // This is the sequence number of the SDU record.
    uint64_t sequence_number:48;
    // This represents a selector within context as context may change its settings during flow lifetime.
    uint16_t context_index;
    // This represent an SDU protected fragment. It contains encrypted data and HMAC protection depending on secure channel context.
    uint8_t fragment[0];
} SC_SDU_SECURED;

typedef struct  {
    // This represents a plain data having the specified length.
    uint8_t data[0];
} SC_SDU_PLAIN;

typedef struct {
    SC_SDU_HEADER header;
    union {
        SC_SDU_PLAIN plain;
        SC_SDU_SECURED secured;
    } content;
} SC_SDU;

/*
 * Initializes secure channel environment.
 */
int SC_initialize();
/*
 * Closes secure channel environment.
 */
int SC_finalize();

/*
 * Loads and validates security profile from the specified file.
 */
int SC_PROFILE_load_and_validate(char *fname, SC_PROFILE *profile);
/*
 * Prints out the information about security profile.
 */
void SC_PROFILE_print(FILE *f, SC_PROFILE *profile);




/*
 * Creates an initial security context from a given profile.
 */
int SC_CTX_create(SC_CTX *ctx, SC_PROFILE *profile, int ctx_id, void *this_nonce, void *that_nonce, int nonce_length);
/*
 * Removes a security context and deallocates all related resources.
 */
void SC_CTX_destroy(SC_CTX *ctx);
int SC_CTX_key_length(SC_CTX *ctx);

/*
 * Prints out the information about security context.
 */
void SC_CTX_print(FILE *f, SC_CTX *ctx);

/*
 * Gets the limit on number of bytes that can be securely computed
 * using a single key.
 */
uint64_t SC_CTX_bytes_limit(SC_CTX *ctx);

/*
 * Computes a counter block based on the provided context and information from SDU object.
 */
int SC_compute_counter(char *counter_block, int block_size, SC_CTX *ctx, SC_SDU *sdu);
/*
 * Encrypts (decrypts) a data block using the specified security context and counter block.
 */
int SC_encrypt(EVP_CIPHER_CTX *ctx, char *target, char *source, int length, char *counter);

/*
 * Allocates a new SDU object for the specified SDU type and message length.
 */
SC_SDU *SC_SDU_allocate(int sdu_type, SC_CTX *ctx, int message_length);
/*
 * Deallocates sdu allocated by SC_SDU_allocate function.
 */
void SC_SDU_free(SC_SDU *sdu);
/*
 * Initializes (creates) SDU header using provided buffer. It returns SC_SDU pointer to this buffer.
 */
SC_SDU *SC_SDU_init(int sdu_type, SC_CTX *ctx, int message_length, void*buffer);
/* 
 * computes expected length in bytes of SDU for the given parameters.
 */
int SC_SDU_expected_length(int sdu_type, SC_CTX *ctx, int message_length);


/*
 * Computes SDU message digest using parameters from the specified context.
 */
void SC_SDU_compute_digest(SC_CTX *ctx, SC_SDU *sdu);
/*
 * Verifies SDU message digest using parameters of the specified context.
 */
int SC_SDU_verify_digest(SC_CTX *ctx, SC_SDU *sdu);


void SC_expand_prf(const EVP_MD* evp_md, char *key, int key_length, const char *data, int data_length, char *output, int required_output_length);

int SC_SDU_total_length(SC_SDU *sdu);
int SC_SDU_message_length(SC_CTX *ctx, SC_SDU *sdu);
void SC_SDU_dump_fp(FILE*f, SC_CTX *ctx, SC_SDU *sdu);
#endif
