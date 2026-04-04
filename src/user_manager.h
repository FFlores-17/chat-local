#ifndef USER_MANAGER_H
#define USER_MANAGER_H
#include "protocol.h"

/*Firma de función para la gestion de usuarios, uso en server*/
void *user_management(void *arg);
/* Agrega un usuario a la UserTable y reescribe usuarios.txt.
 * Retorna 1 si tuvo éxito, 0 si el nombre ya existe. */
int add_user(char *name, pid_t pid);
/* Elimina un usuario de la UserTable y reescribe usuarios.txt. */
void remove_user(pid_t pid);
/* Llena el buffer con los nombres de usuarios activos separados
 * por comas. Ejemplo: "ana, carlos, pedro" */
void get_user_list(char *buffer, int buffer_size);
/* Devuelve el pid de un usuario buscado por nombre.
 * Retorna -1 si no está conectado. */
int find_user_pid(char *name);
#endif
