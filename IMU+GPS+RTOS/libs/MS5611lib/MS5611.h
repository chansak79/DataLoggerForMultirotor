#define MS5611_I2C                  I2C2

#define MS5611_ADDRESS_AD0_LOW     0x77 // address pin low (GND), default for InvenSense evaluation board
#define MS5611_ADDRESS_AD0_HIGH    0x78 // address pin high (VCC)
#define MS5611_DEFAULT_ADDRESS     (MS5611_ADDRESS_AD0_LOW<<1)

#define MS5611_CMD_ADC_READ         0x00
#define MS5611_CMD_RESET            0x1E
#define MS5611_CMD_CONV_D1          0x40
#define MS5611_CMD_CONV_D2          0x50
#define MS5611_CMD_READ_PROM        0xA2

#define MS5611_ULTRA_HIGH_RES       0x08
#define MS5611_HIGH_RES             0x06
#define MS5611_STANDARD             0x04
#define MS5611_LOW_POWER            0x02
#define MS5611_ULTRA_LOW_POWER      0x00


void MS5611_I2C_ByteWrite(u8 slaveAddr, u8* pBuffer, u8 writeAddr)
{
    // ENTR_CRT_SECTION();

    /* Send START condition */
    I2C_GenerateSTART(MS5611_I2C, ENABLE);

    /* Test on EV5 and clear it */
    while (!I2C_CheckEvent(MS5611_I2C, I2C_EVENT_MASTER_MODE_SELECT));

    /* Send MS5611 address for write */
    I2C_Send7bitAddress(MS5611_I2C, slaveAddr, I2C_Direction_Transmitter);

    /* Test on EV6 and clear it */
    while (!I2C_CheckEvent(MS5611_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    /* Send the MS5611's internal address to write to */
    I2C_SendData(MS5611_I2C, writeAddr);

    /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(MS5611_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    /* Send the byte to be written */
    I2C_SendData(MS5611_I2C, *pBuffer);

    /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(MS5611_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    /* Send STOP condition */
    I2C_GenerateSTOP(MS5611_I2C, ENABLE);

    // EXT_CRT_SECTION();
}

void MS5611_I2C_BufferRead(u8 slaveAddr, u8* pBuffer, u8 readAddr, u16 NumByteToRead)
{
    // ENTR_CRT_SECTION();

    /* While the bus is busy */
    while (I2C_GetFlagStatus(MS5611_I2C, I2C_FLAG_BUSY));

    /* Send START condition */
    I2C_GenerateSTART(MS5611_I2C, ENABLE);

    /* Test on EV5 and clear it */
    while (!I2C_CheckEvent(MS5611_I2C, I2C_EVENT_MASTER_MODE_SELECT));

    /* Send MPU6050 address for write */
    I2C_Send7bitAddress(MS5611_I2C, slaveAddr, I2C_Direction_Transmitter);

    /* Test on EV6 and clear it */
    while (!I2C_CheckEvent(MS5611_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    /* Clear EV6 by setting again the PE bit */
    I2C_Cmd(MS5611_I2C, ENABLE);

    /* Send the MS5611's internal address to write to */
    I2C_SendData(MS5611_I2C, readAddr);

    /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(MS5611_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    /* Send STRAT condition a second time */
    I2C_GenerateSTART(MS5611_I2C, ENABLE);

    /* Test on EV5 and clear it */
    while (!I2C_CheckEvent(MS5611_I2C, I2C_EVENT_MASTER_MODE_SELECT));

    /* Send MS5611 address for read */
    I2C_Send7bitAddress(MS5611_I2C, slaveAddr, I2C_Direction_Receiver);

    /* Test on EV6 and clear it */
    while (!I2C_CheckEvent(MS5611_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    /* While there is data to be read */
    while (NumByteToRead)
    {
        if (NumByteToRead == 1)
        {
            /* Disable Acknowledgement */
            I2C_AcknowledgeConfig(MS5611_I2C, DISABLE);

            /* Send STOP Condition */
            I2C_GenerateSTOP(MS5611_I2C, ENABLE);
        }

        /* Test on EV7 and clear it */
        if (I2C_CheckEvent(MS5611_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED))
        {
            /* Read a byte from the MS5611 */
            *pBuffer = I2C_ReceiveData(MS5611_I2C);

            /* Point to the next location where the byte read will be saved */
            pBuffer++;

            /* Decrement the read bytes counter */
            NumByteToRead--;
        }
    }

    /* Enable Acknowledgement to be ready for another reception */
    I2C_AcknowledgeConfig(MS5611_I2C, ENABLE);
}

uint32_t MS5611_GetRawPressure(){
  u8 tmp[3] = {0};
  MS5611_I2C_ByteWrite(MS5611_DEFAULT_ADDRESS, 0x00, MS5611_CMD_CONV_D1 + MS5611_HIGH_RES);
  Delay(100000);
  MS5611_I2C_BufferRead(MS5611_DEFAULT_ADDRESS, tmp, MS5611_CMD_ADC_READ, 3);
  return (uint32_t)(tmp[0]<<16) | (uint32_t)(tmp[1]<<8) | tmp[2] ;
}

uint32_t MS5611_GetRawTemperature(){
  u8 tmp[3] = {0};
  MS5611_I2C_ByteWrite(MS5611_DEFAULT_ADDRESS, 0x00, MS5611_CMD_CONV_D2 + MS5611_HIGH_RES);
  Delay(100000);
  MS5611_I2C_BufferRead(MS5611_DEFAULT_ADDRESS, tmp, MS5611_CMD_ADC_READ, 3);
  return (uint32_t)(tmp[0]<<16) | (uint32_t)(tmp[1]<<8) | tmp[2] ;
}

void MS5611_Calculate(uint16_t *fc, uint32_t rawTemp, uint32_t rawPres, int32_t *Temp, int32_t *Pres){


	int32_t dT = rawTemp - (uint32_t)fc[4] * 256;
	// Offset at actual temperature.
	int64_t OFF = (int64_t)fc[1] * 65536 + (int64_t)fc[3] * dT / 128;
	// Sensitivity at actual temperature.
	int64_t SENS = (int64_t)fc[0] * 32768 + (int64_t)fc[2] * dT / 256;
	// Calculate temperature.
	int32_t TEMP = 2000 + ((int64_t) dT * fc[5]) / 8388608;

	int64_t OFF2 = 0;
	int64_t SENS2 = 0;

	// Calculate temperature compensated pressure.
	if (TEMP < 2000)
	{
		// Low temperature
		OFF2 = 5 * ((TEMP - 2000) * (TEMP - 2000)) / 2;
	    SENS2 = 5 * ((TEMP - 2000) * (TEMP - 2000)) / 4;
	}

	if (TEMP < -1500)
	{
		// Very low temperature
	    OFF2 = OFF2 + 7 * ((TEMP + 1500) * (TEMP + 1500));
	    SENS2 = SENS2 + 11 * ((TEMP + 1500) * (TEMP + 1500)) / 2;
	}

	// If high temperature OFF2 = 0 and SENS2 = 0
	OFF = OFF - OFF2;
	SENS = SENS - SENS2;

	*Temp = TEMP;

	// Calculate pressure.
	*Pres = ((rawPres * SENS / 2097152) - OFF) / 32768;
}

uint16_t * MS5611_readPROM(){
  static uint16_t fc[6] = {0};
  u8 tmp[2] = {0};
  int i;
  for(i = 0;i < 6;i++){
	MS5611_I2C_BufferRead(MS5611_DEFAULT_ADDRESS, tmp, MS5611_CMD_READ_PROM+(i*2), 2);
    fc[i] = (u16)(tmp[0]<<8) | (tmp[1]);
  }
  return fc;
}

void MS5611_Initialize()
{
  // Reset device.
  MS5611_I2C_ByteWrite(MS5611_DEFAULT_ADDRESS, 0x00, MS5611_CMD_RESET);

  // Wait for device.
  Delay(100000);
}
