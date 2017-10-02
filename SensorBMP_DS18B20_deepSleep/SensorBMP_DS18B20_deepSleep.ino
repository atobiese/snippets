//-------------------------------------
//-------------------------------------

// updated for Homie 2.0

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
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BMP085.h>

// I2C:
// D1 <-> SCL (wemos D1)
// D2 <-> SDA

//voltage measurement
//ADC_MODE(ADC_VCC); //this only measures inner circuit voltage


#define       FW_NAME       "node-BMP-deepsleep"
#define       FW_VERSION    "0.3"
const int     PUB_INTERVAL    = 10;  //seconds (when not running deep sleep mode)

//flags for deep sleep modus
bool deepSleepModus = true; // or false: set to true if deep sleep option is to be used
                            //remember to connect RST to D0 for this to function
                            // set false to run in contineous mode with no deep sleep
                            
const int SLEEP_INTERVAL = 1; //minutes, turn off for given minutes, start sensor, transmit data, go to sleep again
//Homie reset pin
const int RST_PIN    = D6; //add a pushbutton here or wire D6 to GND for 2 seconds

#define ONE_WIRE_BUS D3  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorDeviceAddress;

// How many bits to use for temperature values: 9, 10, 11 or 12
#define SENSOR_RESOLUTION 12
// Index of sensors connected to data pin, default: 0
#define SENSOR_INDEX 0


bool sleepFlag = false; //always set to false
unsigned long lastPublish = 0;

int counter = 0;

//count offline time before closing down in deepsleep
unsigned long attemptLogonTime = 0;

Adafruit_BMP085 bmp;

HomieNode temperatureNode("temperature", "temperature");
HomieNode humidityNode("humidity", "humidity");

void setupHandler() {
  temperatureNode.setProperty("unit").setRetained(true).send("c");
  humidityNode.setProperty("unit").setRetained(true).send("%");
}

void loopHandler() {

  if (!deepSleepModus) {
        if (millis() - lastPublish >= PUB_INTERVAL * 1000UL) {
          
          DEBUG_PRINTLN("attempting to get properties") ;
          sensors.requestTemperatures(); 
          
          float t = sensors.getTempCByIndex(0);
          float h = -1.0; //bmp.readPressure()/100; //mPa
          float b = battery_level(); //ESP.getVcc()/1000.0;

         
          if (!isnan(t) && temperatureNode.setProperty("degrees").send(String(t))) {
             DEBUG_PRINT("set new value to MQTT: "); DEBUG_PRINTDEC(t);  DEBUG_PRINTLN("");
            lastPublish = millis();
          }
          if (!isnan(h) && humidityNode.setProperty("relative").send(String(h))) {
             DEBUG_PRINT("set new value to MQTT: "); DEBUG_PRINTDEC(h);  DEBUG_PRINTLN("");
             DEBUG_PRINT("set new battterly level to MQTT: "); DEBUG_PRINTDEC(b); DEBUG_PRINTLN("");
            lastPublish = millis();
          }    
        }
    }else{
        //running in deep sleep modus
        
        sensors.requestTemperatures(); 
        float t = sensors.getTempCByIndex(0);
        float h = 0.0; //bmp.readPressure()/100; //mPa
        float b = battery_level(); //ESP.getVcc()/1000.0;
        
        DEBUG_PRINTLN("attempting to get properties") ;
         if (!isnan(t) && temperatureNode.setProperty("degrees").send(String(t))) {
           DEBUG_PRINT("set new value to MQTT: "); DEBUG_PRINTDEC(t);  DEBUG_PRINTLN("");
           //reset counter
           counter = 0;
        }
        if (!isnan(h) && humidityNode.setProperty("relative").send(String(h)) && humidityNode.setProperty("battery").send(String(h))) {
          DEBUG_PRINT("set new value to MQTT: "); DEBUG_PRINTDEC(h);  DEBUG_PRINTLN("");
          DEBUG_PRINT("set new battterly level to MQTT: "); DEBUG_PRINTDEC(b); DEBUG_PRINTLN("");
          //reset counter
          counter = 0;
        }
        
        counter = counter + 1;
        
        if (!isnan(t) && !isnan(h))  {
          DEBUG_PRINTLN("-Disconnecting MQTT "); 
          //Homie.disconnectMQTT();
          Homie.getLogger() << "MQTT connected, received values, preparing for deep sleep..." << endl;
          Homie.prepareToSleep();
          //delay(10);
        }
        if (counter>100)  {
          DEBUG_PRINTLN("-Sensor attempted 100 times. No measurement obtained "); 
          DEBUG_PRINTLN("-Disconnecting MQTT ");
          temperatureNode.setProperty("degrees").send("-1"); 
          humidityNode.setProperty("relative").send("-1"); 
          //Homie.disconnectMQTT();
          //delay(100);
          Homie.getLogger() << "MQTT connected, received values, preparing for deep sleep..." << endl;
          Homie.prepareToSleep();
        }
     }
}

void setup() {
  
  #ifdef DEBUG 
    Serial.begin(115200);
  #endif

  bmp.begin();
  sensors.begin();
  sensors.getAddress(sensorDeviceAddress, 0);
  sensors.setResolution(sensorDeviceAddress, SENSOR_RESOLUTION);
    
  Homie_setFirmware(FW_NAME, FW_VERSION);

  //Homie.registerNode(temperatureNode);
  //Homie.registerNode(humidityNode);
  
  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);
  
  //no leds blinking
  Homie.disableLedFeedback();  
  
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
//  if (millis() - attemptLogonTime >= 10 * 1000UL && !(HomieEventType::CONFIGURATION_MODE)) {
//     DEBUG_PRINTLN("10 seconds has passed and no wifi conn, shutting down to save energy..");
//     sleepFlag = true;
//  }

//  Serial.print(Homie.type);
  
  Homie.loop();
}


// deep sleep event is activated when mqtt is shut off.
void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_DISCONNECTED:
        sleepFlag = true;
        DEBUG_PRINTLN("mqtt disconnected") ;
        break;
    case HomieEventType::NORMAL_MODE:
      attemptLogonTime = millis();
      
      break;
    case HomieEventType::WIFI_CONNECTED:
        DEBUG_PRINTLN("wifi connected");
        attemptLogonTime = 0.0;
      break;  
    case HomieEventType::CONFIGURATION_MODE:
        DEBUG_PRINTLN("this is: CONFIGURATION_MODE");
        //DEBUG_PRINTLN(HomieEventType);
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

