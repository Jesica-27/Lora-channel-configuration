#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ============================================================
// BLE UUIDs - SAME AS KAVIN
// ============================================================

#define SERVICE_UUID       "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define RX_CHARACTERISTIC  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_CHARACTERISTIC  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// ============================================================
// E220 PINS
// ============================================================

#define LORA_RX 16
#define LORA_TX 17

#define PIN_M0  25
#define PIN_M1  26
#define PIN_AUX 27

HardwareSerial LoRaSerial(2);

// ============================================================
// GLOBAL
// ============================================================

BLECharacteristic *txCharacteristic = nullptr;

bool deviceConnected = false;

// ============================================================
// BLE SERVER CALLBACKS
// ============================================================

class ServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *server)
  {
    deviceConnected = true;

    Serial.println();
    Serial.println("================================");
    Serial.println("PHONE CONNECTED");
    Serial.println("================================");
  }

  void onDisconnect(BLEServer *server)
  {
    deviceConnected = false;

    Serial.println();
    Serial.println("PHONE DISCONNECTED");

    delay(300);

    server->startAdvertising();

    Serial.println("BLE ADVERTISING RESTARTED");
  }
};

// ============================================================
// PHONE -> ESP32 -> LORA
// ============================================================

class RxCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *characteristic)
  {
    // IMPORTANT:
    // getValue() returns std::string in your BLE library

    std::string rxValue = characteristic->getValue();

    if (rxValue.length() == 0)
      return;

    // Convert std::string -> Arduino String
    String message = String(rxValue.c_str());

    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("MESSAGE FROM PHONE");
    Serial.println("--------------------------------");

    Serial.println(message);

    Serial.println("--------------------------------");

    // ========================================================
    // SEND MESSAGE TO E220
    // ========================================================

    // START MARKER
    LoRaSerial.write(0x02);

    // MESSAGE
    LoRaSerial.print(message);

    // END MARKER
    LoRaSerial.write(0x03);

    LoRaSerial.flush();

    Serial.println("MESSAGE SENT THROUGH LORA");
    Serial.println("--------------------------------");
  }
};

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("ESP32 LORA TRACKER");
  Serial.println("BLE + E220");
  Serial.println("================================");

  // ==========================================================
  // E220 SETUP
  // ==========================================================

  pinMode(PIN_M0, OUTPUT);
  pinMode(PIN_M1, OUTPUT);
  pinMode(PIN_AUX, INPUT);

  // NORMAL / TRANSPARENT MODE
  digitalWrite(PIN_M0, LOW);
  digitalWrite(PIN_M1, LOW);

  delay(100);

  LoRaSerial.begin(
    9600,
    SERIAL_8N1,
    LORA_RX,
    LORA_TX
  );

  Serial.println("E220 SERIAL READY");

  // ==========================================================
  // BLE SETUP
  // ==========================================================

  BLEDevice::init("KAVIN_2");

  BLEServer *server = BLEDevice::createServer();

  server->setCallbacks(
    new ServerCallbacks()
  );

  // ==========================================================
  // BLE SERVICE
  // ==========================================================

  BLEService *service =
      server->createService(
        SERVICE_UUID
      );

  // ==========================================================
  // ESP32 -> PHONE
  // TX CHARACTERISTIC
  // ==========================================================

  txCharacteristic =
      service->createCharacteristic(
        TX_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_NOTIFY
      );

  txCharacteristic->addDescriptor(
    new BLE2902()
  );

  // ==========================================================
  // PHONE -> ESP32
  // RX CHARACTERISTIC
  // ==========================================================

  BLECharacteristic *rxCharacteristic =
      service->createCharacteristic(
        RX_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR
      );

  rxCharacteristic->setCallbacks(
    new RxCallbacks()
  );

  // ==========================================================
  // START BLE SERVICE
  // ==========================================================

  service->start();

  // ==========================================================
  // BLE ADVERTISING
  // ==========================================================

  BLEAdvertising *advertising =
      BLEDevice::getAdvertising();

  advertising->addServiceUUID(
    SERVICE_UUID
  );

  advertising->setScanResponse(true);

  BLEDevice::startAdvertising();

  // ==========================================================
  // READY
  // ==========================================================

  Serial.println();
  Serial.println("================================");
  Serial.println("ESP32 READY");
  Serial.println("BLE NAME : KAVIN_2");
  Serial.println("E220     : READY");
  Serial.println("WAITING FOR PHONE...");
  Serial.println("================================");
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  // ==========================================================
  // E220 -> ESP32 -> PHONE
  // ==========================================================

  static String receivedMessage = "";

  while (LoRaSerial.available())
  {
    byte data = LoRaSerial.read();

    // --------------------------------------------------------
    // START OF MESSAGE
    // --------------------------------------------------------

    if (data == 0x02)
    {
      receivedMessage = "";
    }

    // --------------------------------------------------------
    // END OF MESSAGE
    // --------------------------------------------------------

    else if (data == 0x03)
    {
      if (receivedMessage.length() > 0)
      {
        Serial.println();
        Serial.println("--------------------------------");
        Serial.println("MESSAGE FROM LORA");
        Serial.println("--------------------------------");

        Serial.println(receivedMessage);

        Serial.println("--------------------------------");

        // ====================================================
        // SEND TO PHONE
        // ====================================================

        if (deviceConnected)
        {
          txCharacteristic->setValue(
            receivedMessage.c_str()
          );

          txCharacteristic->notify();

          Serial.println("SENT TO PHONE");
        }
        else
        {
          Serial.println("PHONE NOT CONNECTED");
          Serial.println("MESSAGE NOT SENT TO PHONE");
        }

        Serial.println("--------------------------------");

        receivedMessage = "";
      }
    }

    // --------------------------------------------------------
    // NORMAL MESSAGE DATA
    // --------------------------------------------------------

    else
    {
      receivedMessage += (char)data;
    }
  }

  // Nothing is printed continuously here.
  // ESP32 simply waits for BLE or LoRa messages.

  delay(5);
}
