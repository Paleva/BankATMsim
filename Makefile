# Makefile for bank_app

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -g

# Source files
SRCS_BANK = src/bank.c
SRCS_SERVER = src/server.c
SRCS_CLIENT = src/client.c

# Object files
OBJS_BANK = $(SRCS_BANK:.c=.o)
OBJS_SERVER = $(SRCS_SERVER:.c=.o)
OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)
# Executable
TARGET_BANK = bank
TARGET_SERVER = server
TARGET_CLIENT = client

# Default target
all: $(TARGET_BANK) $(TARGET_SERVER) $(TARGET_CLIENT) 


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
