//#include "_Plugin_Helper.h"
#ifdef USES_P248

# include "src/Helpers/_Plugin_Helper_serial.h"
# include "src/PluginStructs/P020_data_struct.h"
# include <ESPeasySerial.h>

//#######################################################################################################
//######################## Plugin 248: Energy - AEConversion Inverter ########################
//#######################################################################################################
/*
  Plugin written by: PeterSpringmann@online.de

  This plugin communicats with AEConversion Grid Inverter (inv350)

*/

#define PLUGIN_248
#define PLUGIN_ID_248         248
#define PLUGIN_NAME_248       "Energy - AEConversion"

#define P248_DEV_ID          PCONFIG(0)
#define P248_DEV_ID_LABEL    PCONFIG_LABEL(0)
#define P248_MODEL           PCONFIG(1)
#define P248_MODEL_LABEL     PCONFIG_LABEL(1)
#define P248_BAUDRATE        PCONFIG(2)
#define P248_BAUDRATE_LABEL  PCONFIG_LABEL(2)
#define P248_QUERY1          PCONFIG(3)
#define P248_QUERY2          PCONFIG(4)
#define P248_QUERY3          PCONFIG(5)
#define P248_QUERY4          PCONFIG(6)
#define P248_DEPIN           CONFIG_PIN3

#define P248_DEV_ID_DFLT     1
#define P248_MODEL_DFLT      0  // SDM120C
#define P248_BAUDRATE_DFLT   3  // 9600 baud
#define P248_QUERY1_DFLT     0  // Voltage (V)
#define P248_QUERY2_DFLT     1  // Current (A)
#define P248_QUERY3_DFLT     2  // Power (W)
#define P248_QUERY4_DFLT     3  // Power Factor (cos-phi)

#define P248_NR_OUTPUT_VALUES          4
#define P248_NR_OUTPUT_OPTIONS        10
#define P248_QUERY1_CONFIG_POS  3

unsigned long int Errors = 0;
unsigned long int Success = 0;

#include <ESPeasySerial.h>

// These pointers may be used among multiple instances of the same plugin,
// as long as the same serial settings are used.

ESPeasySerial* Plugin_248_SoftSerial = NULL;

unsigned int Plugin_248_Counter = 0;

boolean Plugin_248_init = false;

