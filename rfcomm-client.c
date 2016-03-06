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
    char buf[1024] = {0};

    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_bdaddr = *target;
    addr.rc_channel = port;
    printf("Connecting to port %d.\n", port);

    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    char dest[32];
    ba2str(&addr.rc_bdaddr, dest);
    printf("Connected to address %s.\n", dest);

    // send a message
    if( status == 0 ) {
        status = write(s, "hello!", 6);
        printf("Sended.\n");
    }
    status = read(s, buf, 1023);
    buf[1023] = 0;
    printf("Received: \"%s\".\n", buf);

    if( status < 0 ) perror("uh oh");

    close(s);
    return 0;
}
