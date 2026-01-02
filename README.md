# Localizer

ESP-IDF project for Localizer application.

## Prerequisites

- ESP-IDF installed and configured
- Supported ESP32 target (ESP32, ESP32-S2, ESP32-S3, ESP32-C3, etc.)

## Build and Flash

1. Set the target (replace esp32 with your chip):
   ```
   idf.py set-target esp32
   ```

2. Configure the project:
   ```
   idf.py menuconfig
   ```

3. Build the project:
   ```
   idf.py build
   ```

4. Flash to device:
   ```
   idf.py -p PORT flash monitor
   ```

   Replace PORT with your device's serial port (e.g., COM3 on Windows).

## Project Structure

- `main/` - Main application code
- `CMakeLists.txt` - Project build configuration
- `sdkconfig.defaults` - Default SDK configuration
