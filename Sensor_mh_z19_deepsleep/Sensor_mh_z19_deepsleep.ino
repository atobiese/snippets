//-------------------------------------
//note: for homie to work with deep sleep, add the following, as per https://github.com/marvinroger/homie-esp8266/issues/77:
//in Homie.cpp at the end of the file, before class definition:
//
//void HomieClass::disconnectMQTT() {
//  if (!this->isReadyToOperate()) {
//    this->_logger.logln(F("✖ disconnect(): impossible now"));
//    return;
//  }
//
//  this->_mqttClient.disconnect();
//}
//
//HomieClass Homie;
//
//and in Homie.hpp:
//after bool setNodeProperty(const HomieNode& node, const char* property, const char* value, bool retained = true);
//      void disconnectMQTT();
//-------------------------------------

//-----------Debug script----------

#define DEBUG  //select if debug or not
#ifdef DEBUG
 #define DEBUG_PRINT(x)         Serial.print (x)
 #define DEBUG_PRINTDEC(x)      Serial.print (x, DEC)
 #define DEBUG_PRINTLN(x)       Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x)   
#endif
//---------------------------------

#include <Homie.h>
#include <Adafruit_BMP085.h>
// I2C:
// D1 <-> SCL (wemos D1)
// D2 <-> SDA

#include <stdio.h>
#include "mhz19.h"
#include "SoftwareSerial.h"

#define PIN_RX  D3
#define PIN_TX  D4

SoftwareSerial sensor(PIN_RX, PIN_TX);

//voltage measurement
//ADC_MODE(ADC_VCC); //this only measures inner circuit voltage

Adafruit_BMP085 bmp;

#define       FW_NAME       "node-BMP-CO2-ds"
#define       FW_VERSION    "0.3"
const int     PUB_INTERVAL    = 10;  //seconds (when not running deep sleep mode)

//flags for deep sleep modus
bool deepSleepModus = false; // or false: set to true if deep sleep option is to be used
                            //remember to connect RST to D0 for this to function
                            // set false to run in contineous mode with no deep sleep
                            
const int SLEEP_INTERVAL = 10; //minutes, turn off for given minutes, start sensor, transmit data, go to sleep again
//Homie reset pin
const int RST_PIN    = D6; //add a pushbutton here or wire D6 to GND for 2 seconds


bool sleepFlag = false; //always set to false
unsigned long lastPublish = 0;

int counter = 0;

//count offline time before closing down in deepsleep
unsigned long attemptLogonTime = 0;

HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");
HomieNode co2Node("co2level", "co2level");

void setupHandler() {
  Homie.setNodeProperty(temperatureNode, "unit", "c", true);
  Homie.setNodeProperty(humidityNode, "unit", "%", true);
  Homie.setNodeProperty(co2Node, "unit", "ppm", true);
}

