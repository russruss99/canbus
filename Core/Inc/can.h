/*
 * can.h
 *
 *  Created on: Oct 8, 2026
 *      Author: capunom
 */

#ifndef INC_CAN_H_
#define INC_CAN_H_

#include "stm32f4xx.h"

// define target standard messaging identifier frame
#define LOGGER_ID 0x245

// Functions
void Loopback_Init(void);
uint8_t Transmit_Frame(uint32_t std_id, uint8_t *data_payload, uint8_t dlc);
uint8_t Receive_Frame(uint32_t *r_id, uint8_t *rx_payload);

#endif /* INC_CAN_H_ */
