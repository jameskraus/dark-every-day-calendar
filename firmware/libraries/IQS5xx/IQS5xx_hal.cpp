#include "IQS5xx_hal.h"
#include <Arduino.h>
#include <inttypes.h>
#include <Wire_BigBuffer.h>

static bool i2c_Initialized = false;

void IQS5xx_I2C_Setup(void)
{
  if(i2c_Initialized){
    return;
  }
  Wire.begin();
  Wire.setClock(400000);
  i2c_Initialized = true;
}


uint8_t IQS5xx_I2C_Write_Read(uint8_t deviceAddr, const uint8_t * iaddrData, uint8_t iaddrLength, const uint8_t * writeData, uint8_t writeLength, uint8_t *pReadData, uint8_t readLength)
{
  bool retval = 0;
  uint8_t *pData_iterator = pReadData;
  Wire.beginTransmission(deviceAddr);
  if(Wire.write(iaddrData, (unsigned int)iaddrLength) != iaddrLength
  || Wire.write(writeData, (unsigned int)writeLength) != writeLength){
      Wire.endTransmission(true);
      IQS5xx_print("I2C read error\tDevice Address: 0x");
      IQS5xx_print(deviceAddr,16);
      IQS5xx_print("\tError Code: ");
      IQS5xx_print(4);
      IQS5xx_print("\tLine# ");
      IQS5xx_println(__LINE__);
      return 4;
  }
  if((retval = Wire.endTransmission(false)) != 0){
      Wire.endTransmission(true);
      IQS5xx_print("I2C read error.\tDevice Address: 0x");
      IQS5xx_print(deviceAddr,16);
      IQS5xx_print("\tError Code: ");
      IQS5xx_print(retval);
      IQS5xx_print("\tLine# ");
      IQS5xx_println(__LINE__);
      return retval;
  }
  if(readLength != 0 && pReadData!=NULL){
      Wire.requestFrom(deviceAddr, readLength, (uint8_t)true);
      while(Wire.available() && (pData_iterator < (pReadData + readLength))){
        *pData_iterator = Wire.read();
        pData_iterator++;
      }
      if(pData_iterator != (pReadData + readLength)){
        IQS5xx_print("I2C read error.\tDevice Address: 0x");
        IQS5xx_print(deviceAddr,16);
        IQS5xx_print("\tError Code: ");
        IQS5xx_print(4);
        IQS5xx_print("\tLine# ");
        IQS5xx_print(__LINE__);
        IQS5xx_print("\tBytes RX'ed: ");
        IQS5xx_print(pData_iterator - pReadData);
        IQS5xx_print("/");
        IQS5xx_println(readLength);
        return 4; // Didn't receive enough bytes
      }
  }else{
    if((retval = Wire.endTransmission(true)) != 0){
        IQS5xx_print("I2C write error\tDevice Address: 0x");
        IQS5xx_print(deviceAddr,16);
        IQS5xx_print("\tError Code: ");
        IQS5xx_print(4);
        IQS5xx_print("\tLine# ");
        IQS5xx_println(__LINE__);
        return 4;
    }
  }
  return 0;
}

uint8_t IQS5xx_I2C_Write(uint8_t deviceAddr, uint8_t registerAddr, const uint8_t* pData, uint8_t length)
{
    return IQS5xx_I2C_Write_Read(deviceAddr, &registerAddr, sizeof(registerAddr), pData, length, NULL, 0);
}

uint8_t IQS5xx_I2C_Write(uint8_t deviceAddr, uint16_t registerAddr, const uint8_t* pData, uint8_t length)
{
    uint16_t registerAddr_bigEndian = (registerAddr << 8) | ((registerAddr >> 8 ) & 0xFF);
    return IQS5xx_I2C_Write_Read(deviceAddr, (uint8_t*)&registerAddr_bigEndian, sizeof(registerAddr_bigEndian), pData, length, NULL, 0);
}

uint8_t IQS5xx_I2C_Read(uint8_t deviceAddr, uint8_t registerAddr, uint8_t *pData, uint8_t length)
{
    return IQS5xx_I2C_Write_Read(deviceAddr, &registerAddr, 1, NULL, 0, pData, length);
}

uint8_t IQS5xx_I2C_Read(uint8_t deviceAddr, uint16_t registerAddr, uint8_t *pData, uint8_t length)
{
    uint16_t registerAddr_bigEndian = (registerAddr << 8) | ((registerAddr >> 8 ) & 0xFF);
    return IQS5xx_I2C_Write_Read(deviceAddr, (uint8_t*)&registerAddr_bigEndian, 2, NULL, 0, pData, length);
}

void IQS5xx_ReadySignal_Init(uint8_t readyPin){
    pinMode(readyPin, INPUT);
}

bool IQS5xx_ReadySignal(uint8_t readyPin){
    return digitalRead(readyPin) == HIGH;
}

void IQS5xx_ResetSignal_Init(uint8_t resetPin){
    pinMode(resetPin, OUTPUT);
}
void IQS5xx_ResetSet(uint8_t resetPin, uint8_t state){
    digitalWrite(resetPin, state);
}
