#pragma once
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#ifdef USE_ETHERNET
#include <ETH.h>
#endif

#include "include_html.hpp"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


bool wifiOK1time = false;
static bool eth_connected = false;

String uniqueid = "?";
String version_major = "?";
String version_minor = "?";

bool logPacket = false;

boolean isValidNumber(String str)
{
  for (byte i = 0; i < str.length(); i++)
  {
    if (!isDigit(str.charAt(i)))
    {
      return false;
    }
  }
  return str.length() > 0;
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

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
    DEBUG("  FILE: ");
    DEBUG(file.name());
    DEBUG("\tSIZE: ");
    DEBUGln(file.size());
    file = root.openNextFile();
  }
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
      break;
#endif
    default:
      DEBUGf("[WiFi-event] %d \n", event);
      break;
  }
}

String getHTMLforDevice(uint8_t d, Device* dev)
{
  String blocD = html_device;
  String replaceGen = "";
  blocD.replace("#D#", String(d));
  blocD.replace("%DEVCOUNT%", String(d + 1));
  if (dev != nullptr)
  {
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%CNFNAME%",  dev->getName());
    blocD.replace("%CNFRLVERSION%",  String(dev->getRLversion()));
    //
    for (int c = 0; c < dev->getNbChild(); c++)
    {
      replaceGen += "<div id='conf_child_" + String(d) + "_" + String(c) + "' class='row'>chargement...</div>\n";
    }
  } else {
    blocD.replace("%CNFADDRESS%", "");
    blocD.replace("%CNFNAME%",  "");
    blocD.replace("%CNFRLVERSION%",  "0");
    replaceGen += "<div id='conf_child_" + String(d) + "_0' class='row'>chargement...</div>\n";
  }
  //
  blocD.replace("%GENCHILDS%", replaceGen);
  return blocD;
}

String getHTMLforChild(uint8_t d, uint8_t c, Child* ch)
{
  String blocC = html_child;
  String kcnf;
  blocC.replace("%TITLEC_ID%", c ? "" : "ID");
  blocC.replace("%TITLEC_NAME%", c ? "" : "Label");
  blocC.replace("%TITLEC_DATA%", c ? "" : "Data");
  blocC.replace("%TITLEC_HA%", c ? "" : "HA configuration");

  blocC.replace("#D#", String(d));
  blocC.replace("#C#", String(c));
  if (ch != nullptr)
  {
    blocC.replace("%CNFC_ID%", String(ch->getId()));
    blocC.replace("%CNFC_LABEL%", String(ch->getLabel()));
    for (int n = 0; n <= 5; n++) {
      kcnf = "%CNFC_D" + String(n) + "%";
      blocC.replace(kcnf, (int)ch->getDataType() == n ? "selected" : "");
    }
    const char* sensortype[12] = {"Binary sensor", "Numeric sensor", "Switch", "Light", "Cover", "Fan", "HVac", "Select", "Trigger", "Custom", "Tag", "Text"};
    blocC.replace("%CNFC_STYPE_INT%", String((int)ch->getSensorType()));
    blocC.replace("%CNFC_STYPE_STR%", sensortype[(int)ch->getSensorType()]);
    blocC.replace("%CNFC_CLASS%", String(ch->getClass()));
    blocC.replace("%CNFC_UNIT%", String(ch->getUnit()));
    blocC.replace("%CNFC_EXPIRE%", String(ch->getExpire()));
    blocC.replace("%CNFC_MINI%", ch->getMini() == LONG_MIN ? "" : String(ch->getMini()));
    blocC.replace("%CNFC_MAXI%", ch->getMaxi() == LONG_MAX ? "" : String(ch->getMaxi()));
  } else
  {
    blocC.replace("%CNFC_ID%", "1");
    blocC.replace("%CNFC_LABEL%", "");
    for (int n = 0; n <= 5; n++) {
      kcnf = "%CNFC_D" + String(n) + "%";
      blocC.replace(kcnf, "");
    }
    blocC.replace("%CNFC_STYPE_INT%", "");
    blocC.replace("%CNFC_STYPE_STR%", "");
    blocC.replace("%CNFC_CLASS%", "");
    blocC.replace("%CNFC_UNIT%", "");
    blocC.replace("%CNFC_EXPIRE%", "");
    blocC.replace("%CNFC_MINI%", "");
    blocC.replace("%CNFC_MAXI%", "");
  }
  return blocC;
}

