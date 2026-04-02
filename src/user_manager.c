#include <stdio.h>
#include "user_manager.h"

#include "protocol.h"


void *user_management(void *arg) {
    while (g_running) {
        /*Agregar aca lo relacionado del servidor con respecto a los usuarios */
        usleep(POLL_INTERVAL*5);
    }
}