void loopHandler() {

  if (!deepSleepModus) {
        if (millis() - lastPublish >= PUB_INTERVAL * 1000UL) {
          DEBUG_PRINTLN("attempting to get properties") ;
          float t, h;
          if (bmp.begin()) 
          {
            t = bmp.readTemperature();
            h = bmp.readPressure()/100; //mPa
          }            
          else
          {
            Serial.println("BMP180 init fail \n\n");
            t = -1;
            h = -1;
          }  
          float b = battery_level(); //ESP.getVcc()/1000.0;

          int co2, temp;
          if (read_temp_co2(&co2, &temp)) {
              Serial.print("CO2:");
              Serial.println(co2, DEC);
              Serial.print("TEMP:");
              Serial.println(temp, DEC);
              
          } else {
              Serial.println("failed to read CO2 sensor!");
              co2 = -1;
              temp= -1;
          }
          

          if (!isnan(t) && Homie.setNodeProperty(temperatureNode, "degrees", String(t), true)) {
             DEBUG_PRINT("set new value to MQTT: "); DEBUG_PRINTDEC(t);  DEBUG_PRINTLN("");
            lastPublish = millis();
          }
          if (!isnan(h) && Homie.setNodeProperty(humidityNode, "relative", String(h), true) && Homie.setNodeProperty(humidityNode, "battery", String(b), true)) {
             DEBUG_PRINT("set new value to MQTT: "); DEBUG_PRINTDEC(h);  DEBUG_PRINTLN("");
             DEBUG_PRINT("set new battterly level to MQTT: "); DEBUG_PRINTDEC(b); DEBUG_PRINTLN("");
            lastPublish = millis();
          }
          
          if (!isnan(t) && Homie.setNodeProperty(co2Node, "ppm", String(co2), true)) {
             DEBUG_PRINT("set new value to MQTT: "); DEBUG_PRINTDEC(co2);  DEBUG_PRINTLN("");
            lastPublish = millis();
          }    
        }
    }else{
        //running in deep sleep modus
        float t = 0.0; //bmp.readTemperature();
        float h = 0.0; //bmp.readPressure()/100; //mPa
        float b = 0.0; //battery_level(); //ESP.getVcc()/1000.0;

         int co2, temp;
          if (read_temp_co2(&co2, &temp)) {
              Serial.print("CO2:");
              Serial.println(co2, DEC);
              Serial.print("TEMP:");
              Serial.println(temp, DEC);
              
          } else {
              Serial.println("failed to read CO2 sensor!");
          }
        
        DEBUG_PRINTLN("attempting to get properties") ;
        if (!isnan(t) && Homie.setNodeProperty(temperatureNode, "degrees", String(t), true)) {
           DEBUG_PRINT("set new value to MQTT: "); DEBUG_PRINTDEC(t);  DEBUG_PRINTLN("");
           //reset counter
           counter = 0;
        }
        if (!isnan(h) && Homie.setNodeProperty(humidityNode, "relative", String(h), true) && Homie.setNodeProperty(humidityNode, "battery", String(b), true)) {
          DEBUG_PRINT("set new value to MQTT: "); DEBUG_PRINTDEC(h);  DEBUG_PRINTLN("");
          DEBUG_PRINT("set new battterly level to MQTT: "); DEBUG_PRINTDEC(b); DEBUG_PRINTLN("");
          //reset counter
          counter = 0;
        }
        if (!isnan(t) && Homie.setNodeProperty(co2Node, "ppm", String(co2), true)) {
             DEBUG_PRINT("set new value to MQTT: "); DEBUG_PRINTDEC(co2);  DEBUG_PRINTLN("");
           //reset counter
           counter = 0;
        }
        
        
        counter = counter + 1;
        
        if (!isnan(t) && !isnan(h))  {
          DEBUG_PRINTLN("-Disconnecting MQTT "); 
          Homie.disconnectMQTT();
          delay(10);
        }
        if (counter>100)  {
          DEBUG_PRINTLN("-Sensor attempted 100 times. No measurement obtained "); 
          DEBUG_PRINTLN("-Disconnecting MQTT ");
          Homie.setNodeProperty(temperatureNode, "degrees", "-1", true); 
          Homie.setNodeProperty(humidityNode, "relative", "-1", true); 
          Homie.disconnectMQTT();
          //delay(100);
        }
     }
}

void setup() {
  
  #ifdef DEBUG 
    Serial.begin(115200);
  #endif

  bmp.begin();
  sensor.begin(9600);
  
  Homie.setFirmware(FW_NAME, FW_VERSION);

  Homie.registerNode(temperatureNode);
  Homie.registerNode(humidityNode);
  Homie.registerNode(co2Node);
  
  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);
  
  //no leds blinking
  Homie.enableBuiltInLedIndicator(false);  
  
  //enable event to grigger when MQTT disconnects
  if (deepSleepModus) {
    Homie.onEvent(onHomieEvent);
  }

  //a wemos is used. reset if D6 is pressed for 2 seconds 
  Homie.setResetTrigger(RST_PIN, LOW, 2000); 
  
  Homie.setup();
}

