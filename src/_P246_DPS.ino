//#include "_Plugin_Helper.h"

#ifdef USES_P246

# include "src/Helpers/_Plugin_Helper_serial.h"
# include "src/PluginStructs/P020_data_struct.h"
# include <ESPeasySerial.h>

//#######################################################################################################
//######################## Plugin 246: DSP - Digital Power Supplay ########################
//#######################################################################################################
/*
  Plugin written by: PeterSpringmann@online.de

  This plugin communicate with DSP such like this: https://www.youtube.com/watch?v=igY0s4drtlc

*/

#define PLUGIN_246
#define PLUGIN_ID_246         246
#define PLUGIN_NAME_246       "Power - DPS5020 Digital Power Supply"

#define P246_DEV_ID          PCONFIG(0)
#define P246_DEV_ID_LABEL    PCONFIG_LABEL(0)
#define P246_MODEL           PCONFIG(1)
#define P246_MODEL_LABEL     PCONFIG_LABEL(1)
#define P246_BAUDRATE        PCONFIG(2)
#define P246_BAUDRATE_LABEL  PCONFIG_LABEL(2)
#define P246_QUERY1          PCONFIG(3)
#define P246_QUERY2          PCONFIG(4)
#define P246_QUERY3          PCONFIG(5)
#define P246_QUERY4          PCONFIG(6)
#define P246_DEPIN           CONFIG_PIN3

#define P246_QUERY1_CONFIG_POS  0
#define P246_SENSOR_TYPE_INDEX  P246_QUERY1_CONFIG_POS + (VARS_PER_TASK + 1)


#define P246_DEV_ID_DFLT     1
#define P246_MODEL_DFLT      0  // SDM120C
#define P246_BAUDRATE_DFLT   1  // 9600 baud
#define P246_QUERY1_DFLT     0  // Voltage (V)
#define P246_QUERY2_DFLT     1  // Current (A)
#define P246_QUERY3_DFLT     2  // Power (W)
#define P246_QUERY4_DFLT     5  // Power Factor (cos-phi)

#define P246_NR_OUTPUT_VALUES          4
#define P246_NR_OUTPUT_OPTIONS         13
//#define P246_QUERY1_CONFIG_POS         3

#define P246_MODBUS_TIMEOUT   500

// ---------------------------------------
// Modbus Holding register (3)

#define DPS_VOLTAGE_SET        0
#define DPS_CURRENT_SET        1
#define DPS_OUTPUT_VOLTAGE     2
#define DPS_OUTPUT_CURRENT     3
#define DPS_OUTPUT_POWER       4
#define DPS_INPUT_VOLTAGE      5
#define DPS_LOCK_STATUS        6
#define DPS_PROTECTION_STATUS  7
#define DPS_MODE_CVCC          8
#define DPS_OUTPUT_ONOFF       9
#define DPS_BACH_LIGHT        10
#define DPS_MODEL             11
#define DPS_FIRMWARE          12
// ---------------------------------------
//
#define DPS_SET_VOLTAGE        0
#define DPS_SET_CURRENT        1
#define DPS_SET_LOCK           6
#define DPS_SET_OUTPUT         9
#define DPS_SET_BACKLIGHT      10
// ---------------------------------------

struct P246_data_struct : public PluginTaskData_base {
  P246_data_struct() {}

  ~P246_data_struct() {
    reset();
  }

  void reset() {
    modbus.reset();
  }

  bool init(int port, const int16_t serial_rx, const int16_t serial_tx, int ModBusAddress) {
    return modbus.init(static_cast<ESPEasySerialPort>(port),serial_rx, serial_tx, 9600, ModBusAddress,12);
  }

  bool isInitialized() const {
    return modbus.isInitialized();
  }

  int16_t values[13] ;

  ModbusRTU_struct modbus;
  byte             sensortype;
};

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

