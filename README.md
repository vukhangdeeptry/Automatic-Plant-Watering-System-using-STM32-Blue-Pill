
#  Automatic Plant Watering System Based on STM32 Blue Pill

**Group 3 | MCP201 | FPT University Ho Chi Minh City | 2026**

##  1. Project Overview

This project presents the design and implementation of an **Automatic Plant Watering System using STM32 Blue Pill (STM32F103)**.

The system monitors soil moisture and automatically controls a submersible water pump based on predefined moisture thresholds. It also supports scheduled watering using a DS3231 Real-Time Clock and manual watering through push buttons.

A DHT11 sensor monitors ambient temperature and humidity, while an LCD 1602 with I2C interface displays system information.

The project is developed for educational purposes to demonstrate practical applications of embedded systems in plant care.

##  2. Team Members

**Group 3 – MCP201**

| No. | Full Name | Student ID |
|---|---|---|
| 1 | Hoàng Anh Tuấn | SE205206 |
| 2 | Nguyễn Cao Khánh | SE205076 |
| 3 | Nguyễn Minh Khôi | SE201660 |
| 4 | Ngô Trần Vũ Khang | DE200076 |

**University:** FPT University – Ho Chi Minh City  
**Course:** MCP201  
**Instructor:** VinhPT30  
**Academic Year:** 2026

##  3. Project Objectives

- Design and implement a plant watering system using STM32.
- Measure soil moisture through an analog sensor.
- Automatically activate and deactivate the water pump.
- Support scheduled watering using an RTC module.
- Allow manual pump operation.
- Monitor environmental temperature and humidity.
- Display real-time information on an LCD.
- Store configuration settings in Flash memory.
- Improve operational reliability through software protection mechanisms.

##  4. Hardware Components

| No. | Component | Function |
|---|---|---|
| 1 | STM32 Blue Pill (STM32F103) | Main controller |
| 2 | Soil Moisture Sensor | Measures relative soil moisture |
| 3 | DHT11 Sensor | Measures air temperature and humidity |
| 4 | DS3231 RTC Module | Provides real-time information |
| 5 | LCD 1602 I2C | Displays system data and settings |
| 6 | 5V Relay Module | Controls the water pump circuit |
| 7 | Submersible Water Pump | Supplies water to plants |
| 8 | Four Push Buttons | MENU, UP, DOWN and PUMP |
| 9 | On-board LED | Indicates system activity |

##  5. Main Features

### 5.1. Automatic Watering

The system measures soil moisture and controls the pump using two thresholds.

- Pump ON when soil moisture is below the Start threshold.
- Pump OFF when soil moisture reaches the Stop threshold.
- Uses hysteresis to reduce frequent relay switching.

Default settings:
- Start threshold: 30%
- Stop threshold: 60%

### 5.2. Scheduled Watering

The DS3231 RTC provides real-time clock information.

The pump can automatically operate at a configured time.

Default schedule:
- Watering time: 07:00
- Watering duration: 10 seconds

### 5.3. Manual Watering

Users can control timed watering through the PUMP button.

- Press PUMP to start timed watering.
- Press again to cancel the timed watering request.

Automatic watering may continue if its conditions are still satisfied.

### 5.4. Environmental Monitoring

The DHT11 sensor measures:

- Air temperature
- Relative air humidity

These values are displayed on the LCD and do not directly determine pump operation.

### 5.5. LCD Display and Settings

The LCD 1602 displays:

- Soil moisture percentage
- Temperature and air humidity
- Real-time clock information
- Pump operating status
- Watering schedule
- Moisture thresholds
- Sensor calibration values

The MENU, UP and DOWN buttons allow users to adjust system settings.

### 5.6. Protection Mechanisms

The firmware includes:

- Maximum automatic watering duration of 60 seconds.
- A 300-second cooldown when the automatic watering time limit is reached.
- Independent Watchdog (IWDG).
- I2C communication recovery.
- Configuration storage in STM32 Flash memory.

##  6. Hardware Pin Configuration

| Component | STM32 Pin | Description |
|---|---|---|
| Soil Sensor AO | PA2 | ADC1 Channel 2 |
| Soil Sensor Power | PB0 | Sensor power control |
| DHT11 DATA | PA1 | Digital data communication |
| Relay IN | PA0 | Water pump control |
| RTC / LCD SCL | PB6 | I2C1 Clock |
| RTC / LCD SDA | PB7 | I2C1 Data |
| MENU Button | PB12 | Menu navigation |
| UP Button | PB13 | Increase parameter |
| DOWN Button | PB14 | Decrease parameter |
| PUMP Button | PB15 | Manual watering |
| Status LED | PC13 | System status |

