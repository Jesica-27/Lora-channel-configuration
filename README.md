# LoRa Track Chat
### Offline LoRa Mesh Communication and Tracking System with Secure Multi-Hop Routing

## 📌 Project Overview

**LoRa Track Chat** is an offline communication and tracking system designed to enable users to exchange messages and share location information without relying on the internet, Wi-Fi, or a LoRaWAN gateway.

The system uses ESP32-WROOM microcontrollers and EBYTE E220 LoRa modules for wireless communication between nodes. A Flutter-based mobile application connects to each ESP32 node through Bluetooth Low Energy (BLE), allowing users to send and receive messages through the LoRa network.

The project focuses on reliable offline communication, private messaging, node identification, and multi-hop routing for situations where conventional network connectivity is unavailable.

## 🎯 Objectives

- Develop an offline messaging system using LoRa technology.
- Enable communication between multiple nodes without internet connectivity.
- Integrate a Flutter mobile application with ESP32 using BLE.
- Implement node identification and destination-based message delivery.
- Support multi-hop message forwarding through intermediate nodes.
- Explore secure message transmission and node-health monitoring.

## ✨ Key Features

- **Offline Communication:** Operates without internet access, Wi-Fi, or a LoRaWAN gateway.
- **Long-Range Communication:** Uses LoRa radio technology for communication between nodes.
- **Mobile Application:** Provides a user interface through a Flutter application.
- **BLE Integration:** Enables communication between the smartphone and ESP32 node.
- **Private Messaging:** Uses an encrypted message format for end-to-end communication.
- **Multi-Hop Routing:** Supports forwarding messages through intermediate nodes when routing is implemented and enabled.
- **Node Identification:** Uses unique node IDs to identify communication endpoints.
- **Signal Monitoring:** Supports RSSI-based observations to evaluate radio-link quality.

## 🛠️ Hardware Requirements

- ESP32-WROOM development board
- EBYTE E220 LoRa transceiver modules
- Android smartphone
- Antennas compatible with the selected LoRa module
- USB cables and connecting wires

## 💻 Software and Technologies

- Embedded C/C++ for ESP32 firmware
- Flutter and Dart for mobile application development
- Bluetooth Low Energy (BLE)
- LoRa wireless communication
- UART serial communication
- Arduino IDE or ESP32 development toolchain
- Visual Studio Code / Android Studio

## 🏗️ System Architecture

The system consists of three main layers:

1. **Mobile Application:** The Flutter application provides the interface for sending and receiving messages.
2. **Embedded Node:** The ESP32 processes application data and communicates with the LoRa module through UART.
3. **LoRa Communication Network:** EBYTE E220 modules transmit data between nodes, with intermediate-node forwarding handled by the routing firmware.

### Communication Flow

1. The user enters a message in the Flutter application.
2. The application sends the message to the ESP32 through BLE.
3. The ESP32 processes the message and transfers the required data to the EBYTE E220 module through UART.
4. The LoRa module transmits the data to the intended destination node.
5. Intermediate nodes can forward messages when supported by the routing logic.
6. The receiving ESP32 delivers the message to its connected mobile application through BLE.

## 🔌 Example Hardware Configuration

| Parameter | Configuration |
|---|---|
| Microcontroller | ESP32-WROOM |
| LoRa Module | EBYTE E220 |
| UART Interface | UART2 |
| ESP32 RX Pin | GPIO 16 |
| ESP32 TX Pin | GPIO 17 |
| M0 Pin | GPIO 25 |
| M1 Pin | GPIO 26 |
| AUX Pin | GPIO 27 |
| UART Baud Rate | 9600 |
| UART Format | 8N1 |
| Example Channel | 18 |
| Example Node IDs | 1, 2, 3, 4 |

*Note: Verify these values against the current firmware and exact E220 module variant before deployment.*

## 🔐 Secure Messaging

The project uses an end-to-end message format structured as:

`E2E | nonce | ciphertext | authentication tag`

- **E2E:** Identifies the end-to-end message format.
- **Nonce:** Provides a value required by the encryption protocol.
- **Ciphertext:** Contains the encrypted message.
- **Authentication Tag:** Enables integrity and authenticity verification.

The encryption algorithm, key management, nonce generation, and authentication checks must be correctly implemented and tested to provide actual security. Secret keys and credentials should never be committed to a public repository.

## 🚀 Getting Started

### 1. Clone the Repository

```bash
git clone <YOUR_GITHUB_REPOSITORY_URL>
cd <YOUR_REPOSITORY_FOLDER>
```

### 2. Configure the ESP32 Firmware

1. Open the firmware project in your ESP32 development environment.
2. Install the required board support packages and libraries.
3. Connect the EBYTE E220 module to the ESP32 using the configured UART and control pins.
4. Configure the node ID and compatible LoRa parameters.
5. Compile and upload the firmware to each ESP32 node.
6. Verify communication through the serial monitor.

### 3. Run the Flutter Application

Install Flutter and the required Android development tools, then open the mobile application project.

Run the following commands:

```bash
flutter pub get
flutter run
```

Enable Bluetooth on the smartphone and grant the necessary permissions to connect to the ESP32 node.

*Note: The commands assume that the terminal is opened inside the Flutter application's directory.*

## 🧪 Testing and Validation

The system can be evaluated through the following tests:

- BLE communication between the smartphone and ESP32.
- UART communication between ESP32 and EBYTE E220.
- Message transmission between addressed nodes.
- Communication testing between selected nodes.
- RSSI measurement and radio-link quality evaluation.
- Multi-hop forwarding and message-delivery verification.
- Message integrity and authentication testing.

Communication range, latency, packet delivery rate, and power consumption should be measured under documented test conditions.

## 🌍 Applications

- Emergency communication during network outages
- Remote-area communication
- Outdoor activities and field coordination
- Disaster-response communication prototypes
- Offline messaging in areas with limited connectivity
- Research on LoRa networking and embedded wireless systems

## 🔮 Future Enhancements

- Improve multi-hop route selection and node-failure detection.
- Add message acknowledgements and automatic retransmission.
- Integrate GPS-based location sharing.
- Improve node-health monitoring using RSSI, latency, battery level, and packet loss.
- Enhance mobile application usability and node-status visualization.
- Evaluate network performance under different environmental conditions.
- Strengthen security testing and replay-attack protection.

## 📌 Conclusion

LoRa Track Chat explores a practical approach to offline communication by combining ESP32, LoRa, BLE, and Flutter technologies. By reducing dependence on conventional network infrastructure, the project provides a foundation for developing portable communication systems for remote environments and emergency scenarios.

The project also offers opportunities to explore embedded firmware development, wireless networking, mobile application integration, secure messaging, and multi-hop communication.

## 👩‍💻 Project Domain

**Embedded Systems | IoT | LoRa Communication | BLE | Flutter | Secure Wireless Communication**
