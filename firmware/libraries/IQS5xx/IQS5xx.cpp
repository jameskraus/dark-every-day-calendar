#include "IQS5xx.h"
#include "IQS5xx_hal.h"
#include "IQS550_B000_Trackpad_40_15_2_2_BL.h"

IQS::IQS()
{
    x = 15;
    y = 15;
    i2cDeviceAddr = 0x74;
    ready_pin = 255;
    reset_pin = 255;
}
IQS::IQS(uint8_t address, uint8_t readyPin, uint8_t resetPin)
{
    x = 15;
    y = 15;
    i2cDeviceAddr = address;
    ready_pin = readyPin;
    reset_pin = resetPin;
}

void IQS::initialize()
{
    IQS5xx_I2C_Setup();
    IQS5xx_ReadySignal_Init(ready_pin);
    IQS5xx_ResetSignal_Init(reset_pin);
    IQS5xx_delay(20);
}

void IQS::holdReset(bool held)
{
    bool state = held ? 0 : 1;
    IQS5xx_ResetSet(reset_pin, state);
}

bool IQS::isReady()
{
    return IQS5xx_ReadySignal(ready_pin);
}

int8_t IQS::programFirmware(uint8_t newDeviceAddress)
{
    uint8_t retval;
    static uint8_t writeBuffer[BL_BLOCK_SIZE_BYTES];
    static uint8_t readBuffer[BL_BLOCK_SIZE_BYTES];
    uint8_t bootloaderAddress = BL_ADDR(i2cDeviceAddr);
    uint16_t offset;
    uint16_t bootloaderVersion;
    bool bootloaderEntered, nvFail;

    // Reset
    holdReset(true);
    IQS5xx_delay(20);
    holdReset(false);
    IQS5xx_delay(1);

    bootloaderEntered = false;
    for (uint8_t t=0; t<10; t++){
        if(IQS5xx_I2C_Read(bootloaderAddress, (uint8_t)BL_CMD_VERSION, readBuffer , 2) != 0){
            continue;
        }
        bootloaderEntered = true;
        bootloaderVersion = (readBuffer[0] << 8) + readBuffer[1];
        if(bootloaderVersion == BL_SUPPORTED_VERSION){
            break;
        }
    }

    if(bootloaderEntered){
        if(bootloaderVersion != BL_SUPPORTED_VERSION){
            // Bootloader is an unsupported version
            return -1;
        }
  	}else{
      // bootloader couldn't be entered.
      return -2;
    }

    //
  	// Now write the program area, which is address 0x8400 to 0xBFFF.
  	// This is written in blocks of 64 bytes, and gives a total of 240
  	// such blocks required to write.
  	//
    for (uint8_t blockNo = 0; blockNo<240; blockNo++){
        offset = (blockNo * BL_BLOCK_SIZE_BYTES);
        memcpy_P(writeBuffer, hex_array + offset, BL_BLOCK_SIZE_BYTES);
        if(blockNo == 239){
            // This is the last block of NV settings.
            // The last byte of this block configures the IQS550's I2C address
            writeBuffer[BL_BLOCK_SIZE_BYTES-1] = newDeviceAddress;
        }
        if ((retval = IQS5xx_I2C_Write(bootloaderAddress, (uint16_t)BL_APP_START_ADDR + offset, writeBuffer, BL_BLOCK_SIZE_BYTES)) !=0){
            // Communication error
            return -3;
        }
        // This delay is necessary to give the bootloader time to write
        // the 64 byte buffer into memory
        // This delay is only necessary after each block write
        IQS5xx_delay(7);
    }

    //
    // Now program the checksum descriptor, so that the CRC check can be run
    //
    IQS5xx_I2C_Write(bootloaderAddress, (uint16_t)BL_CHECKSUM_START_ADDR, crc_array, BL_BLOCK_SIZE_BYTES);
    IQS5xx_delay(7);

    //
    // Perform CRC check, a '0x00' returned is pass, '0x01' is fail
    //
    readBuffer[0] == 0xFF;
    IQS5xx_I2C_Read(bootloaderAddress, (uint8_t)BL_CMD_CRCCHECK, readBuffer, 1);
    if(readBuffer[0] != 0x00){
       // CRC Failure
       return -4;
    }

    //
  	// The IQS5xx-B000 non-volatile memory does not form part of the CRC
  	// checksum, so a read back and compare is needed to verify this section,
  	// which is located 0xBE00 to 0xBFFF (512 bytes, or 8x64 blocks)
  	//
    nvFail = false;
    for (uint8_t blockNo = 232; blockNo<240; blockNo++){
        offset = (blockNo * BL_BLOCK_SIZE_BYTES);
        memcpy_P(writeBuffer, hex_array + offset, BL_BLOCK_SIZE_BYTES);
        if(blockNo == 239){
            // This is the last block of NV settings.
            // The last byte of this block configures the IQS550's I2C address
            writeBuffer[BL_BLOCK_SIZE_BYTES-1] = newDeviceAddress;
        }

        uint16_t memAddress = BL_APP_START_ADDR+offset;
        uint8_t writeBytes[3];
        writeBytes[0] = BL_CMD_READ;
        writeBytes[1] = (uint8_t)(memAddress>>8);
        writeBytes[2] = (uint8_t)memAddress;
        IQS5xx_I2C_Write_Read(bootloaderAddress, writeBytes, sizeof(writeBytes), NULL, 0, readBuffer, 64);

        if(memcmp(writeBuffer, readBuffer, BL_BLOCK_SIZE_BYTES) != 0){
            nvFail = true;
            IQS5xx_print("Block compare failure # ");
            IQS5xx_println(blockNo);
        }
    }
    if(nvFail){
      // NV Settings read-back failure
      return -5;
    }

    //
  	// Give command to enter application code, thus exiting bootloader
  	//
    IQS5xx_I2C_Write(bootloaderAddress, (uint8_t)BL_CMD_EXIT, writeBuffer, 1);
    delay(1000);

    // Set device address to new address
    i2cDeviceAddr = newDeviceAddress;

    IQS_Version version;
    checkVersion(&version);
    if((version.product != 40)
    || (version.project != 15)
    || (version.major_version != 2)
    || (version.minor_version != 2))
    {
      // Version check failure
      // Looks like the firmware did not actually stick
      return -6;
    }

    return 0;
}

