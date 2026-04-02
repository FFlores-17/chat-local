#include <stdio.h>
#include "protocol.h"
#include "user_manager.h"
#include "file_comm.h"

volatile int  g_running  = 1;
UserTable     g_user_table;

int main(void) {
    //Creación de todos los archivos necesarios para el funcionamiento del programa
    create_users_file();
    create_chat_log();
    pid_t pid = fork();
    if (pid < 0) {perror ("Error in fork"); return -1;}
    if (pid == 0) {
        //Proceso logger
        printf("[server] Creación de proceso para logger exitosa, falta implementación \n");
    }
    /*Proceso Principal del server */
    /*Creamos hilo para revisión de outbox */
    /*
     *FALTA IMPLEMENTACIÓN
    pthread_t thread_review_outbox;
    pthread_create(&thread_review_outbox, NULL, review_outbox, NULL);
    Creamos hilo de gestion de usuarios
    pthread_t thread_user_management;
    pthread_create(&thread_user_management, NULL, user_management, NULL);

    pthread_join(thread_review_outbox, NULL);
    pthread_join(thread_user_management, NULL);
    */
    waitpid(pid, NULL, 0);
    return 0;
}
