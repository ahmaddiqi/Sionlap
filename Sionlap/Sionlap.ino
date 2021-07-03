#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#define HOSTIDENTIFY  "esp8266"
#define mDNSUpdate(c)  do { c.update(); } while(0)
using WebServerClass = ESP8266WebServer;
using HTTPUpdateServerClass = ESP8266HTTPUpdateServer;
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "HTTPUpdateServer.h"
#define HOSTIDENTIFY  "esp32"
#define mDNSUpdate(c)  do {} while(0)
using WebServerClass = WebServer;
using HTTPUpdateServerClass = HTTPUpdateServer;
#endif
#include <WiFiClient.h>
#include <AutoConnect.h>

#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include "DebugMacros.h"
#include <DHT.h>
#include <time.h>

// This page for an example only, you can prepare the other for your application.
static const char AUX_AppPage[] PROGMEM = R"(
{
  "title": "SIONLAP 2021",
  "uri": "/home",
  "menu": true,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h2>SIONLAP 2021</h2>",
      "style": "text-align:center;color:#2f4f4f;padding:10px;"
    },
    {
      "name": "content",
      "type": "ACText",
      "value": "silahkan tekan tombol menu di atas kanan (untuk tampilan mobile) lalu klik "configure new AP" untuk sambungkan dengan AP baru "
    }
  ]
}
)";

// Fix hostname for mDNS. It is a requirement for the lightweight update feature.
static const char* host = HOSTIDENTIFY "-webupdate";
#define HTTP_PORT 80

// ESP8266WebServer instance will be shared both AutoConnect and UpdateServer.
WebServerClass  httpServer(HTTP_PORT);

#define USERNAME "user"   //*< Replace the actual username you want */
#define PASSWORD "pass"   //*< Replace the actual password you want */
// Declare AutoConnectAux to bind the HTTPWebUpdateServer via /update url
// and call it from the menu.
// The custom web page is an empty page that does not contain AutoConnectElements.
// Its content will be emitted by ESP8266HTTPUpdateServer.
HTTPUpdateServerClass httpUpdater;
AutoConnectAux  update("/update", "Update");

// Declare AutoConnect and the custom web pages for an application sketch.
AutoConnect     portal(httpServer);
AutoConnectAux  hello;
AutoConnectConfig config;


#define Relay1 D2
// #define Relay2 D3
// #define Relay3 D4
// #define DHTPIN D1                                                           // what digital pin we're connected to
#define DHTTYPE DHT11                                                       // select dht type as DHT 11 or DHT22
DHT dht1(D3, DHTTYPE);
DHT dht2(D4, DHTTYPE);
DHT dht3(D6, DHTTYPE);
DHT dht4(D7, DHTTYPE);
DHT dht5(D8, DHTTYPE);

String sheetHumid = "";
String sheetTemp = "";

int timezone = 7;
int dst = 0;


const char* hostscript = "script.google.com";
const char *GScriptId = "AKfycbzTGqBfsmbVzfizawAnwNR0wo0OxlDOk2PIutWl-kR5BxO4UbM"; // Replace with your own google script id
const int httpsPort = 443; //the https port is same

// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
const char* fingerprint = "";

//const uint8_t fingerprint[20] = {};

String url = String("/macros/s/") + GScriptId + "/exec?value=Temperature";  // Write Teperature to Google Spreadsheet at cell A1
// Fetch Google Calendar events for 1 week ahead
String url2 = String("/macros/s/") + GScriptId + "/exec?cal";  // Write to Cell A continuosly

//replace with sheet name not with spreadsheet file name taken from google
String payload_base =  "{\"command\": \"appendRow\", \
                    \"sheet_name\": \"ELKA\", \
                       \"values\": ";
String payload = "";

HTTPSRedirect* client = nullptr;

// used to store the values of free stack and heap before the HTTPSRedirect object is instantiated
// so that they can be written to Google sheets upon instantiation

void setup() {
  
  delay(1000);
  Serial.begin(115200);
  pinMode(Relay1, OUTPUT);
  // pinMode(Relay2, OUTPUT);
  // pinMode(Relay3, OUTPUT);
  httpUpdater.setup(&httpServer, USERNAME, PASSWORD);
  config.autoReconnect = true;    // Attempt automatic reconnection.
  config.reconnectInterval = 6;   // Seek interval time is 180[s].
  config.apid = "SIONLAP 2021";
  config.psk  = "12345678";
  portal.config(config);
  hello.load(AUX_AppPage);
  portal.join({ hello, update });
    if (portal.begin()) {
    if (MDNS.begin(host)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
    }
    else
      Serial.println("Error setting up MDNS responder");
  }
  // initialise DHT11  
  dht1.begin(); 
  dht2.begin(); 
  dht3.begin();     
  dht4.begin(); 
  dht5.begin(); 

  Serial.println();
  Serial.print("Connecting to wifi: ");
  // Serial.println(ssid);
  
  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  configTime(timezone * 3600, dst * 0, "id.pool.ntp.org", "pool.ntp.org");

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  Serial.print("Connecting to ");
  Serial.println(hostscript);          //try to connect with "script.google.com"

  // Try to connect for a maximum of 5 times then exit
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(hostscript, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(hostscript);
    Serial.println("Exiting...");
    return;
  }
// Finish setup() function in 1s since it will fire watchdog timer and will reset the chip.
//So avoid too many requests in setup()

  Serial.println("\nWrite into cell 'A1'");
  Serial.println("------>");
  // fetch spreadsheet data
//  client->GET(url, hostscript);
   
  Serial.println("\nGET: Fetch Google Calendar Data:");
  Serial.println("------>");
  // fetch spreadsheet data
  
  client->GET(url2, hostscript); //edit

 Serial.println("\nStart Sending Sensor Data to Google Spreadsheet");

  
  // delete HTTPSRedirect object
  delete client;
  client = nullptr;
}