void loop() {
  
  if (sleepFlag == true) {
    DEBUG_PRINT("-Going to sleep for (minutes): "); DEBUG_PRINTDEC(SLEEP_INTERVAL);   
    ESP.deepSleep(SLEEP_INTERVAL * 1000000UL * 60UL);
  }

  //only in normal run shut down event
  if (millis() - attemptLogonTime >= 10 * 1000UL && !(HOMIE_CONFIGURATION_MODE)) {
     DEBUG_PRINTLN("10 seconds has passed and no wifi conn, shutting down to save energy..");
     sleepFlag = true;
  }
  
  Homie.loop();
}


// deep sleep event is activated when mqtt is shut off.
void onHomieEvent(HomieEvent event) {
  switch(event) {
    case HOMIE_MQTT_DISCONNECTED:
        sleepFlag = true;
        DEBUG_PRINTLN("mqtt disconnected") ;
        break;
    case HOMIE_NORMAL_MODE:
      attemptLogonTime = millis();
      
      break;
    case HOMIE_WIFI_CONNECTED:
        DEBUG_PRINTLN("wifi connected");
        attemptLogonTime = 0.0;
      break;  
    case HOMIE_CONFIGURATION_MODE:
        DEBUG_PRINTLN("this is: HOMIE_CONFIGURATION_MODE");
        DEBUG_PRINTLN(event);
        attemptLogonTime = 0.0;
      break;  
  }
}

float battery_level() {
 
  // read the battery level from the ESP8266 analog in pin.
  // analog read level is 10 bit 0-1023 (0V-1V).
  // our 1M & 220K voltage divider takes the max
  // lipo value of 4.2V and drops it to 0.758V max.
  // this means our min analog read value should be 580 (3.14V)
  // and the max analog read value should be 774 (4.2V).
  float norm = 1023.0;
  float correction = 1.0; //1.30;
  float voltagemultiplier = 0.00496f;
  float level = analogRead(A0)*voltagemultiplier; //(analogRead(A0))/norm/correction;
  float level_corr = 1.0; //level*(1220.0/220.0);

  //const float voltagemultiplier = 0.00496f; // Using a 200k resistor between the battery voltage and analog input A0 this experimentally results in the correct voltage being calculated in my setup, this might need to be changed for yours
  
  // convert battery level to percent
  float lowerLim = 330.0;
  float upperLim = 415.0;
  float level_norm = map(level_corr*100, lowerLim, upperLim, 0, 100);
  DEBUG_PRINT("Battery level: "); DEBUG_PRINTLN(level); 
  DEBUG_PRINT("Battery voltage: "); DEBUG_PRINTLN(level_corr);
  DEBUG_PRINT("Battery percent: "); DEBUG_PRINTLN(level_norm); 
  return level_corr;
}

static bool exchange_command(uint8_t cmd, uint8_t data[], int timeout)
{
    // create command buffer
    uint8_t buf[9];
    int len = prepare_tx(cmd, data, buf, sizeof(buf));

    // send the command
    sensor.write(buf, len);

    // wait for response
    long start = millis();
    while ((millis() - start) < timeout) {
        if (sensor.available() > 0) {
            uint8_t b = sensor.read();
            if (process_rx(b, cmd, data)) {
                return true;
            }
        }
    }

    return false;
}


static bool read_temp_co2(int *co2, int *temp)
{
    uint8_t data[] = {0, 0, 0, 0, 0, 0};
    int timeout = 300; //ms
    bool result = exchange_command(0x86, data, timeout);
    if (result) {
        *co2 = (data[0] << 8) + data[1];
        *temp = data[2] - 40;
#if 1
        char raw[32];
        sprintf(raw, "RAW: %02X %02X %02X %02X %02X %02X", data[0], data[1], data[2], data[3], data[4], data[5]);
        Serial.println(raw);
#endif
    }
    return result;
}
