/**
 * Copyright (c) 2016 Inversebit
 * 
 * This code is freed under the MIT License.
 * Full license text: https://opensource.org/licenses/MIT
 * 
 * ### Kikimora ###
 * It'll watch over you systems, and warn you in
 * timely fashion.
 */

 /* IMPORTS */
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

extern "C" {
  #include "user_interface.h"
}

/* CONSTANTS DEFINITIONS */

const int gled = 2;
const int rled = 0;

/* GLOBAL VARS */
//Timer
os_timer_t myTimer;

//WiFi thingies
ESP8266WiFiMulti WiFiMulti;

//HTPP thingies
HTTPClient http;
String dest_page = "http://192.168.0.1/";
//String dest_page = "https://google.com";
//String cert_fingerprint = "10 62 AA 33 06 6F D4 B2 00 F2 C9 B4 C7 B0 ED 39 46 03 0D EB";

//Control vars
bool shouldPing = false;


/* SETUP FUNCTIONS */

//TIMER FUNCTIONS
void timerCallback(void *pArg) {
  shouldPing = true;
}

void user_init(void) {
  os_timer_setfn(&myTimer, timerCallback, NULL);
  os_timer_arm(&myTimer, 15000, true);
}

//OTHER SETUP
void setupPins()
{
  pinMode(gled, OUTPUT);
  pinMode(rled, OUTPUT);
}

void setupWiFiConn()
{
  WiFiMulti.addAP("SSID", "PASSWORD");
}

//*THE* SETUP
void setup() {
  setupPins();
  user_init();
  setupWiFiConn();
}

/* LED FUNCTIONS */
int litPin()
{
  if(digitalRead(gled) == HIGH)
  {
    return gled;
  }
  else
  {
    if(digitalRead(rled) == HIGH)
    {
      return rled;
    }
  }

  return -1;
}

void blinkLEDOff(int ledPin, int times, int blinkDuration)
{
  for(int i = 0; i < times; i++)
  {
    digitalWrite(ledPin, !digitalRead(ledPin));
    delay(blinkDuration);
    digitalWrite(ledPin, !digitalRead(ledPin));
    delay(blinkDuration);
  }
}

void showStatus(bool isUp)
{
  digitalWrite(gled, LOW);
  digitalWrite(rled, LOW);
  
  if(isUp)
  {
    digitalWrite(gled, HIGH);
  }
  else
  {
    digitalWrite(rled, HIGH);
  }
}

/* HTPP FUNCTIONS */
bool pingWeb()
{
  bool res = false;
  
  //http.begin(dest_page, cert_fingerprint);
  http.begin(dest_page);
  
  int httpCode = http.GET();
  if(httpCode > 0) {
    if(httpCode == HTTP_CODE_OK) {
      res = true;
    }
  }
  
  http.end();

  return res;
}

/* (THE REST OF THE) FUNCTIONS */
void loop() 
{
  if((WiFiMulti.run() == WL_CONNECTED)) 
  {
    if (shouldPing == true)
    {
      blinkLEDOff(litPin(), 2, 250);
      bool isUp = pingWeb();
      showStatus(isUp);
      shouldPing = false;
    }
  }
   
  yield();  // or delay(0);
}