void loop() {
  mDNSUpdate(MDNS);
  portal.handleClient();
  float h[5], t[5];
  String sh[5], st[5];
  h[0] = dht1.readHumidity();
  t[0] = dht1.readTemperature();
  h[1] = dht2.readHumidity();
  t[1] = dht2.readTemperature();
  h[2] = dht3.readHumidity();
  t[2] = dht3.readTemperature();
  h[3] = dht4.readHumidity();
  t[3] = dht4.readTemperature();
  h[4] = dht5.readHumidity();
  t[4] = dht5.readTemperature();
  float hAvg = 0;
  float tAvg = 0; 
  for(int i = 0; i<5; i++){
  Serial.print(i);
  Serial.print("     Humidity: ");  Serial.print(h[i]);
  Serial.print("  Temperature: ");  Serial.print(t[i]);  Serial.println(" ");
  hAvg += h[i];
  tAvg += t[i];
  }
  hAvg/=5;
  tAvg/=5;
  bool relayStatus;
  



if (tAvg >= 18.00 && tAvg <= 28.00 && hAvg >= 40.00 && hAvg <= 60.00 )
{ 
  relayStatus = false;
   digitalWrite(Relay1, LOW);
   Serial.println("Humidity : Normal, Temperature : Normal");
     }
else if(tAvg >= 18.00 && tAvg <= 28.00 && hAvg > 60.00 )
{
  relayStatus = true;
   digitalWrite(Relay1,HIGH); 
   Serial.println("Humidity : Tinggi, Temperature : Normal");
  }

else if(tAvg >= 18.00 && tAvg <= 28.00 && hAvg < 40.00 )
{
  relayStatus = true;
   digitalWrite(Relay1,LOW); 
   Serial.println("Humidity : Rendah, Temperature : Normal");
  }
else if (tAvg > 28.00 && hAvg >= 40.00 && hAvg <= 60.00 )
{
  relayStatus = true;
   digitalWrite(Relay1,HIGH);
   Serial.println("Humidity : Normal, Temperature : Panas");
  }
else if (tAvg < 18.00 && hAvg >= 40.00 && hAvg <= 60.00 )
{
  relayStatus = true;
  digitalWrite(Relay1,LOW);
   Serial.println("Humidity : Normal, Temperature : Dingin");
  }
else if (tAvg > 28.00 && hAvg > 60.00 )
{
  relayStatus = true;
  digitalWrite(Relay1,HIGH);
   Serial.println(" Humidity : Tinggi, Temperature : Panas");
}


  static int error_count = 0;
  static int connect_count = 0;
  const unsigned int MAX_CONNECT = 20;
  static bool flag = false;
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now);
  String years = String(timeinfo->tm_year + 1900);
  String tanggal = String(years + "-" + (checktime(timeinfo->tm_mon+ 1)) + "-" + (checktime(timeinfo->tm_mday))); 
  String jam = String(checktime(timeinfo->tm_hour) + ":" + (checktime(timeinfo->tm_min))+ ":" + (checktime(timeinfo->tm_sec)));
  payload = payload_base + "\""+ tanggal + " " + jam +","+ String(t[0]) + "," + String(t[1]) + "," + String(t[2]) + "," + String(t[3]) + "," +String(t[4]) + "," + String(h[0]) + "," + String(h[1]) + "," + String(h[2])+ "," + String(h[3]) + "," + String(h[4]) + ","  + String(relayStatus) + "\"}";
  Serial.print("");
  Serial.println(payload);
  Serial.println("");
  if (!flag) {
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }

  if (client != nullptr) {
    if (!client->connected()) {
      client->connect(hostscript, httpsPort);
      client->POST(url2, hostscript, payload, false);
      Serial.print("Sent : ");  Serial.println("Temp and Humid");
    }
  }
  else {
    DPRINTLN("Error creating client object!");
    error_count = 5;
  }

  if (connect_count > MAX_CONNECT) {
    connect_count = 0;
    flag = false;
    delete client;
    return;
  }

//  Serial.println("GET Data from cell 'A1':");
//  if (client->GET(url3, hostscript)) {
//    ++connect_count;
//  }
//  else {
//    ++error_count;
//    DPRINT("Error-count while connecting: ");
//    DPRINTLN(error_count);
//  }

  Serial.println("POST or SEND Sensor data to Google Spreadsheet:");
  if (client->POST(url2, hostscript, payload)) {
    ;
  }
  else {
    ++error_count;
    DPRINT("Error-count while connecting: ");
    DPRINTLN(error_count);
  }

  if (error_count > 3) {
    Serial.println("Halting processor...");
    delete client;
    client = nullptr;
    Serial.printf("Final free heap: %u\n", ESP.getFreeHeap());
    Serial.printf("Final stack: %u\n", ESP.getFreeContStack());
    Serial.flush();
    ESP.deepSleep(0);
  }
  delay(60000);    // keep delay of minimum 1 menit as dht allow reading after 2 seconds interval and also for google sheet
}


String checktime (int check){
  String aftercheck;
  if(check < 10){
  aftercheck = String(String(0) + check);
  return aftercheck;
  }
else { //if (check >= 10)
  aftercheck = String(check);
  return aftercheck;
}
}
