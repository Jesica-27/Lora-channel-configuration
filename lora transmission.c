#include <LoRa_E220.h>

#define TX_PIN 17
#define RX_PIN 16
#define AUX_PIN 27
#define M0_PIN 25
#define M1_PIN 26

HardwareSerial E220Serial(2);

LoRa_E220 e220ttl(&E220Serial, AUX_PIN, M0_PIN, M1_PIN);

void setup() {
  Serial.begin(115200);

  E220Serial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  delay(1000);

  Serial.println("================================");
  Serial.println("NODE 1 - LORA SENDER");
  Serial.println("================================");

  e220ttl.begin();

  delay(500);
}

void loop() {

  String message = "Hello from Node 1";

  ResponseStatus rs = e220ttl.sendMessage(message);

  Serial.print("Sending: ");
  Serial.println(message);

  Serial.print("Status: ");
  Serial.println(rs.getResponseDescription());

  delay(2000);
}
