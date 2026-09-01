#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <LoRa_E220.h>
#include "mbedtls/gcm.h"

// ============================================================
// CHANGE ONLY THESE TWO VALUES FOR EACH ESP32
// ============================================================

// NODE 1
// #define MY_NAME "THANU"
// #define MY_NODE_ID 1

// NODE 2
#define MY_NAME "JESS"
#define MY_NODE_ID 2

// NODE 3
// #define MY_NAME "ASMI"
// #define MY_NODE_ID 3

// NODE 4
// #define MY_NAME "KAVIN"
// #define MY_NODE_ID 4


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
// LORA
// ============================================================

#define LORA_CHANNEL 18


// ============================================================
// NODE ADDRESSES
// ============================================================

#define THANU_ADDH 0
#define THANU_ADDL 1

#define JESS_ADDH 0
#define JESS_ADDL 2

#define ASMI_ADDH 0
#define ASMI_ADDL 3

#define KAVIN_ADDH 0
#define KAVIN_ADDL 4


// ============================================================
// BLE UUID
// ============================================================

#define SERVICE_UUID \
"6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

#define RX_CHARACTERISTIC \
"6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

#define TX_CHARACTERISTIC \
"6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


// ============================================================
// AES-256 KEY
// SAME KEY ON ALL NODES
// ============================================================

const uint8_t AES_KEY[32] = {

  0x10, 0x22, 0x34, 0x48,
  0x55, 0x61, 0x73, 0x89,
  0x91, 0xA2, 0xB3, 0xC4,
  0xD5, 0xE6, 0xF7, 0x08,
  0x19, 0x2A, 0x3B, 0x4C,
  0x5D, 0x6E, 0x7F, 0x80,
  0x90, 0xA1, 0xB2, 0xC3

};


// ============================================================
// AES-GCM
// ============================================================

#define GCM_NONCE_SIZE 12
#define GCM_TAG_SIZE   16


// ============================================================
// BLE GLOBALS
// ============================================================

BLECharacteristic *txCharacteristic = nullptr;

bool deviceConnected = false;


// ============================================================
// HEX DIGIT
// ============================================================

char hexDigit(uint8_t value)
{
  if (value < 10)
    return '0' + value;

  return 'A' + (value - 10);
}


// ============================================================
// BYTES TO HEX
// ============================================================

String bytesToHex(
  const uint8_t *data,
  size_t length
)
{
  String result = "";

  for (size_t i = 0; i < length; i++)
  {
    result += hexDigit(
      (data[i] >> 4) & 0x0F
    );

    result += hexDigit(
      data[i] & 0x0F
    );
  }

  return result;
}


// ============================================================
// HEX VALUE
// ============================================================

int hexValue(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  return -1;
}


// ============================================================
// HEX TO BYTES
// ============================================================

bool hexToBytes(
  String hex,
  uint8_t *output,
  size_t outputSize
)
{
  hex.trim();

  if (hex.length() != outputSize * 2)
    return false;

  for (size_t i = 0; i < outputSize; i++)
  {
    int high = hexValue(hex[i * 2]);
    int low  = hexValue(hex[i * 2 + 1]);

    if (high < 0 || low < 0)
      return false;

    output[i] =
      (high << 4) | low;
  }

  return true;
}


// ============================================================
// AES-256-GCM ENCRYPT
//
// PACKET:
//
// E2E|SOURCE|DESTINATION|NONCE|CIPHERTEXT|TAG
//
// Example:
//
// E2E|JESS|KAVIN|....
//
// ============================================================

