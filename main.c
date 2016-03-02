#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "rfcomm-server.h"
#include "rfcomm-client.h"
#include "rfcomm-register.h"

int main(int argc, char *argv[])
{
    int flags, opt;

    flags = 0;
    sdp_session_t *session;

    while ((opt = getopt(argc, argv, "scr")) != -1) {
        switch (opt) {
        case 's':
            flags = 1;
            break;
        case 'c':
            flags = 2;
            break;
        case 'r':
            flags = 4;
            break;
        default: /* ''?'' */
            fprintf(stderr, "Usage: %s [-s] [-c] [-r]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    printf("flags=%d\n", flags);

    switch (flags) {
    case 1:
        fprintf(stdout, "Starting server.\n");
        session = register_service();
        rfcomm_server();
        sdp_close( session );
        break;
    case 2:
        fprintf(stdout, "Starting client.\n");
        rfcomm_client();
        break;
    case 4:
        fprintf(stdout, "Registering action.\n");
        register_service();
        break;
    default:
        fprintf(stderr, "no action\n");
    }
    fprintf(stdout, "Done.\n");

    exit(EXIT_SUCCESS);
}