void sendConfigDevice()
{
  String blocD = "";
  for (int d = 0; d < hub.getNbDevice(); d++)
  {
    Device* dev = hub.getDevice(d);
    blocD += getHTMLforDevice(d, dev);
  }
  docJson.clear();
  docJson["cmd"] = "html";
  docJson["#conf_dev"] = blocD;
  String js;
  serializeJson(docJson, js);
  ws.textAll(js);
}

void sendConfigChild()
{
  for (uint8_t d = 0; d < hub.getNbDevice(); d++)
  {
    Device* dev = hub.getDevice(d);
    for (uint8_t c = 0; c < dev->getNbChild(); c++)
    {
      Child* ch = dev->getChild(c);
      if (ch != nullptr)
      {
        docJson.clear();
        docJson["cmd"] = "html";
        docJson["#conf_child_" + String(d) + "_" + String(c)] = getHTMLforChild(d, c, ch);
        String js;
        serializeJson(docJson, js);
        ws.textAll(js);
      }
    }
  }
}

void notifyConfig()
{
  if (ws.availableForWriteAll())
  {
    sendConfigDevice();
    sendConfigChild();
    ws.textAll("{\"cmd\":\"loadselect\"}");
  }
}

void notifyLogPacket(rl_packet_t* p)
{
  if (ws.availableForWriteAll())
  {
    String s = String(p->destinationID) + " <= " + String(p->senderID) + ":" + String(p->childID) + " = " + String(p->data.num.value);
    docJson.clear();
    docJson["cmd"] = "log";
    docJson["packet"] = s;
    String js;
    serializeJson(docJson, js);
    ws.textAll(js);
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  DEBUGln("handleWebSocketMessage");
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    String js;
    data[len] = 0;
    DEBUGln((char*)data);
    String cmd = getValue((char*)data, ';', 0);
    String val = getValue((char*)data, ';', 1);
    if (cmd == "adddev")
    {
      // add blank device
      docJson.clear();
      docJson["cmd"] = "replace";
      docJson["#newdev_" + val] = getHTMLforDevice(val.toInt(), NULL);
      serializeJson(docJson, js);
      ws.textAll(js);

      // add blank child
      docJson.clear();
      docJson["cmd"] = "html";
      docJson["#conf_child_" + val + "_0"] = getHTMLforChild(val.toInt(), 0, NULL);
      js = "";
      serializeJson(docJson, js);
      ws.textAll(js);
    }
    if (cmd == "addchild")
    {
      String chd = getValue((char*)data, ';', 2);
      // add blank child
      docJson.clear();
      docJson["cmd"] = "html";
      docJson["#conf_child_" + val + "_" + chd] = getHTMLforChild(val.toInt(), chd.toInt(), NULL);
      js = "";
      serializeJson(docJson, js);
      ws.textAll(js);
    }
    if (cmd == "logpacket")
    {
      logPacket = (val == "1");
    }
    ws.textAll("{\"cmd\":\"loadselect\"}");
    return;
  }
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
    case WS_EVT_CONNECT:
      DEBUGf("WebSocket client #%u/%d connected from %s\n", client->id(), server->count(), client->remoteIP().toString().c_str());
      logPacket = false;
      notifyConfig();
      break;
    case WS_EVT_DISCONNECT:
      DEBUGf("WebSocket client #%u disconnected\n", client->id());
      logPacket = false;
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void onIndexRequest(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  File f = SPIFFS.open("/index.html", "r");
  if (f)
  {
    String html;
    while (f.available()) {
      html = f.readStringUntil('\n') + '\n';
      if (html.indexOf('%') > 0)
      {
        html.replace("%CNFCODE%", String(AP_ssid[7]));
#ifdef USE_ETHERNET
        html.replace("%WIFIMAC%", ETH.macAddress());
#else
        html.replace("%WIFIMAC%", WiFi.macAddress());
#endif
        html.replace("%CNFFREQ%", String(RadioFreq));

        html.replace("%WIFISSID%", Wifi_ssid);
        html.replace("%WIFIPASS%", Wifi_pass);
        html.replace("%MQTTHOST%", mqtt_host);
        html.replace("%MQTTUSER%", mqtt_user);
        html.replace("%MQTTPASS%", mqtt_pass);
        html.replace("%VERSION%", VERSION);
      }
      response->print(html);
    }
    f.close();
  }
  request->send(response);
}

void onConfigRequest(AsyncWebServerRequest * request)
{
  DEBUGln("web set config");
  /*
    for (int i = 0; i < request->params(); i++)
    {
    AsyncWebParameter* p = request->getParam(i);
    DEBUGf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  */
  if (request->hasParam("dev_0_address", true))
  {
    DynamicJsonDocument docJSon(JSON_MAX_SIZE);
    JsonObject Jconfig = docJSon.to<JsonObject>();
    int params = request->params();
    for (int i = 0; i < params; i++)
    {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isPost() && p->name() != "cnf")
      {
        //DEBUGf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        String str = p->name();
        if (str == "uniqueid") {
          Jconfig["uniqueid"] = p->value();
        } else if (str == "version_major") {
          Jconfig["version_major"] = p->value();
        } else if (str == "version_minor") {
          Jconfig["version_minor"] = p->value();
        } else {
          // exemple : devices_0_name
          //      or : devices_1_childs_2_label
          String valdev = getValue(str, '_', 0);
          String keydev = getValue(str, '_', 1);
          String attrdev = getValue(str, '_', 2);
          String keychild = getValue(str, '_', 3);
          String attrchild = getValue(str, '_', 4);
          String sval(p->value().c_str());

          DEBUGf("[%s][%s][%s][%s][%s] = %s\n", valdev, keydev, attrdev, keychild, attrchild, sval);
          if (keychild == "")
          {
            if (isValidNumber(sval))
            {
              Jconfig[valdev][keydev.toInt()][attrdev] = sval.toInt();
            } else
            {
              Jconfig[valdev][keydev.toInt()][attrdev] = sval;
            }
          } else
          {
            // on laisse le Label vide pour supprimer une ligne
            if (request->getParam(valdev + "_" + keydev + "_" + attrdev + "_"  + keychild + "_label", true)->value() != "")
            {
              if (isValidNumber(sval))
              {
                Jconfig[valdev][keydev.toInt()][attrdev][keychild.toInt()][attrchild] = sval.toInt();
              } else
              {
                Jconfig[valdev][keydev.toInt()][attrdev][keychild.toInt()][attrchild] = sval;
              }
            }
          }
        }
      }
    }
    // nettoyage des pages vides
    for (JsonArray::iterator it = Jconfig["devices"].as<JsonArray>().begin(); it != Jconfig["devices"].as<JsonArray>().end(); ++it)
    {
      if (!(*it).containsKey("childs"))
      {
        Jconfig["devices"].as<JsonArray>().remove(it);
      }
    }
    //
    Jconfig["end"] = true;
    String Jres;
    size_t Lres = serializeJson(docJSon, Jres);
    //DEBUGln(Jres);

    File file = SPIFFS.open("/config.json", "w");
    if (file)
    {
      file.write((byte*)Jres.c_str(), Lres);
      file.close();
      DEBUGln("Fichier de config enregistré");
    } else {
      DEBUGln("Erreur sauvegarde config");
    }
  }
  if (request->hasParam("cnfcode", true))
  {
    AP_ssid[7] = request->getParam("cnfcode", true)->value().c_str()[0];
    strcpy(Wifi_ssid, request->getParam("wifissid", true)->value().c_str() );
    strcpy(Wifi_pass, request->getParam("wifipass", true)->value().c_str() );
    strcpy(mqtt_host, request->getParam("mqtthost", true)->value().c_str() );
    strcpy(mqtt_user, request->getParam("mqttuser", true)->value().c_str() );
    strcpy(mqtt_pass, request->getParam("mqttpass", true)->value().c_str() );
    if (request->hasParam("cnffreq", true))
    {
      RadioFreq = request->getParam("cnffreq", true)->value().toInt();
    }

    DEBUGln(AP_ssid);
    DEBUGln(Wifi_ssid);
    DEBUGln(Wifi_pass);
    DEBUGln(mqtt_host);
    DEBUGln(mqtt_user);
    DEBUGln(mqtt_pass);
    DEBUGln(RadioFreq);

    EEPROM.writeChar(0, AP_ssid[7]);
    EEPROM.writeUShort(1, RadioFreq);
    EEPROM.writeString(EEPROM_TEXT_OFFSET, Wifi_ssid);
    EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 1), Wifi_pass);
    EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 2), mqtt_host);
    EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 3), mqtt_user);
    EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 4), mqtt_pass);
    EEPROM.commit();
  }

  request->send(200, "text/plain", "OK");
}