boolean Plugin_248(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_248;
        Device[deviceCount].Type = DEVICE_TYPE_SERIAL_PLUS1;     // connected through 3 datapins
        Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = P248_NR_OUTPUT_VALUES;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_248);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        for (byte i = 0; i < VARS_PER_TASK; ++i) {
          if ( i < P248_NR_OUTPUT_VALUES) {
            byte choice = PCONFIG(i + P248_QUERY1_CONFIG_POS);
            safe_strncpy(
              ExtraTaskSettings.TaskDeviceValueNames[i],
              P248_getQueryValueString(choice),
              sizeof(ExtraTaskSettings.TaskDeviceValueNames[i]));
          } else {
            ZERO_FILL(ExtraTaskSettings.TaskDeviceValueNames[i]);
          }
        }
        break;
      }

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
        P248_DEV_ID = P248_DEV_ID_DFLT;
        P248_MODEL = P248_MODEL_DFLT;
        P248_BAUDRATE = P248_BAUDRATE_DFLT;
        P248_QUERY1 = P248_QUERY1_DFLT;
        P248_QUERY2 = P248_QUERY2_DFLT;
        P248_QUERY3 = P248_QUERY3_DFLT;
        P248_QUERY4 = P248_QUERY4_DFLT;

        success = true;
        break;
      }


        case PLUGIN_WEBFORM_SHOW_SERIAL_PARAMS:
        {
  
          if (P248_DEV_ID == 0 || P248_DEV_ID > 247 || P248_BAUDRATE >= 6) {
            // Load some defaults
            P248_DEV_ID = P248_DEV_ID_DFLT;
            P248_MODEL = P248_MODEL_DFLT;
            P248_BAUDRATE = P248_BAUDRATE_DFLT;
            P248_QUERY1 = P248_QUERY1_DFLT;
            P248_QUERY2 = P248_QUERY2_DFLT;
            P248_QUERY3 = P248_QUERY3_DFLT;
            P248_QUERY4 = P248_QUERY4_DFLT;
          }
  

          {
            String options_baudrate[6];
            for (int i = 0; i < 6; ++i) {
              options_baudrate[i] = String(P248_storageValueToBaudrate(i));
            }
            addFormSelector(F("Baud Rate"), P248_BAUDRATE_LABEL, 6, options_baudrate, nullptr, P248_BAUDRATE );
            addUnit(F("baud"));
          }

          break;
        }

        case PLUGIN_WEBFORM_LOAD:
        {

             addFormNumericBox(F("AeSGI Address"), P248_DEV_ID_LABEL, P248_DEV_ID, 1, 9999);
   
             addRowLabel(F("Checksum (pass/fail)"));
             addHtml( String ( String(Success) + "/"+String(Errors) ));
   
           {
             // In a separate scope to free memory of String array as soon as possible
             sensorTypeHelper_webformLoad_header();
             String options[P248_NR_OUTPUT_OPTIONS];
             for (int i = 0; i < P248_NR_OUTPUT_OPTIONS; ++i) {
               options[i] = P248_getQueryString(i);
             }
             
             for (byte i = 0; i < P248_NR_OUTPUT_VALUES; ++i) {
               const byte pconfigIndex = i + P248_QUERY1_CONFIG_POS;
               sensorTypeHelper_loadOutputSelector(event, pconfigIndex, i, P248_NR_OUTPUT_OPTIONS, options);
             }
        }

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
         // serialHelper_webformSave(event);
          // Save output selector parameters.
          for (byte i = 0; i < P248_NR_OUTPUT_VALUES; ++i) {
            const byte pconfigIndex = i + P248_QUERY1_CONFIG_POS;
            const byte choice = PCONFIG(pconfigIndex);
            sensorTypeHelper_saveOutputSelector(event, pconfigIndex, i, P248_getQueryValueString(choice));
          }

          P248_DEV_ID = getFormItemInt(P248_DEV_ID_LABEL);
          P248_MODEL = getFormItemInt(P248_MODEL_LABEL);
          P248_BAUDRATE = getFormItemInt(P248_BAUDRATE_LABEL);

          Plugin_248_init = false; // Force device setup next time
          success = true;
          break;
      }

    case PLUGIN_INIT:
      {
        addLog(LOG_LEVEL_INFO, "Init AECon");

        Plugin_248_init = true;
        if (Plugin_248_SoftSerial != NULL) {
          delete Plugin_248_SoftSerial;
          Plugin_248_SoftSerial=NULL;
        }

        addLog(LOG_LEVEL_INFO, "Init AECon SoftwareSerial");

       Plugin_248_SoftSerial = new ESPeasySerial(static_cast<ESPEasySerialPort>(CONFIG_PORT), CONFIG_PIN1,CONFIG_PIN2,50);

       addLog(LOG_LEVEL_INFO, "Init AECon get baudrate");

      unsigned int baudrate = P248_storageValueToBaudrate(P248_BAUDRATE);

       addLog(LOG_LEVEL_INFO, String ("Init AECon baudrate: " + String(baudrate)) );

       if(Plugin_248_SoftSerial != NULL) Plugin_248_SoftSerial->begin(baudrate);

        addLog(LOG_LEVEL_INFO, "Init AECon done");

        success = true;

        break;
      }

    case PLUGIN_EXIT:
    {
      Plugin_248_init = false;
      if (Plugin_248_SoftSerial != NULL) {
        delete Plugin_248_SoftSerial;
        Plugin_248_SoftSerial=NULL;
      }
      break;
    }

    case PLUGIN_FIFTY_PER_SECOND:

       Plugin_248_Counter++;

    break;

    case PLUGIN_WRITE:
     {

        if(Plugin_248_init && (Plugin_248_SoftSerial != NULL)){

        String arguments = String(string);

         String cmd = parseString(arguments, 1);
         String arg1 = parseStringKeepCase(arguments, 2);
         String arg2 = parseStringKeepCase(arguments, 3);

             double result=0;
             if (!isError(Calculate(arg1, result))) {
                addLog(LOG_LEVEL_INFO,"CalcError: " + String(arg1) );
             }


        addLog(LOG_LEVEL_INFO, arguments +", p1: " + String(cmd) + ", p2: " +String(arg1) + ", p3: " +String(arg2));

        String log  = "";

        if ( cmd.equalsIgnoreCase(F("AeConSetMPPMode")))  {
              log = P248_Transmit(P248_DEV_ID, "B 0 0.00" );
              success = true;
        } else if ( cmd.equalsIgnoreCase(F("AeConSetUIMode"))) {
              log = P248_Transmit_f(P248_DEV_ID, "B 2 ", result );
              success = true;
        } else if ( cmd.equalsIgnoreCase(F("AeConSetCurrent"))) {
              log = P248_Transmit_f(P248_DEV_ID, "S ", result );
              success = true;
        } else if ( cmd.equalsIgnoreCase(F("AeConGetCurrent"))) {
              log = P248_Transmit(P248_DEV_ID, "S" );
              success = true;
        } else if ( cmd.equalsIgnoreCase(F("AeConSetPowerLimit"))) {
              log = P248_Transmit_i(P248_DEV_ID, "L ",  result );
              success = true;
        } else if ( cmd.equalsIgnoreCase(F("AeConGetType"))) {
              log = P248_Transmit(P248_DEV_ID, "9");
              success = true;
        } else if ( cmd.equalsIgnoreCase(F("AeConGetValues"))) {
              log = P248_Transmit(P248_DEV_ID, "0");
              success = true;
        } else if ( cmd.equalsIgnoreCase(F("AeConGet"))) {
              log = P248_Transmit(P248_DEV_ID, arg1.c_str());
              success = true;
        } else if ( cmd.equalsIgnoreCase(F("AeConGetPower"))) {
              log = P248_Transmit(P248_DEV_ID, "L" );
              success = true;
        }

         if(log.length() > 0) {
            SendStatus(event, log);
          }
        }
     }

    break;

    case PLUGIN_READ:
      {
        if (Plugin_248_init && (Plugin_248_SoftSerial != NULL))
        {

          String Datas = P248_Transmit(P248_DEV_ID, "0");

          if(Datas[0] != 'E') {
             char d[200];
             char* c;
             Datas.toCharArray(d, Datas.length() + 1);

              c = strtok(d, " ");

              if(c!=NULL)  {

              float v[8];
              for(int i = 0; i<8;i++){
                  v[i] = atof(c);
                   c = strtok(NULL, " ");
                   if(c==0) break;
              }

              UserVar[event->BaseVarIndex]     = v[P248_QUERY1];
              UserVar[event->BaseVarIndex + 1] = v[P248_QUERY2];
              UserVar[event->BaseVarIndex + 2] = v[P248_QUERY3];
              UserVar[event->BaseVarIndex + 3] = v[P248_QUERY4];
              success = true;
            }
          }
        }
        break;
      }
  }
  return success;
}

