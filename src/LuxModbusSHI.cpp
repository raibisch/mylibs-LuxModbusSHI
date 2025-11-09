#include "LuxModbusSHI.h"

//----------------------------------------------------public Functions----------------------------------------------------

bool LuxModbusSHI::init(const char* sIP)
{
  _ipInitok = _ipLux.fromString(sIP);
  setTimeout(1000);
  debug_printf("[SHI] LuxModbusSHI-IP: %s\r\n",sIP);
  return _ipInitok;
}

#ifdef DEBUG_PRINT
int16_t LuxModbusSHI::readRegister(SHI_MODBUS_REGISTER reg)
{
 int16_t ival =holdingRegisterRead(reg);
 debug_printf("[SHI] REG:%d, value:%d\r\n", reg, ival);  
 return ival;
}

int16_t LuxModbusSHI::readInput(SHI_MODBUS_INPUT reg)
{ 
 int16_t ival =inputRegisterRead(reg);
 debug_printf("[SHI] INP:%d, value:%d\r\n",reg, ival);  
 return ival;
}
#endif

/// @brief  read Modbus Values from Luxtronik
void LuxModbusSHI::readSHIValues()
{  // Input-Register (read)
   _temp_outdoor = inputRegisterRead(SHI_MODBUS_INPUT::SHI_TEMP_OUTDOOR);
   _temp_ww      = inputRegisterRead(SHI_MODBUS_INPUT::SHI_TEMP_WW);
   _temp_ww_soll = inputRegisterRead(SHI_MODBUS_INPUT::SHI_TEMP_WW_SOLL);
   _power_out    = inputRegisterRead(SHI_MODBUS_INPUT::SHI_PWR_HEAT_OUT);
   _power_in     = inputRegisterRead(SHI_MODBUS_INPUT::SHI_PWR_ELECT_IN);
   _workingmode  = inputRegisterRead(SHI_MODBUS_INPUT::SHI_WORKINGMODE);
   _rl_ist       = inputRegisterRead(SHI_MODBUS_INPUT::SHI_TEMP_RL);
   _rl_soll      = inputRegisterRead(SHI_MODBUS_INPUT::SHI_TEMP_RL_SOLL);
   _vl_ist       = inputRegisterRead(SHI_MODBUS_INPUT::SHI_TEMP_VL);
   
   _sumenergy_out  = inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_SUM_OUT) << 16 | inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_SUM_OUT +1);
   _sumenergy_in   = inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_SUM_IN)  << 16 | inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_SUM_IN  +1);

   _heizenergy_out = inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_HEI_OUT) << 16 | inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_HEI_OUT +1);
   _heizenergy_in  = inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_HEI_IN)  << 16 | inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_HEI_IN  +1);

   _wwenergy_out   = inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_WW_OUT) << 16  | inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_WW_OUT +1);
   _wwenergy_in    = inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_WW_IN)  << 16  | inputRegisterRead(SHI_MODBUS_INPUT::SHI_ENERGY_WW_IN  +1);

   // Holding-Register (read/write)
   _heat_mode   = holdingRegisterRead(SHI_MODBUS_REGISTER::SHI_HEAT_MODE);
   _heat_offset = holdingRegisterRead(SHI_MODBUS_REGISTER::SHI_HEAT_OFFSET);

   _pc_mode     = holdingRegisterRead(SHI_MODBUS_REGISTER::SHI_PC_MODE);
   _pc_setpoint = holdingRegisterRead(SHI_MODBUS_REGISTER::SHI_PC_SETPOINT);

   _ww_extra    = holdingRegisterRead(SHI_MODBUS_REGISTER::SHI_WW_EXTRA);
 
  // ... Add more functions or define in derivied class
  //debug_println("Warning: no LuxModbusSHI.getvalues()  defined, derivate class");
}

void LuxModbusSHI::setDaylyEnergy(bool bNewDay)
{
  // test mit friend class  LUX_WEBSOCKET in c++
  wsFriend.energyInSum  = _sumenergy_in *1000;
  wsFriend.energyInHE   = _heizenergy_in *1000;
  wsFriend.energyInWW   = _wwenergy_in * 1000;
  wsFriend.energyOutSum = _sumenergy_out * 1000;
  wsFriend.energyOutHE  = _heizenergy_out * 1000;
  wsFriend.energyOutWW  = _wwenergy_out  * 1000;

   if (wsFriend.energyInSum == 0)
   {
    return;
   }

   if (bNewDay || wsFriend.energyOldInSum == 0)
   {
    debug_println("[LUX] ***bNewDay***")
     wsFriend.energyOldInSum    =  wsFriend.energyInSum;
     wsFriend.energyOldOutSum   =  wsFriend.energyOutSum;
    
     wsFriend.energyOldInWW     =  wsFriend.energyInWW;
     wsFriend.energyOldOutWW    =  wsFriend.energyOutWW;
    
     wsFriend.energyOldInHE     =  wsFriend.energyInHE;
     wsFriend.energyOldOutHE    =  wsFriend.energyOutHE;
   } 

}

