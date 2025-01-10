# -STM32L0_Wi-Fi_Driver
This project provides a bare-metal Wi-Fi driver for STM32L0 series microcontrollers, utilizing pure register programming. It facilitates network connectivity through an ESP32 module running the AT command firmware, with communication handled via UART.

## Features
- **State Machine Management**: Employs a state machine to manage network operations, including:
      - Checking network connectivity
      - Establishing UDP connections
      - Sending messages
      - Receiving server responses
      - Closing connections
- **Ease of Integration**: Designed as a stable base project for developers to extend. Easily customizable Wi-Fi credentials and server IP address.
- **Potential for Expansion**: Future enhancements could include MQTT support, broadening applicability in IoT development.

## Getting Started
1. **Clone the Repository**:
```
   git clone https://gitlab.com/nikosgri/stm32l0_wi-fi.git
```
2. **Configure Wi-Fi and Server Settings**:
- Modify the Wi-Fi SSID and password in the configuration file.
- Set the target server's IP address for network communication.
3. **Build and Deploy**:
- Compile the code using your preferred toolchain for STM32L0 microcontrollers.
- Flash the firmware onto the STM32L0 device.
4. **Extend Functionality**:
- Integrate additional sensors or modules as needed.
- Incorporate sensor data into the payload for transmission.**

## Contributing
Contributions are welcome. Feel free to fork the repository and submit pull requests for enhancements or bug fixes.

## Contact

For any inquiries or support, please contact the project maintainer, Nick Grigoriadis, at:
```
n.grigoriadis09@gmail.com
```
