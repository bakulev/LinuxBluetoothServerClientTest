#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
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
    char dest[18] = "C0:18:85:DC:BA:61";
    int res;

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
    printf("Connected.\n");

    // send a message
    if( status == 0 ) {
        status = write(s, "hello!", 6);
        printf("Sended.\n");
    }

    if( status < 0 ) perror("uh oh");

    close(s);
    return 0;
}