size_t content_len;
void handleDoUpdate(AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    DEBUGln("Update start");
    content_len = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
      Update.printError(Serial);
    }
  }
  if (Update.write(data, len) != len)
  {
    Update.printError(Serial);
  }
  if (final)
  {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true))
    {
      Update.printError(Serial);
    } else
    {
      DEBUGln("Update complete");
      delay(100);
      yield();
      delay(100);
      ESP.restart();
    }
  }
}

void handleDoFile(AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    if (filename.endsWith(".png"))
    {
      request->_tempFile = SPIFFS.open("/i/" + filename, "w");
    } else if (filename.endsWith(".css"))
    {
      request->_tempFile = SPIFFS.open("/css/" + filename, "w");
    } else if (filename.endsWith(".js"))
    {
      request->_tempFile = SPIFFS.open("/js/" + filename, "w");
    } else
    {
      request->_tempFile = SPIFFS.open("/" + filename, "w");
    }
  }
  if (len)
  {
    request->_tempFile.write(data, len);
  }
  if (final)
  {
    request->_tempFile.close();
    request->redirect("/");
  }
}

void updateProgress(size_t prg, size_t sz)
{
  DEBUGf("Progress: %d%%\n", (prg * 100) / content_len);
}

void initWeb()
{
  server.serveStatic("/fonts", SPIFFS, "/fonts");
  server.serveStatic("/css", SPIFFS, "/css");
  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/i", SPIFFS, "/i");
  server.serveStatic("/config.json", SPIFFS, "/config.json");
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/doconfig", HTTP_POST, onConfigRequest);
  server.on("/doupdate", HTTP_POST, [](AsyncWebServerRequest * request) {},
  [](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final) {
    handleDoUpdate(request, filename, index, data, len, final);
  });
  server.on("/dofile", HTTP_POST, [](AsyncWebServerRequest * request) {
    request->send(200);
  },
  [](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final) {
    handleDoFile(request, filename, index, data, len, final);
  });
  server.on("/restart", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "10");
    response->addHeader("Location", "/");
    request->send(response);
    delay(500);
    ESP.restart();
  });
  server.begin();
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  Update.onProgress(updateProgress);
  DEBUGln("HTTP server started");
}

