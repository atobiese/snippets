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

//control a light sensor, and send voltage info to mqtt. 

#include <Homie.h>

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

#define FW_NAME       "node-dht"
#define FW_VERSION    "0.0.1"
//#define DHT_TYPE      DHT22

const int SENSOR_PIN    = D7;  //deep sleep does not work on D4 (same as led)
const int PUB_INTERVAL  = 5;  //seconds (if deepsleep is not used)

unsigned long lastPublish = 0;

//flags for deep sleep modus
bool deepSleepModus = true; //set to true if deep sleep option is to be used
                            //remember to connect RST to D0 for this to function
                            // set false to run in contineous mode with no deep sleep
                            
const int SLEEP_INTERVAL = 30; //minutes, turn off for given minutes, start sensor, transmit data, go to sleep again

bool sleepFlag = false; //always set to false

int counter = 0;
//voltage measurement
ADC_MODE(ADC_VCC);
float h;
int t;
String tt;
HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");

void setupHandler() {
  Homie.setNodeProperty(temperatureNode, "unit", "c", true);
  Homie.setNodeProperty(humidityNode, "unit", "%", true);
  
  h = 0.0;
  t = 0;
  pinMode(SENSOR_PIN,INPUT);
}

void loopHandler() {

  if (!deepSleepModus) {
        if (millis() - lastPublish >= PUB_INTERVAL * 1000UL) {
           DEBUG_PRINTLN("attempting to get properties") ;
           h = ESP.getVcc()/1000.0;
           int val = digitalRead(SENSOR_PIN); //dht.readTemperature();
           DEBUG_PRINT(val); 
           t = val; //100-map(val, 0, 1023, 0, 100);
           if (t == 1) {
             tt = "det er mørkt ute";
           }
           else {
             tt = "det er lyst ute";
           }
          if (!isnan(t) && Homie.setNodeProperty(temperatureNode, "degrees", tt, true)) {
             DEBUG_PRINTLN(""); 
             DEBUG_PRINTLN("set new t value to MQTT: "); DEBUG_PRINT(tt);  
            lastPublish = millis();
          }
          if (!isnan(h) && Homie.setNodeProperty(humidityNode, "relative", String(h), true)) {
             DEBUG_PRINTLN("");
             DEBUG_PRINTLN("set new h value to MQTT: "); DEBUG_PRINTDEC(h);  
            lastPublish = millis();
          }    
        }
    }else{
        //running in deep sleep modus
          DEBUG_PRINTLN("attempting to get properties") ;
           h = ESP.getVcc()/1000.0;
           int val = digitalRead(SENSOR_PIN); //dht.readTemperature();
           DEBUG_PRINT(val); 
           t = val; //100-map(val, 0, 1023, 0, 100);
           if (t == 1) {
             tt = "det er mørkt ute";
           }
           else {
             tt = "det er lyst ute";
           }
        if (!isnan(t) && Homie.setNodeProperty(temperatureNode, "degrees", tt, true)) {
           DEBUG_PRINTLN(""); 
           DEBUG_PRINTLN("set new t value to MQTT: "); DEBUG_PRINTDEC(t);  
           //reset counter
           counter = 0;
        }
        if (!isnan(h) && Homie.setNodeProperty(humidityNode, "relative", String(h), true)) {
          DEBUG_PRINTLN(""); 
          DEBUG_PRINTLN("set new h value to MQTT: "); DEBUG_PRINTDEC(h);
          //reset counter
          counter = 0;
        }
        
        counter = counter + 1;
        
        if (!isnan(t) && !isnan(h))  {
          DEBUG_PRINTLN("-Disconnecting MQTT "); 
          Homie.disconnectMQTT();
          delay(100);
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
  Homie.setFirmware(FW_NAME, FW_VERSION);

  Homie.registerNode(temperatureNode);
  Homie.registerNode(humidityNode);
  
  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);

  Homie.enableBuiltInLedIndicator(false);  
  
  //enable event to grigger when MQTT disconnects
  if (deepSleepModus) {
    Homie.onEvent(onHomieEvent);
  }
 
  Homie.setup();
}

void loop() {
  
  if (sleepFlag == true) {
    DEBUG_PRINT("-Going to sleep for (seconds): "); DEBUG_PRINTDEC(SLEEP_INTERVAL); 
      //following may be needed for DHT to function after deep sleep
      //check  http://www.esp8266.com/viewtopic.php?f=32&t=8114&start=20#sthash.DxVCjSm8.dpuf
      //pinMode(SENSOR_PIN, OUTPUT);
      //digitalWrite(SENSOR_PIN, LOW); 
    
    ESP.deepSleep(SLEEP_INTERVAL * 1000000UL * 60UL);
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
  }
}
