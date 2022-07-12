#include "_Plugin_Helper.h"
#ifdef USES_P249

# include "src/Helpers/_Plugin_Helper_serial.h"
# include "src/PluginStructs/P020_data_struct.h"
# include <ESPeasySerial.h>

//#######################################################################################################
//######################## Plugin 249:  MANSON  HCS3602 - Digital Power Supplay ########################
//#######################################################################################################
/*
  Plugin written by: PeterSpringmann@online.de

  This plugin communicats with MANSON  HCS3602 (remove USB to Serial converter)

*/

#define PLUGIN_249
#define PLUGIN_ID_249         249
#define PLUGIN_NAME_249       "Power - MANSON  HCS3602"

#define P249_BAUDRATE        PCONFIG(2)
#define P249_BAUDRATE_LABEL  PCONFIG_LABEL(2)
#define P249_QUERY1          PCONFIG(3)
#define P249_QUERY2          PCONFIG(4)
#define P249_QUERY3          PCONFIG(5)
#define P249_QUERY4          PCONFIG(6)

#define P249_BAUDRATE_DFLT   3  // 9600 baud
#define P249_QUERY1_DFLT     0  // Voltage (V)
#define P249_QUERY2_DFLT     1  // Current (A)
#define P249_QUERY3_DFLT     2  // Power (W)
#define P249_QUERY4_DFLT     5  // Status 

#define P249_NR_OUTPUT_VALUES          4
#define P249_QUERY1_CONFIG_POS  3

#include <ESPeasySerial.h>

// These pointers may be used among multiple instances of the same plugin,
// as long as the same serial settings are used.

ESPeasySerial* Plugin_249_SoftSerial = NULL;

boolean Plugin_249_init = false;