void IQS::readTouch()
{
    uint8_t maxTouch = 0xFF;
    IQS5xx_I2C_Read(i2cDeviceAddr, (uint16_t)MaxTouch_adr, &maxTouch, 1);
    x = maxTouch>>4;
    y = maxTouch&0x0F;
}

bool IQS::isTouched(){
    return (x!=15) && (y!=15);
}

bool IQS::registerWriteWithMask(uint16_t registerAddr, uint8_t value, uint8_t mask){
    uint8_t registerValue;
    uint8_t retval;
    if ((retval = IQS5xx_I2C_Read(i2cDeviceAddr, (uint16_t)registerAddr, &registerValue, 1)) !=0){
      return false;
    }
    registerValue &= ~mask;
    registerValue |= (value & mask);
    if ((retval = IQS5xx_I2C_Write(i2cDeviceAddr, (uint16_t)registerAddr, &registerValue, 1)) !=0){
      return false;
    }
    return true;
}

bool IQS::registerWrite(uint16_t registerAddr, const uint8_t* pData, uint8_t length){
    return (IQS5xx_I2C_Write(i2cDeviceAddr, (uint16_t)registerAddr, pData, length) == 0);
}
bool IQS::registerWrite(uint16_t registerAddr, uint8_t byte){
    return (IQS5xx_I2C_Write(i2cDeviceAddr, (uint16_t)registerAddr, &byte, 1) == 0);
}
bool IQS::registerWrite_twoBytesBigEndian(uint16_t registerAddr, uint16_t value){
  uint16_t bigEndian= (value << 8) | ((value >> 8 ) & 0xFF);
  return (IQS5xx_I2C_Write(i2cDeviceAddr, (uint16_t)registerAddr, (uint8_t*)&bigEndian, 2) == 0);
}

