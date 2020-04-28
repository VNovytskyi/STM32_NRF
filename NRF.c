extern SPI_HandleTypeDef hspi1;

//TODO: SPI_HandleTypeDef *NRF_SPI = &hspi1;



bool NRF_EnableDynamicPayloadLength = false;



void NRF_Init(const uint8_t *NRF_TX_Addr, const uint8_t *NRF_RX1_Addr)
{
	NRF_CE_LOW;
	HAL_Delay(1);

	//RX mode and Power Up
	NRF_WriteReg(NRF_REG_CONFIG, 0x02);

	//Enable auto acknowledgment
	NRF_WriteReg(NRF_REG_EN_AA, 0x3F);

	// Enable pipe0 and pipe1
  NRF_WriteReg(NRF_REG_EN_RXADDR, 0x03);

  // Address width 5 bytes
  NRF_WriteReg(NRF_REG_SETUP_AW, 0x03);

  // 4000us, 15 retrans (0x5F)
	NRF_WriteReg(NRF_REG_SETUP_RETR, 0x5F);

	// Set 96 channel
	NRF_WriteReg(NRF_REG_RF_CH, 0x60);

	//0dBm, 250kbps
	NRF_WriteReg(NRF_REG_RF_SETUP, 0x27);

	//Clear RX_DR, TX_DS, MAX_RT
	NRF_WriteReg(NRF_REG_STATUS, 0x70);


	NRF_ToggleFeatures();
	NRF_WriteReg(NRF_REG_FEATURE, 0x07);

	//Enable dynamic payloads on all pipes
	NRF_WriteReg(NRF_REG_DYNPD, 0x3F);

	NRF_EnableDynamicPayloadLength = NRF_ReadReg(NRF_REG_FEATURE) & _BV(EN_DPL);

	NRF_WriteMBReg(NRF_REG_TX_ADDR, NRF_TX_Addr, 5);
	NRF_WriteMBReg(NRF_REG_RX_ADDR_P0, NRF_TX_Addr, 5);

	NRF_WriteMBReg(NRF_REG_RX_ADDR_P1, NRF_RX1_Addr, 5);

	NRF_FlushRX();
	NRF_FlushTX();

	NRF_ClearTxBuff();
	NRF_ClearRxBuff();

  NRF_RX_Mode();
}

uint8_t NRF_ReadReg(uint8_t regAddr)
{
	uint8_t regValue = 0x00;

	NRF_CSN_LOW;
	HAL_SPI_TransmitReceive(&hspi1, &regAddr, &regValue, 1, 1000);

	if(regAddr != NRF_REG_STATUS)
	{
		HAL_SPI_TransmitReceive(&hspi1, &NRF_CMD_NOP, &regValue,1,1000);
	}
	NRF_CSN_HIGH;

	return regValue;
}

void NRF_WriteReg(uint8_t regAddr, uint8_t regValue)
{
	uint8_t cmd = regAddr | W_REGISTER;
	NRF_CSN_LOW;
	HAL_SPI_Transmit(&hspi1, &cmd, 1, 1000);
	HAL_SPI_Transmit(&hspi1, &regValue, 1, 1000);
	NRF_CSN_HIGH;
}

void NRF_ON(void)
{
	uint8_t config = NRF_ReadReg(NRF_REG_CONFIG);

	config |= (1 << PWR_UP);

	NRF_WriteReg(NRF_REG_CONFIG, config);

	HAL_Delay(5);
}

void NRF_OFF(void)
{
	uint8_t config = NRF_ReadReg(NRF_REG_CONFIG);

	config &= ~(1 << PWR_UP);

	NRF_WriteReg(NRF_REG_CONFIG, config);
}

void NRF_ToggleFeatures(void)
{
  uint8_t dt[1] = {ACTIVATE};

  NRF_CSN_LOW;
  HAL_SPI_Transmit(&hspi1, dt, 1,1000);
  NRF_Delay(1);
  dt[0] = 0x73;
  HAL_SPI_Transmit(&hspi1, dt, 1,1000);

  NRF_CSN_HIGH;
}

void NRF_ReadMBReg(uint8_t regAddr, uint8_t *pBuf, uint8_t countBytes)
{
	NRF_CSN_LOW;
	HAL_SPI_Transmit(&hspi1, &regAddr, 1, 1000);
	HAL_SPI_Receive(&hspi1, pBuf, countBytes, 1000);
	NRF_CSN_HIGH;
}

