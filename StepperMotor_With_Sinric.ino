/*
 * If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
 */
// Uncomment the following line to enable serial debug output
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
       #define DEBUG_ESP_PORT Serial
       #define NODEBUG_WEBSOCKETS
       #define NDEBUG
#endif 

#include <Arduino.h>
#ifdef ESP8266 
       #include <ESP8266WiFi.h>
#endif 
#ifdef ESP32   
       #include <WiFi.h>
#endif

#include "SinricPro.h"
#include "SinricProFanUS.h"
#include "Adafruit_MotorShield.h"

#define WIFI_SSID         "Purple Cow Internet 16349"    
#define WIFI_PASS         "mentionmerry90"
#define APP_KEY           "58da18d2-0075-47e0-a4c6-4b33809a76ed"      // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET        "f4bb44ab-70ae-412f-b0c5-a8b8737f9516-7c42a88b-2585-4a38-991e-75f705096015"   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define FAN_ID            "633f74e716440f13ff765a09"    // Should look like "5dc1564130xxxxxxxxxxxxxx"

#define BAUD_RATE         9600                // Change baudrate to your need

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2); //(X1,X2) ; X1 = STEPS/REV : X2 = MOTOR HAT PORT #2 (M2 and M4)
// we use a struct to store all states and values for our fan
// fanSpeed (1..3)
struct {
  bool powerState = false;
  int Motorstate = 1;
} device_state;

struct { /* NOTE THIS WILL ONLY WORK PROPERLY IF level.one = 1/3(level.three) & level.two = (2/3)(level.three) */
  uint16_t one = 100; //SET COUSTOM
  uint16_t two = 200; //SET COUSTOM
  uint16_t three = 300; //SET COUSTOM
  uint8_t mode = MICROSTEP; //SETS STEPPER MOTOR MODE e.g.(SINGLE,DOUBLE, INTERLEAVE, and MICROSTEP )
  int old = 0; //USED TO DETERMINE CURRENT HIGHT
  
} level;

bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("Fan turned %s\r\n", state?"on":"off");
  device_state.powerState = state;
  MotorControl();
  return true; // request handled properly
}

// Fan rangeValue is from 1..3
bool onRangeValue(const String &deviceId, int& rangeValue) {
  device_state.Motorstate = rangeValue;
  Serial.printf("Fan speed changed to %d\r\n", device_state.Motorstate);
  MotorControl();
  return true;
}

// Fan rangeValueDelta is from -3..+3
bool onAdjustRangeValue(const String &deviceId, int& rangeValueDelta) {
  device_state.Motorstate += rangeValueDelta;
  Serial.printf("Fan speed changed about %i to %d\r\n", rangeValueDelta, device_state.Motorstate);
  rangeValueDelta = device_state.Motorstate; // return absolute fan speed
  MotorControl();
  return true;
}

void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

void setupSinricPro() {
  SinricProFanUS &myFan = SinricPro[FAN_ID];

  // set callback function to device
  myFan.onPowerState(onPowerState);
  myFan.onRangeValue(onRangeValue);
  myFan.onAdjustRangeValue(onAdjustRangeValue);

  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}
void MotorControl(void){
  /* FROM OFF TO LEVEL */
 if(level.old == 0){
   if((device_state.Motorstate == 1) && (device_state.powerState != false)){ //Set hight too "LOW" 
     myMotor->step(level.one, FORWARD, level.mode);
     level.old = 1;
     return; //paranoia
    }else if((device_state.Motorstate == 2) && (device_state.powerState != false)){ //Set hight too "MID" 
      myMotor->step(level.two, FORWARD, level.mode);
      level.old = 2;
      return; //paranoia
     }else if((device_state.Motorstate == 3) && (device_state.powerState != false)){ // Set height too "FULL"
       myMotor->step(level.three, FORWARD, level.mode);
       level.old = 3;
       return; //paranoia
       }
      }
  /* INCREMENTING LEVEL UP */
 if(device_state.powerState != false){
   if((level.old == 1) && (device_state.Motorstate == 2)){//FROM 1--->2 
       myMotor->step(level.one, FORWARD, level.mode);
        level.old = 2;
        return; //paranoia
   }else if((level.old == 2) && (device_state.Motorstate == 3)){ //FROM 2--->3
        myMotor->step(level.one, FORWARD, level.mode);
        level.old = 3;
        return; //paranoia
    }else if((level.old == 1) && (device_state.Motorstate == 3)){ //FROM 1--->3
         myMotor->step(level.two, FORWARD, level.mode);
         level.old = 3;
         return; //paranoia
     }
    }
  /* INCREMENTING LEVEL DOWN */
 if(device_state.powerState != false){
   if((level.old == 2) && (device_state.Motorstate = 1)){ //FROM 2--->1
    myMotor->step(level.one, BACKWARD, level.mode);
    level.old = 1;
    return; //paranoia
   }else if((level.old == 3) && (device_state.Motorstate = 2)){ //FROM 3--->2
     myMotor->step(level.one, BACKWARD, level.mode);
     level.old = 2;
     return; //paranoia
    }else if((level.old == 3) && (device_state.Motorstate = 2)){ //FROM 3--->1
      myMotor->step(level.two, BACKWARD, level.mode);
      level.old = 1;
      return; //paranoia
      } 
     }
  /* CLOSING THE WINDOW */
  if(level.old !=0){
   if((device_state.Motorstate == 1) && (device_state.powerState == false)){ //FROM 1-->OFF
    myMotor->step(level.one, BACKWARD, level.mode);
    level.old = 0;
    return; //paranoia
   }else if((device_state.Motorstate == 2) && (device_state.powerState == false)){ //FROM 2-->OFF
     myMotor->step(level.two, BACKWARD, level.mode);
     level.old = 0;
     return; //paranoia
    }else if((device_state.Motorstate == 3) && (device_state.powerState == false)){ //FROM 3-->OFF
      myMotor->step(level.three, BACKWARD, level.mode);
      level.old = 0;
      return; //paranoia
      }
     }        
 //Finish Motor Function
}
// main setup function
void setup() {
  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
  setupWiFi();
  setupSinricPro();
   if(!AFMS.begin()){
     Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
 myMotor->setSpeed(50); //Set motor Speed in RMP
}

void loop() {
  SinricPro.handle();
}