#ifndef IQS5XX_HAL_h
#define IQS5XX_HAL_h
#include <stdint.h>
#include <Arduino.h>

#if defined(IQS5xx_LOGGING) && IQS5xx_LOGGING
#define IQS5xx_print(...) Serial.print(__VA_ARGS__)
#define IQS5xx_println(...) Serial.println(__VA_ARGS__)
#else
#define IQS5xx_print(...)
#define IQS5xx_println(...)
#endif

#define IQS5xx_delay(ms) delay(ms)

void IQS5xx_I2C_Setup();
uint8_t IQS5xx_I2C_Write_Read(uint8_t deviceAddr, const uint8_t * iaddrData, uint8_t iaddrLength, const uint8_t * writeData, uint8_t writeLength, uint8_t *pReadData, uint8_t readLength);
uint8_t IQS5xx_I2C_Write(uint8_t deviceAddr, uint8_t registerAddr, const uint8_t* pData, uint8_t length);
uint8_t IQS5xx_I2C_Write(uint8_t deviceAddr, uint16_t registerAddr, const uint8_t* pData, uint8_t length);
uint8_t IQS5xx_I2C_Read(uint8_t deviceAddr, uint8_t registerAddr, uint8_t *pData, uint8_t length);
uint8_t IQS5xx_I2C_Read(uint8_t deviceAddr, uint16_t registerAddr, uint8_t *pData, uint8_t length);
void IQS5xx_ReadySignal_Init(uint8_t readyPin);
bool IQS5xx_ReadySignal(uint8_t readyPin);
void IQS5xx_ResetSignal_Init(uint8_t resetPin);
void IQS5xx_ResetSet(uint8_t resetPin, uint8_t state);

#endif // IQS5XX_HAL_h
