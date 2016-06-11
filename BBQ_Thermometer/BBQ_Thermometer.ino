
#include <ESP8266WiFi.h>
#include <SPI.h>

// Set these to run example.
#define WIFI_SSID "GoogleGuest"
#define WIFI_PASSWORD ""

const int cs = D8;
const int sclk = D5;
const int miso = D6;

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
  
  //SPI.begin();
  delay(100);
}

void loop() {
  Serial.print("C = "); 
  Serial.println(readCelsius());
  delay(1000);
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
