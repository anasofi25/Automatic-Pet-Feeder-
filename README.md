# 🐾 Automated Pet Feeder for Animals 🐶🐱

An **Automated Pet Feeder** controlled by an **ESP32 microcontroller** designed to simplify feeding schedules for pets. This project allows remote control and monitoring of food dispensing through a **web interface** while also monitoring food levels in real-time. It is an affordable, flexible, and customizable solution for pet owners.

---

## 🚀 Features

- **Automated Food Dispensing**: Dispenses food at regular intervals or on demand using a stepper motor.
- **Real-Time Food Monitoring**: Uses an **IR sensor** to check food levels in the bowl.
- **Remote Control**: Control the feeder via a web-based interface.
- **Food Status Display**: **LCD screen** shows the feeding schedule and food status.
- **Wi-Fi Connectivity**: Managed by the **ESP32** for remote control and real-time monitoring.

---

## 🔧 Hardware Requirements

- **ESP32 Microcontroller**: Controls Wi-Fi connectivity and web server functionality.
- **Stepper Motor**: Dispenses food.
- **IR Sensor**: Monitors food levels in the bowl.
- **LCD (LiquidCrystal I2C)**: Displays information on feeding schedules and food levels.
- **Power Supply**: Powers the system.
- **Other Components**: Breadboard, jumper wires, resistors, etc.

---

## 🛠️ Circuit Design

The circuit connects the **ESP32** to:
- **Stepper Motor**: Pins 27, 26, 25, and 33.
- **IR Sensor**: Pin 14.
- **LCD**: SDA (Pin 21) and SCL (Pin 22).

---

## 💻 Software Overview

### Web Server
- Hosts a web page to control the motor and display food levels.
- Uses **HTTP requests** to trigger motor rotation or update food status.

### Motor Control
- Implements stepper motor rotation using a state machine.
- The motor rotates for **2 seconds** when triggered.

### Timer Functionality
- Automatically triggers the motor every **12 hours** for a scheduled feeding.

### Food Monitoring
- Uses **IR sensor data** to determine if the food level is sufficient.
- Displays food status on the **LCD** and **web interface**.

### Debugging
- Ensured **precise motor timing**.
- Calibrated the **IR sensor** for accurate readings.
- Improved **Wi-Fi connectivity** to minimize disconnects.

---


