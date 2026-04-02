#ifndef USER_MANAGER_H
#define USER_MANAGER_H

/*Firma de función para el monitoreo de outbox, uso en server */
void *review_outbox(void *arg);
/*Firma de función para la gestion de usuarios, uso en server*/
void *user_management(void *arg);

#endif
