#pragma once
#include <HTTPClient.h>  // native library
#include <ESPmDNS.h>     // native library
#include <Update.h>      // native library
//#include <WireGuard-ESP32.h>
#include "time.h"        // inside file

#ifdef USE_ETHERNET
#include <ETH.h>         // native library
#endif

bool wifiOK1time = false;
bool eth_connected = false;

enum WMode {
  Mode_WifiSTA = 0,
  Mode_WifiAP,
  Mode_Ethernet
};

//static WireGuard wg;
bool vpnNeeded = false;
bool vpnok = false;

void listDir(const char * dirname)
{
  DEBUGf("Listing directory: %s\n", dirname);
  File root = SPIFFS.open(dirname);
  if (!root.isDirectory())
  {
    DEBUGln("No Dir");
  }
  File file = root.openNextFile();
  while (file)
  {
    DEBUGf(" %-30s %d\n", file.name(), file.size());
    file = root.openNextFile();
  }
}

void downloadFile(String url, String path)
{
  HTTPClient http;

  // Démarre la requête GET
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    // Crée le fichier sur SPIFFS
    File file = SPIFFS.open(path, FILE_WRITE);
    if (file)
    {
      // Récupère la taille du fichier
      int totalLength = http.getSize();
      int len = totalLength;
      uint8_t buff[128] = { 0 };  // Taille du buffer pour les fragments
      WiFiClient* stream = http.getStreamPtr();
      while (http.connected() && (len > 0 || len == -1))
      {
        size_t size = stream->available();
        if (size)
        {
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          // Écrit dans le fichier SPIFFS
          file.write(buff, c);
          if (len > 0)
          {
            len -= c;
          }
        }
        delay(1);
      }
    }
    file.close();
  }
  http.end();
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
    case ARDUINO_EVENT_WIFI_READY:
      DEBUGln("WiFi ready");
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      DEBUGln("WiFi Completed scan");
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      DEBUGln("STA started");
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      DEBUGln("STA stopped");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      DEBUGln("STA Connected");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      DEBUGln("STA Disconnected");
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      DEBUGln("STA Authmode change");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      DEBUG("STA IP address: ");
      DEBUGln(WiFi.localIP());
      wifiOK1time = true;
      hub.MQTTconnect();
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      DEBUGln("STA Lost IP");
      break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      DEBUGln("WPS succeeded in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
      DEBUGln("WPS failed in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      DEBUGln("WPS timeout in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_PIN:
      DEBUGln("WPS pin code in enrollee mode");
      break;
    case ARDUINO_EVENT_WIFI_AP_START:
      DEBUGln("AP started");
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      DEBUGln("AP  stopped");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      DEBUGln("AP Client connected");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      DEBUGln("AP Client disconnected");
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      DEBUGln("AP Assigned IP address to client");
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      DEBUGln("AP Received probe request");
      break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
      DEBUGln("AP IPv6 is preferred");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
      DEBUGln("STA IPv6 is preferred");
      break;
#ifdef USE_ETHERNET
    case ARDUINO_EVENT_ETH_GOT_IP6:
      DEBUGln("ETH IPv6 is preferred");
      break;
    case ARDUINO_EVENT_ETH_START:
      DEBUGln("ETH started");
      ETH.setHostname(AP_ssid);
      break;
    case ARDUINO_EVENT_ETH_STOP:
      DEBUGln("ETH stopped");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      DEBUGln("ETH connected");
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      DEBUGln("ETH disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      DEBUGln("ETH Obtained IP address");
      DEBUG("ETH MAC: ");
      DEBUG(ETH.macAddress());
      DEBUG(", IPv4: ");
      DEBUG(ETH.localIP());
      if (ETH.fullDuplex()) {
        DEBUG(", FULL_DUPLEX");
      }
      DEBUG(", ");
      DEBUG(ETH.linkSpeed());
      DEBUGln("Mbps");
      eth_connected = true;
      hub.MQTTconnect();
      break;
#endif
    default:
      DEBUGf("[WiFi-event] %d \n", event);
      break;
  }
}

uint8_t getWMODE()
{
  uint8_t wmode = Mode_WifiSTA;

#ifdef USE_ETHERNET

#ifdef FORCE_ETHERNET
  DEBUGln("Force ETH");
  return Mode_Ethernet;
#endif

  uint8_t ethOK = false;
#ifdef PIN_ETH_LINK
  ethOK = digitalRead(PIN_ETH_LINK) == LOW;
#endif
  int wm = analogRead(PIN_WMODE);
  DEBUGf("WM(mv) : %d\n", wm);
  if (ethOK || (wm > 125 && wm < 3970)) {  //  Ethernet from 0.1 to 3.2v or LINK
    wmode = Mode_Ethernet;
    DEBUGln("WMode : Ethernet");
  } else if (wm < 125) {                         // AP under 0.1v
    wmode = Mode_WifiAP;
    DEBUGln("WMode : Wifi AP");
  } else {                                 // STA up to 3.2v
    DEBUGln("WMode : Wifi STA");
  }
#else
  if (digitalRead(PIN_WMODE) == LOW || strlen(Wifi_ssid) == 0) {
    wmode = Mode_WifiAP;
    DEBUGf("WMode : Wifi AP\n");
  } else {
    DEBUGf("WMode : Wifi STA\n");
  }
#endif
  return wmode;
}

#ifdef USE_ETHERNET
void startETH()
{
  ETH.begin();
}
#endif

void startWifiSTA()
{
  DEBUG("MAC : ");
  DEBUGln(WiFi.macAddress());
  WiFi.mode(WIFI_STA);
  WiFi.begin(Wifi_ssid, Wifi_pass);
  int tentativeWiFi = 0;
  // Attente de la connexion au réseau WiFi / Wait for connection
  while ( WiFi.status() != WL_CONNECTED && tentativeWiFi < 20)
  {
    delay( 500 ); DEBUG( "." );
    tentativeWiFi++;
  }
}

void startWifiAP()
{
  DEBUGln("set Wifi AP mode.");
  // Mode AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_ssid, AP_pass);
  // Default IP Address is 192.168.4.1
  // if you want to change uncomment below
  // softAPConfig (local_ip, gateway, subnet)

  DEBUGf("AP WIFI : %s\n", AP_ssid);
  DEBUG("AP IP Address: "); DEBUGln(WiFi.softAPIP());
}

bool checkVPN()
{
  if (vpnNeeded)
  {
    DEBUGln("Check for available wireguard.conf");
    File f = SPIFFS.open("/wireguard.conf", "r");
    if (f)
    {
      String line, elt;
      while (f.available()) {
        line = f.readStringUntil('\n');
        if (line.startsWith("PrivateKey"))
        {
          elt = getValue(line, ' ', 2);
          strcpy(private_key, elt.c_str());
        }
        if (line.startsWith("Address"))
        {
          elt = getValue(line, ' ', 2);
          local_ip.fromString(getValue(elt, '/', 0));
        }
        if (line.startsWith("PublicKey"))
        {
          elt = getValue(line, ' ', 2);
          strcpy(public_key, elt.c_str());
        }
        if (line.startsWith("EndPoint"))
        {
          elt = getValue(line, ' ', 2);
          strcpy(endpoint_address, getValue(elt, ':', 0).c_str());
          endpoint_port = getValue(elt, ':', 1).toInt();
        }
      }
      f.close();
      DEBUGf("privateK %s\n", private_key);
      DEBUGf("address  %s\n", local_ip.toString().c_str());
      DEBUGf("publicK  %s\n", public_key);
      DEBUGf("endP     %s:%d\n", endpoint_address, endpoint_port);
    } else
    {
      DEBUGln("wireguard.conf not found");
    }
  }
  return false;
}

void initNetwork()
{
  byte mac[6];
  WiFi.onEvent(WiFiEvent);

#ifdef PIN_ETH_LINK
  pinMode(PIN_ETH_LINK, INPUT);
#endif

#ifdef USE_ETHERNET
  ETH.macAddress(mac);
#else
  pinMode(PIN_WMODE, INPUT_PULLUP); // Wifi STA by default (Short cut to GND to have AP)
  WiFi.macAddress(mac);
#endif
  byteArrayToStr(HAuniqueId, mac, 6);

  switch (getWMODE()) {
    case Mode_WifiSTA:
      startWifiSTA();
      break;
    case Mode_WifiAP:
      startWifiAP();
      break;
#ifdef USE_ETHERNET
    case Mode_Ethernet:
      startETH();
      break;
#endif
  }

  if (MDNS.begin(AP_ssid))
  {
    MDNS.addService("http", "tcp", 80);
    DEBUGf("MDNS on %s\n", AP_ssid);
  }
  if (checkVPN())
  {
    //vpnok = wg.begin(local_ip, private_key, endpoint_address, public_key, endpoint_port);
  }
}