void initNetwork()
{
  char txt[40];
  bool wifiok = false;

  WiFi.onEvent(WiFiEvent);
#ifdef USE_ETHERNET
  ETH.begin();
#else
  DEBUG("MAC : ");
  DEBUGln(WiFi.macAddress());

  // Mode normal
  WiFi.begin(Wifi_ssid, Wifi_pass);
  int tentativeWiFi = 0;
  // Attente de la connexion au réseau WiFi / Wait for connection
  while ( WiFi.status() != WL_CONNECTED && tentativeWiFi < 20)
  {
    delay( 500 ); DEBUG( "." );
    tentativeWiFi++;
  }
  wifiok = WiFi.status() == WL_CONNECTED;

  if (wifiok == false)
  {
    DEBUGln("No wifi STA, set AP mode.");
    // Mode AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_ssid, AP_pass);
    // Default IP Address is 192.168.4.1
    // if you want to change uncomment below
    // softAPConfig (local_ip, gateway, subnet)

    DEBUGf("AP WIFI : %s\n", AP_ssid);
    DEBUG("AP IP Address: "); DEBUGln(WiFi.softAPIP());
  }
#endif
  if (MDNS.begin(AP_ssid))
  {
    MDNS.addService("http", "tcp", 80);
    DEBUGf("MDNS on %s\n", AP_ssid);
  }
}
