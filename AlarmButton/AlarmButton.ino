// esp push button on D5, executes a blinker and publishes given state to MQTT
// note that state HIGH is defined as off 
#include <Homie.h>

const int PIN_LED     = D4;
const int PIN_KNAPP   = D5;
const int DEBOUNCE_MILLISEC = 50; 

int   state;
int   lastPress;
int   status1;
bool  currentState;
bool  currentButton = 0;
int   counterPush = 0;
String swstate;
int   changeSt;

//blinker
unsigned long   previousMillis = 0;        
const long      interval = 100;  //ms
int ledState = LOW; 

//debouncer
Bounce debouncer = Bounce();

HomieNode alarmNode("alarm", "bryter");

//handles actions to mqtt for given node
bool alarmOnHandler(String value) {
  if (value == "true") {
    Homie.setNodeProperty(alarmNode, "on", "true"); // oppdater status 
    Serial.println("Alarm er på ");
    //digitalWrite(PIN_LED, LOW);
    //currentButton = changeState(currentButton);
    //previousMillis = 1; //trigger  
    //runBlink(1);
  } else if (value == "false") {
    //digitalWrite(PIN_LED, HIGH);
    Homie.setNodeProperty(alarmNode, "on", "false");
    Serial.println("Alarm er av");
    //currentButton = changeState(currentButton);
    //digitalWrite(PIN_LED, HIGH);
  } else {
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  // define button as pullup
  pinMode(PIN_KNAPP, INPUT_PULLUP);
  
  debouncer.attach(PIN_KNAPP);
  debouncer.interval(DEBOUNCE_MILLISEC);
  
  state=digitalRead(PIN_KNAPP);
  currentState = state;
  
  Homie.setFirmware("alarmV1", "1.0.0");
  alarmNode.subscribe("on",  alarmOnHandler);
  Homie.registerNode(alarmNode);
  Homie.setup();
}

void loop() {
  
  state = debouncer.read(); //digitalRead(PIN_KNAPP);
  if (state == currentState) {
    //ikke gjøre noe foreløpig
    } else {
       if (state == 1) {
          swstate = getState(state);
          counterPush ++;
          Serial.println(counterPush); 
          Serial.println(currentButton); 
          currentButton = changeState(currentButton);
          swstate = getState(currentButton);
          status1 = alarmOnHandler(swstate);
        } else {
         //do nothing for now
        }
       currentState = state;
    } 
       
  runBlink(currentButton);
  
  Homie.loop();
  debouncer.update();
}

//aux functions
String getState(int state) {
   if (state == 1) {
      swstate ="true";
    } else if (state == 0) {
      swstate = "false";
    }
    return swstate;
}

int changeState(int state) {
   if (state == 1) {
    changeSt = 0;
    } else if (state == 0) {
      changeSt = 1;
    }
    return changeSt;
}

void runBlink(int currentButt) {
  if (currentButt == 1) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
      digitalWrite(PIN_LED, ledState);
      }
    } else if (currentButt == 0) {
      digitalWrite(PIN_LED, HIGH);
    }
}
