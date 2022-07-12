//#include "_Plugin_Helper.h"

#ifdef USES_P250


# include "src/Helpers/_Plugin_Helper_serial.h"
# include "src/PluginStructs/P020_data_struct.h"
# include <ESPeasySerial.h>

//#######################################################################################################
//######################## Plugin 250: PZEM -  DC Communication Module ##################################
//#######################################################################################################
/*
  Plugin written by: PeterSpringmann@online.de

  This plugin communicate with PZEM

*/

#define PLUGIN_250
#define PLUGIN_ID_250         250
#define PLUGIN_NAME_250       "Energy - PZEM DC Communication Module "

#define P250_DEV_ID          PCONFIG(0)
#define P250_DEV_ID_LABEL    PCONFIG_LABEL(0)
#define P250_MODEL           PCONFIG(1)
#define P250_MODEL_LABEL     PCONFIG_LABEL(1)
#define P250_BAUDRATE        PCONFIG(2)
#define P250_BAUDRATE_LABEL  PCONFIG_LABEL(2)
#define P250_QUERY1          PCONFIG(3)
#define P250_QUERY2          PCONFIG(4)
#define P250_QUERY3          PCONFIG(5)
#define P250_QUERY4          PCONFIG(6)
#define P250_DEPIN           CONFIG_PIN3

#define P250_QUERY1_CONFIG_POS  0
#define P250_SENSOR_TYPE_INDEX  P250_QUERY1_CONFIG_POS + (VARS_PER_TASK + 1)


#define P250_DEV_ID_DFLT     1
#define P250_MODEL_DFLT      0  // SDM120C
#define P250_BAUDRATE_DFLT   1  // 9600 baud
#define P250_QUERY1_DFLT     0  // Voltage (V)
#define P250_QUERY2_DFLT     1  // Current (A)
#define P250_QUERY3_DFLT     2  // Power (W)
#define P250_QUERY4_DFLT     5  // Power Factor (cos-phi)

#define P250_NR_OUTPUT_VALUES          4
#define P250_NR_OUTPUT_OPTIONS         6
//#define P250_QUERY1_CONFIG_POS         3

#define P250_MODBUS_TIMEOUT   100
// ---------------------------------------
// Modbus Holding register (3)
#define PZEM_OUTPUT_VOLTAGE     2
#define PZEM_OUTPUT_CURRENT     3
#define PZEM_OUTPUT_POWER       4
#define PZEM_INPUT_VOLTAGE      5
#define PZEM_LOCK_STATUS        6
#define PZEM_PROTECTION_STATUS  7
#define PZEM_MODE               8
// ---------------------------------------
//
#define PZEM_SET_VOLTAGE        0
#define PZEM_SET_CURRENT        1
#define PZEM_SET_LOCK           6
#define PZEM_SET_OUTPUT         9
// ---------------------------------------

struct P250_data_struct : public PluginTaskData_base {
  P250_data_struct() {}

  ~P250_data_struct() {
    reset();
  }

  void reset() {
    modbus.reset();
  }

  bool init(int Port, const int16_t serial_rx, const int16_t serial_tx, int ModBusAddress) {
    return modbus.init(static_cast<ESPEasySerialPort>(Port),serial_rx, serial_tx, 9600, ModBusAddress);
  }

  bool isInitialized() const {
    return modbus.isInitialized();
  }

  ModbusRTU_struct modbus;
  byte             sensortype;
};

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

