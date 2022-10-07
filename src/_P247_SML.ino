#include "_Plugin_Helper.h"

#ifdef USES_P247


# include "src/Helpers/_Plugin_Helper_serial.h"
# include "src/PluginStructs/P020_data_struct.h"
# include <ESPeasySerial.h>

//#######################################################################################################
//######################## Plugin 247: DSP - Digital Power Supplay ########################
//#######################################################################################################
/*
  Plugin written by: PeterSpringmann@online.de

  This plugin reads SML Message from ISKRA Power Meter

*/

#define PLUGIN_247
#define PLUGIN_ID_247         247
#define PLUGIN_NAME_247       "Energy - SML Reader for ISKRA Power Meter"

#define P247_BAUDRATE        PCONFIG(0)
#define P247_SERIAL_CONFIG   PCONFIG(1)

#define P247_QUERY1          PCONFIG(3)
#define P247_QUERY2          PCONFIG(4)
#define P247_QUERY3          PCONFIG(5)
#define P247_QUERY4          PCONFIG(6)

#define P247_BAUDRATE_LABEL  PCONFIG_LABEL(0)

#define P247_BAUDRATE_DFLT   1  // 9600 baud
#define P247_QUERY1_DFLT     0  // Voltage (V)
#define P247_QUERY2_DFLT     1  // Current (A)
#define P247_QUERY3_DFLT     2  // Power (W)
#define P247_QUERY4_DFLT     5  // Power Factor (cos-phi)

#define P247_NR_OUTPUT_VALUES          4
#define P247_NR_OUTPUT_OPTIONS        10
#define P247_QUERY1_CONFIG_POS  1


#include <ESPeasySerial.h>

//====================================================================================================================

const uint16_t crc16_x25_table[] = {
	0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
	0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
	0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
	0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
	0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD,
	0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
	0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C,
	0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
	0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
	0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
	0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A,
	0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
	0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9,
	0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
	0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
	0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
	0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
	0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
	0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036,
	0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
	0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
	0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
	0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134,
	0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
	0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3,
	0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
	0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
	0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
	0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1,
	0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
	0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330,
	0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78 };


struct P247_data_struct : public PluginTaskData_base {

// ----------------------------------------------------------------------------------------------

  enum TL_FieldT { BOOLEAN, INTEGER, UNSIGNED, STRING, LIST, FERTIG, GANZFERTIG, NOP };

  #define SML_PARSE_ERROR 1
  #define SML_PARSE_OK    0

// ----------------------------------------------------------------------------------------------

  P247_data_struct() {}

  ~P247_data_struct() {}

 // ---------------------------------------------------------------------------

  ESPeasySerial*  SMLSerial;

  void init(int Port, const int16_t serial_rx, const int16_t serial_tx, const int16_t baudrate) {

//         SMLSerial = new ESPeasySerial(ESPEasySerialPort::serial2,16, serial_tx);
        SMLSerial = new ESPeasySerial(static_cast<ESPEasySerialPort>(Port),3, serial_tx);

        SMLSerial->begin(baudrate);

        addLog(LOG_LEVEL_INFO, String("Init Baud: " + baudrate)  );

  }

  // ---------------------------------------------------------------------------

  uint16_t  RxCrc;
  uint16_t  scrc;
  uint16_t  crc;
  uint8_t   tl_type;
  uint32_t  tl_value;
  int64_t   Data;
  uint8_t   FieldNumber;
  uint8_t   ListNumber;
  int8_t    scale;
  uint16_t  DataLength;
  uint16_t  SMLCrc;
  unsigned long LastMillis = 0;
  uint8_t   SMLState = 0;
  unsigned long Errors = 0;
  unsigned long Success = 0;


  // ---------------------------------------------------------------------------

  struct ob{
    int64_t obis;
    uint8_t unit;
    int8_t  scale;
    double  value;
  }ob;

