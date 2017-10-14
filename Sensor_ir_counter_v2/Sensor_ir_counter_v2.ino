//-------------------------------------
// 
//-------------------------------------

// updated for Homie 2.0

//-----------Debug script----------
//#define DEBUG  //select if debug or not

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

#define       FW_NAME       "node-ir-counter"
#define       FW_VERSION    "0.3"

const int PUB_INTERVAL    = 60;  //seconds (each publish to mqtt)
const int PIN_INTERRUPT   = D7; 
const int DEBOUNCE_MILLISEC = 150; 
float IMPS_PER_KWT = 1000.0;
int nr_blinks=0, total_historic_counts=0, nr_interrupt=0;
float timedelta, kW_leilighet, amps_leilighet, kWt_leilighet;
String string_pub;

//timers
unsigned long current_millis = 0, previous_millis=0, last_publish = 0;

HomieNode blinkNode("ircount", "ircount");

void setupHandler() {
 blinkNode.setProperty("unit").setRetained(true).send("kWh");
}

void loopHandler() {

    if(nr_interrupt>0)
    {
      nr_interrupt--;
      nr_blinks++;

      DEBUG_PRINT("nr_blinks: ");
      DEBUG_PRINTLN(nr_blinks);
    }

    //update to mqtt                  
    current_millis = millis();
    if (millis() - last_publish >= PUB_INTERVAL * 1000UL) {
        publish_data(current_millis, previous_millis, last_publish, total_historic_counts, nr_blinks);
     }

} //end void loophander

void setup() {
  Serial.begin(115200);
  pinMode(PIN_INTERRUPT,INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), handleInterrupt, FALLING);

  Homie_setBrand("PowerMeter"); 
  Homie_setFirmware(FW_NAME, FW_VERSION);
  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);
  Homie.disableLedFeedback(); 
  
  blinkNode.advertise("unit");
  blinkNode.advertise("nr_blinks");
  blinkNode.advertise("nr_tens");
  blinkNode.advertise("nr_kW");
  blinkNode.advertise("nr_kWt");
  blinkNode.advertise("nr_amps");
  blinkNode.advertise("nr_time_imp");
  
  Homie.setup();
}

void loop() {
  Homie.loop();
}

// helpers 
void publish_data(unsigned long & current_millis, unsigned long & previous_millis, unsigned long & last_publish, int & total_historic_counts, int & nr_blinks) {
    // note make input pointers, which returns the values  
    total_historic_counts = total_historic_counts + nr_blinks;
    timedelta = (current_millis-previous_millis)/ 1000.0; //converting to float (is this expensive cpu'wise?)
    //1000 blinks pr kWh, so for nr_blinks 
    kW_leilighet = (nr_blinks/IMPS_PER_KWT) * 3600.0 / (timedelta); //kW
    amps_leilighet = (kW_leilighet*1000.0) / 230; //W = A*V (ampere)
    kWt_leilighet = total_historic_counts / IMPS_PER_KWT
    DEBUG_PRINT("nr_blinks: ");
    DEBUG_PRINTLN(nr_blinks);
    DEBUG_PRINT("sending a packet at time: ");
    DEBUG_PRINTLN(timedelta);
    DEBUG_PRINT("power-usage: ");
    DEBUG_PRINTLN(kW_leilighet);
    DEBUG_PRINT("current usage: ");
    DEBUG_PRINTLN(amps_leilighet);

    //publish to MQTT
    if (!isnan(nr_blinks) && blinkNode.setProperty("nr_blinks").send(String(nr_blinks))) {
        DEBUG_PRINT("MQTT: nr_blinks "); DEBUG_PRINT(nr_blinks);  DEBUG_PRINTLN("");
    }
    if (!isnan(total_historic_counts) && blinkNode.setProperty("nr_tens").send(String(total_historic_counts))) {
        DEBUG_PRINT("MQTT: total_historic_counts "); DEBUG_PRINT(total_historic_counts);  DEBUG_PRINTLN("");
    }
    if (!isnan(kW_leilighet) && blinkNode.setProperty("nr_kW").send(String(kW_leilighet))) {
        DEBUG_PRINT("MQTT: kW_leilighet "); DEBUG_PRINTDEC(kW_leilighet);  DEBUG_PRINTLN("");
    }
    if (!isnan(kWt_leilighet) && blinkNode.setProperty("nr_kWt").send(String(kWt_leilighet))) {
        DEBUG_PRINT("MQTT: kWt_totalt"); DEBUG_PRINTDEC(kWt_leilighet);  DEBUG_PRINTLN("");
    }
    if (!isnan(amps_leilighet) && blinkNode.setProperty("nr_amps").send(String(amps_leilighet))) {
        DEBUG_PRINT("MQTT: amps_leilighet "); DEBUG_PRINTDEC(amps_leilighet);  DEBUG_PRINTLN("");;
    }
    if (!isnan(timedelta) && blinkNode.setProperty("nr_time_imp").send(String(timedelta))) {
        DEBUG_PRINT("MQTT: timedelta "); DEBUG_PRINTDEC(timedelta);  DEBUG_PRINTLN("");
    } 
    
    nr_blinks = 0;
    previous_millis = current_millis;
    last_publish = millis();
}


void handleInterrupt()
{
   static unsigned long last_interrupt_time = 0;
   unsigned long interrupt_time = millis();
   // debounce if signal is faster than speficied time in ms
   if (interrupt_time - last_interrupt_time > DEBOUNCE_MILLISEC)
   {
     nr_interrupt++;
   }
   last_interrupt_time = interrupt_time;
} 