boolean Plugin_250(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_250;
        Device[deviceCount].Type = DEVICE_TYPE_SERIAL;     // 
        Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = P250_NR_OUTPUT_VALUES;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_250);
        break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        for (byte i = 0; i < VARS_PER_TASK; ++i) {
          if ( i < P250_NR_OUTPUT_VALUES) {
            byte choice = PCONFIG(i + P250_QUERY1_CONFIG_POS);
            safe_strncpy(
              ExtraTaskSettings.TaskDeviceValueNames[i],
              P250_getQueryValueString(choice),
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
        P250_DEV_ID = P250_DEV_ID_DFLT;
        P250_MODEL = P250_MODEL_DFLT;
        P250_BAUDRATE = P250_BAUDRATE_DFLT;
        P250_QUERY1 = P250_QUERY1_DFLT;
        P250_QUERY2 = P250_QUERY2_DFLT;
        P250_QUERY3 = P250_QUERY3_DFLT;
        P250_QUERY4 = P250_QUERY4_DFLT;

        success = true;
        break;
      }

    // -----------------------------------------------------------------------------------------------
  case PLUGIN_WEBFORM_SHOW_SERIAL_PARAMS:
    {
        if (P250_BAUDRATE >= 7) {
          // Load some defaults
          P250_BAUDRATE = P250_BAUDRATE_DFLT;
          P250_QUERY1 = P250_QUERY1_DFLT;
          P250_QUERY2 = P250_QUERY2_DFLT;
          P250_QUERY3 = P250_QUERY3_DFLT;
          P250_QUERY4 = P250_QUERY4_DFLT;
        }

      {
        String options_baudrate[9];
        for (int i = 0; i < 6; ++i) {
          options_baudrate[i] = String(P250_storageValueToBaudrate(i));
        }
        addFormSelector(F("Baud Rate"), P250_BAUDRATE_LABEL, 6, options_baudrate, nullptr, P250_BAUDRATE );
        addUnit(F("baud"));
      }
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
      {

        addFormNumericBox(F("Modbus Address"), P250_DEV_ID_LABEL, P250_DEV_ID, 1, 247);

        P250_data_struct *P250_data =  static_cast<P250_data_struct *>(getPluginTaskData(event->TaskIndex));

        if (P250_data != nullptr) {
          addRowLabel(F("Checksum (pass/fail/nodata)"));
          uint32_t reads_pass, reads_crc_failed, reads_nodata;
          P250_data->modbus.getStatistics(reads_pass, reads_crc_failed, reads_nodata);
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
          String options[P250_NR_OUTPUT_OPTIONS];
          for (int i = 0; i < P250_NR_OUTPUT_OPTIONS; ++i) {
            options[i] = P250_getQueryString(i);
          }
          for (byte i = 0; i < P250_NR_OUTPUT_VALUES; ++i) {
            const byte pconfigIndex = i + P250_QUERY1_CONFIG_POS;
            sensorTypeHelper_loadOutputSelector(event, pconfigIndex, i, P250_NR_OUTPUT_OPTIONS, options);
          }
        }

        success = true;
        break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_WEBFORM_SAVE:
      {
         // serialHelper_webformSave(event);
          // Save output selector parameters.
          for (byte i = 0; i < P250_NR_OUTPUT_VALUES; ++i) {
            const byte pconfigIndex = i + P250_QUERY1_CONFIG_POS;
            const byte choice = PCONFIG(pconfigIndex);
            sensorTypeHelper_saveOutputSelector(event, pconfigIndex, i, P250_getQueryValueString(choice));
          }

          P250_DEV_ID = getFormItemInt(P250_DEV_ID_LABEL);
          P250_BAUDRATE = getFormItemInt(P250_BAUDRATE_LABEL);

          success = true;
          break;
      }

    // -----------------------------------------------------------------------------------------------

    case PLUGIN_INIT:
     {

      initPluginTaskData(event->TaskIndex, new P250_data_struct());
      P250_data_struct *P250_data = static_cast<P250_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr == P250_data) {
        return success;
      }

      if (P250_data->init(CONFIG_PORT, CONFIG_PIN1, CONFIG_PIN2, P250_DEV_ID)) {

        P250_data->modbus.setModbusTimeout(P250_MODBUS_TIMEOUT);

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

        P250_data_struct *P250_data = static_cast<P250_data_struct *>(getPluginTaskData(event->TaskIndex));

        if (P250_data)
        {
          UserVar[event->BaseVarIndex]     = P250_readVal(P250_QUERY1,  P250_data );
          UserVar[event->BaseVarIndex + 1] = P250_readVal(P250_QUERY2,  P250_data );
          UserVar[event->BaseVarIndex + 2] = P250_readVal(P250_QUERY3,  P250_data );
          UserVar[event->BaseVarIndex + 3] = P250_readVal(P250_QUERY4,  P250_data );
          success = true;
          break;
        }
        break;
      }

    // -----------------------------------------------------------------------------------------------
    case PLUGIN_WRITE:
     {

        P250_data_struct *P250_data = static_cast<P250_data_struct *>(getPluginTaskData(event->TaskIndex));

        if (P250_data)
        {
           byte  errorcode = 0;
           String arguments = String(string);

           String cmd = parseString(arguments, 1);
           String arg1 = parseStringKeepCase(arguments, 2);
           String arg2 = parseStringKeepCase(arguments, 3);

           addLog(LOG_LEVEL_INFO, arguments +", p1: " + String(cmd) + ", p2: " +String(arg1) + ", p3: " +String(arg2));

           if ( cmd.equalsIgnoreCase(F("PZEMSetLowVoltageAlarm")))  {
               P250_data->modbus.writeSingleRegister(PZEM_SET_VOLTAGE, atof(arg1.c_str()) * 100 ,  errorcode) ;
              success = true;
           } else  if ( cmd.equalsIgnoreCase(F("PZEMSetHighVoltageAlarm")))  {
               P250_data->modbus.writeSingleRegister(PZEM_SET_CURRENT, atof(arg1.c_str()) * 100 ,  errorcode) ;
              success = true;
           } else  if ( cmd.equalsIgnoreCase(F("PZEMSetModBusAddress")))  {
               P250_data->modbus.writeSingleRegister(PZEM_SET_LOCK, atoi(arg1.c_str()) ,  errorcode) ;
              success = true;
           } else  if ( cmd.equalsIgnoreCase(F("PZEMSetCurrentRange")))  {
               P250_data->modbus.writeSingleRegister(PZEM_SET_OUTPUT, atoi(arg1.c_str()) ,  errorcode) ;
           } else  if ( cmd.equalsIgnoreCase(F("PZEMResetEnergy")))  {
               P250_data->modbus.writeSingleRegister(PZEM_SET_OUTPUT, atoi(arg1.c_str()) ,  errorcode) ;
              success = true;
           } else  if ( cmd.equalsIgnoreCase(F("PZEMCalibrate")))  {
               P250_data->modbus.writeSingleRegister(PZEM_SET_OUTPUT, atoi(arg1.c_str()) ,  errorcode) ;
              success = true;
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
    // -----------------------------------------------------------------------------------------------


float P250_readVal(byte query,  struct P250_data_struct *P250_data) {
  if (P250_data == NULL) return 0.0;
   byte  errorcode = 0;
  int t =  P250_data->modbus.readHoldingRegister(P250_getRegister(query), errorcode );

  return t / 10.0;
}

    // -----------------------------------------------------------------------------------------------

unsigned int P250_getRegister(byte query) {
    switch (query) {
      case 0: return PZEM_OUTPUT_VOLTAGE;
      case 1: return PZEM_OUTPUT_CURRENT;
      case 2: return PZEM_OUTPUT_POWER;
      case 3: return PZEM_INPUT_VOLTAGE;
      case 4: return PZEM_LOCK_STATUS;
      case 5: return PZEM_PROTECTION_STATUS;
      case 6: return PZEM_MODE;
    }

  return 0;
}

    // -----------------------------------------------------------------------------------------------

String P250_getQueryString(byte query) {
  switch(query)
  {
    case 0: return F("Voltage (V)");
    case 1: return F("Current (A)");
    case 2: return F("Power (W)");
    case 3: return F("Energy (wh)");
    case 4: return F("High Voltage Alarm (V)");
    case 5: return F("Low Voltage Alarm (V)");
  }
  return "";
}

    // -----------------------------------------------------------------------------------------------

String P250_getQueryValueString(byte query) {
  switch(query)
  {
    case 0: return F("Voltage");
    case 1: return F("Ampere");
    case 2: return F("Power");
    case 3: return F("Energy");
    case 4: return F("-");
    case 5: return F("-");
    case 6: return F("-");
  }
  return "";
}

    // -----------------------------------------------------------------------------------------------

int P250_storageValueToBaudrate(byte baudrate_setting) {
  unsigned int baudrate = 9600;
  switch (baudrate_setting) {
    case 0:  baudrate = 1200; break;
    case 1:  baudrate = 2400; break;
    case 2:  baudrate = 4800; break;
    case 3:  baudrate = 9600; break;
    case 4:  baudrate = 19200; break;
    case 5:  baudrate = 38400; break;
    case 6:  baudrate = 57600; break;
    case 7:  baudrate = 115200; break;
    case 8:  baudrate = 230400; break;
    case 9:  baudrate = 460800; break;

  }
  return baudrate;
}

    // -----------------------------------------------------------------------------------------------


#endif // USES_P250
