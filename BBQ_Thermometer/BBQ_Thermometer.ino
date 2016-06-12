
#include <ESP8266WiFi.h>
#include <SPI.h>

// Set these to run example.
#define WIFI_SSID "GoogleGuest"
#define WIFI_PASSWORD ""

const int cs = D8;
const int sclk = D5;
const int miso = D6;

int startTimestamp = 0;

double currentTemp;
// the temp needs to change by more than this amount
// to trigger an update, this limits oscillation.
const float tempUpdateThreshold = 2.0f;

const char* redisHost = "redis";
const String redisCommandUpdateTemp1 = "LPUSH bbq-temp1 ";
const int redisTimeoutMS = 5000;

int seconds() {
  return millis() / 1000;
}

void setup() {
  Serial.begin(9600);

  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);

  pinMode(sclk, OUTPUT);
  pinMode(miso, INPUT);

  const int serverTimestamp = getServerTimestampSeconds();
  startTimestamp = serverTimestamp - seconds();
  
  //SPI.begin();
  delay(100);
}

void loop() {
  double newTemp = readFahrenheit();
  if (abs(currentTemp - newTemp) > tempUpdateThreshold) {
    Serial.print("Updated temp = "); 
    Serial.println(newTemp);
    updateRedis(newTemp);
    currentTemp = newTemp;
  }
  delay(250);
}

int getServerTimestampSeconds() { 
  WiFiClient client;
  if (!connectToRedisAndSend(&client, "TIME")) {
    return -1;
  }
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  return 1000;
}

void updateRedis(double newTemp) {  
  const String command = redisCommandUpdateTemp1 + String(startTimestamp + seconds())  
      + "," + String(newTemp);
  WiFiClient client;
  if (!connectToRedisAndSend(&client, command)) {
    return;
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
}

bool connectToRedisAndSend(WiFiClient* client, const String& command) {
  const int redisPort = 6379;
  if (!client->connect(redisHost, redisPort)) {
    Serial.println("connection failed");
    return false;
  }

  client->println(command);

  unsigned long timeout = millis();
  while (client->available() == 0) {
    if (millis() - timeout > redisTimeoutMS) {
      Serial.println("Client Timeout !");
      client->stop();
      return false;
    }
  }
  return true; 
}

// The rest of this is "borrowed" from the MAX6675 arduino
// libraray: https://github.com/adafruit/MAX6675-library
// I couldn't use it directly due to some missing arduino
// libraries on the esp8266.

double readFahrenheit(void) {
  return readCelsius() * 9.0/5.0 + 32;
}

double readCelsius() {
  uint16_t v;

  digitalWrite(cs, LOW);
  delay(1);

  v = spiread();
  v <<= 8;
  v |= spiread();

  digitalWrite(cs, HIGH);

  if (v & 0x4) {
    // uh oh, no thermocouple attached!
    return NAN; 
    //return -100;
  }

  v >>= 3;

  return v*0.25;
}

byte spiread() { 
  int i;
  byte d = 0;

  for (i=7; i>=0; i--)
  {
    digitalWrite(sclk, LOW);
    delay(1);
    if (digitalRead(miso)) {
      //set the bit to 0 no matter what
      d |= (1 << i);
    }

    digitalWrite(sclk, HIGH);
    delay(1);
  }

  return d;
}
