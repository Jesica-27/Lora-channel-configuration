#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <LoRa_E220.h>

// ============================================================
// MY NODE
// ============================================================

// CHANGE THESE FOR EACH ESP32
#define MY_NAME "JESS"
#define MY_NODE_ID 2

// ============================================================
// E220 PINS
// ============================================================

#define LORA_RX 16
#define LORA_TX 17

#define PIN_M0  25
#define PIN_M1  26
#define PIN_AUX 27

HardwareSerial E220Serial(2);

LoRa_E220 e220ttl(
  &E220Serial,
  PIN_AUX,
  PIN_M0,
  PIN_M1
);

// ============================================================
// LORA CONFIGURATION
// ============================================================

#define LORA_CHANNEL 18

// ============================================================
// NODE ADDRESSES
// ============================================================

#define THANU_ADDH 0
#define THANU_ADDL 1

#define JESS_ADDH  0
#define JESS_ADDL  2

#define ASMI_ADDH  0
#define ASMI_ADDL  3

// ============================================================
// BLE UUIDs
// ============================================================

#define SERVICE_UUID \
"6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

#define RX_CHARACTERISTIC \
"6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

#define TX_CHARACTERISTIC \
"6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// ============================================================
// BLE GLOBALS
// ============================================================

BLECharacteristic *txCharacteristic = nullptr;

bool deviceConnected = false;

// ============================================================
// SEND DATA TO PHONE
// ============================================================

void sendToPhone(String data)
{
  data.trim();

  if (data.length() == 0)
  {
    return;
  }

  if (!deviceConnected)
  {
    Serial.println();
    Serial.println("PHONE NOT CONNECTED");
    Serial.println("Cannot send data to phone.");
    return;
  }

  txCharacteristic->setValue(data.c_str());
  txCharacteristic->notify();

  Serial.println();
  Serial.println("----------------------------------------");
  Serial.println("DATA SENT TO PHONE");
  Serial.print("Data: ");
  Serial.println(data);
  Serial.println("----------------------------------------");
}

// ============================================================
// GET DESTINATION ADDRESS
// ============================================================

bool getDestinationAddress(
  String name,
  uint8_t &addH,
  uint8_t &addL
)
{
  name.trim();
  name.toUpperCase();

  // ----------------------------------------------------------
  // THANU
  // ----------------------------------------------------------

  if (name == "THANU")
  {
    addH = THANU_ADDH;
    addL = THANU_ADDL;

    return true;
  }

  // ----------------------------------------------------------
  // JESS
  // ----------------------------------------------------------

  if (name == "JESS")
  {
    addH = JESS_ADDH;
    addL = JESS_ADDL;

    return true;
  }

  // ----------------------------------------------------------
  // ASMI
  // ----------------------------------------------------------

  if (name == "ASMI")
  {
    addH = ASMI_ADDH;
    addL = ASMI_ADDL;

    return true;
  }

  return false;
}

// ============================================================
// SEND MESSAGE TO SPECIFIC NODE
// ============================================================

void sendMessageToPerson(
  String destination,
  String message
)
{
  destination.trim();
  message.trim();

  uint8_t addH;
  uint8_t addL;

  // ----------------------------------------------------------
  // FIND DESTINATION
  // ----------------------------------------------------------

  if (!getDestinationAddress(
        destination,
        addH,
        addL
      ))
  {
    Serial.println();
    Serial.println("UNKNOWN DESTINATION!");
    Serial.print("Destination: ");
    Serial.println(destination);

    return;
  }

  // ----------------------------------------------------------
  // DON'T SEND TO YOURSELF
  // ----------------------------------------------------------

  if (destination.equalsIgnoreCase(MY_NAME))
  {
    Serial.println();
    Serial.println("CANNOT SEND MESSAGE TO YOURSELF.");

    return;
  }

  // ----------------------------------------------------------
  // SEND FIXED MESSAGE
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("========================================");
  Serial.println("LORA TRANSMISSION");
  Serial.println("========================================");

  Serial.print("From       : ");
  Serial.println(MY_NAME);

  Serial.print("Destination: ");
  Serial.println(destination);

  Serial.print("Message    : ");
  Serial.println(message);

  Serial.print("Address    : 0x");
  Serial.print(addH, HEX);
  Serial.print(" 0x");
  Serial.println(addL, HEX);

  Serial.print("Channel    : ");
  Serial.println(LORA_CHANNEL);

  ResponseStatus rs =
    e220ttl.sendFixedMessage(
      addH,
      addL,
      LORA_CHANNEL,
      message
    );

  // ----------------------------------------------------------
  // RESULT
  // ----------------------------------------------------------

  if (rs.code == E220_SUCCESS)
  {
    Serial.println();
    Serial.println("MESSAGE SENT SUCCESSFULLY");
  }
  else
  {
    Serial.println();
    Serial.print("LORA ERROR: ");
    Serial.println(
      rs.getResponseDescription()
    );
  }

  Serial.println("========================================");
}

