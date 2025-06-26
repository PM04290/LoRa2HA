![](https://raw.githubusercontent.com/PM04290/LoRa2HA/refs/heads/main/res/LoRa2HA-logo-200x200.png)
# LoRa2HA

Document version 1.4

## Introduction
LoRa2HA permet de combler un vide dans les équipements domotique lorsque qu'il est nécéssaire de surveiller/piloter sur une longue distance, là où Wifi et Zigbee ne peuvent aller.
![](https://raw.githubusercontent.com/PM04290/LoRa2HA/refs/heads/main/res/LoRa2HA-visual.jpg)
Le projet propose des équipements permettant une interconnexion entre des capteurs/actionneurs très éloignés d'un serveur HomeAssistant (via MQTT), par radio en utilisant des modules LoRa.
Le HUB, à base d'ESP32 (Wifi ou Ethernet), reçoit les données radio et les envoi vers Home Assistant (H.A.) au travers d'un serveur MQTT. Le terme de HUB sera utilisé, et non Gateway, afin de ne pas créer de confusion avec les Gateway « LoRaWan ».

Le projet contient les sources de tous les firmwares, les schémas des cartes, les « Gerber » afin de pouvoir les fabriquer et des fichiers 3D pour la fabrication de boîtiers.

## Documentation

Le fichier PDF de la documentation complète est téléchargeable [ici](https://raw.githubusercontent.com/PM04290/LoRa2HA/main/doc/LoRa2HA-readme-14.pdf)