 struct ob Datas[20];

double Bezogen;
double Geliefert;
double Leistung;
  // ---------------------------------------------------------------------------

 void SMLPoll(struct EventStruct *event) {

   
   // Zeichen Timeout 5000 ms

   while ( SMLSerial->available() ) {

    if( (millis() - LastMillis >= 100)  ) {
       LastMillis = millis();
       SMLState = 0;
       if(SMLState != 0 ) Errors++;
       addLog(LOG_LEVEL_INFO, "Timeout" );
    }


     LastMillis = millis();

     uint8_t RxByte = SMLSerial->read();

     if( SMLParse( RxByte,  Datas, 10) ) {

     addLog(LOG_LEVEL_INFO, "smlparse ok" );

      for (int i = 0; i < 10; i++) {

          int64_t o = Datas[i].obis;
          double  v = Datas[i].value;
 
          char sb[50];

          sprintf(sb,"obis: 0x%04X%08X = %.1f", (uint32_t) (o>>32), (uint32_t) (o) , v); addLog(LOG_LEVEL_INFO, String(sb) );

          // addLog(LOG_LEVEL_INFO, "OBIS: " + String( (uint32_t) (o >> 32) , 16) + String( (uint32_t) o  , 16) + " Value: " + String(v) );

          if( o == 0x100010800FF ) {
              Bezogen = v;      
              UserVar[event->BaseVarIndex ] = v;         
          } else if( o == 0x100020800FF ) {
              Geliefert = v;
              UserVar[event->BaseVarIndex + 1 ] = v;         
          } else if( o == 0x100100700FF ) {
              Leistung = v;
              UserVar[event->BaseVarIndex + 2 ] = v;         
             // sprintf(sb,"obis: %0llX, %.1f", o , v); addLog(LOG_LEVEL_INFO, String(sb) );
          }
      }
      sendData(event);
   }
   }
 }

// ---------------------------------------------------------------------------

#define SM_START_0 0
#define SM_START_1 1
#define SM_START_2 2
#define SM_START_3 3
#define SM_START_4 4
#define SM_START_5 5
#define SM_START_6 6
#define SM_START_7 7

#define SM_TL_FIELD  8
#define SM_LENGTH    9
#define SM_DATAS    10

#define SM_CRC1    30
#define SM_CRC2    31
#define SM_CRC3    32


  // ---------------------------------------------------------------------------

