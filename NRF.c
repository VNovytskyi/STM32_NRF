extern SPI_HandleTypeDef hspi1;

//TODO: SPI_HandleTypeDef *NRF_SPI = &hspi1;

void NRF_DefaultInit(void)
{
	NRF_CE_LOW;
	NRF_Delay(1);
	NRF_WriteReg(NRF_REG_EN_AA, 0x3f); //Enable auto Acknowledgment pipe1 0x3f
  NRF_WriteReg(NRF_REG_EN_RXADDR, 0x03); // Enable rx address pipe1
  NRF_WriteReg(NRF_REG_SETUP_AW, 0x03); // Address width 5 bytes
	NRF_WriteReg(NRF_REG_SETUP_RETR, 0x5F); // 1500us, 15 retrans
	NRF_WriteReg(NRF_REG_RF_CH, 0x60); // Set 96 channel
	NRF_WriteReg(NRF_REG_RF_SETUP, 0x27); //0dBm, 250kbps
	NRF_ToggleFeatures();
	NRF_WriteReg(NRF_REG_FEATURE, 0x06);
	NRF_WriteReg(NRF_REG_DYNPD, 0x3F); //Enable dynamic payloads on all pipes

	uint8_t NRF_TX_Addr[] = {'1', 'N', 'o', 'd', 'e'};
	uint8_t *NRF_RX_Addr = NRF_TX_Addr;
	NRF_WriteMBReg(NRF_REG_RX_ADDR_P0, NRF_RX_Addr, 5);
	NRF_WriteMBReg(NRF_REG_RX_ADDR_P1, NRF_RX_Addr, 5);

	NRF_FlushRX();
	NRF_FlushTX();
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

__STATIC_INLINE void DelayMicro(__IO uint32_t micros)
{
  micros *= (SystemCoreClock / 1000000) / 9;

  /* Wait till done */

  while (micros--) ;

}