I2C addresses:
- DS3231: 0x68
- LCD: 0x27 or 0x3F

**Note:** Ensure correct voltage compatibility between the STM32, relay module and other components. The water pump must use an appropriately rated power supply.

##  7. System Operating Principle

The system operates through the following process:

1. Initialize STM32 peripherals.
2. Read the soil moisture sensor through ADC1.
3. Convert the ADC value into relative soil moisture percentage.
4. Read DHT11 temperature and humidity.
5. Read the current time from DS3231.
6. Evaluate automatic and scheduled watering conditions.
7. Process manual watering requests.
8. Control the relay and water pump.
9. Display information on the LCD.
10. Repeat the monitoring and control cycle.

### Soil Moisture Calculation

The firmware converts ADC readings into relative moisture percentage:

```text
soil_percent = clamp((raw - dry) * 100 / (wet - dry), 0, 100)
```

Default calibration values:

- Dry ADC value: 4000
- Wet ADC value: 1500

Actual calibration is necessary because readings depend on soil conditions and sensor characteristics.

## 8. Software Development

**Programming Language:** Embedded C

**Development Environment:**
- STM32CubeIDE
- STM32 HAL Library
- KiCad for circuit schematic design
- GitHub for source code management

**STM32 Peripherals:**
- GPIO
- ADC1
- I2C1
- TIM2
- Flash Memory
- Independent Watchdog

**System Clock:** 72 MHz

**TIM2:** 1 MHz timing reference for DHT11 communication.

##  9. Basic Setup

1. Download or clone the repository.
2. Open the STM32CubeIDE project.
3. Verify hardware connections and power requirements.
4. Build the firmware.
5. Flash the program to STM32 Blue Pill.
6. Check and configure the DS3231 RTC time.
7. Calibrate the soil moisture sensor.
8. Configure watering thresholds and schedule.
9. Test automatic, scheduled and manual watering.

##  10. Testing and Results

The project report describes eight functional test scenarios.

| Test ID | Test Scenario | Reported Result |
|---|---|---|
| T1 | Soil moisture below Start threshold | Passed |
| T2 | Soil moisture reaches Stop threshold | Passed |
| T3 | Automatic watering exceeds 60 seconds | Passed |
| T4 | RTC reaches scheduled watering time | Passed |
| T5 | Manual PUMP button operation | Passed |
| T6 | Configuration recovery after reset | Passed |
| T7 | DHT11 communication error | Passed |
| T8 | LCD / I2C communication recovery | Passed |

### Experimental Measurements

| ADC Raw | Soil Moisture | RTC Time | Pump Status |
|---|---|---|---|
| 3800 | 8% | 14:05:12 | ON |
| 2000 | 80% | 14:07:40 | OFF |
| 2400 | 64% | 14:12:25 | OFF |
| 2800 | 48% | 14:15:03 | ON (Manual) |

The reported results show that the system responded to the tested watering conditions.

However, long-term operation and durability have not yet been fully evaluated.

##  11. Limitations

- Soil moisture measurements require calibration.
- The resistive soil sensor may experience electrode corrosion.
- The system does not include a water-level sensor.
- Pump dry-running cannot be detected directly.
- DHT11 readings are used for monitoring only.
- Long-term reliability has not been fully verified.

##  12. Future Development

Potential improvements include:

- Replacing the resistive sensor with a capacitive soil moisture sensor.
- Adding a water-level sensor.
- Integrating Wi-Fi connectivity.
- Developing a mobile monitoring interface.
- Implementing data logging.
- Adding remote notifications.
- Supporting multiple watering zones.

##  13. Documentation

Detailed information about hardware design, software implementation, circuit schematics, testing and evaluation is provided in the project report:

**REPORT_STM32AutomaticWateringGarden_Group3.docx**

##  14. Conclusion

The Automatic Plant Watering System demonstrates the practical application of STM32 microcontrollers in embedded control systems.

By integrating soil moisture monitoring, scheduled watering, manual control, environmental monitoring and relay-based pump operation, the project provides a functional prototype for small-scale plant care.

The system also provides practical experience in microcontroller programming, sensor interfacing, embedded system design and hardware integration.

---

**Group 3 – MCP201**  
**FPT University Ho Chi Minh City – 2026**

**Automatic Plant Watering System Based on STM32 Blue Pill**