  int SMLParse(uint8_t RxByte, struct ob* datas, uint16_t size) {
    int Rc = 0;
    
    addLog(LOG_LEVEL_INFO, String(F("SMLState: ") ) + String(SMLState) + String(" RxBxte: ") + String(RxByte,16) );
    
    switch (SMLState) {

      // auf startsequenz warten
    case SM_START_0: crc = 0xFFFF; ListNumber = 0; memset(datas, 0, sizeof(ob) *size );
    case SM_START_1:
    case SM_START_2:
    case SM_START_3: CheckRxByte(&SMLState,RxByte, 0x1b, 0); break;

    case SM_START_4:
    case SM_START_5:
    case SM_START_6:
    case SM_START_7: CheckRxByte(&SMLState, RxByte, 0x01, 0); break;

    case SM_TL_FIELD:
                  sml_parse_tlfield(RxByte, &tl_type, &tl_value);
            if (tl_type == LIST) {
              //ListNumber++;
              FieldNumber = 0;
              scale = 0;
              SMLState = SM_TL_FIELD;
              // printf("LIST %d\n", tl_value);
            }
            else if (tl_type == FERTIG)     SMLState = SM_TL_FIELD;
            else if (tl_type == NOP)        SMLState = SM_TL_FIELD;
            else if (tl_type == GANZFERTIG) SMLState = SM_CRC1;
            else   SMLState = SM_LENGTH;
            if (tl_value == 1) {
              FieldNumber++;
              SMLState = SM_TL_FIELD;
            }
          //  printf("\nRxByte: %02X Type: %d Value %d,  %s ", RxByte, tl_type, tl_value,   printType(tl_type));
            break;



    case SM_LENGTH:  Data = 0;
                   if (sml_parse_length(RxByte, &tl_value)==0) {
               break;
                      }
                      DataLength = tl_value;
             SMLState++;
             // no break;

    case SM_DATAS:  // printf(" %02X " , RxByte );

             if (sml_collect_Data(RxByte, &tl_value, &Data) == 0) {
               sml_Data(&Data, DataLength);
               switch (FieldNumber) {

                  case 0:
                  if ((Data & 0xff) == 0xff) {
                    //printf("\nObjectName:     %0llX, %d\n", Data, tl_value);
                    datas[ListNumber].obis = Data;
                  }
                  break;

                case 1:
                  //printf("status:         %0llX, %d\n", Data, tl_value );
                  break;

                case 2:
                  //printf("valTime:        %0llX, %d\n", Data, tl_value);
                  break;

                case 3:
                  datas[ListNumber].unit = Data;
                  //printf("unit:           %0llX, %d\n", Data, tl_value);
                  break;

                case 4:
                  datas[ListNumber].scale = scale = Data;
                  //printf("scaler:         %0llX, %d\n", Data, tl_value);
                  break;

                case 5:
                  datas[ListNumber++].value = ScaleValue(Data, scale);;
                  //printf("value:          %0llX, %.1f\n", Data,f);
                  break;

                case 6:
                  //printf("valueSignature: %0llX, %d\n", Data, Data );
                  break;

               }

               FieldNumber++;
               SMLState = SM_TL_FIELD;
               //printf(" -> %0llX  Len %d", Data, tl_value);
             }
                  break;

    case SM_CRC1:
      //printf("Fertig 1\n");
      SMLState++;
      break;

    case SM_CRC2:
      RxCrc = RxByte;
      scrc = crc ^ 0xffff;
      //printf("Fertig 2\n");
      SMLState++;
      break;

    case SM_CRC3:
      RxCrc |= RxByte << 8;
      if (RxCrc == scrc) {
        Rc = 1;
        //Serial.println("CRC ok");
         addLog(LOG_LEVEL_INFO, F(" CRC OK") );
        Success++;
      } else {
         addLog(LOG_LEVEL_INFO, F(" CRC Fail") );
         addLog(LOG_LEVEL_INFO,String("CRC Rx") + String(RxCrc,16));
         addLog(LOG_LEVEL_INFO,String("Berechnet: ") + String(scrc,16));
        // Serial.println("CRC fail");
        Errors++;
      }
      SMLState = SM_START_0;
      break;

    }

    crc = crc16_x25_table[(RxByte ^ crc) & 0xff] ^ ((crc >> 8) & 0xff);

    return Rc;
  }

  // ---------------------------------------------------------------------------

  void CheckRxByte(uint8_t* SMLState, uint8_t RxByte, uint8_t  x, uint8_t next) {
    if (RxByte == x) {
      (*SMLState)++;
    }
    else {
      *SMLState = next;
    }
  }

  // ---------------------------------------------------------------------------

  double ScaleValue(double v, int8_t s) {
    switch (s) {
    case -4: return v / 10000.0;
    case -3: return v / 1000.0;
    case -2: return v / 100.0;
    case -1: return v / 10.0;
    case  4: return v * 10000.0;
    case  3: return v * 1000.0;
    case  2: return v * 100.0;
    case  1: return v * 10.0;
    }
    return v;
  }

  // ---------------------------------------------------------------------------

  uint16_t sml_collect_Data(uint8_t RxByte, uint32_t* length, int64_t* data) {

    *data = ((*data) << 8) | RxByte;

    if (--(*length) > 1) {
      return 1;
    }
    return 0;
  }

  // ---------------------------------------------------------------------------

