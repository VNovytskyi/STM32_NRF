#ifndef NRF_H_
#define NRF_H_

/* Bit Mnemonics */
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0
#define AW          0
#define ARD         4
#define ARC         0
#define PLL_LOCK    4
#define RF_DR       3
#define RF_PWR      6
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1
#define TX_FULL     0
#define PLOS_CNT    4
#define ARC_CNT     0
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0
#define DPL_P5	    5
#define DPL_P4	    4
#define DPL_P3	    3
#define DPL_P2	    2
#define DPL_P1	    1
#define DPL_P0	    0
#define EN_DPL	    2
#define EN_ACK_PAY  1
#define EN_DYN_ACK  0

#define NRF_CE_LOW  HAL_GPIO_WritePin(SPI1_CE_GPIO_Port, SPI1_CE_Pin, GPIO_PIN_RESET)
#define NRF_CE_HIGH HAL_GPIO_WritePin(SPI1_CE_GPIO_Port, SPI1_CE_Pin, GPIO_PIN_SET)

#define NRF_CSN_LOW  HAL_GPIO_WritePin(SPI1_CSN_GPIO_Port, SPI1_CSN_Pin, GPIO_PIN_RESET)
#define NRF_CSN_HIGH HAL_GPIO_WritePin(SPI1_CSN_GPIO_Port, SPI1_CSN_Pin, GPIO_PIN_SET)

#define NRF_REG_STATUS 0x07
#define W_REGISTER 0x20
#define ACTIVATE 0x50


#define FLUSH_TX 0xE1
#define FLUSH_RX 0xE2

#define PWR_UP      1
#define PRIM_RX     0

/* Register map */
#define NRF_REG_CONFIG 0x00
#define NRF_REG_EN_AA 0x01
#define NRF_REG_EN_RXADDR 0x02
#define NRF_REG_SETUP_AW 0x03
#define NRF_REG_SETUP_RETR 0x04
#define NRF_REG_RF_CH 0x05
#define NRF_REG_RF_SETUP 0x06
#define NRF_REG_PW_P0 0x11
#define NRF_REG_PW_P1 0x12
#define NRF_REG_FIFO_STATUS 0x17
#define NRF_REG_FEATURE 0x1D
#define NRF_REG_DYNPD 0x1C

#define NRF_REG_TX_ADDR 0x10
#define NRF_REG_RX_ADDR_P0 0x0A
#define NRF_REG_RX_ADDR_P1 0x0B

#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD 0xA0


#define NRF_Delay(miliseconds) HAL_Delay(miliseconds)
#define _BV(x) (1<<(x))

#define NRF_txBuffSize 32
#define NRF_rxBuffSize 32

uint8_t NRF_CMD_NOP = 0xFF;

void NRF_SetDefaultSettings(void);

uint8_t NRF_ReadReg(uint8_t regAddr);
void NRF_ReadMBReg(uint8_t regAddr, uint8_t *pBuf, uint8_t countBytes);

void NRF_WriteReg(uint8_t regAddr, uint8_t regValue);
void NRF_WriteMBReg(uint8_t regAddr, uint8_t *pBuf, uint8_t countBytes);

void NRF_ToggleFeatures(void);

void NRF24_FlushRX(void);
void NRF24_FlushTX(void);

void NRF_GetPacket(uint8_t *buf);
int8_t NRF_SendPacket(uint8_t *receiverAddress, uint8_t *buf, uint8_t writeType);
int8_t NRF_SendMessage(uint8_t *receiverAddress, uint8_t *buf);

void NRF_RX_Mode(void);
void NRF_TX_Mode(void);

bool NRF_IsAvailablePacket(void);
bool NRF_IsAvailableMessage(void);

void NRF_ClearRxBuff(void);
void NRF_ClearTxBuff(void);

void NRF_CallbackFunc(void);

__STATIC_INLINE void DelayMicro(__IO uint32_t micros);


#include "NRF.c"
#endif
