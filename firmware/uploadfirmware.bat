@echo off
setlocal enabledelayedexpansion
echo Liste des ports COM disponibles :
:: reg query HKLM\HARDWARE\DEVICEMAP\SERIALCOMM
:: set /p usedcom=Téléversement via port COM


:: Appelle PowerShell pour lister tous les noms contenant (COMx) et extraire COMx
for /f "delims=" %%A in ('powershell -Command "Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match '\(COM[0-9]+\)' } | ForEach-Object { if ($_ -match '\(COM[0-9]+\)') { $matches[0].Trim('()') } }"') do (
    echo !port_index!. %%A
)

:: Demander à l'utilisateur de choisir un port
echo.
set /p usedcom=Entrez le numero du port choisi : COM

:: Lister les fichiers .hex
echo.
echo Liste des fichiers HEX disponibles :
set /a index=0
for %%f in (*.hex) do (
    set /a index+=1
    set "file[!index!]=%%f"
    echo   !index!. %%f
)

:: Demander à l'utilisateur de choisir
echo.
set /p choice=Entrez le numero du fichier qu'il faut flasher :

:: Vérifier que le choix est valide
if not defined file[%choice%] (
    echo Choix invalide.
    pause
    exit /b
)

:: Récupérer le nom du fichier choisi
set HEX=!file[%choice%]!

:: Affichage
echo.
echo --> Flash de "!HEX!" sur le ATtiny3216 via COM%usedcom%
echo.

pymcuprog write -t uart -u com%usedcom% -d attiny3216 -f %HEX%
pymcuprog reset -t uart -u com%usedcom% -d attiny3216
pause
