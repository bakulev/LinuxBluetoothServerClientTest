#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>
#include "rfcomm-client.h"

int dynamic_bind_rc(int sock, struct sockaddr_rc *sockaddr, uint8_t *port)
{
    int err;
    for( *port = 1; *port <= 31; *port++ ) {
        sockaddr->rc_channel = *port;
        err = bind(sock, (struct sockaddr *)sockaddr, sizeof(sockaddr));
        if( ! err || errno == EINVAL ) break;
    }
    if( port == 31 ) {
        err = -1;
        errno = EINVAL;
    }
    return err;
}

int rfcomm_client(void)
{
    struct sockaddr_rc addr = { 0 };
    int s, status;
    uint8_t port = 1;
    char *dest = {0}; //"C0:18:85:DC:BA:61";
    int res;

    printf("Start scanning.\n");
    dest = (char*)malloc(18 * sizeof(char));
    dest[17] = 0;
    dest = hciscan(dest);
    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    //addr.rc_channel = port;
    res = dynamic_bind_rc(s, &addr, &port);
    printf("dynamic_bind returns %d for port %d\n", res, port);   
    str2ba( dest, &addr.rc_bdaddr );

    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));
    ba2str(&addr.rc_bdaddr, dest);
    printf("Connected to address %s.\n", dest);

    // send a message
    if( status == 0 ) {
        status = write(s, "hello!", 6);
        printf("Sended.\n");
    }

    if( status < 0 ) perror("uh oh");

    close(s);
    return 0;
}

char* hciscan(char *dest_addr)
{
	inquiry_info *devices = NULL;
	int max_rsp, num_rsp;
	int adapter_id, sock, len, flags;
	int i;
	char addr[19] = {0};
	char name[248] = {0};
	adapter_id = hci_get_route(NULL);
	sock = hci_open_dev(adapter_id);
	if (adapter_id < 0 || sock < 0)
	{
		perror("opening socket");
		exit(1);
	}

	dest_addr[0] = 0;
	len = 8;
	max_rsp = 255;
	flags = IREQ_CACHE_FLUSH;
	devices =(inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

	num_rsp = hci_inquiry(adapter_id, len, max_rsp, NULL, &devices, flags);
	if (num_rsp < 0) perror("hci_inquiry");

	while(dest_addr[0] == 0)
	{
		printf("Select device to send data:");
		for (i = 0; i < num_rsp; i++)
		{
			ba2str(&(devices + i)->bdaddr, addr);
			memset(name, 0, sizeof(name));
			if (0 != hci_read_remote_name(sock, &(devices + i)->bdaddr, sizeof(name), name, 0))
			{
			    // allocate a socket
				strcpy(name, "[unknown]");
			}
			printf("%d: %s  %s\n", i, addr, name);
		}
		scanf("%d", &i);
		if (i >= 0 && i < num_rsp) strncpy(dest_addr, addr, 18);
	}

	free(devices);
	return dest_addr;
}
