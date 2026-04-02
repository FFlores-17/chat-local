CC      = gcc
CFLAGS  = -Wall -Wextra -g -pthread
SRCDIR  = src

# Archivos fuente de cada binario
SERVER_SRCS = $(SRCDIR)/server.c \
              $(SRCDIR)/user_manager.c \
              $(SRCDIR)/file_comm.c \
              $(SRCDIR)/logger.c

CLIENT_SRCS = $(SRCDIR)/client.c

# Targets
all: chat_server chat_client

chat_server: $(SERVER_SRCS) $(SRCDIR)/protocol.h
	$(CC) $(CFLAGS) -o chat_server $(SERVER_SRCS)

chat_client: $(CLIENT_SRCS) $(SRCDIR)/protocol.h
	$(CC) $(CFLAGS) -o chat_client $(CLIENT_SRCS)

clean:
	rm -f chat_server chat_client
	rm -rf data/

.PHONY: all clean