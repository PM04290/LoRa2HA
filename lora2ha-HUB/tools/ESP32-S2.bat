set /p usedcom=Televersement de "data" via port COM
echo Utilisation de COM%usedcom%
C:\Users\Admin\AppData\Local\Arduino15\packages\esp32\tools\mkspiffs\0.2.3\mkspiffs.exe -c ../data -p 256 -b 4096 -s 0x100000 LoRa2HA.spiffs.bin
C:\Users\Admin\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\4.5.1\esptool.exe --chip esp32s2 --port com%usedcom% --baud 921600 write_flash -z 0x110000 LoRa2HA.spiffs.bin
del LoRa2HA.spiffs.bin
REM esptool.exe --chip esp32s2 --port COM%usedcom% --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 C:\Users\Admin\AppData\Local\Temp\arduino_build_364475/lora2ha-HUB.ino.bootloader.bin 0x8000 C:\Users\Admin\AppData\Local\Temp\arduino_build_364475/lora2ha-HUB.ino.partitions.bin 0xe000 C:\Users\Admin\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.11/tools/partitions/boot_app0.bin 0x10000 C:\Users\Admin\AppData\Local\Temp\arduino_build_364475/lora2ha-HUB.ino.bin 
pause