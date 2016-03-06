/*
 * main.h
 *
 *  Created on: 6 марта 2016 г.
 *      Author: bakulev
 */

#ifndef RFCOMM_SERVER_H_
#define RFCOMM_SERVER_H_

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h> // for struct sockaddr_rc

int dynamic_bind_rc(int sock, struct sockaddr_rc *sockaddr);
int rfcomm_server(uint8_t *port);
void accept_connection(int s);

#endif /* RFCOMM_SERVER_H_ */
