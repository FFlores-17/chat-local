/*
 * ============================================================
 *  client.c — Proceso cliente completo
 *  Integrante 3
 * ============================================================
 *
 *  COMUNICACIÓN CON EL SERVIDOR
 *  ─────────────────────────────
 *  Cada cliente tiene DOS archivos identificados por su PID:
 *
 *    data/outbox/client_<PID>.txt  ← el cliente ESCRIBE aquí
 *    data/inbox/client_<PID>.txt   ← el cliente LEE aquí
 *
 *  FORMATO DE PROTOCOLO (definido en file_comm.c del servidor)
 *  ────────────────────────────────────────────────────────────
 *    LOGIN    →  LOGIN|<pid>|<nombre>
 *    LIST     →  LIST|<pid>|<nombre>
 *    PRIVATE  →  PRIVATE|<pid>|<origen>|<destino>|<mensaje>
 *    GLOBAL   →  GLOBAL|<pid>|<origen>|<mensaje>
 *    LOGOUT   →  LOGOUT|<pid>|<nombre>
 *
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "protocol.h"

/* ============================================================
   VARIABLES GLOBALES
   ============================================================ */

static char           g_username[MAX_NAME_LEN];
static pid_t          g_pid;
static char           g_inbox_path[256];
static char           g_outbox_path[256];
static volatile int   g_client_running = 1;
static pthread_t      g_receiver_tid;


/* ============================================================
   SECCIÓN 1 — MANEJO DE ARCHIVOS
   ============================================================ */

static void create_client_files(void) {
    snprintf(g_outbox_path, sizeof(g_outbox_path),
             "%s/client_%d.txt", OUTBOX_DIR, (int)g_pid);
    snprintf(g_inbox_path,  sizeof(g_inbox_path),
             "%s/client_%d.txt", INBOX_DIR,  (int)g_pid);

    int fd = open(g_outbox_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) { perror("[CLIENT] Error creando outbox"); exit(EXIT_FAILURE); }
    close(fd);

    fd = open(g_inbox_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) { perror("[CLIENT] Error creando inbox");  exit(EXIT_FAILURE); }
    close(fd);

    printf("[CLIENT] outbox → %s\n", g_outbox_path);
    printf("[CLIENT] inbox  → %s\n", g_inbox_path);
}

static void delete_client_files(void) {
    if (remove(g_outbox_path) == 0) printf("[CLIENT] Outbox eliminado.\n");
    else perror("[CLIENT] No se pudo borrar outbox");
    if (remove(g_inbox_path)  == 0) printf("[CLIENT] Inbox eliminado.\n");
    else perror("[CLIENT] No se pudo borrar inbox");
}

static void write_to_outbox(const char *line) {
    FILE *fp = fopen(g_outbox_path, "a");
    if (!fp) { perror("[CLIENT] Error escribiendo en outbox"); return; }
    fprintf(fp, "%s\n", line);
    fflush(fp);
    fclose(fp);
}


/* ============================================================
   SECCIÓN 2 — CONSTRUCCIÓN DE MENSAJES DE PROTOCOLO
   ============================================================
   Formato exacto que espera process_command() en file_comm.c:

     LOGIN    →  LOGIN|<pid>|<nombre>
     LIST     →  LIST|<pid>|<nombre>
     PRIVATE  →  PRIVATE|<pid>|<origen>|<destino>|<mensaje>
     GLOBAL   →  GLOBAL|<pid>|<origen>|<mensaje>
     LOGOUT   →  LOGOUT|<pid>|<nombre>
   ============================================================ */

static void send_login(void) {
    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "%s|%d|%s",
             CMD_LOGIN, (int)g_pid, g_username);
    write_to_outbox(line);
    printf("[CLIENT] Conectado como '%s' (PID=%d)\n", g_username, (int)g_pid);
}

static void send_logout(void) {
    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "%s|%d|%s",
             CMD_LOGOUT, (int)g_pid, g_username);
    write_to_outbox(line);
    printf("[CLIENT] Enviando LOGOUT...\n");
}

static void send_list(void) {
    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "%s|%d|%s",
             CMD_LIST, (int)g_pid, g_username);
    write_to_outbox(line);
}

static void send_private(const char *args) {
    char dest[MAX_NAME_LEN];
    char msg[MAX_MSG_LEN];
    if (sscanf(args, "%63s %511[^\n]", dest, msg) < 2) {
        printf("[CLIENT] Uso: PRIVATE <usuario> <mensaje>\n");
        return;
    }
    char line[MAX_NAME_LEN*2 + MAX_MSG_LEN + 64];
    snprintf(line, sizeof(line), "%s|%d|%s|%s|%s",
             CMD_PRIVATE, (int)g_pid, g_username, dest, msg);
    write_to_outbox(line);
}

