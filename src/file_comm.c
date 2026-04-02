#include "protocol.h"
#include <stdio.h>
#include "file_comm.h"
/*Creación del users.txt*/
void create_users_file() {
    FILE *user_file = fopen(USERS_FILE, "w");
    if (user_file == NULL) {perror("Error creating users.txt"); return;}
    fclose(user_file);
}
/*Creacion del chat.log*/
void create_chat_log() {
    FILE *log_file = fopen(LOG_FILE, "w");
    if (log_file == NULL) {perror("Error creating log.txt"); return;}
    fclose(log_file);
}

