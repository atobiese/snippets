// a module for thermostat control. one relay (turns on or off heater) and a temperature/humidity sensor for regulation.
// runs on an ESP8266

//external code controls the relay at given setpoints, based on temperature readings

// v0.0.1 controls relay and logs temperature, lacks timer watchdog, to turn off relay if no respons from server

#include <Homie.h>
#include <DHT.h>

//debug def to be used after library definitions
//-----------Debug script----------
#define DEBUG

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


#define FW_NAME       "node-dht/relay"
#define FW_VERSION    "0.0.1"
#define DHT_TYPE      DHT22

const int PIN_RELAY1    = D8;
const int DHT_PIN       = D7;

const int PUB_INTERVAL  = 60;  //seconds

unsigned long lastPublish = 0; //timer definition, set to zero

HomieNode relayNode1("relay",  "switch");
HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");

DHT dht(DHT_PIN, DHT_TYPE);

void setupHandler() {
  Homie.setNodeProperty(temperatureNode, "unit", "c", true);
  Homie.setNodeProperty(humidityNode, "unit", "%", true);
  
  dht.begin();
}

void loopHandler() {

    if (millis() - lastPublish >= PUB_INTERVAL * 1000UL) {
      DEBUG_PRINTLN("attempting to get properties") ;
      float t = dht.readTemperature();
      float h = dht.readHumidity();
      
      if (!isnan(t) && Homie.setNodeProperty(temperatureNode, "degrees", String(t), true)) {
         DEBUG_PRINTLN("set new value to MQTT: "); DEBUG_PRINTDEC(t);  DEBUG_PRINT("\n");
        lastPublish = millis();
      } 
      if (!isnan(h) && Homie.setNodeProperty(humidityNode, "relative", String(h), true)) {
         DEBUG_PRINTLN("set new value to MQTT: "); DEBUG_PRINTDEC(h);  DEBUG_PRINT("\n");
        lastPublish = millis();
      }    
    }
}

bool relayOnHandler(String value) {
  if (value == "true") {
    digitalWrite(PIN_RELAY1, HIGH);
    Homie.setNodeProperty(relayNode1, "on", value); // Update state
    DEBUG_PRINTLN("Relay1 is on");
  } else if (value == "false") {
    digitalWrite(PIN_RELAY1, LOW);
    Homie.setNodeProperty(relayNode1, "on", value);
    DEBUG_PRINTLN("Relay1 is off");
  } else {
    return false;
  }
  return true;
}

void setup() {
  // reset relays to off
  pinMode(PIN_RELAY1, OUTPUT);
  digitalWrite(PIN_RELAY1, LOW);
  
   #ifdef DEBUG 
    Serial.begin(115200);
  #endif
  
  Homie.setFirmware(FW_NAME, FW_VERSION);
  
  relayNode1.subscribe("on",  relayOnHandler);
  Homie.registerNode(relayNode1);

  Homie.registerNode(temperatureNode);
  Homie.registerNode(humidityNode);

  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);

  //Homie.enableBuiltInLedIndicator(false);  
  
  
  Homie.setup();
}

void loop() {
  
  Homie.loop();
}

