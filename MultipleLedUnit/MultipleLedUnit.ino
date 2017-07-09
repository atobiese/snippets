// a module for thermostat control. one relay (turns on or off heater) and a temperature/humidity sensor for regulation.
// runs on an ESP8266

//external code controls the relay at given setpoints, based on temperature readings

// v0.1 controls relay and logs temperature, 
// v0.2 simple bypass dht node if only relay is used. 

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

const int PIN_LED1    = D1; 
const int PIN_LED2    = D2; 
const int PIN_LED3    = D3; 
const int PIN_LED4    = D6; //onboard led set to low, so not using it
const int PIN_LED5    = D5; 
 




const int DHT_PIN       = 15; //15 sonoff

const int PIN_LED = D7; // 13 for sonoff //D7 for wemos 

int counter = 0;

const int PUB_INTERVAL  = 60;  //seconds

unsigned long lastPublish = 0; //timer definition, set to zero

//Homie reset pin
const int RST_PIN    = D6; // take out and add 98 for sonoff; //add a pushbutton here or wire D6 to GND for 2 seconds


HomieNode relayNode1("led1",  "switch");
HomieNode relayNode2("led2",  "switch");
HomieNode relayNode3("led3",  "switch");
HomieNode relayNode4("led4",  "switch");
HomieNode relayNode5("led5",  "switch");


bool relay1OnHandler(String value) {
  if (value == "true") {
    digitalWrite(PIN_LED1, HIGH);
    //sonoff led
    digitalWrite(PIN_LED, HIGH);
    Homie.setNodeProperty(relayNode1, "on", value); // Update state
    DEBUG_PRINTLN("Relay1 is on");
    
  } else if (value == "false") {
    digitalWrite(PIN_LED1, LOW);
    Homie.setNodeProperty(relayNode1, "on", value);
    DEBUG_PRINTLN("Relay1 is off");
  } else {
    return false;
  }
  return true;
}

bool relay2OnHandler(String value) {
  if (value == "true") {
    digitalWrite(PIN_LED2, HIGH);
    Homie.setNodeProperty(relayNode2, "on", value); // Update state
    DEBUG_PRINTLN("Relay1 is on");
  } else if (value == "false") {
    digitalWrite(PIN_LED2, LOW);
    Homie.setNodeProperty(relayNode2, "on", value);
    DEBUG_PRINTLN("Relay1 is off");
  } else {
    return false;
  }
  return true;
}

bool relay3OnHandler(String value) {
  if (value == "true") {
    digitalWrite(PIN_LED3, HIGH);
    Homie.setNodeProperty(relayNode3, "on", value); // Update state
    DEBUG_PRINTLN("Relay3 is on");
  } else if (value == "false") {
    digitalWrite(PIN_LED3, LOW);
    Homie.setNodeProperty(relayNode3, "on", value);
    DEBUG_PRINTLN("Relay3 is off");
  } else {
    return false;
  }
  return true;
}

bool relay4OnHandler(String value) {
  if (value == "true") {
    digitalWrite(PIN_LED4, HIGH);
    Homie.setNodeProperty(relayNode4, "on", value); // Update state
    DEBUG_PRINTLN("Relay4 is on");
  } else if (value == "false") {
    digitalWrite(PIN_LED4, LOW);
    Homie.setNodeProperty(relayNode4, "on", value);
    DEBUG_PRINTLN("Relay4 is off");
  } else {
    return false;
  }
  return true;
}

bool relay5OnHandler(String value) {
  if (value == "true") {
    digitalWrite(PIN_LED5, HIGH);
    Homie.setNodeProperty(relayNode5, "on", value); // Update state
    DEBUG_PRINTLN("Relay5 is on");
  } else if (value == "false") {
    digitalWrite(PIN_LED5, LOW);
    Homie.setNodeProperty(relayNode5, "on", value);
    DEBUG_PRINTLN("Relay5 is off");
  } else {
    return false;
  }
  return true;
}
void setup() {
  // reset relays to off
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);
  pinMode(PIN_LED4, OUTPUT);
  pinMode(PIN_LED5, OUTPUT);
 
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);
  digitalWrite(PIN_LED3, LOW);
  digitalWrite(PIN_LED4, LOW);
  digitalWrite(PIN_LED5, LOW);


  
  
   #ifdef DEBUG 
    Serial.begin(115200);
  #endif
  
  Homie.setFirmware(FW_NAME, FW_VERSION);
  
  relayNode1.subscribe("on",  relay1OnHandler);
  Homie.registerNode(relayNode1);
  relayNode2.subscribe("on",  relay2OnHandler);
  Homie.registerNode(relayNode2);
  relayNode3.subscribe("on",  relay3OnHandler);
  Homie.registerNode(relayNode3);
  relayNode4.subscribe("on",  relay4OnHandler);
  Homie.registerNode(relayNode4);
  relayNode5.subscribe("on",  relay5OnHandler);
  Homie.registerNode(relayNode5);
  

  //Homie.enableBuiltInLedIndicator(false); 
  //a wemos is used. reset if D6 is pressed for 2 seconds 
  Homie.setResetTrigger(RST_PIN, LOW, 6000);  
  
  
  Homie.setup();
}

void loop() {
  
  Homie.loop();
}

