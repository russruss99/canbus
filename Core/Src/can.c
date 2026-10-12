/*
 * can.c
 *
 *  Created on: Oct 8, 2026
 *      Author: capunom
 */
#include "can.h"


void Loopback_Init(void)
{
	// 1. Gate clock power to GPIOB and the primary CAN1 Peripheral block
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_CAN1EN; // CAN1 sits on the 42 MHz APB1 Bus
	// 2. Configure PB8 and PB9 to Alternate Function Mode (Binary 10)
	GPIOB->MODER &= ~((3U << 16) | (3U << 18));
	GPIOB->MODER |=  ((2U << 16) | (2U << 18));
	// 3. Map PB8 and PB9 to AF9 (bxCAN Peripheral Controller Core)
	GPIOB->AFR[1] |= (9U << 0); //PB8-> AF9 (Using Low Index Array index 1)
	GPIOB->AFR[1] |= (9U << 4); //PB9-> AF9
	// Crucial Loopback Tweak: The RX line requires an internal hardware pull-up
	// to keep the line logic recessive while starting up independent of external nodes.
	GPIOB->PUPDR &= ~((3U << 16) | (3U << 18));
	GPIOB->PUPDR |=  ((1U << 16) | (1U << 18));
	// 4. Request Entry into CAN Initialization Mode (INRQ bit inside MCR)
	CAN1->MCR |= CAN_MCR_INRQ;
	while (!(CAN1->MSR & CAN_MSR_INAK)); // Wait until the hardware enters initialization state
	// 5. Exit Sleep Mode to activate the internal digital engines
	CAN1->MCR &= ~CAN_MCR_SLEEP;
	// 6. Configure Bit Timings inside the Bit Timing Register (BTR)
	// Goal: 125 Kbps Bitrate on a 42 MHz bus. Total Time Quanta (Tq) count = 16.
	// Prescaler (BRP) = 42,000,000 / (125,000 * 16) = 21 -> Value written = 21 - 1 = 20 (0x14)
	// Set Time Segment 1 (TS1) = 11 Tq (10 written), Time Segment 2 (TS2) = 4 Tq (3 written)
	// Enable Loopback Mode (LBKM bit 30) to bypass external transceivers
	CAN1->BTR |= 0;
	CAN1->BTR |= CAN_BTR_LBKM;
	CAN1->BTR |= (20U << 0) | (10U << 16) | (3U << 20);
	// 7. Request Exit from Initialization Mode to join the internal network
	CAN1->MCR &= ~CAN_MCR_INRQ;
	while (CAN1->MSR & CAN_MSR_INAK);
	// Wait for hardware confirmation
	// --- HARDWARE FILTER BANK CONFIGURATION ---
    // The filter registers require a sub-initialization mode toggle (FINIT bit in FMR)
	// Disable Filter Bank 0 temporarily to change parameters safely
	CAN1->FMR |= CAN_FMR_FINIT;
	// Set FIlter 0 to 32-bit Single Scale Mode (FSC0 bit in FS1R)
	CAN1->FS1R |=  (1U << 0);
	// Set FIlter 0 to Mask Mode (Id + Mask) rather than  Identifier List Mode
	CAN1->FM1R |= ~(1U << 0); // 0= Mask Mode
	// Configure the tracking rules inside Filter Register 1 and 2 (FR1 and FR2)
	// Standard Identifier bits reside in the upper 11 bits of the top half-word
	CAN1->sFilterRegister[0].FR1 = (LOGGER_ID << 21);
	CAN1->sFilterRegister[0].FR2 = (0x7FF << 21);
	// Route messages hitting FIlter Bank 0 directly to Receive HArdware FIFO 0
	CAN1->FFA1R &= ~(1U << 0); // 0 = FIFO 0 allocation
	// Re-enable Filter Bank 0 and drop out filter configuration mode
	CAN1->FA1R |= (1U << 0);
	CAN1->FMR  &= ~CAN_FMR_FINIT;
}

uint8_t Transmit_Frame(uint32_t std_id, uint8_t *data_payload, uint8_t dlc)
{

	if(CAN1->TSR & CAN_TSR_TME0)
	{

		CAN1->sTxMailBox[0].TIR = (std_id << 21);

		CAN1->sTxMailBox[0].TDTR = (dlc & 0x0F);

		CAN1->sTxMailBox[0].TDLR = (data_payload[3] << 24) | (data_payload[2] << 16) | (data_payload[1] << 8) | data_payload[0];
		CAN1->sTxMailBox[0].TDHR = (data_payload[7] << 24) | (data_payload[6] << 16) | (data_payload[5] << 8) | data_payload[4];

		CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
		return 1;
	}
	return 0;
}

uint8_t Receive_Frame(uint32_t *r_id, uint8_t *rx_payload)
{
	// Check if Receive FIFO 0 has any incoming frames queued up (FMP0 field bits [0:1])
	if((CAN1->RF0R & CAN_RF0R_FMP0) != 0)
	{
		// Extract sender identification standard ID frame mask
		*r_id = (CAN1->sFIFOMailBox[0].RIR >> 21);

		//unpack the low and high data register payloads back into our tracking array
		uint32_t low_reg = CAN1->sFIFOMailBox[0].RDLR;
		uint32_t high_reg = CAN1->sFIFOMailBox[0].RDHR;

		rx_payload[0] = (low_reg >> 0) & 0xFF;
		rx_payload[1] = (low_reg >> 8) & 0xFF;
		rx_payload[2] = (low_reg >> 16) & 0xFF;
		rx_payload[3] = (low_reg >> 24) & 0xFF;
		rx_payload[4] = (high_reg >> 0) & 0xFF;
		rx_payload[5] = (high_reg >> 8) & 0xFF;
		rx_payload[6] = (high_reg >> 16) & 0xFF;
		rx_payload[7] = (high_reg >> 24) & 0xFF;

		// Release the FIFI buffer block command handler to pop the stack (RFOM0 bit 5)
		CAN1->RF0R |= CAN_RF0R_RFOM0;

		return 1; // message captured and parsed successfully
	}

return 0; // No pending frames waiting in FIFO queue
}