String P248_Transmit_f(int id, const char* c,  double v) {
   char b[20];
   sprintf(b,"%s%04.1f", c, v );
   return P248_Transmit(id, b);
}

String P248_Transmit_i(int id, const char* c, int v) {
   char b[20];
   sprintf(b,"%s%03d", c, v );
   return P248_Transmit(id, b);
}


      byte P248_Checksum (const byte * buffer, int len) {
      byte a, cs = 0;
          for (a=0; a<len; a++){
             cs += buffer[a];
         }
         return cs;
      }


void P248_enableTx(bool on) {

    if ( Plugin_248_SoftSerial->getRxPin() == Plugin_248_SoftSerial->getTxPin() ) {
        if (on) {
           // Plugin_248_SoftSerial->stopListening();
           // pinMode(Plugin_248_SoftSerial->getTxPin(), OUTPUT);
           // digitalWrite(sdmSer.getTxPin(), !m_invert);
        }
        else {
           // pinMode(Plugin_248_SoftSerial->getTxPin(), INPUT_PULLUP);
           // Plugin_248_SoftSerial->listen();
        }
    }
}

String P248_Transmit(int ID, const char * p) {
  int    Count = 5;
  String RxString;
  Plugin_248_SoftSerial->setTimeout(10); // 20ms

   char b[20];
  sprintf(b,"#%02d%s%c",ID , p, (char) 0x0d);
//   sprintf(b,"#%02d%s\n",ID , p );


 do {

//  MySerial.enableTx(true);
  P248_enableTx(true);

  Plugin_248_SoftSerial->print(b);
  addLog(LOG_LEVEL_INFO,b);

  Plugin_248_SoftSerial->flush();
  delay(5);

  unsigned long m = millis();

  Plugin_248_SoftSerial->listen();
  P248_enableTx(false);
//  Plugin_248_SoftSerial.enableTx(false);

  RxString = Plugin_248_SoftSerial->readStringUntil(0x0d);
  if(RxString.length() > 3 ) {
    if(RxString[0] == 0x0a ) {
      if(RxString[1] == '*' ) {
        if(RxString.substring(2,4) == String(ID) ) {
           // if(RxString[4] == values[0])  {
              byte cs = RxString[RxString.length()-1];
              byte scs = P248_Checksum((const byte*)&RxString.c_str()[1],RxString.length()-2);
              if(cs == scs) {
                Success++;
                if(RxString.length()>7) {
                    RxString = RxString.substring(5,RxString.length()-2);
                }
              } else {
                 Errors++;
                 RxString = "E5"; // Prüfsummenfehler
              }

              // addLog(LOG_LEVEL_INFO, "cs: "+ String(cs) + " berechnet: " + String(scs) );

        } else {
          RxString = "E3"; // falsche ID in der Antwort
          Errors++;
        }
      } else {
        RxString = "E2"; // kein '*'
        Errors++;
      }
    } else {
      RxString = "E1"; // kein <lf>
      Errors++;
    }
  } else {
    RxString = "E0"; // timeout
    Errors++;
  }

      // if(RxString[0] == 'E' && Count < 3)
      // addLog(LOG_LEVEL_INFO, "Tx: " + TxString + ", retrey: " + String(Count) + ", Rx:" + RxString + " Errors: " + String(Errors) );
  m = millis() - m;
  addLog(LOG_LEVEL_INFO,"'"+RxString + "' in " + String(m) + "ms");

 } while (Count-- > 0 && RxString.startsWith("E") );

  return RxString;
}



