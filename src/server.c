#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pthread.h>
#include "protocol.h"
#include "user_manager.h"
#include "file_comm.h"
#include "logger.h"

volatile int g_running   = 1;
UserTable    g_user_table;

void signal_handler(int signum) {
    (void)signum;
    g_running = 0;
}

void shutdown_server(void) {
    int i;
    char inbox_path[256];
    char outbox_path[256];
    DIR *dir;
    struct dirent *entry;

    pthread_mutex_lock(&g_user_table.mutex);
    for (i = 0; i < MAX_USERS; i++) {
        if (g_user_table.users[i].active) {
            snprintf(inbox_path, sizeof(inbox_path),
                     "%s/client_%d.txt", INBOX_DIR,
                     g_user_table.users[i].pid);
            FILE *f = fopen(inbox_path, "a");
            if (f) {
                fprintf(f, "[SERVIDOR] El servidor dejará de funcionar...\n");
                fflush(f);
                fclose(f);
            }
        }
    }
    pthread_mutex_unlock(&g_user_table.mutex);

    sleep(1);

    FILE *uf = fopen(USERS_FILE, "w");
    if (uf) fclose(uf);

    dir = opendir(INBOX_DIR);
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            snprintf(inbox_path, sizeof(inbox_path),
                     "%s/%s", INBOX_DIR, entry->d_name);
            remove(inbox_path);
        }
        closedir(dir);
    }

    dir = opendir(OUTBOX_DIR);
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            snprintf(outbox_path, sizeof(outbox_path),
                     "%s/%s", OUTBOX_DIR, entry->d_name);
            remove(outbox_path);
        }
        closedir(dir);
    }
    /*
     *IMPLEMENTACIÓN CUANDO YA SE TENGA LOGICA DE ESCRITURA EN LOG
    log_event("SERVER SHUTDOWN");
    */
    printf("[SERVER] Apagado completo.\n");
}

int main(void) {
    pthread_t tid_polling, tid_users;
    pid_t     pid;
    /* 1. Inicialización compartida */
    signal(SIGINT, signal_handler);
    create_users_file();
    create_chat_log();
    pthread_mutex_init(&g_user_table.mutex, NULL);
    g_user_table.count = 0;
    /* 2. fork() — separar padre y logger */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    /* 3. Proceso hijo — logger */
    if (pid == 0) {
        printf("[LOGGER] Proceso logger iniciado.\n");
        /*FALTA IMPLEMENTACIÓN
        run_logger(); loop bloqueante — por implementar en logger.c */
        exit(0);        /* nunca cae al código del padre */
    }
    /* 4. Proceso padre — servidor */
    if (pthread_create(&tid_polling, NULL, review_outbox, NULL) != 0) {
        perror("pthread_create polling");
        return 1;
    }
    if (pthread_create(&tid_users, NULL, user_management, NULL) != 0) {
        perror("pthread_create users");
        return 1;
    }
    printf("[SERVER] En línea. Ctrl+C para apagar.\n");
    while (g_running) {
        pause();
    }
    printf("\n[SERVER] Señal recibida. Apagando...\n");
    pthread_join(tid_polling, NULL);
    pthread_join(tid_users,   NULL);
    shutdown_server();
    pthread_mutex_destroy(&g_user_table.mutex);
    waitpid(pid, NULL, 0);   /* esperar al proceso logger */
    return 0;
}