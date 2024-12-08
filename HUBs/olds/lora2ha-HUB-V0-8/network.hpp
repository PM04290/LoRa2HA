#pragma once
#include <ESPmDNS.h>
#include <Update.h>
#include <Update.h>
#include "time.h"

#ifdef USE_ETHERNET
#include <ETH.h>
#endif

#include "include_html.hpp"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool wifiOK1time = false;

bool logPacket = false;

enum WMode {
  Mode_WifiSTA = 0,
  Mode_WifiAP,
  Mode_Ethernet
};

boolean isValidFloat(String str)
{
  if (str.startsWith("-"))
    str.remove(0, 1);
  for (byte i = 0; i < str.length(); i++)
  {
    if (!isDigit(str.charAt(i)) && (str.charAt(i) != '.'))
    {
      return false;
    }
  }
  return str.length() > 0;
}

boolean isValidInt(String str)
{
  if (str.startsWith("-"))
    str.remove(0, 1);
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

String getHTMLforDevice(uint8_t d, Device* dev)
{
  String blocD = html_device;
  String replaceGen = "";
  String replaceFooter = "";
  blocD.replace("%DEVCOUNT%", String(d + 1));
  if (dev != nullptr)
  {
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%CNFNAME%", String(dev->getName()));
    blocD.replace("%CNFMODEL%", String(dev->getModel()));
    blocD.replace("%CNFNAMEVALID%",  strlen(dev->getName()) == 0 ? "aria-invalid='true'" : String(dev->getName()));
    blocD.replace("%CNFRLVERSION%",  String(dev->getRLversion()));
    //
    for (int c = 0; c < dev->getNbChild(); c++)
    {
      replaceGen += "<div id='conf_child_" + String(d) + "_" + String(c) + "' class='row'>Loading...</div>\n";
    }
    if (dev->isConfLoading() == false) {
      replaceFooter = String(html_device_footer_ackdevice);
    } else {
      replaceFooter = String(html_device_footer_add);
    }
  } else {
    blocD.replace("%CNFADDRESS%", "");
    blocD.replace("%CNFNAME%",  "");
    blocD.replace("%CNFMODEL%",  "");
    blocD.replace("%CNFNAMEVALID%",  "aria-invalid='true'");
    blocD.replace("%CNFRLVERSION%",  "0");
    replaceGen += "<div id='conf_child_" + String(d) + "_0' class='row'>Loading...</div>\n";
    replaceFooter = String(html_device_footer_add);
  }
  //
  blocD.replace("%GENCHILDS%", replaceGen);
  blocD.replace("%GENFOOTER%", replaceFooter);
  blocD.replace("#D#", String(d));
  return blocD;
}

String getHTMLforChild(uint8_t d, uint8_t c, Child* ch)
{
  String blocC = html_child;
  String kcnf;
  blocC.replace("#D#", String(d));
  blocC.replace("#C#", String(c));
  if (ch != nullptr)
  {
    blocC.replace("%CNFC_ID%", String(ch->getId()));
    blocC.replace("%CNFC_LABEL%", String(ch->getLabel()));
    blocC.replace("%CNFC_LABELVALID%", strlen(ch->getLabel()) == 0 ? "aria-invalid='true'" : String(ch->getLabel()));
    for (int n = 0; n <= 5; n++) {
      kcnf = "%CNFC_D" + String(n) + "%";
      blocC.replace(kcnf, (int)ch->getDataType() == n ? "selected" : "");
    }
    const char* elementtype[14] = {"Binary sensor", "Numeric sensor", "Switch", "Light", "Cover", "Fan", "HVac", "Select", "Trigger", "Event", "Tag", "Text", "Input", "Custom"};
    blocC.replace("%CNFC_STYPE_INT%", String((int)ch->getElementType()));
    blocC.replace("%CNFC_STYPE_STR%", elementtype[(int)ch->getElementType()]);
    switch (ch->getElementType())
    {
      case E_BINARYSENSOR:
        blocC.replace("%CNFC_CLASS%", String(ch->getClass()));
        blocC.replace("%CNFC_CATEGORY%", String((int)ch->getCategory()));
        blocC.replace("%CNFC_UNIT%", "");
        blocC.replace("%CNFC_EXPIRE%", "");
        blocC.replace("%CNFC_OPT%", "");
        blocC.replace("%CNFC_IMIN%", "");
        blocC.replace("%CNFC_IMAX%", "");
        blocC.replace("%CNFC_IDIV%", "");
        blocC.replace("%CNFC_MINI%", "");
        blocC.replace("%CNFC_MAXI%", "");
        blocC.replace("%CNFC_CA%", "");
        blocC.replace("%CNFC_CB%", "");
        break;
      case E_NUMERICSENSOR:
        blocC.replace("%CNFC_CLASS%", String(ch->getClass()));
        blocC.replace("%CNFC_CATEGORY%", String((int)ch->getCategory()));
        blocC.replace("%CNFC_UNIT%", String(ch->getUnit()));
        blocC.replace("%CNFC_EXPIRE%", String(ch->getExpire()));
        blocC.replace("%CNFC_OPT%", "");
        blocC.replace("%CNFC_IMIN%", "");
        blocC.replace("%CNFC_IMAX%", "");
        blocC.replace("%CNFC_IDIV%", "");
        blocC.replace("%CNFC_MINI%", ch->getMini() == LONG_MIN ? "" : String(ch->getMini()));
        blocC.replace("%CNFC_MAXI%", ch->getMaxi() == LONG_MAX ? "" : String(ch->getMaxi()));
        blocC.replace("%CNFC_CA%", String(ch->getCoefA()));
        blocC.replace("%CNFC_CB%", String(ch->getCoefB()));
        break;
      case E_SELECT:
        blocC.replace("%CNFC_CLASS%", "");
        blocC.replace("%CNFC_CATEGORY%", "");
        blocC.replace("%CNFC_UNIT%", "");
        blocC.replace("%CNFC_EXPIRE%", "");
        blocC.replace("%CNFC_OPT%", String(ch->getSelectOptions()));
        blocC.replace("%CNFC_IMIN%", "");
        blocC.replace("%CNFC_IMAX%", "");
        blocC.replace("%CNFC_IDIV%", "");
        blocC.replace("%CNFC_MINI%", "");
        blocC.replace("%CNFC_MAXI%", "");
        blocC.replace("%CNFC_CA%", String(ch->getCoefA()));
        blocC.replace("%CNFC_CB%", String(ch->getCoefB()));
        break;
      case E_INPUTNUMBER:
        blocC.replace("%CNFC_CLASS%", "");
        blocC.replace("%CNFC_CATEGORY%", "");
        blocC.replace("%CNFC_UNIT%", "");
        blocC.replace("%CNFC_EXPIRE%", "");
        blocC.replace("%CNFC_OPT%", "");
        blocC.replace("%CNFC_IMIN%", String(ch->getNumberMin()));
        blocC.replace("%CNFC_IMAX%", String(ch->getNumberMax()));
        blocC.replace("%CNFC_IDIV%", String(ch->getNumberDiv()));
        blocC.replace("%CNFC_MINI%", "");
        blocC.replace("%CNFC_MAXI%", "");
        blocC.replace("%CNFC_CA%", String(ch->getCoefA()));
        blocC.replace("%CNFC_CB%", String(ch->getCoefB()));
        break;
      default:
        blocC.replace("%CNFC_CLASS%", "");
        blocC.replace("%CNFC_CATEGORY%", "");
        blocC.replace("%CNFC_UNIT%", "");
        blocC.replace("%CNFC_EXPIRE%", "");
        blocC.replace("%CNFC_OPT%", "");
        blocC.replace("%CNFC_IMIN%", "");
        blocC.replace("%CNFC_IMAX%", "");
        blocC.replace("%CNFC_IDIV%", "");
        blocC.replace("%CNFC_MINI%", "");
        blocC.replace("%CNFC_MAXI%", "");
        blocC.replace("%CNFC_CA%", "");
        blocC.replace("%CNFC_CB%", "");
    }
  } else
  {
    blocC.replace("%CNFC_ID%", "1");
    blocC.replace("%CNFC_LABEL%", "");
    blocC.replace("%CNFC_LABELVALID%", "aria-invalid='true'");
    for (int n = 0; n <= 5; n++) {
      kcnf = "%CNFC_D" + String(n) + "%";
      blocC.replace(kcnf, "");
    }
    blocC.replace("%CNFC_STYPE_INT%", "");
    blocC.replace("%CNFC_STYPE_STR%", "");
    blocC.replace("%CNFC_CLASS%", "");
    blocC.replace("%CNFC_CATEGORY%", "");
    blocC.replace("%CNFC_UNIT%", "");
    blocC.replace("%CNFC_EXPIRE%", "");
    blocC.replace("%CNFC_OPT%", "");
    blocC.replace("%CNFC_IMIN%", "");
    blocC.replace("%CNFC_IMAX%", "");
    blocC.replace("%CNFC_MINI%", "");
    blocC.replace("%CNFC_MAXI%", "");
    blocC.replace("%CNFC_CA%", "");
    blocC.replace("%CNFC_CB%", "");
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

void notifyLogPacket(rl_packet_t* p, int lqi)
{
  if (ws.availableForWriteAll())
  {
    String s = "(" + String(lqi) + ") " + String(p->destinationID) + " <= " + String(p->senderID);
    if (p->childID == RL_ID_CONFIG && (rl_element_t)(p->sensordataType >> 3) == E_CONFIG)
    {
      s = s + " CNF ";
      rl_conf_t cnfIdx = (rl_conf_t)(p->sensordataType & 0x07);
      switch (cnfIdx) {
        case C_BASE:
          s = s + "B ";
          break;
        case C_UNIT:
          s = s + "U ";
          break;
        case C_OPTS:
          s = s + "O ";
          break;
        case C_NUMS:
          s = s + "N ";
          break;
        case C_END:
          s = s + "E ";
          break;
        default:
          s = s + "? ";
          break;
      }
    } else {
      s = s + ":" + String(p->childID) + " = ";
      rl_data_t dt = (rl_data_t)(p->sensordataType & 0x07);
      switch (dt) {
        case D_TEXT:
          s = s + String(p->data.text);
          break;
        case D_FLOAT:
          s = s + String(float(p->data.num.value) / float(p->data.num.divider));
          break;
        default:
          s = s + String(p->data.num.value);
          break;
      }
    }
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
    bool needreload = false;
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
      //
      needreload = true;
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
      //
      needreload = true;
    }
    if (cmd == "ackdevice")
    {
      DEBUGf("ackdevice : %s\n", val);
    }
    if (cmd == "logpacket")
    {
      logPacket = (val == "1");
    }
    if (needreload) {
      ws.textAll("{\"cmd\":\"loadselect\"}");
    }
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

        html.replace("%CNFDIST%", String(RadioDist));
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
        html.replace("%TZ%", datetimeTZ);
        html.replace("%NTP%", datetimeNTP);
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
  /* to debug
    for (int i = 0; i < request->params(); i++)
    {
    AsyncWebParameter* p = request->getParam(i);
    DEBUGf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  */
  if (request->hasParam("dev_0_address", true))
  {
    JsonDocument docJSon;
    JsonObject Jconfig = docJSon.to<JsonObject>();
    int params = request->params();
    for (int i = 0; i < params; i++)
    {
      AsyncWebParameter* p = request->getParam(i);
      String str = p->name();
      // exemple : dev_0_address
      //      or : dev_1_child_2_label
      String valdev = getValue(str, '_', 0);
      String keydev = getValue(str, '_', 1);
      String attrdev = getValue(str, '_', 2);
      String keychild = getValue(str, '_', 3);
      String attrchild = getValue(str, '_', 4);
      String sval(p->value().c_str());

      DEBUGf("[%s][%s][%s][%s][%s] = %s\n", valdev.c_str(), keydev.c_str(), attrdev.c_str(), keychild.c_str(), attrchild.c_str(), sval.c_str());
      if (keychild == "")
      {
        // on laisse Name vide pour supprimer un Device
        if (request->getParam(valdev + "_" + keydev + "_name", true)->value() != "")
        {
          if (isValidInt(sval))
          {
            Jconfig[valdev][keydev.toInt()][attrdev] = sval.toInt();
          } else if (isValidFloat(sval))
          {
            Jconfig[valdev][keydev.toInt()][attrdev] = serialized(String(sval.toFloat(), 3));
          } else
          {
            Jconfig[valdev][keydev.toInt()][attrdev] = sval;
          }
        }
      } else
      {
        // on laisse Label vide pour supprimer un Child
        if (request->getParam(valdev + "_" + keydev + "_" + attrdev + "_"  + keychild + "_label", true)->value() != "")
        {
          if (isValidInt(sval))
          {
            Jconfig[valdev][keydev.toInt()][attrdev][keychild.toInt()][attrchild] = sval.toInt();
          } else if (isValidFloat(sval))
          {
            Jconfig[valdev][keydev.toInt()][attrdev][keychild.toInt()][attrchild] = serialized(String(sval.toFloat(), 3));
          } else
          {
            Jconfig[valdev][keydev.toInt()][attrdev][keychild.toInt()][attrchild] = sval;
          }
        }
      }
    }
    /*
      // nettoyage des pages vides
      for (JsonArray::iterator it = Jconfig["dev"].as<JsonArray>().begin(); it != Jconfig["dev"].as<JsonArray>().end(); ++it)
      {
      if (!(*it).containsKey("childs"))
      {
        Jconfig["dev"].as<JsonArray>().remove(it);
      }
      }*/
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
    UIDcode = AP_ssid[7] - '0';
    strcpy(Wifi_ssid, request->getParam("wifissid", true)->value().c_str() );
    strcpy(Wifi_pass, request->getParam("wifipass", true)->value().c_str() );
    strcpy(mqtt_host, request->getParam("mqtthost", true)->value().c_str() );
    strcpy(mqtt_user, request->getParam("mqttuser", true)->value().c_str() );
    strcpy(mqtt_pass, request->getParam("mqttpass", true)->value().c_str() );
    strcpy(datetimeTZ, request->getParam("tz", true)->value().c_str() );
    strcpy(datetimeNTP, request->getParam("ntp", true)->value().c_str() );
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
    DEBUGln(datetimeTZ);
    DEBUGln(datetimeNTP);

    EEPROM.writeChar(EEPROM_DATA_CODE, AP_ssid[7]);
    EEPROM.writeUShort(EEPROM_DATA_FREQ, RadioFreq);
    EEPROM.writeString(EEPROM_TEXT_OFFSET, Wifi_ssid);
    EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 1), Wifi_pass);
    EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 2), mqtt_host);
    EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 3), mqtt_user);
    EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 4), mqtt_pass);
    EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 5), datetimeTZ);
    EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 6), datetimeNTP);
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

uint8_t getWMODE()
{
  uint8_t wmode = Mode_WifiSTA;

  uint8_t ethOK = false;
#ifdef PIN_ETH_LINK
  ethOK = digitalRead(PIN_ETH_LINK) == LOW;
#endif

#ifdef USE_ETHERNET

#ifdef FORCE_ETHERNET
  DEBUGln("Force ETH");
  return Mode_Ethernet;
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

bool getNTP(int &h, int &m, int &s)
{
  //if (!wifiok) return false;
  return true;
}

void initNetwork()
{

  WiFi.onEvent(WiFiEvent);

#ifdef PIN_ETH_LINK
  pinMode(PIN_ETH_LINK, INPUT);
#endif

#ifndef USE_ETHERNET
  pinMode(PIN_WMODE, INPUT_PULLUP); // Wifi STA by default (Short cut to GND to have AP)
#endif

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
}
