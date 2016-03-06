#include <stdio.h>
#include <stdlib.h> // for exit(EXIT_FAILURE);
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/ioctl.h>
#include "rfcomm-server.h"

int dynamic_bind_rc(int sock, struct sockaddr_rc *sockaddr)
{
    int err;
    int port = 0;

    for( port = 1; port <= 31; port++ ) {
        sockaddr->rc_channel = port;
        err = bind(sock, (struct sockaddr *)sockaddr, sizeof(sockaddr));
        if( ! err || errno == EINVAL ) break;
    }
    if( port == 31 ) {
        err = -1;
        errno = EINVAL;
    }
    return err;
}

int rfcomm_server(uint8_t *port)
{
    struct sockaddr_rc loc_addr = { 0 };
    int s;

    // allocate socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (s < 0)
	{
		perror("socket() failed");
		exit(EXIT_FAILURE);
	}

    int on = 1;
    // Allow socket descriptor to be reuseable.
    int rc_opt = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if (rc_opt < 0)
	{
		perror("setsockopt() failed");
		close(s);
		exit(EXIT_FAILURE);
	}

	// Set socket to be nonblocking. All of the sockets for
	// the incoming connections will also be nonblocking since
	// they will inherit that state from the listening socket.
	int rc_ioctl = ioctl(s, FIONBIO, (char *)&on);
	if (rc_ioctl < 0)
	{
		perror("ioctl() failed");
		close(s);
		exit(EXIT_FAILURE);
	}

    // bind socket to first available port of the first available
    // local bluetooth adapter
    loc_addr.rc_family = AF_BLUETOOTH;;
    loc_addr.rc_bdaddr = *BDADDR_ANY;
    int rc_bind = dynamic_bind_rc(s, &loc_addr);
    if (rc_bind < 0)
	{
		perror("bind() failed");
		close(s);
		exit(EXIT_FAILURE);
	}
    *port = loc_addr.rc_channel;

    // Print binded information.
    char bdaddr_str[32] = { 0 };
    ba2str( &loc_addr.rc_bdaddr, bdaddr_str );
    printf("Binded to address \"%s\" and channel %d.\n", bdaddr_str, loc_addr.rc_channel);

    // put socket into listening mode accepting only one connection.
    int rc = listen(s, 1);
	if (rc < 0)
	{
		perror("listen() failed");
		close(s);
		exit(EXIT_FAILURE);
	}
    printf("Listening.\n");

    return s;
}

int process_connection(int *max_sd, fd_set *master_set, int s)
{
	int end_server = 0;
	int client = 0; //, bytes_read;
	do
	{
	    struct sockaddr_rc rem_addr = { 0 };
	    socklen_t opt = sizeof(rem_addr);
		// accept one connection
		client = accept(s, (struct sockaddr *)&rem_addr, &opt);
		if (client < 0)
		{
			if (errno != EWOULDBLOCK)
			{
				perror("  accept() failed");
				end_server = 1;
			}
			break;
		}
		printf("  New incoming connection - %d\n", client);
		char buf[32] = {0};
		ba2str( &rem_addr.rc_bdaddr, buf );
		fprintf(stderr, "accepted connection from %s\n", buf);
		memset(buf, 0, sizeof(buf));

		FD_SET(client, master_set);
		if (client > *max_sd)
		*max_sd = client;
	}
	while (client != -1);

	return end_server;
}

int read_connection(int i)
{
	int close_conn = 0;
	// Receive all incoming data on this socket
	// before we loop back and call select again.
	do
	{
		// Receive data on this connection until the
		// recv fails with EWOULDBLOCK.  If any other
		// failure occurs, we will close the
		//connection.
		char buf[1024] = {0};
		int rc_recv = recv(i, buf, sizeof(buf), 0);
		if (rc_recv < 0)
		{
			if (errno != EWOULDBLOCK)
			{
				perror("  recv() failed");
				close_conn = 1;
			}
			break;
		}
		// Check to see if the connection has been
		// closed by the client
		if (rc_recv == 0)
		{
			printf("  Connection closed\n");
			close_conn = 1;
			break;
		}
		// Data was received
		int len = rc_recv;
		printf("  %d bytes received \"%s\"\n", len, buf);
		// Echo the data back to the client
		int rc_send = send(i, buf, len, 0);
		if (rc_send < 0)
		{
			perror("  send() failed");
			close_conn = 1;
			break;
		}
	} while (1);

	return close_conn;
}

// If the close_conn flag was turned on, we need
// to clean up this active connection.  This
// clean up process includes removing the
// descriptor from the master set and
// determining the new maximum descriptor value
// based on the bits that are still turned on in
// the master set.
void close_connection(int i, int *max_sd, fd_set *master_set)
{
	close(i);
	FD_CLR(i, master_set);
	if (i == *max_sd)
	{
		while (FD_ISSET(*max_sd, master_set) == 0)
		*max_sd -= 1;
	}
}

int loop_descriptors(int rc_select, int *max_sd, fd_set *master_set, fd_set *working_set, int s)
{
	int end_server = 0;
	int desc_ready = rc_select;
	int i;
	for (i=0; i <= *max_sd  &&  desc_ready > 0; ++i)
	{
		// Check to see if this descriptor is ready.
		if (FD_ISSET(i, working_set))
		{
			desc_ready -= 1;
			// Check to see if this is the listening socket.
			if (i == s)
			{
				printf("  Listening socket is readable\n");
				end_server = process_connection(max_sd, master_set, s);
			}
			// This is not the listening socket, therefore an
			// existing connection must be readable
			else
			{
				printf("  Descriptor %d is readable\n", i);
				int close_conn = read_connection(i);
				if (close_conn)
				{
					close_connection(i, max_sd, master_set);
				}
			} /* End of existing connection is readable */
		} /* End of if (FD_ISSET(i, &working_set)) */
	} /* End of loop through selectable descriptors */

	return end_server;
}

void accept_connection(int s)
{
    int max_sd;
    struct timeval       timeout;
    fd_set        master_set, working_set;

    // Initialize the master fd_set
	FD_ZERO(&master_set);
	max_sd = s;
	FD_SET(s, &master_set);

	int end_server = 0;

	// Loop waiting for incoming connects or for incoming data
	// on any of the connected sockets.
	do
	{
		// Copy the master fd_set over to the working fd_set.
		memcpy(&working_set, &master_set, sizeof(master_set));
		// Initialize the timeval struct to 3 minutes.  If no
		// activity after 3 minutes this program will end.
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
			end_server = loop_descriptors(rc_select, &max_sd, &master_set, &working_set, s);
		}
		// Else if rc_select == 0 then the 5 minute time out expired.
		else
		{
			printf("  select() timed out.  End program.\n");
			break;
		}
	}
	while (end_server == 0);

	// Clean up all of the sockets that are open.
	int i;
	for (i=0; i <= max_sd; ++i)
	{
		if (FD_ISSET(i, &master_set))
		close(i);
	}

}
