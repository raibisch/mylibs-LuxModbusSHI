#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Client.h>
#include <AsyncWebLog.h>
#include <XPString.h>

#include "ModubusTCPClient/src/ModbusTCPClient.h"
#include "LuxWebsocket.h"


#ifdef DEBUG_PRINT
#define debug_begin(...) Serial.begin(__VA_ARGS__);
#define debug_print(...) Serial.print(__VA_ARGS__);
#define debug_write(...) Serial.write(__VA_ARGS__);
#define debug_println(...) Serial.println(__VA_ARGS__);
#define debug_printf(...) Serial.printf(__VA_ARGS__);
#else
#define debug_begin(...)
#define debug_print(...)
#define debug_printf(...)
#define debug_write(...)
#define debug_println(...)
#endif


class LuxModbusSHI: private ModbusTCPClient
{
  
  private:
    IPAddress  _ipLux;
    WiFiClient _wificlient;
    bool       _ipInitok = false;
  protected:

    // Holding-Register Values
    bool     _bSet_heat_mode = false;
    uint16_t _heat_mode =  0;
    uint16_t _heat_setpoint  = 34;
    int16_t  _heat_offset    = 0;

    bool     _bSet_pc_mode   = false;
    uint16_t _pc_mode        = 2;
    uint16_t _pc_setpoint    = 14;

    bool     _bSet_wwextra   = false;
    uint16_t _ww_extra       = 0;

    // Input-Register Values
    int16_t  _temp_outdoor   = 0;
    int16_t  _temp_ww        = 0;
    int16_t  _temp_ww_soll   = 0;
    uint16_t _power_in       = 0;
    uint16_t _power_out      = 0;
    uint16_t _workingmode    = 0;
    uint16_t _vl_ist         = 0;
    uint16_t _rl_soll        = 0;
    uint16_t _rl_ist         = 0;
    uint32_t _sumenergy_in   = 0;
    uint32_t _sumenergy_out  = 0;
    uint32_t _heizenergy_in  = 0;
    uint32_t _heizenergy_out = 0;
    uint32_t _wwenergy_in    = 0;
    uint32_t _wwenergy_out   = 0;

   bool     writeHeatOffset(int16_t  varx10);
   bool     writePCSetpoint(uint16_t imode, int16_t  kwx10);

  public:

  LuxWebsocket wsFriend;
  
  enum SHI_MODBUS_REGISTER
  {
  SHI_HEAT_MODE     = 10000,
  SHI_HEAT_SETPOINT = 10001,
  SHI_HEAT_OFFSET   = 10002,
  SHI_HEAT_LEVEL    = 10003,

  SHI_WW_MODE     = 10000,
  SHI_WW_SETPOINT = 10001,
  SHI_WW_OFFSET   = 10002,
  SHI_WW_LEVEL    = 10003,

  SHI_PC_MODE       = 10040,
  SHI_PC_SETPOINT   = 10041,

  SHI_WW_EXTRA      = 10071
  };

  enum SHI_MODBUS_INPUT
  {
  SHI_WORKINGMODE      = 10002,
  SHI_TEMP_RL          = 10100,
  SHI_TEMP_RL_SOLL     = 10101,
  SHI_TEMP_VL          = 10105,
  SHI_TEMP_OUTDOOR     = 10108,
  SHI_TEMP_WW          = 10120,
  SHI_TEMP_WW_SOLL     = 10121,
  
  SHI_PWR_HEAT_OUT     = 10300,
  SHI_PWR_ELECT_IN     = 10301,

  SHI_ENERGY_SUM_IN    = 10310,
  SHI_ENERGY_HEI_IN    = 10312,
  SHI_ENERGY_WW_IN     = 10314,  

  SHI_ENERGY_SUM_OUT   = 10320,
  SHI_ENERGY_HEI_OUT   = 10322,
  SHI_ENERGY_WW_OUT    = 10324
  };
  
  enum SHI_PCMODE
  {
    PC_OFF,
    PC_SMART,
    PC_HARD
  };

   LuxModbusSHI(Client& clients) : ModbusTCPClient(clients) { };
   LuxModbusSHI() : LuxModbusSHI(_wificlient){};
   
   bool init(const char* sIP);
   bool poll(bool bNewDay);

#ifdef DEBUG_PRINT
   int16_t readRegister(SHI_MODBUS_REGISTER reg);
   int16_t readInput(SHI_MODBUS_INPUT reg);
#endif
   void readSHIValues();

  
  void  setHeatOffset(int16_t val)
  {
   _heat_offset=val; 
   _bSet_heat_mode=true;
   debug_printf("[SHI] setHeatOffset val:%d\r\n", val)
  };

  void  setPCSetpoint(int16_t val)
  {
     _pc_setpoint=val; 
     _bSet_pc_mode=true;
    debug_printf("[SHI] setPCSetpoint val:%d\r\n", val)
  };

  void setPCMode(uint16_t val){ _pc_mode=val;}  

  void setWWExtra(){_bSet_wwextra = true;}

  int16_t getHeatMode()       { return _heat_mode;};
  int16_t getHeatOffsetX10()  {return _heat_offset;};

  int16_t getPCMode()         { return _pc_mode;};
  int16_t getPCSetpoint()     {return _pc_setpoint;};

  uint16_t getWWExtra()        {return _ww_extra;};

  int16_t getTempOutdoorX10() {return _temp_outdoor;};  
  int16_t getTempWWX10()      {return _temp_ww;};
  int16_t getTempWWSollX10()  {return _temp_ww_soll;};

  int16_t getPower_InX100()    {return _power_in;};  // Watt * 100
  int16_t getPower_OutX100()   {return _power_out;}; // Watt * 100

  int16_t getRL_SollX10()     {return _rl_soll;};
  int16_t getRL_IstX10()      {return _rl_ist;};  
  int16_t getVL_IstX10()      {return _vl_ist;};
  uint32_t getSumEnergy_InX100()   {return _sumenergy_in;};  // in Wh
  uint32_t getSumEnergy_OutX100()  {return _sumenergy_out;}; // in Wh

  inline float getCOPHE() {return ((_heizenergy_out*1.0) / _heizenergy_in);};
  inline float getCOPWW() {return ((_wwenergy_out*1.0)  / _wwenergy_in);};
  inline float getCOPSUM() {return ((_sumenergy_out*1.0)  / _sumenergy_in);};

  inline float getCOPdayHE() {return wsFriend.COPdayHE;};
  inline float getCOPdayWW() {return wsFriend.COPdayWW;};
  inline float getCOPdaySUM(){return wsFriend.COPdaySumLux;};

  const char* getStringWorkingMode();
  const uint16_t getNumWorkingMode() {return _workingmode;}; // return 0...4

  int16_t calcRoomOffset(int16_t tempRoomX10, int16_t  tempRoomSetpointX10);
  void setDaylyEnergy(bool bNewDay);

};

