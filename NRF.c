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

void NRF_WriteMBReg(uint8_t regAddr, uint8_t *pBuf, uint8_t countBytes)
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

  NRF_CE_HIGH;

  NRF_Delay(1);

  NRF_FlushRX();
  NRF_FlushTX();
}

void NRF_TX_Mode(void)
{
	NRF_CE_LOW;

	uint8_t config = NRF_ReadReg(NRF_REG_CONFIG);

	if(!(config & _BV(PWR_UP)))
	{
		config |= _BV(PWR_UP);
		NRF_WriteReg(NRF_REG_CONFIG, config);
		NRF_Delay(5); //1.5ms
	}

	config = NRF_ReadReg(NRF_REG_CONFIG);
	config &= ~_BV(PRIM_RX);
	NRF_WriteReg(NRF_REG_CONFIG, config);

	NRF_Delay(1);

	NRF_FlushRX();
	NRF_FlushTX();
}


void NRF_GetPacket(uint8_t *buf)
{
	uint8_t nop = 0xFF;
	uint8_t reg = R_RX_PAYLOAD;
	uint8_t status = NRF_ReadReg(NRF_REG_STATUS);

	NRF_CSN_LOW;
	HAL_SPI_Transmit(&hspi1, &reg, 1, 1000);

	uint8_t dataLength = 0;
	HAL_SPI_TransmitReceive(&hspi1, &nop, &dataLength, 1, 1000);

	if(dataLength == 0xff)
		return;


	HAL_SPI_TransmitReceive(&hspi1, &nop, buf, dataLength, 1000);

	uint8_t en_dpl = NRF_ReadReg(NRF_REG_FEATURE) & (1<<(2));
	if(en_dpl)
	{
		uint8_t blank = 32 - dataLength;
		HAL_SPI_Transmit(&hspi1, &nop, blank, 1000);
	}

	NRF_CSN_HIGH;
	NRF_WriteReg(NRF_REG_STATUS, _BV(RX_DR) | _BV(MAX_RT) | _BV(TX_DS));
}


/*
void NRF_GetPacket(uint8_t *buf)
{
	uint8_t nop = 0xFF;
	uint8_t reg = R_RX_PAYLOAD;

	NRF_CSN_LOW;
	HAL_SPI_Transmit(&hspi1, &reg, 1, 1000);

	uint8_t currentBytePosition = -1;
	uint8_t currentReadByte = 0;

	char debugBuff[32];
	while(++currentBytePosition < 32 && currentReadByte != '\n')
	{
		HAL_SPI_TransmitReceive(&hspi1, &nop, &currentReadByte, 1, 1000);
		buf[currentBytePosition] = currentReadByte;
		debugBuff[currentBytePosition] = currentReadByte;

	}

	uint8_t en_dpl = NRF_ReadReg(NRF_REG_FEATURE) & (1<<(2));
	if(en_dpl)
	{
		uint8_t blank = 32 - currentBytePosition;
		HAL_SPI_Transmit(&hspi1, &nop, blank, 1000);
	}

	NRF_CSN_HIGH;
	NRF_WriteReg(NRF_REG_STATUS, _BV(RX_DR) | _BV(MAX_RT) | _BV(TX_DS));
}
*/
bool NRF_SendPacket(uint8_t *receiverAddress,uint8_t *buf, uint8_t length, uint8_t writeType)
{
	NRF_WriteMBReg(NRF_REG_TX_ADDR, receiverAddress, 5);

	uint8_t feature = NRF_ReadReg(NRF_REG_FEATURE);
	bool en_dpl = feature & _BV(EN_DPL);

	NRF_CSN_LOW;
	HAL_SPI_Transmit(&hspi1, &writeType, 1, 1000);
	HAL_SPI_Transmit(&hspi1, buf, length, 1000);

	if(!en_dpl)
	{
		uint8_t blank = 32 - length;
		HAL_SPI_Transmit(&hspi1, &NRF_CMD_NOP, blank, 1000);
	}
	NRF_CSN_HIGH;

	NRF_CE_HIGH;
	DelayMicro(15);
	NRF_CE_LOW;

	uint8_t status = NRF_ReadReg(NRF_REG_STATUS);

	if(status & _BV(TX_DS))
	{
		NRF_WriteReg(NRF_REG_STATUS, 0x20);
		return true;
	}

	if(status & _BV(MAX_RT))
	{
		NRF_WriteReg(NRF_REG_STATUS, 0x10);
		NRF_FlushTX();
		return false;
	}
}

bool NRF_IsAvailablePacket(void)
{
	return !(NRF_ReadReg(NRF_REG_FIFO_STATUS) & _BV(RX_EMPTY));
}

void NRF_ClearMessageBuff(void)
{
	for(int i = 0; i < NRF_MessageBuffSize; ++i)
		NRF_MessageBuff[i] = 0;
}

__STATIC_INLINE void DelayMicro(__IO uint32_t micros)
{
  micros *= (SystemCoreClock / 1000000) / 9;

  /* Wait till done */

  while (micros--) ;

}