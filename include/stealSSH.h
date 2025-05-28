#pragma once
#include <stddef.h>

#define MAX_KEY_FILES 128

typedef struct {
    char *publicKeyPath;
    char *secretKeyPath;
} sshKey;

const char* resolve_home_path(const char* path);

void harvest_key_pairs(const char *rootDir, sshKey keys[], size_t *keyCount);

