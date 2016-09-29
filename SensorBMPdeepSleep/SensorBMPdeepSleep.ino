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

//voltage measurement
ADC_MODE(ADC_VCC);

Adafruit_BMP085 bmp;

#define       FW_NAME       "node-BMP"
#define       FW_VERSION    "0.0.1"
const int     PUB_INTERVAL    = 60;  //seconds
unsigned long lastPublish = 0;

//flags for deep sleep modus
bool deepSleepModus = true; // or false: set to true if deep sleep option is to be used
                            //remember to connect RST to D0 for this to function
                            // set false to run in contineous mode with no deep sleep
                            
const int SLEEP_INTERVAL = 60; //minutes, turn off for given minutes, start sensor, transmit data, go to sleep again

bool sleepFlag = false; //always set to false

int counter = 0;

//count offline time before closing down in deepsleep
unsigned long attemptLogonTime = 0;

//Homie reset pin
const int RST_PIN    = D6;
HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");

void setupHandler() {
  Homie.setNodeProperty(temperatureNode, "unit", "c", true);
  Homie.setNodeProperty(humidityNode, "unit", "%", true);
}

void loopHandler() {

  if (!deepSleepModus) {
        if (millis() - lastPublish >= PUB_INTERVAL * 1000UL) {
          DEBUG_PRINTLN("attempting to get properties") ;
          float t = bmp.readTemperature();
          float h = bmp.readPressure()/100; //mPa
          float b = ESP.getVcc()/1000.0;

          if (!isnan(t) && Homie.setNodeProperty(temperatureNode, "degrees", String(t), true)) {
             DEBUG_PRINTLN("set new value to MQTT: "); DEBUG_PRINTDEC(t);  
            lastPublish = millis();
          }
          if (!isnan(h) && Homie.setNodeProperty(humidityNode, "relative", String(h), true) && Homie.setNodeProperty(humidityNode, "battery", String(b), true)) {
             DEBUG_PRINTLN("set new value to MQTT: "); DEBUG_PRINTDEC(h);  
            lastPublish = millis();
          }    
        }
    }else{
        //running in deep sleep modus
        float t = bmp.readTemperature();
        float h = bmp.readPressure()/100; //mPa
        float b = ESP.getVcc()/1000.0;
        
        DEBUG_PRINTLN("attempting to get properties") ;
        if (!isnan(t) && Homie.setNodeProperty(temperatureNode, "degrees", String(t), true)) {
           DEBUG_PRINTLN("set new value to MQTT: "); DEBUG_PRINTDEC(t);  
           //reset counter
           counter = 0;
        }
        if (!isnan(h) && Homie.setNodeProperty(humidityNode, "relative", String(h), true) && Homie.setNodeProperty(humidityNode, "battery", String(b), true)) {
          DEBUG_PRINTLN("set new value to MQTT: "); DEBUG_PRINTDEC(h);
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
  
  Homie.setFirmware(FW_NAME, FW_VERSION);

  Homie.registerNode(temperatureNode);
  Homie.registerNode(humidityNode);
  
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

  if (millis() - attemptLogonTime >= 10 * 1000UL) {
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
  }
}