boolean Plugin_249(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_249;
        Device[deviceCount].Type = DEVICE_TYPE_SERIAL;     // connected through 2 datapins
        Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = P249_NR_OUTPUT_VALUES;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_249);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
        {           
            strcpy( ExtraTaskSettings.TaskDeviceValueNames[0],   "V"     );
            strcpy( ExtraTaskSettings.TaskDeviceValueNames[1],   "A"     );
            strcpy( ExtraTaskSettings.TaskDeviceValueNames[2],   "W"     );
            strcpy( ExtraTaskSettings.TaskDeviceValueNames[3],   "CC/CV" );
        }
        break;
      

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        serialHelper_getGpioNames(event);
        event->String3 = formatGpioName_output_optional(F("DE"));
        break;
      }

    case PLUGIN_WEBFORM_SHOW_CONFIG:
      {
        string += serialHelper_getSerialTypeLabel(event);
        success = true;
        break;
      }

    case PLUGIN_SET_DEFAULTS:
      {
        P249_BAUDRATE = P249_BAUDRATE_DFLT;
        P249_QUERY1 = P249_QUERY1_DFLT;
        P249_QUERY2 = P249_QUERY2_DFLT;
        P249_QUERY3 = P249_QUERY3_DFLT;
        P249_QUERY4 = P249_QUERY4_DFLT;

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SHOW_SERIAL_PARAMS:
    {
        if (P249_BAUDRATE >= 7) {
          // Load some defaults
          P249_BAUDRATE = P249_BAUDRATE_DFLT;
          P249_QUERY1 = P249_QUERY1_DFLT;
          P249_QUERY2 = P249_QUERY2_DFLT;
          P249_QUERY3 = P249_QUERY3_DFLT;
          P249_QUERY4 = P249_QUERY4_DFLT;
        }

      {
        String options_baudrate[9];
        for (int i = 0; i < 9; ++i) {
          options_baudrate[i] = String(P249_storageValueToBaudrate(i));
        }
        addFormSelector(F("Baud Rate"), P249_BAUDRATE_LABEL, 8, options_baudrate, nullptr, P249_BAUDRATE );
        addUnit(F("baud"));
      }
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
      {
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
          serialHelper_webformSave(event);

          P249_BAUDRATE = getFormItemInt(P249_BAUDRATE_LABEL);

          Plugin_249_init = false; // Force device setup next time
          success = true;
          break;
      }

    case PLUGIN_INIT:
      {
        Plugin_249_init = true;

        if (Plugin_249_SoftSerial != NULL) {
          delete Plugin_249_SoftSerial;
          Plugin_249_SoftSerial=NULL;
        }

        Plugin_249_SoftSerial = new ESPeasySerial(static_cast<ESPEasySerialPort>(CONFIG_PORT),CONFIG_PIN1, CONFIG_PIN2);
        unsigned int baudrate = P249_storageValueToBaudrate(P249_BAUDRATE);
        Plugin_249_SoftSerial->begin(baudrate);

        addLog(LOG_LEVEL_INFO, "init !!"); 

        success = true;
        break;
      }

    case PLUGIN_EXIT:
    {
      Plugin_249_init = false;
      if (Plugin_249_SoftSerial != NULL) {
        delete Plugin_249_SoftSerial;
        Plugin_249_SoftSerial=NULL;
      }
      break;
    }

   // --------------------------------------------------------------------------------------------
   case PLUGIN_WRITE:
     {
        static float Current = 0;
        static float Voltage = 0;

        if(Plugin_249_init && (Plugin_249_SoftSerial != NULL)){

        String arguments = String(string);

         String cmd = parseString(arguments, 1);
         String arg1 = parseStringKeepCase(arguments, 2);
         String arg2 = parseStringKeepCase(arguments, 3);

        addLog(LOG_LEVEL_INFO, arguments +", p1: " + String(cmd) + ", p2: " +String(arg1) + ", p3: " +String(arg2));

        String log  = "123";

        if ( cmd.equalsIgnoreCase(F("DspSetV")))  {
                 Plugin_249_SoftSerial->flush();
                 Voltage = atof(arg1.c_str());
                 Plugin_249_SoftSerial->printf("VOLT%03d\r", (int)(Voltage * 10) );
                 CheckOK( );
                 Plugin_249_SoftSerial->printf("ENDS\r");
                 CheckOK( );
                 success = true;
        } else if ( cmd.equalsIgnoreCase(F("DspSetI"))) {
                 Plugin_249_SoftSerial->flush();
                 Current = atof(arg1.c_str());
                 Plugin_249_SoftSerial->printf("CURR%03d\r",(int)(Current *10 ) );
                 CheckOK( );
                 Plugin_249_SoftSerial->printf("ENDS\r");
                 CheckOK( );
                 success = true;
        } else if ( cmd.equalsIgnoreCase(F("DspIncV"))) {
                 Plugin_249_SoftSerial->flush();
                 Voltage += atof(arg1.c_str());
                 if(Voltage > 30.0) Voltage = 30.0;
                 Plugin_249_SoftSerial->printf("VOLT%03d\r",(int)(Voltage * 10)  );
                 CheckOK( );
                 Plugin_249_SoftSerial->printf("ENDS\r");
                 CheckOK( );
                 success = true;
        } else if ( cmd.equalsIgnoreCase(F("DspIncI"))) {
                 Plugin_249_SoftSerial->flush();
                 Current += atof(arg1.c_str());
                 if(Current > 30.0) Current = 30.0;
                 Plugin_249_SoftSerial->printf("CURR%03d\r",(int)(Current * 10)  );
                 CheckOK( );
                 Plugin_249_SoftSerial->printf("ENDS\r");
                 CheckOK( );
                 success = true;
        } else if ( cmd.equalsIgnoreCase(F("DspOutputOn"))) {
                 Plugin_249_SoftSerial->flush();
                 Plugin_249_SoftSerial->printf("SOUT1\r");
                 CheckOK( );
                 Plugin_249_SoftSerial->printf("ENDS\r");
                 CheckOK( );
                 success = true;
        } else if ( cmd.equalsIgnoreCase(F("DspOutputOff"))) {
                 Plugin_249_SoftSerial->flush();
                 Plugin_249_SoftSerial->printf("SOUT0\r");
                 CheckOK( );
                 Plugin_249_SoftSerial->printf("ENDS\r");
                 CheckOK( );
                success = true;
        }

         if(log.length() > 0) {
           // SendStatus(event, log);
          }
        }
     }

    break;
   // --------------------------------------------------------------------------------------------

    case PLUGIN_READ:
      {
        if (Plugin_249_init)
        {

          addLog(LOG_LEVEL_INFO, F("SendReadRequset") );
          Plugin_249_SoftSerial->flush();
          Plugin_249_SoftSerial->println("GETD");
          Plugin_249_SoftSerial->setTimeout(50);
  
          String RxString = Plugin_249_SoftSerial->readStringUntil(0x0d);

          addLog(LOG_LEVEL_INFO, String( F("Read: ")  )+ RxString);

          if(RxString.length() > 8) {
  
             if(CheckOK()) {
                float V = RxString.substring(0,4).toInt() / 100.0 ;;
                float A = RxString.substring(4,8).toInt() / 100.0 ;;
                 
                UserVar[event->BaseVarIndex]     = V;
                UserVar[event->BaseVarIndex + 1] = A;
                UserVar[event->BaseVarIndex + 2] = V*A;
                UserVar[event->BaseVarIndex + 3] = RxString.substring(8,9).toInt();
                Plugin_249_SoftSerial->printf("ENDS\r");
                CheckOK( );
                success = true;
              }
              else
              {
                addLog(LOG_LEVEL_INFO, F("kein OK empfangen ")  );  
              }
          }
          else
          {
            addLog(LOG_LEVEL_INFO, F("zu wenig gelesen ")  );
          }
        }
        else
        {
            addLog(LOG_LEVEL_INFO, F("nicht inizialisiert") );
        }
  
        
        break;
      }
  }
  return success;
}

bool CheckOK(void) {
   String Rx = Plugin_249_SoftSerial->readStringUntil( 0x0d );
   if (Rx == "OK") return true;
   addLog(LOG_LEVEL_INFO, F("Timeout, kein OK empfangen") );
   
   return false;   
}


int P249_storageValueToBaudrate(byte baudrate_setting) {
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
    default: baudrate = 9600; break;
  }
  return baudrate;
}



#endif // USES_P249
