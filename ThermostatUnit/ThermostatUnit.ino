// a module for thermostat control. one relay (turns on or off heater) and a temperature/humidity sensor for regulation.
// runs on an ESP8266

//external code controls the relay at given setpoints, based on temperature readings

// v0.1 controls relay and logs temperature, 
// v0.2 simple bypass dht node if only used. 
// updated for Homie 2.0

//TODO: lacks timer watchdog, to turn off relay if no response from server

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
#define FW_VERSION    "0.0.2"
#define DHT_TYPE      DHT22

const int PIN_RELAY1    = D8; // 12 sonoff //D1; // D1 wemos //D8 for brettet med nodemcu
const int DHT_PIN       = D7; //15 sonoff D7 nodemcu

const int PIN_LED = D4; // 13 for sonoff //D7 for wemos //D4 for built in led wemos

int counter = 0;

const int PUB_INTERVAL  = 60;  //seconds

unsigned long lastPublish = 0; //timer definition, set to zero

//Homie reset pin
const int RST_PIN    = D6; // take out and add 98 for sonoff; //add a pushbutton here or wire D6 to GND for 2 seconds


HomieNode relayNode1("relay",  "switch");
HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");

DHT dht(DHT_PIN, DHT_TYPE);

void setupHandler() {
  //Homie.setNodeProperty(temperatureNode, "unit", "c", true);
  //Homie.setNodeProperty(humidityNode, "unit", "%", true);
  temperatureNode.setProperty("unit").setRetained(true).send("c");
  humidityNode.setProperty("unit").setRetained(true).send("%");
  
  dht.begin();
}

void loopHandler() {

    if (millis() - lastPublish >= PUB_INTERVAL * 1000UL) {
      DEBUG_PRINTLN("attempting to get properties") ;
      float t = dht.readTemperature();
      float h = dht.readHumidity();
      
      if (!isnan(t) && temperatureNode.setProperty("degrees").send(String(t))) {
         DEBUG_PRINTLN("set new value to MQTT: "); DEBUG_PRINTDEC(t);  DEBUG_PRINT("\n");
        lastPublish = millis();
        counter = 0;
      } 
      if (!isnan(h) && humidityNode.setProperty("relative").send(String(h))) {
         DEBUG_PRINTLN("set new value to MQTT: "); DEBUG_PRINTDEC(h);  DEBUG_PRINT("\n");
        lastPublish = millis();
        counter = 0;
      }
      counter = counter + 1; 
      if (counter>10)  {
          DEBUG_PRINTLN("-Sensor attempted 10 times. No measurement obtained "); 
          DEBUG_PRINTLN("Publishing an error (-1)");
          //Homie.setNodeProperty(temperatureNode, "degrees", "-1", false); 
          //Homie.setNodeProperty(humidityNode, "relative", "-1", false); 
          
          // continue running as if there is no measurement or sensor connected
          lastPublish = millis();
        }   
    }
}

bool relayOnHandler(const HomieRange& range, const String& value) {

  if (value != "true" && value != "false") return false;

  bool on = (value == "true");
  digitalWrite(PIN_RELAY1, on ? HIGH : LOW);
  digitalWrite(PIN_LED, on ? HIGH : LOW);
  relayNode1.setProperty("on").send(value); // Update state
  Homie.getLogger() << "Light is " << (on ? "on" : "off") << endl;
  if (value == "true") {
    DEBUG_PRINTLN("Relay1 is on");
  } else if (value == "false") {
    DEBUG_PRINTLN("Relay1 is off");
  }
return true;
  
//  if (value == "true") {
//    digitalWrite(PIN_RELAY1, HIGH);
//    //sonoff led
//    digitalWrite(PIN_LED, HIGH);
//    relayNode1.setProperty("on").send(value); // Update state
//    DEBUG_PRINTLN("Relay1 is on");
//    
//  } else if (value == "false") {
//    digitalWrite(PIN_RELAY1, LOW);
//    relayNode1.setProperty("on").send(value); // Update state
//    DEBUG_PRINTLN("Relay1 is off");
//  } else {
//    return false;
//  }
//  return true;
}

void setup() {
  // reset relays to off
  pinMode(PIN_RELAY1, OUTPUT);
  digitalWrite(PIN_RELAY1, LOW);
  //sonoff led
  digitalWrite(PIN_LED, LOW);
  
   #ifdef DEBUG 
    Serial.begin(115200);
  #endif
  
  Homie_setFirmware(FW_NAME, FW_VERSION);
  
  //relayNode1.subscribe("on",  relayOnHandler);
  //Homie.registerNode(relayNode1);
  relayNode1.advertise("on").settable(relayOnHandler);
  //Homie.registerNode(temperatureNode);
  //Homie.registerNode(humidityNode);
  temperatureNode.advertise("degrees");
  humidityNode.advertise("relative");
  
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);

  //Homie.enableBuiltInLedIndicator(false); 
  //a wemos is used. reset if D6 is pressed for 2 seconds (D6 wired to gnd)
  Homie.setResetTrigger(RST_PIN, LOW, 6000);  
  
  
  Homie.setup();
}

void loop() {
  
  Homie.loop();
}

