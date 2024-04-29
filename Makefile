# Makefile for bank_app

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -g

# Source files
SRCS_BANK = src/bank.c
SRCS_SERVER = src/server.c
SRCS_CLIENT = src/client.c
SRCS_SHM = src/readshm.c

# Object files
OBJS_BANK = $(SRCS_BANK:.c=.o)
OBJS_SERVER = $(SRCS_SERVER:.c=.o)
OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)
OBJS_SHM = $(SRCS_SHM:.c=.o)

# Executable
TARGET_BANK = bank
TARGET_SERVER = server
TARGET_CLIENT = client
TARGET_SHM = readshm

# Default target
all: $(TARGET_BANK) $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_SHM)

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
$(TARGET_SHM): $(OBJS_SHM)
	$(CC) $(CFLGAS) $^ -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJS_BANK) $(OBJS_SERVER) $(OBJS_CLIENT) $(OBJS_SHM) $(TARGET_BANK) $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_SHM)
