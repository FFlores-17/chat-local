#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "protocol.h"
#include "user_manager.h"
#include "file_comm.h"
#include "logger.h"
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

/* ─────────────────────────────────────────
 * Procesa UN comando ya leído del outbox.
 * line es la línea cruda, por ejemplo:
 * "LOGIN|1234|ana"
 * ───────────────────────────────────────── */
static void process_command(char *line) {
    char cmd[32];
    char name[MAX_NAME_LEN];
    char dest[MAX_NAME_LEN];
    char message[MAX_MSG_LEN];
    char inbox[256];
    char log_msg[256];
    int  pid;
    int  dest_pid;
    int  i;
    FILE *f;

    /* Leer solo el primer campo para saber qué comando es */
    sscanf(line, "%31[^|]", cmd);

    /* ── LOGIN|pid|nombre ── */
    if (strcmp(cmd, CMD_LOGIN) == 0) {
        sscanf(line, "%*[^|]|%d|%63[^\n]", &pid, name);
        snprintf(inbox, sizeof(inbox),
                 "%s/client_%d.txt", INBOX_DIR, pid);

        if (add_user(name, (pid_t)pid) == 0) {
            /* Nombre duplicado */
            f = fopen(inbox, "a");
            if (f) {
                fprintf(f, "[SERVIDOR] Nombre '%s' ya esta en uso.\n", name);
                fflush(f);
                fclose(f);
            }
            snprintf(log_msg, sizeof(log_msg),
                     "LOGIN RECHAZADO: %s (pid %d)", name, pid);
        } else {
            /* Login exitoso */
            f = fopen(inbox, "a");
            if (f) {
                fprintf(f, "[SERVIDOR] Bienvenido, %s.\n", name);
                fflush(f);
                fclose(f);
            }
            snprintf(log_msg, sizeof(log_msg),
                     "LOGIN %s (pid %d)", name, pid);
        }
        log_event(log_msg);
    }

    /* ── LIST|pid|nombre ── */
    else if (strcmp(cmd, CMD_LIST) == 0) {
        sscanf(line, "%*[^|]|%d|%63[^\n]", &pid, name);
        snprintf(inbox, sizeof(inbox),
                 "%s/client_%d.txt", INBOX_DIR, pid);

        char user_list[MAX_USERS * (MAX_NAME_LEN + 2)];
        get_user_list(user_list, sizeof(user_list));

        f = fopen(inbox, "a");
        if (f) {
            fprintf(f, "[SERVIDOR] Conectados: %s\n", user_list);
            fflush(f);
            fclose(f);
        }
    }

    /* ── PRIVATE|pid|origen|destino|mensaje ── */
    else if (strcmp(cmd, CMD_PRIVATE) == 0) {
        sscanf(line, "%*[^|]|%d|%63[^|]|%63[^|]|%511[^\n]",
               &pid, name, dest, message);

        dest_pid = find_user_pid(dest);
        snprintf(inbox, sizeof(inbox),
                 "%s/client_%d.txt", INBOX_DIR, pid);

        if (dest_pid == -1) {
            /* Destinatario no conectado */
            f = fopen(inbox, "a");
            if (f) {
                fprintf(f, "[SERVIDOR] '%s' no esta conectado.\n", dest);
                fflush(f);
                fclose(f);
            }
            return;
        }

        /* Escribir en el inbox del destinatario */
        snprintf(inbox, sizeof(inbox),
                 "%s/client_%d.txt", INBOX_DIR, dest_pid);
        f = fopen(inbox, "a");
        if (f) {
            fprintf(f, "[%s] %s\n", name, message);
            fflush(f);
            fclose(f);
        }

        snprintf(log_msg, sizeof(log_msg),
                 "PRIVATE %s -> %s", name, dest);
        log_event(log_msg);
    }

    /* ── GLOBAL|pid|origen|mensaje ── */
    else if (strcmp(cmd, CMD_GLOBAL) == 0) {
        sscanf(line, "%*[^|]|%d|%63[^|]|%511[^\n]",
               &pid, name, message);

        /* El mutex YA está suelto cuando llegamos aquí
         * (lo soltamos en review_outbox antes de llamar
         * a process_command), así que podemos tomarlo
         * con seguridad para iterar la tabla            */
        pthread_mutex_lock(&g_user_table.mutex);
        for (i = 0; i < MAX_USERS; i++) {
            if (!g_user_table.users[i].active) continue;

            snprintf(inbox, sizeof(inbox),
                     "%s/client_%d.txt", INBOX_DIR,
                     g_user_table.users[i].pid);
            f = fopen(inbox, "a");
            if (f) {
                fprintf(f, "[GLOBAL|%s] %s\n", name, message);
                fflush(f);
                fclose(f);
            }
        }
        pthread_mutex_unlock(&g_user_table.mutex);

        snprintf(log_msg, sizeof(log_msg), "GLOBAL %s", name);
        log_event(log_msg);
    }

    /* ── LOGOUT|pid|nombre ── */
    else if (strcmp(cmd, CMD_LOGOUT) == 0) {
        sscanf(line, "%*[^|]|%d|%63[^\n]", &pid, name);
        remove_user((pid_t)pid);

        snprintf(log_msg, sizeof(log_msg),
                 "LOGOUT %s (pid %d)", name, pid);
        log_event(log_msg);
    }
}

/* ─────────────────────────────────────────
 * review_outbox — función del hilo polling
 * ───────────────────────────────────────── */
void *review_outbox(void *arg) {
    (void)arg;

    char  path[256];
    char  line[MAX_LINE_LEN];
    char  pending[64][MAX_LINE_LEN]; /* líneas leídas en esta vuelta */
    int   n_pending;
    int   i;
    FILE *f;

    while (g_running) {

        pthread_mutex_lock(&g_user_table.mutex);

        for (i = 0; i < MAX_USERS; i++) {
            if (!g_user_table.users[i].active) continue;

            /* Construir ruta del outbox de este usuario */
            snprintf(path, sizeof(path), "%s/client_%d.txt",
                     OUTBOX_DIR, g_user_table.users[i].pid);

            f = fopen(path, "r");
            if (f == NULL) continue;

            /* Ir al último byte leído */
            fseek(f, g_user_table.users[i].outbox_offset, SEEK_SET);

            /* Leer todas las líneas nuevas y guardarlas */
            n_pending = 0;
            while (fgets(line, sizeof(line), f) != NULL
                   && n_pending < 64) {
                /* Quitar el salto de línea */
                size_t len = strlen(line);
                if (len > 0 && line[len - 1] == '\n')
                    line[len - 1] = '\0';

                if (strlen(line) == 0) continue;

                strncpy(pending[n_pending], line, MAX_LINE_LEN - 1);
                pending[n_pending][MAX_LINE_LEN - 1] = '\0';
                n_pending++;
            }

            /* Actualizar el offset antes de soltar el mutex */
            g_user_table.users[i].outbox_offset = ftell(f);
            fclose(f);

            /* Soltar el mutex ANTES de procesar los comandos */
            pthread_mutex_unlock(&g_user_table.mutex);

            /* Procesar las líneas acumuladas sin el mutex */
            for (int j = 0; j < n_pending; j++) {
                process_command(pending[j]);
            }

            /* Retomar el mutex para el siguiente usuario */
            pthread_mutex_lock(&g_user_table.mutex);
        }

        pthread_mutex_unlock(&g_user_table.mutex);

        usleep(POLL_INTERVAL);
    }

    return NULL;
}


