#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"
#include <ESP.h>

// Referenced from http://randomnerdtutorials.com/esp32-dht11-dht22-temperature-humidity-web-server-arduino-ide/

//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321
#define uS_TO_S_FACTOR 1000000

unsigned long now;
int DEEPSLEEP_SECONDS = 1800;

WiFiServer server(80);

HTTPClient http;

uint64_t chipid;

long timeout;

const int dhtpin = 22;  //GI Verified
const int soilpin = 32;
const int POWER_PIN = 34; // Possibly pin 16 is connected to power LED
const int LIGHT_PIN = 33;

// Initialize DHT sensor.
DHT dht(dhtpin, DHTTYPE);

// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

// Client variables 
char linebuf[80];
int charcount=0;

const char* ssid = "iotech-2.4GHz";
const char* password = "Athena&George";

char deviceid[21];

void setup() {
  dht.begin();
  
  Serial.begin(115200);
  while(!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  esp_sleep_enable_timer_wakeup(1800 * uS_TO_S_FACTOR);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

  pinMode(16, OUTPUT); 
  pinMode(POWER_PIN, INPUT);
  digitalWrite(16, LOW);  

  timeout = 0;

  chipid = ESP.getEfuseMac();
  sprintf(deviceid, "%" PRIu64, chipid);
  Serial.print("DeviceId: ");
  Serial.println(deviceid);

   WiFi.begin(ssid, password);
  
  // attempt to connect to Wifi network:
  while(WiFi.status() != WL_CONNECTED) {
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.begin();
}

void loop() {

  // GI Added
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client");
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        //read char by char HTTP request
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
            // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
            float h = dht.readHumidity();
            // Read temperature as Celsius (the default)
            float t = dht.readTemperature();
            // Read temperature as Fahrenheit (isFahrenheit = true)
            float f = dht.readTemperature(true);
            // Check if any reads failed and exit early (to try again).
            if (isnan(h) || isnan(t) || isnan(f)) {
              Serial.println("Failed to read from DHT sensor!");
              strcpy(celsiusTemp,"Failed");
              strcpy(fahrenheitTemp, "Failed");
              strcpy(humidityTemp, "Failed");         
            }
            else{
              // Computes temperature values in Celsius + Fahrenheit and Humidity
              float hic = dht.computeHeatIndex(t, h, false);       
              dtostrf(hic, 6, 2, celsiusTemp);             
              float hif = dht.computeHeatIndex(f, h);
              dtostrf(hif, 6, 2, fahrenheitTemp);         
              dtostrf(h, 6, 2, humidityTemp);
              // You can delete the following Serial.print's, it's just for debugging purposes
              /*Serial.print("Humidity: ");
              Serial.print(h);
              Serial.print(" %\t Temperature: ");
              Serial.print(t);
              Serial.print(" *C ");
              Serial.print(f);
              Serial.print(" *F\t Heat index: ");
              Serial.print(hic);
              Serial.print(" *C ");
              Serial.print(hif);
              Serial.print(" *F");
              Serial.print("Humidity: ");
              Serial.print(h);
              Serial.print(" %\t Temperature: ");
              Serial.print(t);
              Serial.print(" *C ");
              Serial.print(f);
              Serial.print(" *F\t Heat index: ");
              Serial.print(hic);
              Serial.print(" *C ");
              Serial.print(hif);
              Serial.println(" *F");*/
          }
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<meta http-equiv=\"refresh\" content=\"30\"></head>");
          client.println("<body><div style=\"font-size: 3.5rem;\"><p>ESP32 - DHT</p><p>");
          if(atoi(celsiusTemp)>=25){
            client.println("<div style=\"color: #930000;\">");
          }
          else if(atoi(celsiusTemp)<25 && atoi(celsiusTemp)>=5){
            client.println("<div style=\"color: #006601;\">");
          }
          else if(atoi(celsiusTemp)<5){
            client.println("<div style=\"color: #009191;\">");
          }
          client.println(celsiusTemp);
          client.println("*C</p><p>");
          client.println(fahrenheitTemp);
          client.println("*F</p></div><p>");
          client.println(humidityTemp);
          client.println("%</p></div>");
          client.println("</body></html>");     
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  // End of GI Added
 /* 
  char body[1024];
  digitalWrite(16, LOW); //switched on

  sensorsData(body);
  http.begin("http://api.higrow.tech/api/records");
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(body);
  Serial.print("The httpResponseCode is: ");
  Serial.println(httpResponseCode);
  esp_sleep_enable_timer_wakeup(DEEPSLEEP_SECONDS * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
  */
}

void sensorsData(char* body){

  //This section read sensors
  timeout = millis();
  
  int waterlevel = analogRead(soilpin);
  int lightlevel = analogRead(LIGHT_PIN);
  
  waterlevel = map(waterlevel, 0, 4095, 0, 1023);
  waterlevel = constrain(waterlevel, 0, 1023);
  lightlevel = map(lightlevel, 0, 4095, 0, 1023);
  lightlevel = constrain(lightlevel, 0, 1023);
  
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temperature = dht.readTemperature();
  
  float hic = dht.computeHeatIndex(temperature, humidity, false);       
  dtostrf(hic, 6, 2, celsiusTemp);               
  dtostrf(humidity, 6, 2, humidityTemp);
  
  String did = String(deviceid);
  String water = String((int)waterlevel);
  String light = String((int)lightlevel);

  strcpy(body, "{\"deviceId\":\"");
  strcat(body, did.c_str());
  strcat(body, "\",\"water\":\"");
  strcat(body, water.c_str());
  strcat(body, "\",\"light\":\"");
  strcat(body, light.c_str());
  strcat(body, "\",\"humidity\":\"");
  strcat(body, humidityTemp);
  strcat(body, "\",\"temperature\":\"");
  strcat(body, celsiusTemp);
  strcat(body, "\"}");

  Serial.print("Humidity is: ");
  Serial.println(humidityTemp);

  if(lightlevel<100){
    DEEPSLEEP_SECONDS = 10800;
  }
}