String encryptMessage(String plaintext)
{
  plaintext.trim();

  if (plaintext.length() == 0)
    return "";

  uint8_t nonce[GCM_NONCE_SIZE];
  uint8_t tag[GCM_TAG_SIZE];

  size_t plaintextLength =
    plaintext.length();

  uint8_t *ciphertext =
    new uint8_t[plaintextLength];

  // RANDOM NONCE

  for (int i = 0; i < GCM_NONCE_SIZE; i++)
  {
    nonce[i] =
      (uint8_t)(esp_random() & 0xFF);
  }

  // AES-GCM

  mbedtls_gcm_context gcm;

  mbedtls_gcm_init(&gcm);

  int result =
    mbedtls_gcm_setkey(
      &gcm,
      MBEDTLS_CIPHER_ID_AES,
      AES_KEY,
      256
    );

  if (result != 0)
  {
    mbedtls_gcm_free(&gcm);

    delete[] ciphertext;

    return "";
  }

  result =
    mbedtls_gcm_crypt_and_tag(
      &gcm,
      MBEDTLS_GCM_ENCRYPT,
      plaintextLength,
      nonce,
      GCM_NONCE_SIZE,
      nullptr,
      0,
      (const uint8_t *)plaintext.c_str(),
      ciphertext,
      GCM_TAG_SIZE,
      tag
    );

  mbedtls_gcm_free(&gcm);

  if (result != 0)
  {
    delete[] ciphertext;

    return "";
  }

  String nonceHex =
    bytesToHex(
      nonce,
      GCM_NONCE_SIZE
    );

  String cipherHex =
    bytesToHex(
      ciphertext,
      plaintextLength
    );

  String tagHex =
    bytesToHex(
      tag,
      GCM_TAG_SIZE
    );

  delete[] ciphertext;

  // ==========================================================
  // CREATE PACKET
  // ==========================================================

  String packet =
    "E2E|" +
    String(MY_NAME) +
    "|" +
    "DEST" +
    "|" +
    nonceHex +
    "|" +
    cipherHex +
    "|" +
    tagHex;

  return packet;
}


// ============================================================
// REPLACE DESTINATION IN ENCRYPTED PACKET
// ============================================================

String addDestination(
  String encrypted,
  String destination
)
{
  int p1 = encrypted.indexOf('|');

  int p2 = encrypted.indexOf(
    '|',
    p1 + 1
  );

  if (p1 < 0 || p2 < 0)
    return "";

  String result =
    encrypted.substring(0, p2 + 1);

  result += destination;

  result += encrypted.substring(
    p2 + 6
  );

  return result;
}


// ============================================================
// GET NODE ADDRESS
// ============================================================

bool getNodeAddress(
  String name,
  uint8_t &addH,
  uint8_t &addL
)
{
  name.trim();
  name.toUpperCase();

  if (name == "THANU")
  {
    addH = THANU_ADDH;
    addL = THANU_ADDL;
    return true;
  }

  if (name == "JESS")
  {
    addH = JESS_ADDH;
    addL = JESS_ADDL;
    return true;
  }

  if (name == "ASMI")
  {
    addH = ASMI_ADDH;
    addL = ASMI_ADDL;
    return true;
  }

  if (name == "KAVIN")
  {
    addH = KAVIN_ADDH;
    addL = KAVIN_ADDL;
    return true;
  }

  return false;
}


// ============================================================
// GET NEXT HOP
//
// THIS IS THE ROUTING DECISION
//
// Example:
//
// JESS -> KAVIN
//
// Next hop = ASMI
//
// ============================================================

