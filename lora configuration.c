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
  Serial.println("E220 868 MHz CONFIGURATION");
  Serial.println("================================");

  e220ttl.begin();

  delay(500);

  // Read current configuration
  ResponseStructContainer c = e220ttl.getConfiguration();

  if (c.status.code != 1) {
    Serial.println("Failed to read configuration.");
    c.close();
    return;
  }

  Configuration configuration = *(Configuration*)c.data;

  // 868 MHz E220-900 configuration
  configuration.ADDH = 0;
  configuration.ADDL = 0;

  // Channel 18 = 868.125 MHz
  configuration.CHAN = 18;

  // 2.4 kbps air data rate
  configuration.SPED.airDataRate = AIR_DATA_RATE_000_24;

  // 9600 baud, 8N1
  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.uartParity = MODE_00_8N1;

  // 22 dBm transmit power
  configuration.OPTION.transmissionPower = POWER_22;

  // Transparent transmission
  configuration.TRANSMISSION_MODE.fixedTransmission = FT_TRANSPARENT_TRANSMISSION;

  ResponseStatus rs = e220ttl.setConfiguration(
      configuration,
      WRITE_CFG_PWR_DWN_SAVE
  );

  Serial.println(rs.getResponseDescription());

  c.close();

  delay(1000);

  // Read configuration again
  ResponseStructContainer check = e220ttl.getConfiguration();

  if (check.status.code == 1) {

    Configuration verify = *(Configuration*)check.data;

    Serial.println();
    Serial.println("----- FINAL CONFIGURATION -----");

    Serial.print("Address High: ");
    Serial.println(verify.ADDH);

    Serial.print("Address Low: ");
    Serial.println(verify.ADDL);

    Serial.print("Channel: ");
    Serial.println(verify.CHAN);

    Serial.println("Frequency: 868.125 MHz");

    Serial.println("Air Data Rate: 2.4 kbps");
    Serial.println("UART: 9600 8N1");
    Serial.println("Power: 22 dBm");

    Serial.println("-------------------------------");
    Serial.println("Configuration completed!");

  } else {
    Serial.println("Verification failed.");
  }

  check.close();
}

void loop() {
}