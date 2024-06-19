# Makefile for bank_app
# Compiler
CC = gcc
# Compiler flags
CFLAGS = -Wall -Wextra -O3
CFLAGS_DEBUG = -Wall -Wextra -g
# Source files
SRCS_BANK = src/bank/bank.c src/bank/session.c src/bank/signals.c src/bank/shared_mem.c src/bank/db.c
SRCS_SERVER = src/server/server.c
SRCS_CLIENT = src/client/client.c
# Object files
OBJS_BANK = $(SRCS_BANK:.c=.o)
OBJS_SERVER = $(SRCS_SERVER:.c=.o)
OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)
# Executable
TARGET_BANK = build/bank
TARGET_SERVER = build/server
TARGET_CLIENT = build/client
# Debug executables
TARGET_BANK_DEBUG = debug/bank
TARGET_SERVER_DEBUG = debug/server
TARGET_CLIENT_DEBUG = debug/client

# Default target
all: build debug 

build: $(TARGET_BANK) $(TARGET_SERVER) $(TARGET_CLIENT)

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: $(TARGET_BANK_DEBUG) $(TARGET_SERVER_DEBUG) $(TARGET_CLIENT_DEBUG)
# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
# Link object files into executable
$(TARGET_BANK): $(OBJS_BANK)
	$(CC) $(CFLAGS) $^ -o $@
$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(CFLAGS) $^ -o $@
$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(CFLAGS) $^ -o $@
# Clean up object files and executable
clean:
	rm -f $(OBJS_BANK) $(OBJS_SERVER) $(OBJS_CLIENT) $(TARGET_BANK) $(TARGET_SERVER) $(TARGET_CLIENT)