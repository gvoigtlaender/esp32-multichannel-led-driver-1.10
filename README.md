## This is led controller firmware.

Device use ESP32 as main CPU and MCP7940 as RTC.
Communication between processor realised by I2C bus.

## Web UI Auth
http://192.168.4.1 or http://ledcontroller.local

Login: admin  
Password: 12345678

## Build firmware
1. Download sdk and toolchain and setup [instruction](https://github.com/espressif/esp-idf)
2. erase device `idf.py erase_flash`
2. build firmware and flash `idf.py flash`

## OTA
1. build firmware `idf.py build`
2. Goto Settings -> Identity
3. Select firmware file
4. Press Upload