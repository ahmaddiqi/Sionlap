#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include "DebugMacros.h"
#include <DHT.h>
#include <time.h>

#define Relay1 D2
#define Relay2 D3
#define Relay3 D4
#define DHTPIN D1                                                           // what digital pin we're connected to
#define DHTTYPE DHT11                                                       // select dht type as DHT 11 or DHT22
DHT dht(DHTPIN, DHTTYPE);
DHT dht(DHTPIN, DHTTYPE);
DHT dht(DHTPIN, DHTTYPE);
DHT dht(DHTPIN, DHTTYPE);
DHT dht(DHTPIN, DHTTYPE);

float h;
float t;
String sheetHumid = "";
String sheetTemp = "";

int timezone = 7;
int dst = 0;

const char* ssid = "Dosen TE UM";                //replace with our wifi ssid
const char* password = "2019-wifi"; 
const char* host = "script.google.com";
const char *GScriptId = "AKfycbw-2-5ypuOcwM0QF369LuVK9wbDrhfyQOGoXwOskIidlH1d7QUQ"; // Replace with your own google script id
const int httpsPort = 443; //the https port is same

// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
const char* fingerprint = "";

//const uint8_t fingerprint[20] = {};

String url = String("/macros/s/") + GScriptId + "/exec?value=Temperature";  // Write Teperature to Google Spreadsheet at cell A1
// Fetch Google Calendar events for 1 week ahead
String url2 = String("/macros/s/") + GScriptId + "/exec?cal";  // Write to Cell A continuosly

//replace with sheet name not with spreadsheet file name taken from google
String payload_base =  "{\"command\": \"appendRow\", \
                    \"sheet_name\": \"Lab_kendali\", \
                       \"values\": ";
String payload = "";

HTTPSRedirect* client = nullptr;

// used to store the values of free stack and heap before the HTTPSRedirect object is instantiated
// so that they can be written to Google sheets upon instantiation

void setup() {
  delay(1000);
  Serial.begin(115200);
  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  pinMode(Relay3, OUTPUT);  
  dht.begin();     //initialise DHT11
  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  configTime(timezone * 3600, dst * 0, "pool.ntp.org", "time.nist.gov");

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  Serial.print("Connecting to ");
  Serial.println(host);          //try to connect with "script.google.com"

  // Try to connect for a maximum of 5 times then exit
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }
// Finish setup() function in 1s since it will fire watchdog timer and will reset the chip.
//So avoid too many requests in setup()

  Serial.println("\nWrite into cell 'A1'");
  Serial.println("------>");
  // fetch spreadsheet data
  client->GET(url, host);
   
  Serial.println("\nGET: Fetch Google Calendar Data:");
  Serial.println("------>");
  // fetch spreadsheet data
  
  client->GET(url2, host); //edit

 Serial.println("\nStart Sending Sensor Data to Google Spreadsheet");

  
  // delete HTTPSRedirect object
  delete client;
  client = nullptr;
}

void loop() {

  h = dht.readHumidity();                                              // Reading temperature or humidity takes about 250 milliseconds!
  t = dht.readTemperature();                                           // Read temperature as Celsius (the default)
  if (isnan(h) || isnan(t)) {                                                // Check if any reads failed and exit early (to try again).
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  Serial.print("Humidity: ");  Serial.print(h);
  sheetHumid = String(h);                                         //convert integer humidity to string humidity
  Serial.print("  Temperature: ");  Serial.print(t);  Serial.println(" ");
  sheetTemp = String(t);



if (t >= 18.00 && t <= 28.00 && h >= 40.00 && h <= 60.00 )
{ 
    digitalWrite(Relay1, HIGH);
    digitalWrite(Relay2, LOW);
    digitalWrite(Relay3, LOW);
    Serial.println("Humidity : Normal, Temperature : Normal");
      }
else if(t >= 18.00 && t <= 28.00 && h > 60.00 )
{
    digitalWrite(Relay1,LOW); // Turns Relay Off
    digitalWrite(Relay2,HIGH);  // Turns ON Relays
    digitalWrite(Relay3,LOW);  // Turns ON Relays
    Serial.println("Humidity : Tinggi, Temperature : Normal");
   }

else if(t >= 18.00 && t <= 28.00 && h < 40.00 )
{
    digitalWrite(Relay1,LOW); // Turns Relay Off
    digitalWrite(Relay2,LOW);  // Turns ON Relays
    digitalWrite(Relay3,HIGH);  // Turns ON Relays
    Serial.println("Humidity : Rendah, Temperature : Normal");
   }
else if (t > 28.00 && h >= 40.00 && h <= 60.00 )
{
    digitalWrite(Relay1,LOW); // Turns Relay Off
    digitalWrite(Relay2,LOW);  // Turns ON Relays
    digitalWrite(Relay3,HIGH);  // Turns ON Relays
    Serial.println("Humidity : Normal, Temperature : Panas");
   }
else if (t < 18.00 && h >= 40.00 && h <= 60.00 )
{
    digitalWrite(Relay1,LOW); // Turns Relay Off
    digitalWrite(Relay2,LOW);  // Turns ON Relays
    digitalWrite(Relay3,HIGH);  // Turns ON Relays
    Serial.println("Humidity : Normal, Temperature : Dingin");
   }
else if (t > 28.00 && h > 60.00 )
{
    digitalWrite(Relay1,LOW); // Turns Relay Off
    digitalWrite(Relay2,LOW);  // Turns ON Relays
    digitalWrite(Relay3,HIGH);  // Turns ON Relays
    Serial.println(" Humidity : Tinggi, Temperature : Panas");
   }


  {
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
  payload = payload_base + "\"" + sheetTemp + "," + sheetHumid + "," + tanggal + "," + jam + "\"}";

  if (!flag) {
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }

  if (client != nullptr) {
    if (!client->connected()) {
      client->connect(host, httpsPort);
      client->POST(url2, host, payload, false);
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
//  if (client->GET(url3, host)) {
//    ++connect_count;
//  }
//  else {
//    ++error_count;
//    DPRINT("Error-count while connecting: ");
//    DPRINTLN(error_count);
//  }

  Serial.println("POST or SEND Sensor data to Google Spreadsheet:");
  if (client->POST(url2, host, payload)) {
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