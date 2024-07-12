@echo off
set /p usedcom=Choix du port COM
cls
echo Utilisation de COM%usedcom%
echo Voulez-vous telecharger les DATA, attention la
echo configuration precedente va Etre ecrasee !
choice /C ON /M "(O/N) ? "
if %errorlevel% == 1 (
echo ======================
echo Telechargement de DATA
.\bin\mkspiffs.exe -c ../data -p 256 -b 4096 -s 0x160000 LoRa2HA.spiffs.bin
.\bin\esptool.exe --chip esp32 --port com%usedcom% --baud 921600 --after no_reset write_flash -z 0x290000 LoRa2HA.spiffs.bin
del LoRa2HA.spiffs.bin
)
echo ==========================
echo Telechargement du Firmware
.\bin\esptool.exe --chip esp32 --port COM%usedcom% --baud 921600 --before no_reset --after no_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 ./bin/WROOM.bootloader.bin 0x8000 ./bin/WROOM.partitions.bin 0xe000 ./bin/boot_app0.bin 0x10000 ../lora2ha-HUB.ino.wt32-eth01-eth.bin
pause
