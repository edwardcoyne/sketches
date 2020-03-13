
/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
*/

#include <functional>
#include <memory>
#include <list>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "esp_system.h"

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

constexpr int kWdtTimeout = 5000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

// They return whether they should be rescheduled.
std::vector<std::function<bool()>> run_queue_;

class PwmPinCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      const std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.print("*********  PWM ");
        Serial.print(channel_);
        Serial.println("  *********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }

        Serial.println();

        // Do stuff based on the command received from the app
        if (rxValue.find("On") != -1 && !state_) { 
          Serial.println("Turning ON!");
          for (int i = kBottom; i < kTop; i++) {
            ledcWrite(channel_, i);
            delay(kDelayMs);
          }
          state_ = true;
        } else if (rxValue.find("Off") != -1 && state_) {          
          Serial.println("Turning OFF!");
          for (int i = kTop; i > kBottom; i--) {
            ledcWrite(channel_, i);
            delay(kDelayMs);
          }
          ledcWrite(channel_, 0);
          state_ = false;
        }

        Serial.println();
        Serial.println("*********");
      }
    }

public:
    PwmPinCallback(uint8_t channel, uint8_t pin) : channel_(channel) {
      ledcSetup(channel_, 500, 8);
      ledcAttachPin(pin, channel_);
    }

private:
    static const uint8_t kBottom = 100;
    static const uint8_t kTop = 180;
    static const uint8_t kDelayMs = 15;
    const uint8_t channel_;
    bool state_ = false;
};

class PinSafetyCallback : public BLECharacteristicCallbacks {
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
          if (last_cb_active_) {
            *last_cb_active_ = false;
          }
          
          Serial.println("Turning ON!");
          digitalWrite(pin_, HIGH);
        
          last_cb_active_ = std::shared_ptr<bool>(new bool(true));
          run_queue_.push_back([this, cb_active = last_cb_active_, start = millis()]() {
            if (*cb_active && millis() - start > off_timeout_s_ * 1000) {                
              Serial.println("Turning OFF after timeout!");          
              digitalWrite(pin_, LOW);
              *cb_active = false;
            }

            return *cb_active;
          });
        }
        else if (rxValue.find("Off") != -1) {
          if (last_cb_active_) {
            *last_cb_active_ = false;
          }
          
          Serial.println("Turning OFF!");
          digitalWrite(pin_, LOW);
        }

        Serial.println();
        Serial.println("*********");
      }
    }

public:
    PinSafetyCallback(uint8_t pin, uint8_t off_timeout_s) : pin_(pin), off_timeout_s_(off_timeout_s) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
    }

private:
    const uint8_t pin_;
    const uint8_t off_timeout_s_;
    std::shared_ptr<bool> last_cb_active_;
};

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
    PinCallback(uint8_t pin) : pin_(pin) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
    }

private:
    const uint8_t pin_;
};

void setupRelay(BLEServer* pServer) {
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
  BLEService* pService = pServer->createService(SOLENOID_SERVICE_UUID);
  
  BLECharacteristic* pCharacteristic = pService->createCharacteristic(
                                         SOLENOID_1_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setValue("Off");
  pCharacteristic->setCallbacks(new PinSafetyCallback(SOLENOID_1_PIN, 120));
  //pCharacteristic->setCallbacks(new PwmPinCallback(0, SOLENOID_1_PIN));

  pCharacteristic = pService->createCharacteristic(
                       SOLENOID_2_UUID,
                       BLECharacteristic::PROPERTY_READ |
                       BLECharacteristic::PROPERTY_WRITE
                     );
  pCharacteristic->setValue("Off");
  pCharacteristic->setCallbacks(new PinSafetyCallback(SOLENOID_2_PIN, 120));
  //pCharacteristic->setCallbacks(new PwmPinCallback(1, SOLENOID_2_PIN));
  
  pService->start();
}

void IRAM_ATTR resetModule() {
  ets_printf("Watchdog triggered, resetting... \n");
  esp_restart();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE!");

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, kWdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt

  BLEDevice::init("desk-buddy");
  BLEServer* pServer = BLEDevice::createServer();
  setupRelay(pServer);
  setupSolenoid(pServer);
  
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("Characteristics published, BLE up!");
}

void loop() {
  // Pet watchdog.
  timerWrite(timer, 0);
  
  std::vector<int> to_remove;
  int i = 0;
  for (const auto& element : run_queue_) {
    if (!element()) {
      to_remove.push_back(i);
    }
    i++;
  }
  for (int i = 0; i < to_remove.size(); i++) {
    auto it = run_queue_.begin();
    advance(it, (to_remove[i] - i));
    run_queue_.erase(it);
  }
  delay(1000);
}