static void send_global(const char *msg) {
    if (strlen(msg) == 0) { printf("[CLIENT] Uso: GLOBAL <mensaje>\n"); return; }
    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "%s|%d|%s|%s",
             CMD_GLOBAL, (int)g_pid, g_username, msg);
    write_to_outbox(line);
}


/* ============================================================
   SECCIÓN 3 — PARSEO DE COMANDOS DEL USUARIO
   ============================================================ */

static int process_input(char *line) {
    line[strcspn(line, "\n")] = '\0';
    if (strlen(line) == 0) return 1;

    if (strncmp(line, CMD_LIST, strlen(CMD_LIST)) == 0) {
        send_list();

    } else if (strncmp(line, CMD_PRIVATE, strlen(CMD_PRIVATE)) == 0) {
        const char *args = line + strlen(CMD_PRIVATE);
        if (*args == ' ') args++;
        send_private(args);

    } else if (strncmp(line, CMD_GLOBAL, strlen(CMD_GLOBAL)) == 0) {
        const char *msg = line + strlen(CMD_GLOBAL);
        if (*msg == ' ') msg++;
        send_global(msg);

    } else if (strncmp(line, CMD_LOGOUT, strlen(CMD_LOGOUT)) == 0) {
        return 0;

    } else {
        printf("[CLIENT] Comando no reconocido: '%s'\n", line);
        printf("         Disponibles: LIST | PRIVATE <user> <msg> | GLOBAL <msg> | LOGOUT\n");
    }
    return 1;
}


/* ============================================================
   SECCIÓN 4 — HILO RECEPTOR
   ============================================================ */

static void *receiver_thread_func(void *arg) {
    (void)arg;
    long offset = 0;
    char buffer[MAX_LINE_LEN];

    while (g_client_running) {
        FILE *fp = fopen(g_inbox_path, "r");
        if (fp) {
            fseek(fp, offset, SEEK_SET);
            while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                offset = ftell(fp);
                buffer[strcspn(buffer, "\n")] = '\0';
                printf("\n\033[1;36m%s\033[0m\n> ", buffer);
                fflush(stdout);

                /* Si el servidor rechazó el nombre, cerrar el cliente */
                if (strstr(buffer, "ya esta en uso") != NULL) {
                    g_client_running = 0;
                    break;
                }
                //esta verificacion se añadio por el caso 2 ya que al detectar que un usuario ya tiene
                //iniciada la sesion en otra terminal, si muestra que ya esta en uso pero entra en un bucle
                //infinito mostrando este mensaje
            }
            fclose(fp);
        }
        usleep(POLL_INTERVAL);
    }
    return NULL;
}


/* ============================================================
   SECCIÓN 5 — SIGINT y LIMPIEZA
   ============================================================ */

static void sigint_handler(int signum) {
    (void)signum;
    printf("\n[CLIENT] Señal SIGINT recibida. Cerrando...\n");
    g_client_running = 0;
}

static void setup_signal_handler(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("[CLIENT] Error registrando SIGINT");
        exit(EXIT_FAILURE);
    }
}

//se hizo un ligero cambio a la funcion al hacer el caso 5
//en resumen simplemente se movio g_client:running = 0 y pthread_join al inicio
//antes del send_logout, esto debido a que al hacer logout el servido le daba tiempo de
//releer el inbox mientras el logout se efectuava, volviendo a mostrar un mensaje de bienvenida
static void cleanup(void) {
    printf("[CLIENT] Iniciando limpieza...\n");
    g_client_running = 0;              /* detener hilo receptor primero */
    pthread_join(g_receiver_tid, NULL);
    send_logout();
    usleep(200000);
    delete_client_files();
    printf("[CLIENT] Sesión de '%s' cerrada.\n", g_username);
}

/* ============================================================
   SECCIÓN 6 — main()
   ============================================================ */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <username>\n", argv[0]);
        return EXIT_FAILURE;
    }

    strncpy(g_username, argv[1], MAX_NAME_LEN - 1);
    g_username[MAX_NAME_LEN - 1] = '\0';
    g_pid = getpid();

    printf("=============================================\n");
    printf(" Chat  |  usuario: %-12s |  PID: %d\n", g_username, (int)g_pid);
    printf("=============================================\n");

    setup_signal_handler();
    create_client_files();
    send_login();

    if (pthread_create(&g_receiver_tid, NULL, receiver_thread_func, NULL) != 0) {
        perror("[CLIENT] Error lanzando hilo receptor");
        delete_client_files();
        return EXIT_FAILURE;
    }

    char input[MAX_LINE_LEN];
    printf("\nComandos:\n");
    printf("  LIST                     ver usuarios conectados\n");
    printf("  PRIVATE <user> <msg>     mensaje privado\n");
    printf("  GLOBAL <msg>             mensaje a todos\n");
    printf("  LOGOUT                   cerrar sesión\n\n");

    while (g_client_running) {
        printf("> ");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        if (process_input(input) == 0) break;
    }

    cleanup();
    return EXIT_SUCCESS;
}


