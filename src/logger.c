#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "logger.h"
#include "protocol.h"

void log_event(char *message) {
    FILE *f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        perror("Error opening log fie");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    fprintf(f, "[%s] %s\n", timestamp, message);
    fflush(f);
    fclose(f);
}

void run_logger(void) {
    log_event("LOGGER: Process initialized successfully.");
    
    while (1) {
        pause(); 
    }
}