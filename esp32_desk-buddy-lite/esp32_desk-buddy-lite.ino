
/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define RELAY_SERVICE_UUID        "e3bed634-e1cc-48d6-bb64-84ca226a6e7c"
#define RELAY_CONTROL_UUID        "e3bed634-e1cc-48d6-bb64-84ca226a640d"
#define RELAY_PIN 12                                                                                                                               

#define SOLENOID_SERVICE_UUID        "19fbf94e-d0e6-11e9-bb65-2a2ae2dbcce4"
#define SOLENOID_1_UUID              "19fbf94e-d0e6-11e9-bb65-2a2ae2dbcce5"
#define SOLENOID_2_UUID              "19fbf94e-d0e6-11e9-bb65-2a2ae2dbcce6"
#define SOLENOID_1_PIN 22        
#define SOLENOID_2_PIN 23                                                                                                                            


class PinCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      const std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.print("*********  ");
        Serial.print(pin_);
        Serial.println("  *********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }

        Serial.println();

        // Do stuff based on the command received from the app
        if (rxValue.find("On") != -1) { 
          Serial.println("Turning ON!");
          digitalWrite(pin_, HIGH);
        }
        else if (rxValue.find("Off") != -1) {
          Serial.println("Turning OFF!");
          digitalWrite(pin_, LOW);
        }

        Serial.println();
        Serial.println("*********");
      }
    }

public:
    PinCallback(uint8_t pin) : pin_(pin) {}

private:
    const uint8_t pin_;
};

void setupRelay(BLEServer* pServer) {
  pinMode(RELAY_PIN, OUTPUT);
  
  BLEService* pService = pServer->createService(RELAY_SERVICE_UUID);
  
  BLECharacteristic* pCharacteristic = pService->createCharacteristic(
                                         RELAY_CONTROL_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setValue("Off");
  pCharacteristic->setCallbacks(new PinCallback(RELAY_PIN));
  
  pService->start();
}

void setupSolenoid(BLEServer* pServer) {
  pinMode(SOLENOID_1_PIN, OUTPUT);
  pinMode(SOLENOID_2_PIN, OUTPUT);
  
  BLEService* pService = pServer->createService(SOLENOID_SERVICE_UUID);
  
  BLECharacteristic* pCharacteristic = pService->createCharacteristic(
                                         SOLENOID_1_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setValue("Off");
  pCharacteristic->setCallbacks(new PinCallback(SOLENOID_1_PIN));

  pCharacteristic = pService->createCharacteristic(
                       SOLENOID_2_UUID,
                       BLECharacteristic::PROPERTY_READ |
                       BLECharacteristic::PROPERTY_WRITE
                     );
  pCharacteristic->setValue("Off");
  pCharacteristic->setCallbacks(new PinCallback(SOLENOID_2_PIN));
  
  pService->start();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE!");

  BLEDevice::init("desk-buddy");
  BLEServer* pServer = BLEDevice::createServer();
  setupRelay(pServer);
  setupSolenoid(pServer);
  
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("Characteristics published, BLE up!");
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
}