  void sml_Data(int64_t* data, uint16_t DataLength) {
    switch (DataLength-1) {
    case 1: *data = *(int8_t*)data; break;
    case 2: *data = *(int16_t*)data; break;
    case 3:
    case 4:  *data = *(int32_t*)data; break;
    default: *data = *(int64_t*)data; break;

    }
  }

  // ---------------------------------------------------------------------------

  uint8_t sml_parse_length(uint8_t RxByte, uint32_t* length) {
    if ((*length & 0x80) == 0x80) {
      *length = ((*length & 0x7f) << 4) | ( RxByte & 0x8F);
      if ((RxByte & 0x80) == 0x80) {
        return 0;
      }
    }
       return 1;
  }

  //-------------------------------------------------------------------------------------------------------------------------

  uint8_t sml_parse_tlfield(uint8_t data, uint8_t* tl_type, uint32_t* tl_value) {
     if (data == 0) {
       *tl_value = 0;
       *tl_type = NOP;
       return SML_PARSE_OK;
     }
     else if (data == 0x1b) {
       *tl_type = FERTIG;
       *tl_value = 0;
       return SML_PARSE_OK;
     }
     else if (data == 0x1a) {
       *tl_type = GANZFERTIG;
       *tl_value = 0;
       return SML_PARSE_OK;
     }

     char typeBits = (data & 0x70);
     // uint32_t i = 0;

     /* check tl-field type */
     if (typeBits == 0x00) {
       *tl_type = STRING;
     }
     else if (typeBits == 0x40) {
       *tl_type = BOOLEAN;
     }
     else if (typeBits == 0x50) {
       *tl_type = INTEGER;
     }
     else if (typeBits == 0x60) {
       *tl_type = UNSIGNED;
     }
     else if (typeBits == 0x70) {
       *tl_type = LIST;
     }
     else {
       return SML_PARSE_ERROR;
     }

     /* Set length of first tl-field */
     *tl_value = (data & 0x8F);

     return SML_PARSE_OK;
  }

};
//-------------------------------------------------------------------------------------------------------------------------
//
//====================================================================================================================



//-----------------------------------------------------------------------------------------