void NRF_WriteMBReg(uint8_t regAddr, const uint8_t *pBuf, uint8_t countBytes)
{
	uint8_t cmd = regAddr | W_REGISTER;

	NRF_CSN_LOW;
	HAL_SPI_Transmit(&hspi1, &cmd, 1, 1000);
	NRF_Delay(1);
	HAL_SPI_Transmit(&hspi1, pBuf, countBytes, 1000);
	NRF_CSN_HIGH;
}

void NRF_FlushRX(void)
{
  uint8_t dt[1] = {FLUSH_RX};

  NRF_CSN_LOW;
  HAL_SPI_Transmit(&hspi1,dt,1,1000);
  NRF_Delay(1);

  NRF_CSN_HIGH;
}

void NRF_FlushTX(void)
{
  uint8_t dt[1] = {FLUSH_TX};

  NRF_CSN_LOW;
  HAL_SPI_Transmit(&hspi1,dt,1,1000);
  NRF_Delay(1);

  NRF_CSN_HIGH;
}

void NRF_RX_Mode(void)
{
  uint8_t regval = 0x00;
  regval = NRF_ReadReg(NRF_REG_CONFIG);
  regval |= (1<<PWR_UP)|(1<<PRIM_RX);

  NRF_WriteReg(NRF_REG_CONFIG, regval);

  if(!(regval & _BV(PWR_UP)))
  	HAL_Delay(5);

  NRF_CE_HIGH;

  HAL_Delay(1);

  NRF_FlushRX();

  HAL_Delay(5);
}

void NRF_TX_Mode(void)
{
	NRF_CE_LOW;
	HAL_Delay(15);

	uint8_t config = NRF_ReadReg(NRF_REG_CONFIG);

	if(!(config & _BV(PWR_UP)))
	{
		config |= _BV(PWR_UP);
		NRF_WriteReg(NRF_REG_CONFIG, config);
		HAL_Delay(5); //1.5ms
	}

	config = NRF_ReadReg(NRF_REG_CONFIG);
	config &= ~_BV(PRIM_RX);
	NRF_WriteReg(NRF_REG_CONFIG, config);

	NRF_FlushTX();

	HAL_Delay(5);
}


int8_t NRF_GetPacket(uint8_t *buf)
{
	uint8_t nop = 0xFF;
	uint8_t reg = R_RX_PAYLOAD;

	NRF_CSN_LOW;
	HAL_SPI_Transmit(&hspi1, &reg, 1, 1000);

	//First byte in packet tells us length of packet
	uint8_t dataLength = 0;
	HAL_SPI_TransmitReceive(&hspi1, &nop, &dataLength, 1, 1000);

	if(dataLength == 0xff)
		return -1;

	HAL_SPI_TransmitReceive(&hspi1, &nop, buf, dataLength, 1000);

	if(NRF_EnableDynamicPayloadLength == false)
	{
		uint8_t blank = 32 - dataLength;
		HAL_SPI_Transmit(&hspi1, &nop, blank, 1000);
	}
	NRF_CSN_HIGH;

	NRF_WriteReg(NRF_REG_STATUS, _BV(RX_DR));
	return 1;
}

/*
 * @brief Send one packet
 * @param
 */
int8_t NRF_SendPacket(uint8_t *receiverAddress, uint8_t *buf, uint8_t dataLength, uint8_t writeType)
{
	if(receiverAddress != NULL)
		NRF_WriteMBReg(NRF_REG_TX_ADDR, receiverAddress, 5);

	NRF_CSN_LOW;
	HAL_SPI_Transmit(&hspi1, &writeType, 1, 1000);
	HAL_SPI_Transmit(&hspi1, &dataLength, 1, 1000);
	HAL_SPI_Transmit(&hspi1, buf, dataLength, 1000);

	if(NRF_EnableDynamicPayloadLength == false)
	{
		uint8_t blank = 32 - dataLength;
		HAL_SPI_Transmit(&hspi1, &NRF_CMD_NOP, blank, 1000);
	}

	NRF_CSN_HIGH;
	NRF_CE_HIGH;
	HAL_Delay(1);
	NRF_CE_LOW;

	uint8_t status = NRF_ReadReg(NRF_REG_STATUS);

	//Wait for interrupt
	while(HAL_GPIO_ReadPin(NRF_IRQ_GPIO_Port, NRF_IRQ_Pin) != 0);

	status = NRF_ReadReg(NRF_REG_STATUS);

	//Packet was sent succesfully
	if(status & _BV(TX_DS))
	{
		//TODO: Send statisticks from OBSERVE_TX
		NRF_WriteReg(NRF_REG_STATUS, 0x20);
		return 1;
	}

	//Packet was not sent. Maximum number of TX retransmits.
	if(status & _BV(MAX_RT))
	{
		NRF_WriteReg(NRF_REG_STATUS, 0x10);
		NRF_FlushTX();
		return 0;
	}

	// TX_DS and MAX_RT registers did not change. Too little time has passed since sending!
	return -1;
}

