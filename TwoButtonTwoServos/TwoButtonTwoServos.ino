// example script that controls 2 leds (as relays at D1 and D4), where the last is a "pusbutton" relay. Two servos at D5 and D6. 
// the servos are used to control a camera module.
// runs on an ESP8266

#include <Homie.h>
#include <Servo.h> 

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

#define OPENER_EVENT_MS 1000
const int PIN_RELAY1 = D1;
const int PIN_RELAY2 = D4;
const int PIN_SERVO1 = D7;
const int PIN_SERVO2 = D8;

String inString = "";     //string to hold input to servos
static int val = 0;       //value to be sent to servo (0-100)

const int minA1 = 108;   const int maxA1 = 144;  //min and max angles allowed for servo (calibrated)  
const int minA2 = 0;     const int maxA2 = 180;; //min and max angles allowed for servo

Servo servo1; Servo servo2; 

// define a monitor timerRelay
unsigned long timerRelay = 0;

HomieNode relay1Node("relay1",  "switch");
HomieNode relay2Node("relay2",  "switch");
HomieNode servo1Node("servo1",  "servoNode");
HomieNode servo2Node("servo2",  "servoNode");


bool relay1OnHandler(String value) {
  if (value == "true") {
    digitalWrite(PIN_RELAY1, HIGH);
    Homie.setNodeProperty(relay1Node, "on", value); // Update state
    DEBUG_PRINTLN("Relay1 is on");
  } else if (value == "false") {
    digitalWrite(PIN_RELAY1, LOW);
    Homie.setNodeProperty(relay1Node, "on", value);
    DEBUG_PRINTLN("Relay1 is off");
  } else {
    return false;
  }
  return true;
}

bool relay2OnHandler(String value) {
  if (value == "true") {
    digitalWrite(PIN_RELAY2, LOW);
    Homie.setNodeProperty(relay2Node, "on", value); // Update state
    //monitor timer for this particular node
    timerRelay = millis();
    
  } else if (value == "false") {
    digitalWrite(PIN_RELAY2, HIGH);
    Homie.setNodeProperty(relay2Node, "on", value);
    DEBUG_PRINTLN("Relay2 is off");
  } else {
    return false;
  }
  return true;
}

bool servo1NodeHandler(String value) {
  int len = value.length();
  if (len <= 3) { 
    Homie.setNodeProperty(servo1Node, "on", value); // Update state 
    runServo(value, servo1, PIN_SERVO1, minA1, maxA1);
  } else {
    return false;
  }
  return true;
}

bool servo2NodeHandler(String value) {
  int len = value.length();
  if (len <= 3) {
    Homie.setNodeProperty(servo2Node, "on", value); // Update state 
    runServo(value, servo2, PIN_SERVO2, minA2, maxA2); //min and max angles allowed for servo
  } else {
    return false;
  }
  return true;
}

void setup() {
  // reset relays to off
  pinMode(PIN_RELAY1, OUTPUT);
  digitalWrite(PIN_RELAY1, LOW);
  pinMode(PIN_RELAY2, OUTPUT);
  digitalWrite(PIN_RELAY2, HIGH);
   pinMode(PIN_SERVO1, OUTPUT);
  digitalWrite(PIN_SERVO1, LOW);
  pinMode(PIN_SERVO2, OUTPUT);
  digitalWrite(PIN_SERVO2, HIGH);
  
  Serial.begin(115200);
  DEBUG_PRINTLN("Servos ready");
  
  Homie.setFirmware("rele med kamerakontroll", "1.0.0");
  relay1Node.subscribe("on",  relay1OnHandler);
  relay2Node.subscribe("on",  relay2OnHandler);
  servo1Node.subscribe("on",  servo1NodeHandler);
  servo2Node.subscribe("on",  servo2NodeHandler);
  Homie.registerNode(relay1Node);
  Homie.registerNode(relay2Node);
  Homie.registerNode(servo1Node);
  Homie.registerNode(servo2Node);

  Homie.setup();
}

void loop() {
  if (timerRelay && (millis() - timerRelay >= OPENER_EVENT_MS)) {
    digitalWrite(PIN_RELAY2, LOW);
    timerRelay = 0;
    relay2OnHandler("false");
  }
  Homie.loop();
}

//function to invoke action on a given servo
void runServo(String inString, Servo servoObj, const int PIN_SERVO, int minA, int maxA) {
    
    DEBUG_PRINT("\n"); DEBUG_PRINT("Input char to servo function:"); DEBUG_PRINTLN(inString);    
    bool checkIfNumber= isValidNumber(inString);
    DEBUG_PRINT("Check if is number: "); DEBUG_PRINTLN(checkIfNumber);
    if (checkIfNumber == true) {
       val = inString.toInt();
       DEBUG_PRINT("Value before normalization: "); DEBUG_PRINTLN(val); 
       val = map(val, 0, 100, minA, maxA); 
       DEBUG_PRINT("Normalized value:"); DEBUG_PRINTLN(val);
        if (servoObj.attached()) {
          servoObj.write(val);
        } else{
          DEBUG_PRINT("Must attach servo before usage, run 'a'\n");
        }
    } else {
       if (inString == "d") {
          servoObj.detach();
          DEBUG_PRINT("Servo detached\n");
        } else if (inString == "a")    
        {
          servoObj.attach(PIN_SERVO);
          DEBUG_PRINT("Servo activated\n");
        }
    } 
  } //function


//function that checks if a string is a number
boolean isValidNumber(String str){
   boolean isNum=false;
   for(byte i=0;i<str.length();i++)
   {
       isNum = isDigit(str.charAt(i)) || str.charAt(i) == '+' || str.charAt(i) == '.' || str.charAt(i) == '-';
       if(!isNum) return false;
   }
   return isNum;
}
