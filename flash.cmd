@echo off
make
echo Port %1
if %ERRORLEVEL% EQU 0 (
	python C:/Espressif/esp8266_stuff/esptool.py --port %1 write_flash 0x00000 firmware/0x00000.bin
	sleep 1
)
if %ERRORLEVEL% EQU 0 (
	python C:/Espressif/esp8266_stuff/esptool.py --port %1 write_flash 0x40000 firmware/0x40000.bin
	sleep 1  
)
