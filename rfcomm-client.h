/*
 * main.h
 *
 *  Created on: 6 марта 2016 г.
 *      Author: bakulev
 */

#ifndef RFCOMM_CLIENT_H_
#define RFCOMM_CLIENT_H_

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

int service_search(bdaddr_t *target, uuid_t *service_uuid);
int rfcomm_client(bdaddr_t *target, uint8_t port);

#endif /* RFCOMM_CLIENT_H_ */
