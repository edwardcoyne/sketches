
#include <WiFi.h>
#include <esp32_bt.h>

BT bt;

void debug(const char* message) {
  Serial.println(message);
}

void StartAP() {
  String ssid("CarSensor");

  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(ssid.c_str())) {
    return;
  }

  debug((String("WiFi AP : ") + ssid).c_str());
  debug((String("Wifi AP IP : ") + WiFi.softAPIP().toString()).c_str());
}


void setup() {
  Serial.begin(9600);
  bt.Start();
  Serial.println("Start..");
}

void loop() {
  Serial.println("Available:");
  delay(1000);

}