boolean Plugin_247(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_247;
        Device[deviceCount].Type = DEVICE_TYPE_SERIAL;     // connected through 3 datapins
        Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = P247_NR_OUTPUT_VALUES;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_247);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        for (byte i = 0; i < VARS_PER_TASK; ++i) {
          if ( i < P247_NR_OUTPUT_VALUES) {
            byte choice = PCONFIG(i + P247_QUERY1_CONFIG_POS);
            safe_strncpy(
              ExtraTaskSettings.TaskDeviceValueNames[i],
              P247_getQueryValueString(choice),
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
        P247_BAUDRATE = P247_BAUDRATE_DFLT;
        P247_QUERY1 = P247_QUERY1_DFLT;
        P247_QUERY2 = P247_QUERY2_DFLT;
        P247_QUERY3 = P247_QUERY3_DFLT;
        P247_QUERY4 = P247_QUERY4_DFLT;

        success = true;
        break;
      }
   case PLUGIN_WEBFORM_SHOW_SERIAL_PARAMS:
    {
        if (P247_BAUDRATE >= 7) {
          // Load some defaults
          P247_BAUDRATE = P247_BAUDRATE_DFLT;
          P247_QUERY1 = P247_QUERY1_DFLT;
          P247_QUERY2 = P247_QUERY2_DFLT;
          P247_QUERY3 = P247_QUERY3_DFLT;
          P247_QUERY4 = P247_QUERY4_DFLT;
        }

      {
        String options_baudrate[9];
        for (int i = 0; i < 9; ++i) {
          options_baudrate[i] = String(P247_storageValueToBaudrate(i));
        }
        addFormSelector(F("Baud Rate"), P247_BAUDRATE_LABEL, 8, options_baudrate, nullptr, P247_BAUDRATE );
        addUnit(F("baud"));
      }
      break;
    }
    case PLUGIN_WEBFORM_LOAD:
      {

         P247_data_struct *P247_data =  static_cast<P247_data_struct *>(getPluginTaskData(event->TaskIndex));
      
         if (nullptr == P247_data) 
         {
           addLog(LOG_LEVEL_INFO, "PLUGIN_WEBFORM_LOAD: nullptr == P247_data" );
         } else {
          addRowLabel(F("Checksum (pass/fail) "));
          String chksumStats;
          chksumStats = String(P247_data->Success);
          chksumStats += '/';
          chksumStats += String(P247_data->Errors);
          addHtml(chksumStats);
        }

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
          serialHelper_webformSave(event);

          P247_BAUDRATE = getFormItemInt(P247_BAUDRATE_LABEL);

         // Plugin_247_init = false; // Force device setup next time

          success = true;
          break;
      }

    case PLUGIN_INIT:
      {

         initPluginTaskData(event->TaskIndex, new P247_data_struct());
         P247_data_struct *P247_data = static_cast<P247_data_struct *>(getPluginTaskData(event->TaskIndex));

         if (nullptr == P247_data) {
           return false;
         }

          unsigned int baudrate = P247_storageValueToBaudrate(P247_BAUDRATE);
          P247_data->init(CONFIG_PORT,CONFIG_PIN1, CONFIG_PIN2,baudrate) ;
          success = true;
          break;
      }

    case PLUGIN_EXIT:
    {

      break;
    }

    case PLUGIN_READ:
      {

        P247_data_struct *P247_data =  static_cast<P247_data_struct *>(getPluginTaskData(event->TaskIndex));
         if (nullptr != P247_data){ 
         // UserVar[event->BaseVarIndex]     = P247_data->Bezogen;
         // UserVar[event->BaseVarIndex + 1] = P247_data->Geliefert;
         // UserVar[event->BaseVarIndex + 2] = P247_data->Leistung;
          UserVar[event->BaseVarIndex + 3] = P247_data->Errors;
         }
         else
         {
          
         }

          // byte dev_id = P247_DEV_ID;
          success = true;
          break;
        break;
      }

      case PLUGIN_TEN_PER_SECOND:
      {
        P247_data_struct *P247_data =  static_cast<P247_data_struct *>(getPluginTaskData(event->TaskIndex));
         if (nullptr != P247_data)
         { 
           P247_data->SMLPoll(event);
         }
         else
         {
           UserVar[event->BaseVarIndex + 3]++;
         }
         break;
      }

      case PLUGIN_SERIAL_IN:  
      {
        addLog(LOG_LEVEL_INFO, "PLUGIN_SERIAL_IN" );
        break;
      }



  }
  return success;
}



String P247_getQueryString(byte query) {
  switch(query)
  {
    case 0: return F("Voltage (V)");
    case 1: return F("Current (A)");
    case 2: return F("Power (W)");
    case 3: return F("Active Apparent Power (VA)");
    case 4: return F("Reactive Apparent Power (VAr)");
    case 5: return F("Power Factor (cos-phi)");
    case 6: return F("Frequency (Hz)");
    case 7: return F("Import Active Energy (Wh)");
    case 8: return F("Export Active Energy (Wh)");
    case 9: return F("Total Active Energy (Wh)");
  }
  return "";
}

String P247_getQueryValueString(byte query) {
  switch(query)
  {
    case 0: return F("V");
    case 1: return F("A");
    case 2: return F("W");
    case 3: return F("VA");
    case 4: return F("VAr");
    case 5: return F("cos_phi");
    case 6: return F("Hz");
    case 7: return F("Wh_imp");
    case 8: return F("Wh_exp");
    case 9: return F("Wh_tot");
  }
  return "";
}


int P247_storageValueToBaudrate(byte baudrate_setting) {
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


#endif // USES_P247