int8_t NRF_SendOnePacketTo(uint8_t *receiverAddress, uint8_t *buf, uint8_t dataLength, uint8_t attemptCount, uint8_t delayBetweenAttemps)
{
	int8_t result = -2;

	NRF_TX_Mode();

	for(uint8_t i = 0; i < attemptCount && result != 1; ++i)
	{
		result = NRF_SendPacket(receiverAddress, buf, dataLength, W_TX_PAYLOAD);
	  HAL_Delay(delayBetweenAttemps);
	}

	NRF_RX_Mode();

	return result;
}

int8_t NRF_SendMessage(const uint8_t *receiverAddress, const uint8_t *buf)
{
	NRF_WriteMBReg(NRF_REG_TX_ADDR, receiverAddress, 5);

	const int8_t dataLength = strlen(buf);
	uint8_t amountPackets = ceil((double)dataLength / 25.0);

	NRF_TX_Mode();
	HAL_Delay(10);

	if(amountPackets == 1)
	{
		int8_t result = NRF_SendPacket(NULL, buf, 25, W_TX_PAYLOAD);

		if(result == -1)
			return -1;
	}
	else
	{
		for(uint8_t i = 0; i < amountPackets; ++i)
		{
			uint8_t currentData[30] = {0};
			memcpy(currentData, buf + (25 * i), 25);

			int8_t result = NRF_SendPacket(NULL, currentData, 25, W_TX_PAYLOAD);

			if(result == -1)
				return -1;

			//TODO: Уменьшить значение
			HAL_Delay(50);
		}
	}

	HAL_Delay(10);
	NRF_RX_Mode();

	return 1;
}

bool NRF_IsAvailablePacket(void)
{
	return !(NRF_ReadReg(NRF_REG_FIFO_STATUS) & _BV(RX_EMPTY));
	//return NRF_ReadReg(NRF_REG_STATUS) & _BV(6);
}

void NRF_ClearRxBuff(void)
{
	for(int i = 0; i < NRF_rxBuffSize; ++i)
		NRF_rxBuff[i] = 0;
}

void NRF_ClearTxBuff(void)
{
	for(int i = 0; i < NRF_txBuffSize; ++i)
			NRF_txBuff[i] = 0;
}

uint8_t NRF_GetStatus(void)
{
	return NRF_ReadReg(NRF_REG_STATUS);
}

uint8_t NRF_GetPipeNum(void)
{
	return (NRF_GetStatus() & 0b01110) >> 1;
}

void NRF_CallbackFunc(void)
{
	//char *buff = "NRF";
		//HAL_UART_Transmit(&huart1, buff, strlen(buff), 100);

	//char *buff = "_IRQ: ";
	//HAL_UART_Transmit(&huart1, buff, strlen(buff), 100);

	if(!(NRF_ReadReg(NRF_REG_FIFO_STATUS) & _BV(RX_EMPTY)))
	{
		//NRF_AvailablePacket = true;
		//char *buff = "RX FIFO not empty\n";
		//HAL_UART_Transmit(&huart1, buff, strlen(buff), 100);
	}

	/*
	uint8_t status = NRF_ReadReg(NRF_REG_STATUS);
	HAL_UART_Transmit(&huart1, &status, 1, 100);

	if(status & _BV(RX_DR))
	{
		char *buff = "RX_DR\n";
		HAL_UART_Transmit(&huart1, buff, strlen(buff), 100);
	}

	if(status & _BV(TX_DS))
	{
		char *buff = "TX_DS\n";
		HAL_UART_Transmit(&huart1, buff, strlen(buff), 100);
	}

	if(status & _BV(MAX_RT))
	{
		char *buff = "MAX_RT\n";
		HAL_UART_Transmit(&huart1, buff, strlen(buff), 100);
	}
*/

}

__STATIC_INLINE void DelayMicro(__IO uint32_t micros)
{
  micros *= (SystemCoreClock / 1000000) / 9;

  /* Wait till done */

  while (micros--) ;

}
