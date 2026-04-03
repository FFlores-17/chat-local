#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "protocol.h"
#include "logger.h"
#include "user_manager.h"

extern UserTable g_user_table;
extern volatile int g_running;

static void sync_physical_user_file() {
    FILE *f = fopen(USERS_FILE, "w");
    if (!f) return;

    for (int i=0; i<MAX_USERS; i++) {
        if (g_user_table.users[i].active) {
            fprintf(f, "%s|%d\n", g_user_table.users[i].name, g_user_table.users[i].pid);
        }
    }
    fclose(f);
}

int add_user(char *name, pid_t pid) {
    pthread_mutex_lock(&g_user_table.mutex);
    
    for (int i = 0; i < MAX_USERS; i++) {
        if (g_user_table.users[i].active && strcmp(g_user_table.users[i].name, name) == 0) {
            pthread_mutex_unlock(&g_user_table.mutex);
            return 0; 
        }
    }
    
    for (int i = 0; i < MAX_USERS; i++) {
        if (!g_user_table.users[i].active) {
            strncpy(g_user_table.users[i].name, name, MAX_NAME_LEN);
            g_user_table.users[i].pid = pid;
            g_user_table.users[i].outbox_offset = 0;
            g_user_table.users[i].active = 1;
            g_user_table.count++;
            
            sync_physical_user_file();
            pthread_mutex_unlock(&g_user_table.mutex);
            return 1;
        }
    }

    pthread_mutex_unlock(&g_user_table.mutex);
    return 0;
}

void remove_user(pid_t pid) {
    pthread_mutex_lock(&g_user_table.mutex);
    for (int i = 0; i < MAX_USERS; i++) {
        if (g_user_table.users[i].active && g_user_table.users[i].pid == pid) {
            g_user_table.users[i].active = 0;
            g_user_table.count--;
            sync_physical_user_file();
            break;
        }
    }
    pthread_mutex_unlock(&g_user_table.mutex);
}

void get_user_list(char *buffer, int buffer_size) {
    pthread_mutex_lock(&g_user_table.mutex);
    buffer[0] = '\0';
    int first = 1;

    for (int i = 0; i < MAX_USERS; i++) {
        if (g_user_table.users[i].active) {
            if (!first) strncat(buffer, ", ", buffer_size - strlen(buffer) - 1);
            strncat(buffer, g_user_table.users[i].name, buffer_size - strlen(buffer) - 1);
            first = 0;
        }
    }
    pthread_mutex_unlock(&g_user_table.mutex);
}

int find_user_pid(char *name) {
    int pid = -1;
    pthread_mutex_lock(&g_user_table.mutex);
    for (int i = 0; i < MAX_USERS; i++) {
        if (g_user_table.users[i].active && strcmp(g_user_table.users[i].name, name) == 0) {
            pid = g_user_table.users[i].pid;
            break;
        }
    }
    pthread_mutex_unlock(&g_user_table.mutex);
    return pid;
}

void *user_management(void *arg) {
    while (g_running) {
        pthread_mutex_lock(&g_user_table.mutex);
        int changed = 0;

        for (int i = 0; i < MAX_USERS; i++) {
            if (g_user_table.users[i].active) {
                // kill(pid, 0) checks if a process is still alive
                if (kill(g_user_table.users[i].pid, 0) == -1) {
                    char log_msg[128];
                    snprintf(log_msg, sizeof(log_msg), "GHOST DETECTED: %s (PID %d) removed.", 
                             g_user_table.users[i].name, g_user_table.users[i].pid);
                    log_event(log_msg);

                    g_user_table.users[i].active = 0;
                    g_user_table.count--;
                    changed = 1;
                }
            }
        }

        if (changed) sync_physical_user_file();
        
        pthread_mutex_unlock(&g_user_table.mutex);
        usleep(POLL_INTERVAL * 10);
    }
    return NULL;
}