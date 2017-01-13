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
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

extern "C" {
  #include "user_interface.h"
}

/* CONSTANTS DEFINITIONS */

const int gled = 2;
const int rled = 0;

/* GLOBAL VARS */
//Timer
os_timer_t myTimer;

//Server vars
ESP8266WebServer server(80);

//HTPPClient vars
HTTPClient httpClient;
String dest_page = "";
String cert_fingerprint = "";

//Control vars
bool shouldPing = false;

//Simple page
const char PROGMEM webpageRoot[] = "\
<!DOCTYPE html>\
<html>\
  <body>\
    <h1>Kikimora</h1>\
    <h2>ESP8266-based Systems Monitoring Service</h2>\
    <p>Please enter URL to monitor. In case it's an HTTPS webpage do also indicate the page certificate's SHA1 fingerprint below.</p>\
    <form action=\"/submit\" method=\"POST\">\
      <fieldset>\
        <legend>Monitoring target information:</legend>\
        URL to monitor:<br>\
        <input type=\"text\" name=\"url\" value=\"@@current_url\" autofocus required/><br><br>\
        Cert SHA1 fingerprint:<br>\
        <input type=\"text\" name=\"fingerprint\" value=\"@@current_fingerprint\"/><br><br>\
        <input type=\"submit\" value=\"Submit\">\
      </fieldset>\
    </form>\
  </body>\
</html>";

const char PROGMEM webpageForm[] = "\
<!DOCTYPE html>\
<html>\
  <body>\
    <h1>Kikimora</h1>\
    <h2>ESP8266-based Systems Monitoring Service</h2>\
    <p>@@message</p>\
  </body>\
</html>";

/* SETUP FUNCTIONS */
//SERVER FUNCTIONS
void handleAccess() 
{
  String msg = String(webpageRoot);
  msg.replace("@@current_url", dest_page);
  msg.replace("@@current_fingerprint", cert_fingerprint);
  server.send(200, "text/html", msg);
}

void handleForm()
{
  bool formOk = false;
  String temp_dest = "";
  String temp_fingerp = "";
  
  if (server.args() > 0 ) {
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "url") {
        temp_dest = server.arg(i);
      }
      else if(server.argName(i) == "fingerprint")
      {
        temp_fingerp = server.arg(i);
      }
    }
  }

  if(temp_dest.length() > 0)
  {
    dest_page = temp_dest;
    cert_fingerprint = temp_fingerp;
    formOk = true;
  }

  String res = "Info <b>NOT</b> saved. An error has occurred.";
  if(formOk)
  {
    res = "Info <b>SUCCESSFULLY</b> saved. <a href=\"/\">Return to index</a>.";
  }
  
  String msg = String(webpageForm);
  msg.replace("@@message", res);
  server.send(200, "text/html", msg);
}

void handleNotFound()
{
  server.send(404, "text/plain", "");
}

void setupServer()
{
  server.on("/", handleAccess);
  server.on("/submit", handleForm);
  server.onNotFound(handleNotFound);
  server.begin();
}

//TIMER FUNCTIONS
void timerCallback(void *pArg) {
  shouldPing = true;
}

void user_init(void) {
  os_timer_setfn(&myTimer, timerCallback, NULL);
  os_timer_arm(&myTimer, 5000, true);
}

//OTHER SETUP
void setupPins()
{
  pinMode(gled, OUTPUT);
  pinMode(rled, OUTPUT);
}

void setupWiFiConn()
{
  WiFi.begin("SSID", "PASSWD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

//*THE* SETUP
void setup() {
  setupPins();
  user_init();
  setupWiFiConn();
  setupServer();
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

void blinkLED(int ledPin, int times, int blinkDuration)
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
  
  if(dest_page.length() == 0)
  {
    blinkLED(rled, 5, 100);
  }
  
  if(cert_fingerprint.length() == 0)
  {
    //HTTP Conn
    httpClient.begin(dest_page);  
  }
  else
  {
    //HTTPS Conn
    httpClient.begin(dest_page, cert_fingerprint);
    httpClient.begin(dest_page);
  }
  
  int httpCode = httpClient.GET();
  if(httpCode > 0) {
    if(httpCode == HTTP_CODE_OK) {
      res = true;
    }
  }

  httpClient.end();

  return res;
}

/* (THE REST OF THE) FUNCTIONS */
void loop() 
{
  server.handleClient();
  
  if (shouldPing == true)
  {
    blinkLED(litPin(), 2, 250);
    bool isUp = pingWeb();
    showStatus(isUp);
    shouldPing = false;
  }
   
  yield();  // or delay(0);
}
