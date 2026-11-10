# 🌱 Automatic Plant Watering System Using STM32 Blue Pill

## 📌 Project Overview

This project develops an **Automatic Plant Watering System** using the STM32 Blue Pill microcontroller.

The system monitors soil moisture and controls a water pump to support plant watering. It also integrates environmental monitoring, a real-time clock, and an LCD display to provide useful information to users.

The project aims to apply knowledge of embedded systems, sensor interfacing, and microcontroller programming to a practical agricultural application.

## 🎯 Project Objectives

- Design and implement an automatic plant watering system using STM32.
- Monitor soil moisture using a soil moisture sensor.
- Control a water pump through a relay module.
- Display system information on an LCD.
- Read temperature and humidity using a DHT11 sensor.
- Integrate a DS3231 real-time clock.
- Develop and test the embedded control program.

## 🔧 Hardware Components

| No. | Component | Function |
|---|---|---|
| 1 | STM32 Blue Pill | Main microcontroller |
| 2 | Soil Moisture Sensor | Measures soil moisture |
| 3 | DHT11 Sensor | Measures temperature and air humidity |
| 4 | DS3231 RTC Module | Provides real-time clock information |
| 5 | LCD 1602 with I2C | Displays system information |
| 6 | 5V Relay Module | Controls the water pump |
| 7 | Submersible Water Pump | Supplies water to plants |

## ⚙️ System Features

### 1. Automatic Watering
The system monitors soil moisture and controls the water pump based on configured watering conditions.

### 2. Manual Watering
The water pump can be controlled manually through the system's input controls.

### 3. Environmental Monitoring
The DHT11 sensor collects temperature and air humidity data for monitoring and display.

### 4. Real-Time Clock
The DS3231 module provides date and time information for time-related functions.

### 5. LCD Display
The LCD 1602 displays relevant information, such as sensor readings and system status.

## 🔄 System Operation

The general operating process is as follows:

1. Initialize the STM32 microcontroller and connected peripherals.
2. Read data from the soil moisture sensor.
3. Obtain temperature and humidity information from the DHT11.
4. Read date and time information from the DS3231.
5. Evaluate the configured watering conditions.
6. Activate or deactivate the water pump through the relay.
7. Update information on the LCD display.
8. Repeat the monitoring and control process.

## 💻 Development Tools

- STM32CubeIDE
- STM32CubeMX
- STM32 HAL Library
- Embedded C
- Git and GitHub
- KiCad (circuit schematic design)

## 📂 Project Structure

The repository is intended to organize the following materials:

```text
Automatic-Plant-Watering-System/
│
├── Core/
│   ├── Inc/
│   └── Src/
│
├── Drivers/
│
├── Documentation/
│
├── Hardware/
│
├── README.md
│
└── STM32_Project.ioc
```

*Note: The actual repository structure may differ depending on the uploaded STM32CubeIDE project files.*

## 🧪 Testing and Evaluation

The system is evaluated based on the following criteria:

- Soil moisture sensor response
- Water pump activation and deactivation
- Manual watering operation
- DHT11 sensor readings
- RTC communication
- LCD information display
- Overall system functionality

Testing results and supporting evidence will be included in the project documentation.

## 👥 Development Team

**Group 3 – Automatic Plant Watering System**

| No. | Member Name | Student ID | Role |
|---|---|---|---|
| 1 | To be updated | To be updated | Group Leader |
| 2 | To be updated | To be updated | Team Member |
| 3 | To be updated | To be updated | Team Member |
| 4 | To be updated | To be updated | Team Member |

## 📚 Project Documentation

The repository may include:

- Source code
- Circuit schematic
- Hardware connection diagrams
- Project report
- Testing results
- Photos of the implemented system

## 📝 Conclusion

This project demonstrates the application of STM32 microcontrollers in an automatic plant watering system.

By integrating sensors, a real-time clock, an LCD, and relay-based pump control, the project provides practical experience in embedded hardware and software development.

---

**Group 3 | Automatic Plant Watering System | STM32 Blue Pill**
