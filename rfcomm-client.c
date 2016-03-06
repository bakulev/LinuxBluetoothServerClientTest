#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <errno.h>
#include "rfcomm-client.h"

#define STDIN 0 // for selecting stdin input in select()

int service_search(bdaddr_t *target, uuid_t *service_uuid)
{
	int status;
	uint8_t port = 0;
	sdp_session_t *session = 0;
	sdp_list_t *response_list, *search_list, *attrid_list;
    uint32_t range = 0x0000ffff;

    search_list = sdp_list_append(0, service_uuid);
    attrid_list = sdp_list_append(0, &range);

	session = sdp_connect(BDADDR_ANY, target, 0);
	printf("Sending SDP search request.\n");
	status = sdp_service_search_attr_req(session, search_list, SDP_ATTR_REQ_RANGE, attrid_list, &response_list);

	if (status == 0)
	{
		printf("Success received SDP response.\n");
		sdp_list_t *proto_list = NULL;
		sdp_list_t *r = response_list;
		//go through each service records
		for (; r; r = r->next)
		{
			sdp_record_t *req = (sdp_record_t*)r->data;
			//get list of the protocol sequences
			if (sdp_get_access_protos(req, &proto_list) == 0)
			{
				//get RFCOMM port number
				port = sdp_get_proto_port(proto_list, RFCOMM_UUID);
				sdp_list_free(proto_list, 0);
			}
			sdp_record_free(req);
		}
	}
	else
		printf("Failed received SDP response.\n");
	sdp_list_free(response_list, 0);
	sdp_list_free(search_list, 0);
	sdp_list_free(attrid_list, 0);
	sdp_close(session);

	return port;
}

int rfcomm_client(bdaddr_t *target, uint8_t port)
{
    struct sockaddr_rc addr = { 0 };
    int s, status;
    //char buf[1024] = {0};

    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_bdaddr = *target;
    addr.rc_channel = port;
    printf("Connecting to port %d.\n", port);

    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (status < 0)
	{
		perror("connect() failed");
		close(s);
		exit(EXIT_FAILURE);
	}
    char dest[32];
    ba2str(&addr.rc_bdaddr, dest);
    printf("Connected to address %s.\n", dest);

	int max_sd;
	struct timeval       timeout;
	fd_set        master_set, working_set;

	// Initialize the master fd_set
	FD_ZERO(&master_set);
	max_sd = s;
	FD_SET(s, &master_set);
	// Read from STDIN also.
	FD_SET(STDIN, &master_set);

	int end_client = 0;

	do
	{
		// Copy the master fd_set over to the working fd_set.
		memcpy(&working_set, &master_set, sizeof(master_set));
		timeout.tv_sec  = 3 * 60;
		timeout.tv_usec = 0;
		// Call select() and wait 5 minutes for it to complete.
		printf("Waiting on select() %ld sec...\n", timeout.tv_sec);
		int rc_select = select(max_sd + 1, &working_set, NULL, NULL, &timeout);
		// Check to see if the select call failed.
		if (rc_select < 0)
		{
			perror("  select() failed");
			break;
		}
		// One or more descriptors are readable.  Need to
		// determine which ones they are.
		else if (rc_select > 0)
		{
			//end_client = loop_descriptors(rc_select, &max_sd, &master_set, &working_set, s);
			int desc_ready = rc_select;
			int i;
			for (i=0; i <= max_sd  &&  desc_ready > 0; ++i)
			{
				printf("Checkin FD_ISSET %d socket.\n", i);
				// Check to see if this descriptor is ready.
				if (FD_ISSET(i, &working_set))
				{
					desc_ready -= 1;
					// Check to see if this is the listening socket.
					if (i == s)
					{
						printf("  Connected socket is readable\n");
						/*int close_conn = read_connection(i);
						if (close_conn)
						{
							close_connection(i, max_sd, master_set);
						}*/
						int close_conn = 0;
						char buf[1024] = {0};
						int rc_recv = recv(i, buf, sizeof(buf), 0);
						if (rc_recv < 0)
						{
							if (errno != EWOULDBLOCK)
							{
								perror("  recv() failed");
								//close_conn = 1;
							}
							//break;
						}
						// Check to see if the connection has been
						// closed by the client
						if (rc_recv == 0)
						{
							printf("  Connection closed\n");
							//close_conn = 1;
							//break;
						}
						// Data was received
						int len = rc_recv;
						printf("  %d bytes received \"%s\"\n", len, buf);

						if (close_conn)
						{
							close(i);
							end_client = 1;
						}
					}
					// Check stdin input.
					else if(i == STDIN)
					{
						char buf[1024] = {0};
						// Read data from stdin using fgets.
						fgets(buf, sizeof(buf), stdin);
						// Remove trailing newline character from the input buffer if needed.
						int len = strlen(buf) - 1;
						if (buf[len] == '\n') buf[len] = '\0';

						printf("'%s' was read from stdin.\n", buf);
						int rc_send = send(s, buf, len, 0);
						if (rc_send < 0)
						{
							perror("  send() failed");
							//close_conn = 1;
							break;
						}
						printf("Sended \"%s\" in %d socket\n", buf, s);					}
					// This is not the listening socket, therefore an
					// existing connection must be readable
					else
					{
						printf("  Descriptor %d is readable\n", i);
					} /* End of existing connection is readable */
				} /* End of if (FD_ISSET(i, &working_set)) */
			} /* End of loop through selectable descriptors */
		}
		// Else if rc_select == 0 then the 5 minute time out expired.
		else
		{
			printf("  select() timed out.  End program.\n");
			break;
		}
	}
	while (end_client == 0);
	/*
    // send a message
    if( status == 0 ) {
        status = write(s, "hello!", 6);
        printf("Sended.\n");
    }
    status = read(s, buf, 1023);
    buf[1023] = 0;
    printf("Received: \"%s\".\n", buf);

    if( status < 0 ) perror("uh oh");
	 */
    close(s);
    return 0;
}