/// @brief  integrate in main loop() with a timer (don't call more than one sec. per loop)
/// @return connected() status
bool LuxModbusSHI::poll(bool bNewDay)
{
  if (!connected() ) 
  {
    if (!_ipInitok)
    {
      debug_println("[SHI] LuxModbusSHI: call init() before loop()");
      return false;
    }
    // client not connected: start the Modbus TCP client
    debug_println("[SHI] try to connect to Modbus TCP server");

    if (!this->begin(_ipLux))
    {
      AsyncWebLog.printf("[SHI] Modbus TCP Error!")
      debug_println("[SHI] Modbus TCP Client failed to connect!");
    } else {
      debug_println("[SHI] Modbus TCP Client connected");
    }
  }
  else
  {
	  
#ifndef SHI_READONLY
    // client connected: write and read values
    // Luxtronik allows only ONE client to write to SHI-Interface over Modbus !!
    // set 'SHI_READONLY' for n+1 clients
    if (_bSet_heat_mode)
    {
      writeHeatOffset(_heat_offset);
      _bSet_heat_mode = false;
    }
    if (_bSet_pc_mode)
    {
      writePCSetpoint(SHI_PCMODE::PC_HARD, _pc_setpoint);
      _bSet_pc_mode = false;
    }
    if (_bSet_wwextra)
    {
       int iret = holdingRegisterWrite(SHI_MODBUS_REGISTER::SHI_WW_EXTRA,1);
       debug_printf("[SHI] WriteWWExtra: 1\r\n");
       AsyncWebLog.printf("[SHI] WriteHeatOffset: 1\r\n");
       _bSet_wwextra = false;
    }
#else
    _bSet_heat_mode = false;
    _bSet_pc_mode   = false;
    _bSet_wwextra   = false;
#endif

    readSHIValues();
    
    setDaylyEnergy(bNewDay);
    wsFriend.calcCopDay();

  }
  return connected();
}

static int16_t tempx10_old = 0;
bool LuxModbusSHI::writeHeatOffset(int16_t tempx10)
{ 
  if ((tempx10 < -40) || (tempx10 > 40)) // nur werte +/- 4 Kelvin
  {
    return false;
  }

  if (tempx10 == tempx10_old) // nur neue Werte schreiben
  {
    return true;
  }
  tempx10_old = tempx10;
  

  int iret = 0;
  iret = holdingRegisterWrite(SHI_MODBUS_REGISTER::SHI_HEAT_OFFSET, tempx10);
  iret = holdingRegisterWrite(SHI_MODBUS_REGISTER::SHI_HEAT_MODE, 2);
 
  debug_printf("WriteHeatOffset: mode:%d val:%d\r\n", _heat_mode, tempx10);
  AsyncWebLog.printf("[SHI] WriteHeatOffset: mode:%d val:%d\r\n", 2, tempx10);
  return bool(iret);
}

bool LuxModbusSHI::writePCSetpoint(uint16_t imode,  int16_t iVal)
{
  _bSet_pc_mode = false;
  if (iVal < 8)  // min value = 8 --> 800W limit
  {
    iVal = 8;
  };
  

  if (iVal >= 22) // > 2,2kW --> setpoint mode=0 --> no power limitation
  { imode = 0;}
  else
  {imode = 2;}    // PC-Limit= 2 --> until now fix "Hard-Limit-Mode"

  int iret = holdingRegisterWrite(SHI_MODBUS_REGISTER::SHI_PC_SETPOINT, iVal);
  iret = holdingRegisterWrite(SHI_MODBUS_REGISTER::SHI_PC_MODE,     imode);
  
  debug_printf("WritePCSetpoint: val:%d\r\n", iVal);
  AsyncWebLog.printf("[SHI] WritePCOffset: mode:%d val:%d\r\n",imode, iVal);
  return bool(iret);
}


char SHIvalbuffer [18];
XPString sSHIVal(SHIvalbuffer, sizeof(SHIvalbuffer));   
const char* LuxModbusSHI::getStringWorkingMode()
{
   sSHIVal = "--";
   switch (_workingmode)
   {
       case 0:
         sSHIVal = "HEIZUNG";
         break;
       case 1:
        sSHIVal = "WARMWASSER";
        break;
       case 3:
        sSHIVal = "evu";
        break;
       case 4:
        sSHIVal = "ABTAUEN";
        break;
       case 5:
        sSHIVal = "AUS";
        break;
       default:
        sSHIVal = String(_workingmode);
        break;
   }
   return sSHIVal;
}

int16_t LuxModbusSHI::calcRoomOffset(int16_t tempRoomX10, int16_t  tempRoomSetpointX10)
{
  //AsyncWebLog.printf("[ROOM]SP:       %02.1f °C\r\n", tempRoomSetpointX10/10.0);
  int16_t diff = tempRoomSetpointX10 - tempRoomX10;
  //AsyncWebLog.printf("[ROOM]SP-offset:%02.1f K\r\n", diff/10.0);
  int16_t ret_offset = 0;

  if (diff > 0)
  {
    if (diff >= 10)
    { ret_offset = 25; }

    else if (diff >= 5 )
    { ret_offset = 20; }

    else if (diff >= 2)
    { ret_offset = 15; }

    else
    { ret_offset = 10; }
  }
  else // < 0
  {
    if (diff <= -15 )
    { ret_offset = -25; }

    else if (diff <= -10)
    { ret_offset = -15; }

    else if (diff <= -5)
    { ret_offset = -10;}

    else if (diff <= -2)
    { ret_offset = -5;}

  }
  AsyncWebLog.printf("[ROOM]Offs: temp:%d calc:%d \r\n",diff, ret_offset);
  return ret_offset;
}







