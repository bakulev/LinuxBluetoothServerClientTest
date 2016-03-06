#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h> // for sdp_session_t in start_server

#include "rfcomm-server.h"
#include "rfcomm-client.h"
#include "hciscan.h"
#include "service-register.h"
#include "main.h"
#include "service-register.h"

int main(int argc, char *argv[])
{
    int opt;//, flags;

    // Service class variables. Default value is 0xABCD.
    uuid_t service_uuid_short = {SDP_UUID16, {0xABCD} };
    uuid_t *service_uuid = NULL;

    // Bluetooth address to connect and send data. Default is 00:00:00:00:00:00.
    bdaddr_t target_bdaddr = {{0, 0, 0, 0, 0, 0}};

    // Two types of program operation. Default is receiver (server).
    typedef enum {
    	OPERATION_MODE_SERVER = 1,
		OPERATION_MODE_CLIENT
    } operation_mode_t;
    operation_mode_t operation_mode = OPERATION_MODE_SERVER;

    const char *short_opt = "sa:ru:h";
    struct option   long_opt[] =
       {
    	  {"sender",        no_argument,       NULL, 's'},
    	  {"address",       required_argument, NULL, 'a'},
    	  {"receiver",      no_argument,       NULL, 'r'},
          {"uuid",          required_argument, NULL, 'u'},
    	  {"help",          no_argument,       NULL, 'h'},
          {NULL,            0,                 NULL, 0  }
       };
    while ((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
        switch (opt) {
        case -1:       /* no more arguments */
        case 0:        /* long options toggles */
            break;
        case 's':
        	printf("Sender (client) mode switched.\n");
        	operation_mode = OPERATION_MODE_CLIENT;
		   break;
        case 'a':
        	// Checking and setting address to send data.
        	str2ba(optarg, &target_bdaddr);
            break;
        case 'r':
        	printf("Receiver (server) mode switched.\n");
        	operation_mode = OPERATION_MODE_SERVER;
            break;
        case 'u':
        	sdp_uuid16_create(&service_uuid_short, (uint16_t)strtoul(optarg, NULL, 16));
			break;
        case 'h':
			printf("Usage: %s [OPTIONS]\n", argv[0]);
			printf("  -s                        start program as sender (address and service class\n\
                            may be specified)\n");
			printf("  -a address                bluetooth address for sender to connect. if not\n\
                            specified you'll be able to choose from list of\n\
                            available devices.\n");
			printf("  -r                        start program as receiver (server)\n");
			printf("  -u, --uuid                specify UUID of service class (default is 0xABCD)\n");
			printf("  -h, --help                print this help and exit\n");
			printf("\n");
			exit(EXIT_SUCCESS);
		case ':':
		case '?':
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			exit(EXIT_FAILURE);
		default:
			fprintf(stderr, "%s: invalid option -- %c\n", argv[0], opt);
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			exit(EXIT_FAILURE);
         }
    }

    // Initialize full length service uuid from short service uuid.
	service_uuid = sdp_uuid_to_uuid128 (&service_uuid_short);

	switch( operation_mode ) {
    case OPERATION_MODE_SERVER:
        printf("Operating as receiver (server).\n");
        show_service_uuid(service_uuid);
        printf("Starting receiver (server)...\n");
        server_register_and_start(service_uuid);
    	break;
    case OPERATION_MODE_CLIENT:
        printf("Operating as sender (client).\n");
        show_service_uuid(service_uuid);
        show_bdaddr(&target_bdaddr);
        if (bacmp(&target_bdaddr, BDADDR_ANY) == 0)
        {
        	printf("Scanning started...\n");
        	hciscan(&target_bdaddr);
        }
        printf("You choose address.\n");
        show_bdaddr(&target_bdaddr);
        client_start(&target_bdaddr, service_uuid);
    	break;
    default:
        printf("Unknown operation mode \"%d\".\n", operation_mode);
		exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void show_service_uuid(uuid_t *service_uuid)
{
	char service_uuid_str[MAX_LEN_UUID_STR];
	sdp_uuid2strn(service_uuid, service_uuid_str, MAX_LEN_UUID_STR);
	printf("Service class UUID is \"%s\".\n", service_uuid_str);
}

void show_bdaddr(bdaddr_t *target_bdaddr)
{
	char target_addr[32] = {0};
	ba2str(target_bdaddr, target_addr);
	if (bacmp(target_bdaddr, BDADDR_ANY) == 0)
		printf("Address isn't specified.\n");
	else
		printf("Address is \"%s\".\n", target_addr);
}

void server_register_and_start(uuid_t *service_uuid)
{
    sdp_session_t *session = 0;
    uint8_t port = 0; // Port of service.

    printf("Binding listening server.\n");
    int s = rfcomm_server(&port); // Open server on socket and get binded port.
    // Register service on binded port.
    printf("Registering service.\n");
    session = service_register(session, service_uuid, port);
    printf("Accepting connections.\n");
    accept_connection(s);
    printf("Unregistering service.\n");
    sdp_close( session );
    printf("Closing server socket.\n");
    close(s); // Close socket.
}

void client_start(bdaddr_t *target, uuid_t *service_uuid)
{
	uint8_t port = 0; // Port of service.

	port = service_search(target, service_uuid);
	if (port > 0)
	{
		printf("Starting client...\n");
		rfcomm_client(target, port);
	}
	else
	{
		printf("Service port not found, exiting...\n");
	}
}