bool getNextHop(
  String destination,
  String &nextHop
)
{
  destination.trim();
  destination.toUpperCase();

  // ----------------------------------------------------------
  // JESS -> KAVIN
  // Route through ASMI
  // ----------------------------------------------------------

  if (
    String(MY_NAME) == "JESS" &&
    destination == "KAVIN"
  )
  {
    nextHop = "ASMI";

    return true;
  }


  // ----------------------------------------------------------
  // ASMI -> KAVIN
  // Direct
  // ----------------------------------------------------------

  if (
    String(MY_NAME) == "ASMI" &&
    destination == "KAVIN"
  )
  {
    nextHop = "KAVIN";

    return true;
  }


  // ----------------------------------------------------------
  // KAVIN -> JESS
  // Route through ASMI
  // ----------------------------------------------------------

  if (
    String(MY_NAME) == "KAVIN" &&
    destination == "JESS"
  )
  {
    nextHop = "ASMI";

    return true;
  }


  // ----------------------------------------------------------
  // ASMI -> JESS
  // Direct
  // ----------------------------------------------------------

  if (
    String(MY_NAME) == "ASMI" &&
    destination == "JESS"
  )
  {
    nextHop = "JESS";

    return true;
  }


  // ----------------------------------------------------------
  // JESS -> ASMI
  // Direct
  // ----------------------------------------------------------

  if (
    String(MY_NAME) == "JESS" &&
    destination == "ASMI"
  )
  {
    nextHop = "ASMI";

    return true;
  }


  // ----------------------------------------------------------
  // ASMI -> THANU
  // Direct
  // ----------------------------------------------------------

  if (
    String(MY_NAME) == "ASMI" &&
    destination == "THANU"
  )
  {
    nextHop = "THANU";

    return true;
  }


  // ----------------------------------------------------------
  // THANU -> JESS
  // Direct
  // ----------------------------------------------------------

  if (
    String(MY_NAME) == "THANU" &&
    destination == "JESS"
  )
  {
    nextHop = "JESS";

    return true;
  }


  // ----------------------------------------------------------
  // THANU -> ASMI
  // Direct
  // ----------------------------------------------------------

  if (
    String(MY_NAME) == "THANU" &&
    destination == "ASMI"
  )
  {
    nextHop = "ASMI";

    return true;
  }


  // ----------------------------------------------------------
  // Default: direct destination
  // ----------------------------------------------------------

  nextHop = destination;

  return true;
}


// ============================================================
// SEND PACKET TO NEXT HOP
// ============================================================

void sendPacketToNode(
  String nextHop,
  String packet
)
{
  uint8_t addH;
  uint8_t addL;

  if (
    !getNodeAddress(
      nextHop,
      addH,
      addL
    )
  )
  {
    Serial.println(
      "NEXT HOP NOT FOUND"
    );

    return;
  }

  Serial.println();
  Serial.println(
    "----------------------------------------"
  );

  Serial.print(
    "CURRENT NODE: "
  );

  Serial.println(MY_NAME);

  Serial.print(
    "NEXT HOP: "
  );

  Serial.println(nextHop);

  Serial.println(
    "FORWARDING ENCRYPTED PACKET..."
  );

  ResponseStatus rs =
    e220ttl.sendFixedMessage(
      addH,
      addL,
      LORA_CHANNEL,
      packet
    );

  if (rs.code == E220_SUCCESS)
  {
    Serial.println(
      "PACKET SENT TO NEXT HOP"
    );
  }
  else
  {
    Serial.print(
      "LORA ERROR: "
    );

    Serial.println(
      rs.getResponseDescription()
    );
  }

  Serial.println(
    "----------------------------------------"
  );
}


// ============================================================
// SEND ENCRYPTED MESSAGE
// ============================================================

void sendEncryptedMessage(
  String destination,
  String message
)
{
  destination.trim();
  message.trim();

  String nextHop;

  // ==========================================================
  // FIND NEXT HOP
  // ==========================================================

  if (
    !getNextHop(
      destination,
      nextHop
    )
  )
  {
    Serial.println(
      "NO ROUTE FOUND"
    );

    return;
  }


  // ==========================================================
  // ENCRYPT
  // ==========================================================

  Serial.println();
  Serial.println(
    "========================================"
  );

  Serial.println(
    "AES-256-GCM ENCRYPTION"
  );

  Serial.println(
    "========================================"
  );

  Serial.print(
    "Original message: "
  );

  Serial.println(message);


  String encrypted =
    encryptMessage(
      message
    );


  if (encrypted.length() == 0)
  {
    Serial.println(
      "ENCRYPTION FAILED"
    );

    return;
  }


  // ==========================================================
  // ADD REAL DESTINATION
  // ==========================================================

  encrypted.replace(
    "|DEST|",
    "|" + destination + "|"
  );


  // ==========================================================
  // SHOW ENCRYPTED DATA
  // ==========================================================

  Serial.println();

  Serial.println(
    "ENCRYPTED DATA:"
  );

  Serial.println(
    encrypted
  );


  // ==========================================================
  // ROUTING INFORMATION
  // ==========================================================

  Serial.println();

  Serial.println(
    "ROUTING INFORMATION"
  );

  Serial.print(
    "SOURCE: "
  );

  Serial.println(MY_NAME);

  Serial.print(
    "DESTINATION: "
  );

  Serial.println(destination);

  Serial.print(
    "NEXT HOP: "
  );

  Serial.println(nextHop);


  // ==========================================================
  // SEND TO NEXT HOP
  // ==========================================================

  sendPacketToNode(
    nextHop,
    encrypted
  );

  Serial.println(
    "========================================"
  );
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

    Serial.println(
      "PHONE CONNECTED"
    );
  }


  void onDisconnect(
    BLEServer *server
  )
  {
    deviceConnected = false;

    Serial.println(
      "PHONE DISCONNECTED"
    );

    delay(300);

    server->startAdvertising();
  }
};


