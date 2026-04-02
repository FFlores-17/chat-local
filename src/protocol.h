 #ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/types.h>   /* pid_t */
#include <pthread.h>     /* pthread_mutex_t */
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
/* ─── Límites del sistema ─────────────────────────── */
#define MAX_USERS       32
#define MAX_NAME_LEN    64
#define MAX_MSG_LEN     512
#define MAX_LINE_LEN    (MAX_NAME_LEN + MAX_MSG_LEN + 64)

/* ─── Rutas de archivos ───────────────────────────── */
#define DATA_DIR        "data"
#define USERS_FILE      "data/usuarios.txt"
#define LOG_FILE        "data/chat.log"

/* ─── Comandos del protocolo ──────────────────────── */
#define CMD_LOGIN       "LOGIN"
#define CMD_LIST        "LIST"
#define CMD_PRIVATE     "PRIVATE"
#define CMD_GLOBAL      "GLOBAL"
#define CMD_LOGOUT      "LOGOUT"

/* ─── Separador del protocolo ─────────────────────── */
#define PROTO_SEP       "|"
#define PROTO_SEP_CHAR  '|'

/* ─── Intervalo de polling (microsegundos) ────────── */
#define POLL_INTERVAL   100000   /* 100 ms */

/* ─── Estructura de un usuario ───────────────────── */
typedef struct {
    char    name[MAX_NAME_LEN];
    pid_t   pid;
    long    outbox_offset;
    int     active;
} User;

/* ─── Tabla de usuarios (protegida por mutex) ─────── */
typedef struct {
    User            users[MAX_USERS];
    int             count;
    pthread_mutex_t mutex;
} UserTable;

/* ─── Variable global del servidor ───────────────── */
/* Definida en server.c, declarada extern aquí        */
extern volatile int     g_running;
extern UserTable        g_user_table;

#endif /* PROTOCOL_H */
