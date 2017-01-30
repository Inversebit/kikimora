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

const char* ssid = "SSID";
const char* password = "p4s5w0rd";

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
const char PROGMEM webpageRoot[] = "<!DOCTYPE html><html><head><link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\" integrity=\"sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u\" crossorigin=\"anonymous\"><head><body><div class=\"container\"><div class=\"page-header\"><h1>Kikimora <small>ESP8266-based Systems Monitoring Service</small></h1></div><div class=\"alert alert-info\" role=\"alert\"><p>Please enter URL to monitor. In case it's an HTTPS webpage do also indicate the page certificate's SHA1 fingerprint below.</p></div><div class=\"panel panel-default\"><div class=\"panel-heading\">Monitoring target information:</div><div class=\"panel-body\"><form action=\"/submit\" method=\"POST\"><div class=\"form-group\"><label for=\"monitoring-url\">URL to monitor:</label><input type=\"text\" class=\"form-control\" id=\"monitoring-url\" placeholder=\"http://example.com\" name=\"url\" value=\"@@current_url\" autofocus required></div><div class=\"form-group\"><label for=\"fingerprint\">Cert SHA1 fingerprint:</label><input type=\"text\" class=\"form-control\" id=\"fingerprint\" placeholder=\"0A 1B 2C 3D 4E 5F ...\" name=\"fingerprint\" value=\"@@current_fingerprint\"></div><button type=\"submit\" class=\"btn btn-default\">Submit</button></form></div></div></div></body></html>";
const char PROGMEM webpageFormRes[] = "<!DOCTYPE html><html><head><link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\" integrity=\"sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u\" crossorigin=\"anonymous\"><head><body><div class=\"container\"><div class=\"page-header\"><h1>Kikimora <small>ESP8266-based Systems Monitoring Service</small></h1></div><div class=\"alert alert-@@res-status\" role=\"alert\"><p>@@res-message</p></div><a href=\"/\" class=\"btn btn-default btn-primary\" role=\"button\">Return to the main page</a></div></body></html>";

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

  String stat = "danger";
  String res = "Target <b>NOT</b> saved. An error has occurred.";
  if(formOk)
  {
    stat = "success";
    res = "New monitoring target <b>SUCCESSFULLY</b> saved.";
  }
  
  String msg = String(webpageFormRes);
  msg.replace("@@res-status", stat);
  msg.replace("@@res-message", res);
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
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  IPAddress ip = WiFi.localIP();
  int lastOctet = ip[3];
  
  blinkLED(rled, 1, 2000);

  do{          
      int blinky = lastOctet % 10;
      lastOctet /= 10;
          
      blinkLED(gled, blinky, 400);
      blinkLED(rled, 1, 500);
  }while(lastOctet);  
}

//*THE* SETUP
void setup() {
  setupPins();
  setupWiFiConn();
  setupServer();
  user_init();
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
    return false;
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
    blinkLED(litPin(), 2, 75);
    bool isUp = pingWeb();
    showStatus(isUp);
    shouldPing = false;
  }
   
  yield();
}