bool IQS::registerRead(uint16_t registerAddr, uint8_t* pData, uint8_t length=1){
  if(IQS5xx_I2C_Read(i2cDeviceAddr, (uint16_t)registerAddr, pData, length) !=0){
      return false;
  }
}
bool IQS::registerRead_twoBytesBigEndian(uint16_t registerAddr, uint16_t* pValue){
    uint16_t bigEndian=0;
    if(IQS5xx_I2C_Read(i2cDeviceAddr, (uint16_t)registerAddr, (uint8_t*)&bigEndian, 2) !=0){
        return false;
    }
    uint16_t littleEndian = (bigEndian << 8) | ((bigEndian >> 8 ) & 0xFF);
    *pValue = littleEndian;
    return true;
}

void IQS::setupComplete(){
  registerWriteWithMask(SystemConfig0_adr, SETUP_COMPLETE, SETUP_COMPLETE);
}

void IQS::setManualControl(){
    registerWriteWithMask(SystemConfig0_adr, MANUAL_CONTROL, MANUAL_CONTROL);
    registerWriteWithMask(SystemControl0_adr, 0x00, 0x07); //Active Mode
}

void IQS::setTXRXChannelCount(uint8_t tx_count, uint8_t rx_count){
    if((tx_count > 15) || (rx_count > 10)){
      return;
    }
    registerWrite(TotalTx_adr, tx_count);
    registerWrite(TotalRx_adr, rx_count);
}

// p_txPinMappings should be an array of TX pin numbers.
// count is number of active channels
void IQS::setTXPinMappings(uint8_t* p_txPinMappings, uint8_t count){
    if(count > 15){
      return;
    }
    registerWrite(TxMapping_adr, p_txPinMappings, count);
    registerWrite(TotalTx_adr, count);
}

// p_rxPinMappings should be an array of RX pin numbers.
// count is number of active channels
void IQS::setRXPinMappings(uint8_t* p_rxPinMappings, uint8_t count){
    if(count > 10){
      return;
    }
    registerWrite(RxMapping_adr, p_rxPinMappings, count);
    registerWrite(TotalRx_adr, count);
}

void IQS::enableChannel(uint8_t txChannel, uint8_t rxChannel, bool enable){
  if((txChannel >= 15) || (rxChannel >= 10)){
    return;
  }
  uint16_t registerAddy = ActiveChannels_adr + (txChannel * 2);
  uint8_t mask, value;
  if(rxChannel >= 8){
    mask = 1 << (rxChannel - 8);
  }else{
    registerAddy += 1;
    mask = 1 << rxChannel;
  }
  value = enable ? mask : 0x00;
  registerWriteWithMask(registerAddy, value , mask);
}

void IQS::setChannel_ATI_C_Adjustment(uint8_t txChannel, uint8_t rxChannel, int8_t adjustment){
  if((txChannel >= 15) || (rxChannel >= 10)){
    return;
  }
  uint16_t registerAddy = ATICAdjust_adr + (txChannel * 10) + rxChannel;
  registerWrite(registerAddy, (uint8_t)adjustment);
}

void IQS::swapXY(bool swap){
  uint8_t value = swap ? SWITCH_XY_AXIS : 0x00;
  registerWriteWithMask(XYConfig0_adr, value, SWITCH_XY_AXIS);
}

void IQS::enablePalmRejection(bool enable){
  uint8_t value = enable ? PALM_REJECT : 0x00;
  registerWriteWithMask(XYConfig0_adr, value, PALM_REJECT);
}

void IQS::setAtiGlobalC(uint8_t global_c){
    registerWrite(GlobalATIC_adr, global_c);
}
void IQS::setAtiTarget(uint16_t target){

}