boolean Plugin_246(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_246;
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;     // connected through 3 datapins
        Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = P246_NR_OUTPUT_VALUES;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_246);
        break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        for (byte i = 0; i < VARS_PER_TASK; ++i) {
          if ( i < P246_NR_OUTPUT_VALUES) {
            byte choice = PCONFIG(i + P246_QUERY1_CONFIG_POS);
            safe_strncpy(
              ExtraTaskSettings.TaskDeviceValueNames[i],
              P246_getQueryValueString(choice),
              sizeof(ExtraTaskSettings.TaskDeviceValueNames[i]));
          } else {
            ZERO_FILL(ExtraTaskSettings.TaskDeviceValueNames[i]);
          }
        }
        break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        serialHelper_getGpioNames(event);
        event->String3 = formatGpioName_output_optional(F("DE"));
        break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_WEBFORM_SHOW_CONFIG:
      {
        string += serialHelper_getSerialTypeLabel(event);
        success = true;
        break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_SET_DEFAULTS:
      {
        P246_DEV_ID = P246_DEV_ID_DFLT;
        P246_MODEL = P246_MODEL_DFLT;
        P246_BAUDRATE = P246_BAUDRATE_DFLT;
        P246_QUERY1 = P246_QUERY1_DFLT;
        P246_QUERY2 = P246_QUERY2_DFLT;
        P246_QUERY3 = P246_QUERY3_DFLT;
        P246_QUERY4 = P246_QUERY4_DFLT;

        success = true;
        break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_WEBFORM_LOAD:
      {
        serialHelper_webformLoad(event);

        if (P246_DEV_ID == 0 || P246_DEV_ID > 247 || P246_BAUDRATE >= 6) {
          // Load some defaults
          P246_DEV_ID = P246_DEV_ID_DFLT;
          P246_MODEL = P246_MODEL_DFLT;
          P246_BAUDRATE = P246_BAUDRATE_DFLT;
          P246_QUERY1 = P246_QUERY1_DFLT;
          P246_QUERY2 = P246_QUERY2_DFLT;
          P246_QUERY3 = P246_QUERY3_DFLT;
          P246_QUERY4 = P246_QUERY4_DFLT;
        }
        addFormNumericBox(F("Modbus Address"), P246_DEV_ID_LABEL, P246_DEV_ID, 1, 247);

        {
          String options_baudrate[6];
          for (int i = 0; i < 6; ++i) {
            options_baudrate[i] = String(P246_storageValueToBaudrate(i));
          }
          addFormSelector(F("Baud Rate"), P246_BAUDRATE_LABEL, 6, options_baudrate, NULL, P246_BAUDRATE );
        }

          P246_data_struct *P246_data =  static_cast<P246_data_struct *>(getPluginTaskData(event->TaskIndex));

        if (P246_data != nullptr) {
          addRowLabel(F("Checksum (pass/fail/nodata)"));
          uint32_t reads_pass, reads_crc_failed, reads_nodata;
          P246_data->modbus.getStatistics(reads_pass, reads_crc_failed, reads_nodata);
          String chksumStats;
          chksumStats  = reads_pass;
          chksumStats += '/';
          chksumStats += reads_crc_failed;
          chksumStats += '/';
          chksumStats += reads_nodata;
          addHtml(chksumStats);
        }

        {
          // In a separate scope to free memory of String array as soon as possible
          sensorTypeHelper_webformLoad_header();
          String options[P246_NR_OUTPUT_OPTIONS];
          for (int i = 0; i < P246_NR_OUTPUT_OPTIONS; ++i) {
            options[i] = P246_getQueryString(i);
          }
          for (byte i = 0; i < P246_NR_OUTPUT_VALUES; ++i) {
            const byte pconfigIndex = i + P246_QUERY1_CONFIG_POS;
            sensorTypeHelper_loadOutputSelector(event, pconfigIndex, i, P246_NR_OUTPUT_OPTIONS, options);
          }
        }

        success = true;
        break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_WEBFORM_SAVE:
      {
          serialHelper_webformSave(event);
          // Save output selector parameters.
          for (byte i = 0; i < P246_NR_OUTPUT_VALUES; ++i) {
            const byte pconfigIndex = i + P246_QUERY1_CONFIG_POS;
            const byte choice = PCONFIG(pconfigIndex);
            sensorTypeHelper_saveOutputSelector(event, pconfigIndex, i, P246_getQueryValueString(choice));
          }

          P246_DEV_ID = getFormItemInt(P246_DEV_ID_LABEL);
          P246_BAUDRATE = getFormItemInt(P246_BAUDRATE_LABEL);

          success = true;
          break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_INIT:
     {
      const int16_t serial_rx = CONFIG_PIN1;
      const int16_t serial_tx = CONFIG_PIN2;
      initPluginTaskData(event->TaskIndex, new P246_data_struct());
      P246_data_struct *P246_data = static_cast<P246_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr == P246_data) {
        return success;
      }



      if (P246_data->init(CONFIG_PORT, serial_rx, serial_tx, P246_DEV_ID)) {

        P246_data->modbus.setModbusTimeout(P246_MODBUS_TIMEOUT);

        success = true;
      } else {
        clearPluginTaskData(event->TaskIndex);
      }
      break;
    }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_EXIT:
    {
      clearPluginTaskData(event->TaskIndex);
      success = true;
      break;
    }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_READ:
      {

        P246_data_struct *P246_data = static_cast<P246_data_struct *>(getPluginTaskData(event->TaskIndex));

        if (P246_data)
        {
          unsigned long m = millis();


           /* byte errorcode = */ P246_data->modbus.read_16b_HoldingRegisters(0, 13, P246_data->values) ;

          UserVar[event->BaseVarIndex]     = P246_data->values[P246_getRegister(P246_QUERY1)] / P246_getDevisor(P246_QUERY1);
          UserVar[event->BaseVarIndex + 1] = P246_data->values[P246_getRegister(P246_QUERY2)] / P246_getDevisor(P246_QUERY2);
          UserVar[event->BaseVarIndex + 2] = P246_data->values[P246_getRegister(P246_QUERY3)] / P246_getDevisor(P246_QUERY3);
          UserVar[event->BaseVarIndex + 3] = P246_data->values[P246_getRegister(P246_QUERY4)] / P246_getDevisor(P246_QUERY4);
           m = millis() - m;
           addLog(LOG_LEVEL_INFO,"Ready in " + String(m) + "ms");
          success = true;
          break;
        }
        break;
      }

    // -----------------------------------------------------------------------------------------------
    case PLUGIN_WRITE:
     {

        P246_data_struct *P246_data = static_cast<P246_data_struct *>(getPluginTaskData(event->TaskIndex));

         if(string.startsWith("Dps") ) {

           if (P246_data)  {
             byte  errorcode = 0;

             String arguments = String(string);

             String cmd  = parseString(arguments, 1);
             String arg1 = parseStringKeepCase(arguments, 2);
             String arg2 = parseStringKeepCase(arguments, 3);

             double result=0;
             if (!isError(Calculate(arg1, result))) {
                addLog(LOG_LEVEL_INFO,"CalcError: " + String(arg1) );
             }

              addLog(LOG_LEVEL_INFO, arguments +", p1: " + String(cmd) + ", p2: " +String(arg1) + ", p3: " +String(arg2)+ ", res: " +String(result,2));


              if ( cmd.equalsIgnoreCase(F("DpsSetVoltage")))  {
                  P246_data->modbus.writeSingleRegister(DPS_SET_VOLTAGE, result * 100 ,  errorcode) ;
                 success = true;
              } else if ( cmd.equalsIgnoreCase(F("DpsIncVoltage")))  {
                  P246_data->modbus.writeSingleRegister(DPS_SET_VOLTAGE,  P246_data->values[DPS_OUTPUT_VOLTAGE] + result * 100 ,  errorcode) ;
                 success = true;
              } else  if ( cmd.equalsIgnoreCase(F("DpsSetCurrent")))  {
                  P246_data->modbus.writeSingleRegister(DPS_SET_CURRENT, result * 100 ,  errorcode) ;
                 success = true;
              } else  if ( cmd.equalsIgnoreCase(F("DpsIncCurrent")))  {
                  int i = P246_data->values[DPS_OUTPUT_CURRENT] + result * 100 ;
                  int Max = 2000;
                  if(arg2.length()>0) Max = atof(arg2.c_str()) * 100;
                  if(i < 0) i = 0; if(i > Max) i = Max;
                  addLog(LOG_LEVEL_INFO, "DpsIncCurrent to " + String(i*10) + " mA. (Max is " + String(Max*10)+")");
                  P246_data->modbus.writeSingleRegister(DPS_SET_CURRENT,  i ,  errorcode) ;
                 success = true;
              } else  if ( cmd.equalsIgnoreCase(F("DpsSetLock")))  {
                  P246_data->modbus.writeSingleRegister(DPS_SET_LOCK, atoi(arg1.c_str()) ,  errorcode) ;
                 success = true;
              } else  if ( cmd.equalsIgnoreCase(F("DpsSetOutput")))  {
                  P246_data->modbus.writeSingleRegister(DPS_SET_OUTPUT, atoi(arg1.c_str()) ,  errorcode) ;
                 success = true;
              } else  if ( cmd.equalsIgnoreCase(F("DpsSetBackLight")))  {
                  P246_data->modbus.writeSingleRegister(DPS_SET_BACKLIGHT, result ,  errorcode) ;
                 success = true;
              } else  if ( cmd.equalsIgnoreCase(F("DpsRead")))  {

                   String log;

                   int16_t v[13] ;
                   errorcode = P246_data->modbus.read_16b_HoldingRegisters(0, 13, v) ;
                   log += "Uset  " + String(v[0]) +"\n" ;
                   log += "Iset  " + String(v[1]) +"\n" ;
                   log += "Uout  " + String(v[2]) +"\n" ;
                   log += "Iout  " + String(v[3]) +"\n" ;
                   log += "Pout  " + String(v[4]) +"\n" ;
                   log += "Uin   " + String(v[5]) +"\n" ;
                   log += "Lock  " + String(v[6]) +"\n" ;
                   log += "Prot  " + String(v[7]) +"\n" ;
                   log += "Mode  " + String(v[8]) +"\n" ;
                   log += "OnOf  " + String(v[9]) +"\n" ;
                   log += "Light " + String(v[10]) +"\n";
                   log += "Model " + String(v[11]) +"\n";
                   log += "FirmWare " + String(v[12]) +"\n";

                   addLog(LOG_LEVEL_INFO, log);
                   SendStatus(event, log);

                 success = true;
              }

           }
         // SendStatus(event->Source, log);

        }
     }
     break;
    // -----------------------------------------------------------------------------------------------
  }
  return success;
}
// -----------------------------------------------------------------------------------------------

unsigned int P246_getRegister(byte query) {
    switch (query) {
      case 0:  return DPS_OUTPUT_VOLTAGE;
      case 1:  return DPS_OUTPUT_CURRENT;
      case 2:  return DPS_OUTPUT_POWER;
      case 3:  return DPS_INPUT_VOLTAGE;
      case 4:  return DPS_LOCK_STATUS;
      case 5:  return DPS_PROTECTION_STATUS;
      case 6:  return DPS_VOLTAGE_SET;
      case 7:  return DPS_CURRENT_SET;
      case 8:  return DPS_OUTPUT_ONOFF;
      case 9:  return DPS_BACH_LIGHT;
      case 10: return DPS_MODE_CVCC;
      case 11: return DPS_MODEL;
      case 12: return DPS_FIRMWARE;
    }

  return 0;
}

float P246_getDevisor(byte query) {
    switch (query) {
      case 0: return 100.0;
      case 1: return 100.0;
      case 2: return 100.0;
      case 3: return 100.0;
      case 4: return 1;
      case 5: return 1;
      case 6: return 100.0;
      case 7:  return 100.0;
      case 8:  return 1;
      case 9:  return 1;
      case 10: return 1;
      case 11: return 1;
      case 12: return 1;
    }

  return 0;
}

    // -----------------------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------------------

String P246_getQueryString(byte query) {
  switch(query)
  {
    case 0: return F("Output Voltage (V)");
    case 1: return F("Output Current (A)");
    case 2: return F("Output Power (W)");
    case 3: return F("Input Voltage (V)");
    case 4: return F("Lock Status ");
    case 5: return F("Protection Status");
    case 6: return F("Voltage Set");
    case 7:  return F("Current Set");
    case 8:  return F("Output On/Off");
    case 9:  return F("Bachlight");
    case 10: return F("Mode CV/CC");
    case 11: return F("ModelNr");
    case 12: return F("Firmware");
  }
  return "";
}

    // -----------------------------------------------------------------------------------------------

String P246_getQueryValueString(byte query) {
  switch(query)
  {
    case 0: return F("Uout");
    case 1: return F("Iout");
    case 2: return F("Pout");
    case 3: return F("Vin");
    case 4: return F("LStatus");
    case 5: return F("PStatus");
    case 6: return F("VoltageSet");
    case 7:  return F("CurrentSet");
    case 8:  return F("OutputOnOff");
    case 9:  return F("Bachlight");
    case 10: return F("ModeCVCC");
    case 11: return F("ModelNr");
    case 12: return F("Firmware");
  }
  return "";
}

    // -----------------------------------------------------------------------------------------------

int P246_storageValueToBaudrate(byte baudrate_setting) {
  unsigned int baudrate = 9600;
  switch (baudrate_setting) {
    case 0:  baudrate = 1200; break;
    case 1:  baudrate = 2400; break;
    case 2:  baudrate = 4800; break;
    case 3:  baudrate = 9600; break;
    case 4:  baudrate = 19200; break;
    case 5:  baudrate = 38400; break;
  }
  return baudrate;
}

    // -----------------------------------------------------------------------------------------------


#endif // USES_P246
