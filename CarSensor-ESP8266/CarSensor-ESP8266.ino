#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <DNSServer.h>


Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
WiFiServer server(1701);
WiFiClient connected_client;
DNSServer dns;

void StartAP() {
  String ssid("CarSensor");

  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(ssid.c_str())) {
    return;
  }

  Serial.println(String("WiFi AP : ") + ssid);
  Serial.println(String("Wifi AP IP : ") + WiFi.softAPIP().toString());
  
  dns.start(53, "*", WiFi.softAPIP());
  dns.setTTL(30);
}

void ConnectToNetwork(String ssid, String password) {
  WiFi.begin(ssid.c_str(), password.c_str());
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(9600);

  if(!bmp.begin()) {
    Serial.print("No BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }

  StartAP();
  server.begin();
}

void loop() {
   dns.processNextRequest();
    
  if (server.hasClient()) {
    connected_client = server.available();
    Serial.println("Client connected: ");
    Serial.println(connected_client.remoteIP().toString());
  }
  if (!connected_client) {
    return;
  }

  sensors_event_t event;
  bmp.getEvent(&event);
  if (event.pressure){
    float temperature;
    bmp.getTemperature(&temperature);
    
    connected_client.print(String(millis()) + ", " + String(event.pressure) + ", " + String(temperature) + "\n");
  } else {
    Serial.println("Sensor error");
  }
  delay(1000);
}