// ============================================================
// PHONE -> ESP32
// ============================================================

class RxCallbacks :
  public BLECharacteristicCallbacks
{
  void onWrite(
    BLECharacteristic *characteristic
  )
  {
    std::string rxValue =
      characteristic->getValue();

    String data =
      String(
        rxValue.c_str()
      );

    data.trim();

    if (data.length() == 0)
      return;


    Serial.println();
    Serial.println(
      "PHONE DATA RECEIVED:"
    );

    Serial.println(data);


    // ========================================================
    // FORMAT:
    //
    // KAVIN|Hello
    // ========================================================

    int separator =
      data.indexOf('|');


    if (separator == -1)
    {
      Serial.println(
        "USE: KAVIN|Hello"
      );

      return;
    }


    String destination =
      data.substring(
        0,
        separator
      );


    String text =
      data.substring(
        separator + 1
      );


    destination.trim();
    text.trim();


    if (
      destination.length() == 0 ||
      text.length() == 0
    )
    {
      Serial.println(
        "INVALID MESSAGE"
      );

      return;
    }


    // ========================================================
    // SEND
    // ========================================================

    sendEncryptedMessage(
      destination,
      text
    );
  }
};


// ============================================================
// DECRYPT PACKET
// ============================================================

bool decryptMessage(
  String packet,
  String &plaintext,
  String &source,
  String &destination
)
{
  packet.trim();


  // ==========================================================
  // FIND SEPARATORS
  //
  // E2E|SOURCE|DESTINATION|NONCE|CIPHER|TAG
  // ==========================================================

  int p1 =
    packet.indexOf('|');

  int p2 =
    packet.indexOf('|', p1 + 1);

  int p3 =
    packet.indexOf('|', p2 + 1);

  int p4 =
    packet.indexOf('|', p3 + 1);

  int p5 =
    packet.indexOf('|', p4 + 1);


  if (
    p1 < 0 ||
    p2 < 0 ||
    p3 < 0 ||
    p4 < 0 ||
    p5 < 0
  )
  {
    return false;
  }


  String type =
    packet.substring(
      0,
      p1
    );


  source =
    packet.substring(
      p1 + 1,
      p2
    );


  destination =
    packet.substring(
      p2 + 1,
      p3
    );


  String nonceHex =
    packet.substring(
      p3 + 1,
      p4
    );


  String cipherHex =
    packet.substring(
      p4 + 1,
      p5
    );


  String tagHex =
    packet.substring(
      p5 + 1
    );


  if (type != "E2E")
    return false;


  // ==========================================================
  // DESTINATION CHECK
  // ==========================================================

  if (
    !destination.equalsIgnoreCase(
      MY_NAME
    )
  )
  {
    return false;
  }


  if (
    nonceHex.length() !=
    GCM_NONCE_SIZE * 2
  )
  {
    return false;
  }


  if (
    tagHex.length() !=
    GCM_TAG_SIZE * 2
  )
  {
    return false;
  }


  if (
    cipherHex.length() == 0 ||
    cipherHex.length() % 2 != 0
  )
  {
    return false;
  }


  size_t cipherLength =
    cipherHex.length() / 2;


  uint8_t nonce[GCM_NONCE_SIZE];

  uint8_t tag[GCM_TAG_SIZE];


  uint8_t *ciphertext =
    new uint8_t[cipherLength];


  uint8_t *decrypted =
    new uint8_t[cipherLength + 1];


  if (
    !hexToBytes(
      nonceHex,
      nonce,
      GCM_NONCE_SIZE
    )
  )
  {
    delete[] ciphertext;
    delete[] decrypted;

    return false;
  }


  if (
    !hexToBytes(
      tagHex,
      tag,
      GCM_TAG_SIZE
    )
  )
  {
    delete[] ciphertext;
    delete[] decrypted;

    return false;
  }


  if (
    !hexToBytes(
      cipherHex,
      ciphertext,
      cipherLength
    )
  )
  {
    delete[] ciphertext;
    delete[] decrypted;

    return false;
  }


  // ==========================================================
  // AES-GCM
  // ==========================================================

  mbedtls_gcm_context gcm;

  mbedtls_gcm_init(&gcm);


  int result =
    mbedtls_gcm_setkey(
      &gcm,
      MBEDTLS_CIPHER_ID_AES,
      AES_KEY,
      256
    );


  if (result != 0)
  {
    mbedtls_gcm_free(&gcm);

    delete[] ciphertext;
    delete[] decrypted;

    return false;
  }


  result =
    mbedtls_gcm_auth_decrypt(
      &gcm,
      cipherLength,
      nonce,
      GCM_NONCE_SIZE,
      nullptr,
      0,
      tag,
      GCM_TAG_SIZE,
      ciphertext,
      decrypted
    );


  mbedtls_gcm_free(&gcm);


  if (result != 0)
  {
    delete[] ciphertext;
    delete[] decrypted;

    return false;
  }


  decrypted[cipherLength] =
    '\0';


  plaintext =
    String(
      (char *)decrypted
    );


  delete[] ciphertext;
  delete[] decrypted;


  return true;
}


