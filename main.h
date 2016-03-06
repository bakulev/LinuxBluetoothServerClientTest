/*
 * main.h
 *
 *  Created on: 6 марта 2016 г.
 *      Author: bakulev
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>

void show_service_uuid(uuid_t *service_uuid);
void show_bdaddr(bdaddr_t *target_bdaddr);
void server_register_and_start(uuid_t *service_uuid);
void client_start(bdaddr_t *target, uuid_t *service_uuid);

#endif /* MAIN_H_ */