void P248_Transmit(unsigned int ID, unsigned short MessageID, float Value) {



}




unsigned int P248_getRegister(byte query, byte model) {
  if (model == 0) { // SDM120C
    switch (query) {
      case 0: return 0;
      case 1: return 0;
      case 2: return 0;
      case 3: return 0;
      case 4: return 0;
      case 5: return 0;
      case 6: return 0;
      case 7: return 0;
      case 8: return 0;
      case 9: return 0;
    }
  }
  return 0;
}

String P248_getQueryString(byte query) {
  switch(query)
  {
    case 0: return F("Status");
    case 1: return F("Input Voltage (V)");
    case 2: return F("Input Current (A)");
    case 3: return F("Input Power (W)");
    case 4: return F("Line Voltage (V)");
    case 5: return F("Output Current (A)");
    case 6: return F("Output Power (W)");
    case 7: return F("Temperature (°C)");
    case 8: return F("Dayly Energy (Wh)");
    case 9: return F("-");
  }
  return "";
}

String P248_getQueryValueString(byte query) {
  switch(query)
  {
    case 0: return F("-");
    case 1: return F("inV");
    case 2: return F("inA");
    case 3: return F("inW");
    case 4: return F("lineV");
    case 5: return F("outA");
    case 6: return F("outW");
    case 7: return F("°C");
    case 8: return F("Wh");
    case 9: return F("-");
  }
  return "";
}


int P248_storageValueToBaudrate(byte baudrate_setting) {
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



#endif // USES_P248