// ============================================================
// SEND DATA TO PHONE
// ============================================================

void sendToPhone(
  String data
)
{
  if (!deviceConnected)
  {
    Serial.println(
      "PHONE NOT CONNECTED"
    );

    return;
  }

  txCharacteristic->setValue(
    data.c_str()
  );

  txCharacteristic->notify();
}


// ============================================================
// HANDLE LORA
// ============================================================

void handleLoRa()
{
  if (!E220Serial.available())
    return;


  ResponseContainer rc =
    e220ttl.receiveMessage();


  if (
    rc.status.code !=
    E220_SUCCESS
  )
  {
    Serial.println(
      "LORA RECEIVE ERROR"
    );

    return;
  }


  String packet =
    rc.data;

  packet.trim();


  if (packet.length() == 0)
    return;


  Serial.println();
  Serial.println(
    "========================================"
  );

  Serial.println(
    "ENCRYPTED PACKET RECEIVED"
  );

  Serial.println(
    "========================================"
  );

  Serial.println(packet);


  // ==========================================================
  // EXTRACT DESTINATION WITHOUT DECRYPTING
  // ==========================================================

  int p1 =
    packet.indexOf('|');

  int p2 =
    packet.indexOf('|', p1 + 1);

  int p3 =
    packet.indexOf('|', p2 + 1);


  if (
    p1 < 0 ||
    p2 < 0 ||
    p3 < 0
  )
  {
    Serial.println(
      "INVALID PACKET"
    );

    return;
  }


  String destination =
    packet.substring(
      p2 + 1,
      p3
    );

  destination.trim();


  // ==========================================================
  // IS THIS THE FINAL NODE?
  // ==========================================================

  if (
    destination.equalsIgnoreCase(
      MY_NAME
    )
  )
  {
    // ========================================================
    // FINAL DESTINATION
    // ========================================================

    Serial.println();
    Serial.println(
      "THIS NODE IS DESTINATION"
    );

    Serial.println(
      "STARTING DECRYPTION..."
    );


    String plaintext;
    String source;
    String finalDestination;


    bool success =
      decryptMessage(
        packet,
        plaintext,
        source,
        finalDestination
      );


    if (!success)
    {
      Serial.println();
      Serial.println(
        "DECRYPTION FAILED"
      );

      Serial.println(
        "WRONG KEY / CORRUPTED DATA"
      );

      return;
    }


    // ========================================================
    // SUCCESS
    // ========================================================

    Serial.println();
    Serial.println(
      "***************************************"
    );

    Serial.println(
      "DECRYPTION SUCCESS"
    );

    Serial.println(
      "***************************************"
    );

    Serial.print(
      "SOURCE: "
    );

    Serial.println(source);

    Serial.print(
      "DESTINATION: "
    );

    Serial.println(finalDestination);

    Serial.print(
      "DECRYPTED MESSAGE: "
    );

    Serial.println(plaintext);

    Serial.println(
      "***************************************"
    );


    // Send plaintext to phone

    sendToPhone(
      plaintext
    );

    return;
  }


  // ==========================================================
  // THIS IS AN INTERMEDIATE NODE
  // ==========================================================

  Serial.println();
  Serial.println(
    "THIS NODE IS INTERMEDIATE ROUTER"
  );


  String nextHop;


  if (
    !getNextHop(
      destination,
      nextHop
    )
  )
  {
    Serial.println(
      "NO ROUTE AVAILABLE"
    );

    return;
  }


  // ==========================================================
  // IMPORTANT:
  //
  // DO NOT DECRYPT
  //
  // Forward encrypted packet as-is.
  // ==========================================================

  Serial.println(
    "MESSAGE REMAINS ENCRYPTED"
  );


  Serial.print(
    "DESTINATION: "
  );

  Serial.println(destination);


  Serial.print(
    "NEXT HOP: "
  );

  Serial.println(nextHop);


  sendPacketToNode(
    nextHop,
    packet
  );


  Serial.println(
    "ENCRYPTED PACKET FORWARDED"
  );


  Serial.println(
    "========================================"
  );
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);


  // ==========================================================
  // E220
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


  digitalWrite(
    PIN_M0,
    LOW
  );

  digitalWrite(
    PIN_M1,
    LOW
  );


  delay(100);


  E220Serial.begin(
    9600,
    SERIAL_8N1,
    LORA_RX,
    LORA_TX
  );


  delay(500);


  e220ttl.begin();


  delay(500);


  // ==========================================================
  // DISPLAY
  // ==========================================================

  Serial.println();
  Serial.println(
    "========================================"
  );

  Serial.println(
    "4 NODE AES-256-GCM LORA MESH"
  );

  Serial.println(
    "========================================"
  );

  Serial.print(
    "NODE NAME: "
  );

  Serial.println(MY_NAME);

  Serial.print(
    "NODE ID: "
  );

  Serial.println(MY_NODE_ID);

  Serial.println(
    "CHANNEL: 18"
  );

  Serial.println(
    "ENCRYPTION: AES-256-GCM"
  );

  Serial.println(
    "ROUTING: ENABLED"
  );

  Serial.println(
    "========================================"
  );


  // ==========================================================
  // BLE
  // ==========================================================

  String bleName =
    String(MY_NAME) +
    "_CHAT";


  BLEDevice::init(
    bleName.c_str()
  );


  BLEServer *server =
    BLEDevice::createServer();


  server->setCallbacks(
    new ServerCallbacks()
  );


  BLEService *service =
    server->createService(
      SERVICE_UUID
    );


  // ESP32 -> PHONE

  txCharacteristic =
    service->createCharacteristic(
      TX_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_NOTIFY
    );


  txCharacteristic->addDescriptor(
    new BLE2902()
  );


  // PHONE -> ESP32

  BLECharacteristic *rxCharacteristic =
    service->createCharacteristic(
      RX_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_WRITE_NR
    );


  rxCharacteristic->setCallbacks(
    new RxCallbacks()
  );


  service->start();


  BLEAdvertising *advertising =
    BLEDevice::getAdvertising();


  advertising->addServiceUUID(
    SERVICE_UUID
  );


  advertising->setScanResponse(
    true
  );


  BLEDevice::startAdvertising();


  Serial.println();
  Serial.println(
    "NODE READY"
  );

  Serial.print(
    "BLE NAME: "
  );

  Serial.println(bleName);

  Serial.println(
    "========================================"
  );
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  handleLoRa();

  delay(5);
}