void IQS::setTouchMultipliers(uint8_t set, uint8_t clear){
    registerWrite(GlobalTouchSet_adr, set);
    registerWrite(GlobalTouchClear_adr, clear);
}

void IQS::rxFloat(bool floatWhenInactive){
  uint8_t value = floatWhenInactive ? RX_FLOAT : 0x00;
  registerWriteWithMask(HardwareSettingsA_adr, value, RX_FLOAT);
}


//*****************************************************************************
//
//! Acknowledge the reset flag
//!
//! This function simply sets the ACK_RESET bit found in the SYSTEM_CONTROL_0
//! register.  By setting this bit the SHOW_RESET flag is cleared in the
//! SYSTEM_INFO_0 register.  During normal operation, the SHOW_RESET bit can be
//! monitored and if it becomes set, then an unexpected reset has occurred.
//! If any device configuration is needed, it must then be repeated.
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
void IQS::acknowledgeReset(){
    registerWriteWithMask(SystemControl0_adr, ACK_RESET, ACK_RESET);
}

void IQS::runAtiAlgorithm(){
    registerWriteWithMask(SystemControl0_adr, AUTO_ATI, AUTO_ATI);
}

void IQS::reseed(){
    registerWriteWithMask(SystemControl0_adr, RESEED, RESEED);
}

bool IQS::atiErrorDetected(){
    static  uint8_t System_info_0 = 0;
    if(IQS5xx_I2C_Read(i2cDeviceAddr, (uint16_t)SystemInfo0_adr, &System_info_0, 1) != 0){
      return false;
    }
    return System_info_0 & ATI_ERROR;
}
bool IQS::reAtiOccurred(){
    static  uint8_t System_info_0 = 0;
    IQS5xx_I2C_Read(i2cDeviceAddr, (uint16_t)SystemInfo0_adr, &System_info_0, 1);
    return System_info_0 & REATI_OCCURRED;
}

void IQS::readTxAtiCompensation(uint8_t txChannel, uint8_t compensations[10]){
  if(txChannel >= 15){
    return;
  }
  uint16_t registerAddy = ATICompensation_adr + (txChannel * 10);
  IQS5xx_I2C_Read(i2cDeviceAddr, (uint16_t)registerAddy, compensations, 10);
}


//*****************************************************************************
//
//! Terminate communication window
//!
//! The IQS5xx B000 does not close the communication window on the reception of
//!  the STOP bit; therefore this function sends the END COMMUNICATION WINDOW
//! COMMAND (Please see datasheet for more information). RDY will go low after
//! receiving any write to 0xEEEE followed by a STOP.
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
void IQS::endSession()
{
  IQS5xx_I2C_Write(i2cDeviceAddr, (uint16_t)EndWindow_adr, 0, 1);
  IQS5xx_delay(1);
}

//*****************************************************************************
//
//! Read and display version
//!
//! This function reads the version details from the IQS5xx and sends this to
//! the display port.
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
bool IQS::checkVersion(IQS_Version* pVersion)
{
    uint8_t ui8DataBuffer[6];
    //
    // Dont wait for RDY here, since the device could be in EventMode, and then
    // there will be no communication window to complete this.  Rather do a
    // forced communication, where clock stretching will be done on the IQS5xx
    // until an appropriate time to complete the i2c.
    //
    uint8_t retval = IQS5xx_I2C_Read(i2cDeviceAddr, (uint16_t)ProductNumber_adr, &ui8DataBuffer[0] ,6);
    if(retval != 0){
      IQS5xx_println("Error reading touch controller's version");
      return false;
    }
    pVersion->product = (ui8DataBuffer[0]<<8) + ui8DataBuffer[1];
    pVersion->project = (ui8DataBuffer[2]<<8) + ui8DataBuffer[3];
    pVersion->major_version = ui8DataBuffer[4];
    pVersion->minor_version = ui8DataBuffer[5];
    return true;
}
