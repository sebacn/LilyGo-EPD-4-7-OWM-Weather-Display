# LilyGo EPD 4.7" Weather Display - Complete Documentation

## Table of Contents
1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Requirements](#software-requirements)
4. [Installation & Setup](#installation--setup)
5. [Configuration](#configuration)
6. [Features](#features)
7. [API Integration](#api-integration)
8. [Display Modes](#display-modes)
9. [Power Management](#power-management)
10. [Troubleshooting](#troubleshooting)
11. [Customization](#customization)
12. [License](#license)

## Overview

The LilyGo EPD 4.7" Weather Display is an ESP32-based weather station that displays current weather conditions, forecasts, and air quality data on a 4.7-inch e-paper display. The device fetches weather data from OpenWeatherMap API and presents it in an easy-to-read format with minimal power consumption.

### Key Features
- **4.7-inch E-Paper Display**: High-resolution, low-power display
- **WiFi Connectivity**: Automatic WiFi configuration via captive portal
- **Weather Data**: Current conditions, hourly forecasts, and daily predictions
- **Air Quality Index**: Real-time AQI data display
- **Power Efficient**: Deep sleep mode with configurable wake intervals
- **Multi-language Support**: Internationalization support
- **Customizable UI**: Configurable display layouts and themes

## Hardware Requirements

### Required Components
- **LilyGo T5-4.7 E-Paper Development Board**
  - ESP32 microcontroller
  - 4.7-inch E-Paper display (800x480 resolution)
  - Built-in WiFi and Bluetooth
  - PSRAM support
  - Battery management

### Optional Components
- **3D Printed Case**: Available in the `LilyGo 4.7 EPD case (+stand) - 5638478/` directory
- **Power Supply**: USB-C cable for programming and power
- **Battery**: For portable operation

## Software Requirements

### Development Environment
- **PlatformIO**: Recommended IDE for ESP32 development
- **Arduino Framework**: Version 2.0 or higher
- **ESP32 Board Package**: Version 6.11.0 or higher

### Required Libraries
```ini
# Core Libraries
- LilyGo-EPD47 (from GitHub)
- ArduinoJson@7.2.1
- WiFiManager@2.0.17
- RTClib@2.1.4
- InfluxDB-Client-for-Arduino
- QRCode@^0.0.1
```

### Build Configuration
- **Board**: ESP32 Dev Module with PSRAM Enabled
- **Flash Size**: 16MB
- **Stack Size**: 16384 bytes (minimum)
- **CPU Frequency**: 240MHz

## Installation & Setup

### 1. Clone the Repository
```bash
git clone https://github.com/your-repo/LilyGo-EPD-4-7-OWM-Weather-Display.git
cd LilyGo-EPD-4-7-OWM-Weather-Display
```

### 2. Install PlatformIO
```bash
pip install platformio
```

### 3. Install Dependencies
```bash
pio lib install
```

### 4. Configure Credentials
Edit `owm_credentials.h` and set your API keys and location:

```cpp
// WiFi Configuration
String WiFiSSID = "Your_WiFi_SSID";
String WiFiPass = "Your_WiFi_Password";

// API Keys
String OwmApikey = "Your_OpenWeatherMap_API_Key";
String TimezBBKey = "Your_TimeZoneDB_API_Key";
String PositionStackKey = "Your_PositionStack_API_Key";

// Location Settings
String City = "Your_City";
String Latitude = "51.38";
String Longitude = "-2.36";
```

### 5. Compile and Upload
```bash
pio run -t upload
```

## Configuration

### API Keys Setup

#### OpenWeatherMap API
1. Sign up at [OpenWeatherMap](https://openweathermap.org/)
2. Get your free API key
3. Add the key to `owm_credentials.h`

#### TimeZoneDB API (Optional)
1. Sign up at [TimeZoneDB](https://timezonedb.com/)
2. Get your API key for accurate timezone data

#### PositionStack API (Optional)
1. Sign up at [PositionStack](https://positionstack.com/)
2. Get your API key for geocoding services

### Location Configuration

#### Method 1: City Name
```cpp
String City = "London,UK";
```

#### Method 2: Coordinates
```cpp
String Latitude = "51.5074";
String Longitude = "-0.1278";
```

### Display Settings

#### Units
```cpp
String Units = "M";  // M for Metric, I for Imperial
```

#### Language
```cpp
String Language = "EN";  // EN, FR, DE, ES, etc.
```

#### Sleep Configuration
```cpp
long SleepDuration = 60;    // Update interval in minutes
int WakeupHour = 8;         // Start updates at 8 AM
int SleepHour = 23;         // Stop updates at 11 PM
```

## Features

### Weather Display
- **Current Conditions**: Temperature, humidity, pressure, wind
- **Weather Icons**: Visual representation of weather conditions
- **Feels Like Temperature**: Apparent temperature calculation
- **UV Index**: Sun protection information
- **Precipitation**: Rain and snow data

### Forecast Information
- **Hourly Forecast**: Next 24 hours of weather
- **Daily Forecast**: 5-day weather predictions
- **Temperature Range**: High and low temperatures
- **Precipitation Probability**: Chance of rain/snow

### Air Quality
- **AQI Display**: Real-time air quality index
- **Health Recommendations**: Based on AQI levels
- **Pollutant Levels**: PM2.5, PM10, O3, etc.

### System Features
- **WiFi Manager**: Captive portal for easy WiFi setup
- **QR Code Display**: Easy WiFi configuration
- **Battery Monitoring**: Voltage and percentage display
- **Error Handling**: Graceful failure modes
- **Logging**: Debug and error logging

## API Integration

### OpenWeatherMap API
The device uses OpenWeatherMap's One Call API 3.0 for comprehensive weather data:

```cpp
// API Endpoints
- Current weather: /data/3.0/onecall
- Geocoding: /geo/1.0/direct
- Timezone: /v2.1/get-time-zone
```

### Data Structure
```cpp
struct WeatherRequest {
    WeatherResponseCurrent current;
    WeatherResponseDaily daily[5];
    WeatherResponseHourly hourly[24];
};
```

### Error Handling
- **Network Timeouts**: Configurable retry mechanisms
- **API Limits**: Respects rate limiting
- **Data Validation**: JSON parsing error handling
- **Fallback Modes**: Offline display capabilities

## Display Modes

### 1. Normal Mode
- Current weather conditions
- Hourly forecast
- Air quality data
- System status

### 2. Configuration Mode
- WiFi setup interface
- QR code for easy connection
- Settings display

### 3. Validation Mode
- API connection testing
- Data verification
- Error reporting

### 4. Failed Mode
- Error display
- Retry information
- Manual reset instructions

## Power Management

### Sleep Strategy
The device uses deep sleep to conserve battery:

```cpp
// Sleep Configuration
long SleepDuration = 60;    // Minutes between updates
int WakeupHour = 8;         // Start time
int SleepHour = 23;         // End time
```

### Wake-up Sources
- **Timer**: Scheduled updates
- **Button Press**: Manual wake-up
- **External Trigger**: GPIO wake-up

### Power Optimization
- **PSRAM Usage**: Efficient memory management
- **WiFi Power Saving**: Automatic WiFi sleep
- **Display Refresh**: Minimal e-paper updates
- **CPU Frequency**: Dynamic clock adjustment

## Troubleshooting

### Common Issues

#### 1. WiFi Connection Problems
**Symptoms**: Device stuck in configuration mode
**Solutions**:
- Check WiFi credentials in `owm_credentials.h`
- Ensure WiFi network is 2.4GHz (not 5GHz)
- Verify signal strength

#### 2. API Connection Issues
**Symptoms**: No weather data displayed
**Solutions**:
- Verify API keys are correct
- Check internet connectivity
- Ensure API quota not exceeded

#### 3. Display Issues
**Symptoms**: Blank or corrupted display
**Solutions**:
- Check PSRAM is enabled
- Verify board selection (T5-ePaper)
- Increase stack size if needed

#### 4. Compilation Errors
**Solutions**:
```bash
# Clean and rebuild
pio run -t clean
pio run

# Update libraries
pio lib update
```

### Debug Mode
Enable debug logging in `owm_credentials.h`:
```cpp
bool DbgLogEnable = true;
```

### Serial Monitor
```bash
pio device monitor
```

## Customization

### Adding New Languages
1. Create language file in `lang/` directory
2. Add language constants to `lang.h`
3. Update language selection logic

### Custom Weather Icons
1. Convert images to header files using `imgconvert.py`
2. Add icon definitions to `imgs/` directory
3. Update display logic

### Display Layout
Modify the main display functions in `OWM_EPD47_epaper_v2.72.ino`:
- `DisplayWeatherSection()`
- `DisplayForecastSection()`
- `DisplayAirQualitySection()`

### Font Customization
Add new fonts to `Font Files/` directory:
```cpp
#include "fonts/your_font.h"
```

### Color Schemes
Modify color definitions:
```cpp
#define White         0xFF
#define LightGrey     0xBB
#define Grey          0x88
#define DarkGrey      0x44
#define Black         0x00
```

## File Structure

```
LilyGo-EPD-4-7-OWM-Weather-Display/
├── OWM_EPD47_epaper_v2.72.ino    # Main application
├── config.h                        # Configuration management
├── owm_credentials.h              # API keys and settings
├── api_request.h                  # HTTP request handling
├── forecast_record.h              # Weather data structures
├── lang.h                         # Internationalization
├── units.h                        # Unit conversion utilities
├── aqi_metric.h                   # Air quality calculations
├── loginfo.h                      # Logging system
├── platformio.ini                 # Build configuration
├── assets/                        # Images and documentation
├── fonts/                         # Font files
├── imgs/                          # Weather icons
└── boards/                        # Board configurations
```

## License

This project is licensed under the **GPLv3 License** due to its use of the LilyGo-EPD47 library which is also GPLv3 licensed.

### License Compliance
- **Original Author**: G6EJD (https://github.com/G6EJD/)
- **License**: GPLv3
- **Attribution**: All original copyrights and attributions are preserved

### GPLv3 Requirements
- Source code must be available
- Modifications must be licensed under GPLv3
- License and copyright notices must be preserved

## Support and Contributing

### Getting Help
1. Check the troubleshooting section
2. Review existing issues on GitHub
3. Create a new issue with detailed information

### Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

### Development Guidelines
- Follow existing code style
- Add comments for complex logic
- Test changes thoroughly
- Update documentation as needed

---

**Version**: 2.7 / 4.7in  
**Last Updated**: December 2024  
**Maintainer**: Community-driven project 