// ============================================================
// BLE SERVER CALLBACKS
// ============================================================

class ServerCallbacks :
  public BLEServerCallbacks
{
  void onConnect(
    BLEServer *server
  )
  {
    deviceConnected = true;

    Serial.println();
    Serial.println("========================================");
    Serial.println("PHONE CONNECTED");
    Serial.println("========================================");

    Serial.print("NODE: ");
    Serial.println(MY_NAME);
  }

  void onDisconnect(
    BLEServer *server
  )
  {
    deviceConnected = false;

    Serial.println();
    Serial.println("========================================");
    Serial.println("PHONE DISCONNECTED");
    Serial.println("========================================");

    delay(300);

    server->startAdvertising();

    Serial.println(
      "BLE ADVERTISING RESTARTED"
    );
  }
};

// ============================================================
// PHONE -> ESP32
// ESP32 -> LORA
// ============================================================

class RxCallbacks :
  public BLECharacteristicCallbacks
{
  void onWrite(
    BLECharacteristic *characteristic
  )
  {
    // --------------------------------------------------------
    // GET DATA FROM PHONE
    // --------------------------------------------------------

    std::string rxValue =
      characteristic->getValue();

    String data =
      String(rxValue.c_str());

    data.trim();

    if (data.length() == 0)
    {
      return;
    }

    Serial.println();
    Serial.println("========================================");
    Serial.println("DATA RECEIVED FROM PHONE");
    Serial.println("========================================");

    Serial.print("Phone Data: ");
    Serial.println(data);

    // --------------------------------------------------------
    // EXPECTED FORMAT
    //
    // ASMI|Hello
    // JESS|Hello
    // THANU|Hello
    // --------------------------------------------------------

    int separator =
      data.indexOf('|');

    if (separator == -1)
    {
      Serial.println();
      Serial.println("INVALID PHONE MESSAGE FORMAT.");

      Serial.println();
      Serial.println("USE:");

      Serial.println("ASMI|Hello");
      Serial.println("JESS|Hello");
      Serial.println("THANU|Hello");

      return;
    }

    // --------------------------------------------------------
    // GET DESTINATION
    // --------------------------------------------------------

    String destination =
      data.substring(
        0,
        separator
      );

    // --------------------------------------------------------
    // GET MESSAGE
    // --------------------------------------------------------

    String text =
      data.substring(
        separator + 1
      );

    destination.trim();
    text.trim();

    // --------------------------------------------------------
    // VALIDATE
    // --------------------------------------------------------

    if (
      destination.length() == 0 ||
      text.length() == 0
    )
    {
      Serial.println();
      Serial.println(
        "INVALID DESTINATION OR MESSAGE."
      );

      return;
    }

    // --------------------------------------------------------
    // DISPLAY
    // --------------------------------------------------------

    Serial.println();
    Serial.println("----------------------------------------");

    Serial.print("Destination: ");
    Serial.println(destination);

    Serial.print("Message: ");
    Serial.println(text);

    Serial.println("----------------------------------------");

    // --------------------------------------------------------
    // SEND THROUGH LORA
    // --------------------------------------------------------

    sendMessageToPerson(
      destination,
      text
    );

    Serial.println(
      "========================================"
    );
  }
};

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  // ==========================================================
  // E220 CONTROL PINS
  // ==========================================================

  pinMode(
    PIN_M0,
    OUTPUT
  );

  pinMode(
    PIN_M1,
    OUTPUT
  );

  pinMode(
    PIN_AUX,
    INPUT
  );

  // ==========================================================
  // NORMAL MODE
  // M0 = LOW
  // M1 = LOW
  // ==========================================================

  digitalWrite(
    PIN_M0,
    LOW
  );

  digitalWrite(
    PIN_M1,
    LOW
  );

  delay(100);

  // ==========================================================
  // E220 UART
  // ==========================================================

  E220Serial.begin(
    9600,
    SERIAL_8N1,
    LORA_RX,
    LORA_TX
  );

  delay(500);

  // ==========================================================
  // START E220
  // ==========================================================

  e220ttl.begin();

  delay(500);

  Serial.println();
  Serial.println("========================================");
  Serial.println("E220 LORA INITIALIZED");
  Serial.println("========================================");

  Serial.println("UART       : 9600 8N1");
  Serial.println("CHANNEL    : 18");
  Serial.println("FREQUENCY  : 868.125 MHz");
  Serial.println("AIR RATE   : 2.4 kbps");
  Serial.println("MODE       : NORMAL");
  Serial.println("POWER      : 22 dBm");

  Serial.println("----------------------------------------");

  Serial.print("MY NODE    : ");
  Serial.println(MY_NAME);

  Serial.print("NODE ADDR  : 0x00");
  Serial.println(MY_NODE_ID, HEX);

  Serial.println("========================================");

  // ==========================================================
  // BLE
  // ==========================================================

  String bleName =
    String(MY_NAME) + "_CHAT";

  BLEDevice::init(
    bleName.c_str()
  );

  // ==========================================================
  // CREATE BLE SERVER
  // ==========================================================

  BLEServer *server =
    BLEDevice::createServer();

  server->setCallbacks(
    new ServerCallbacks()
  );

  // ==========================================================
  // CREATE BLE SERVICE
  // ==========================================================

  BLEService *service =
    server->createService(
      SERVICE_UUID
    );

  // ==========================================================
  // ESP32 -> PHONE
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

  advertising->setScanResponse(
    true
  );

  BLEDevice::startAdvertising();

  // ==========================================================
  // READY
  // ==========================================================

  Serial.println();
  Serial.println("========================================");
  Serial.println("            NODE READY");
  Serial.println("========================================");

  Serial.print("BLE NAME : ");
  Serial.print(MY_NAME);
  Serial.println("_CHAT");

  Serial.print("LORA ADDRESS : 0x00");
  Serial.println(MY_NODE_ID, HEX);

  Serial.println("CHANNEL : 18");

  Serial.println("STATUS : READY");

  Serial.println("========================================");
}

