/*
 * hciscan.c
 *
 *  Created on: 6 марта 2016 г.
 *      Author: bakulev
 */

#include <stdlib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "hciscan.h"

void hciscan(bdaddr_t *target_bdaddr)
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
		exit(EXIT_FAILURE);
	}

	len = 8;
	max_rsp = 255;
	flags = IREQ_CACHE_FLUSH;
	devices =(inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

	num_rsp = hci_inquiry(adapter_id, len, max_rsp, NULL, &devices, flags);
	if (num_rsp < 0) perror("hci_inquiry");

	printf("Select device to send data:\n");
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
	do scanf("%d", &i);	while (i < 0 || i > num_rsp);
	if (i >= 0 && i < num_rsp) bacpy(target_bdaddr, &(devices + i)->bdaddr);

	free(devices);
}
