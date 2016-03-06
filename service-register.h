/*
 * main.h
 *
 *  Created on: 6 марта 2016 г.
 *      Author: bakulev
 */

#ifndef SERVICE_REGISTER_H_
#define SERVICE_REGISTER_H_

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

sdp_session_t *service_register(sdp_session_t *session, uuid_t *service_uuid, uint8_t rfcomm_channel);

#endif /* SERVICE_REGISTER_H_ */