// ============================================================
// LORA RECEIVE
//
// E220 -> ESP32 -> BLE -> PHONE
// ============================================================

void handleLoRa()
{
  if (!E220Serial.available())
  {
    return;
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("LORA DATA AVAILABLE");
  Serial.println("========================================");

  ResponseContainer rc =
    e220ttl.receiveMessage();

  // ----------------------------------------------------------
  // CHECK RECEIVE STATUS
  // ----------------------------------------------------------

  if (rc.status.code != E220_SUCCESS)
  {
    Serial.print("LORA RECEIVE ERROR: ");

    Serial.println(
      rc.status.getResponseDescription()
    );

    return;
  }

  // ----------------------------------------------------------
  // GET RECEIVED MESSAGE
  // ----------------------------------------------------------

  String received =
    rc.data;

  received.trim();

  if (received.length() == 0)
  {
    return;
  }

  // ----------------------------------------------------------
  // DISPLAY RECEIVED MESSAGE
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("DATA RECEIVED FROM OTHER ESP32");

  Serial.println("----------------------------------------");

  Serial.print("LoRa RX: ");
  Serial.println(received);

  Serial.println("----------------------------------------");

  // ==========================================================
  // SEND RECEIVED LORA MESSAGE TO PHONE
  // ==========================================================

  sendToPhone(received);

  Serial.println(
    "LORA -> BLE -> PHONE COMPLETE"
  );

  Serial.println("========================================");
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  // ==========================================================
  // CHECK FOR LORA DATA
  // ==========================================================

  handleLoRa();

  delay(5);
}
