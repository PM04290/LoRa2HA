#pragma once
#include "include_html.hpp"  // inside file

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool logPacket = false;
bool pairingActive = false;

size_t content_len;

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

String getFirmwareList(String src)
{
  String Flist = "";
  if (src == "1")
  {
    String url = "https://api.github.com/repos/PM04290/LoRa2HA/contents/firmware";
    HTTPClient Ghttp;
    Ghttp.begin(url);
    int httpResponseCode = Ghttp.GET();
    String payload = "[]";
    if (httpResponseCode == 200)
    {
      payload = Ghttp.getString();
      DeserializationError error = deserializeJson(docJson, payload);
      for (JsonVariant obj : docJson.as<JsonArray>())
      {
        String n = obj["name"].as<String>();
        Flist += "<option>" + n + "</option>";
      }
      docJson.clear();
    }
    else
    {
      Flist = "<option disabled>Error reading github (code " + String(httpResponseCode) + ")</option>";
    }
    Ghttp.end();
  } else
  {
    File root = SPIFFS.open("/hex");
    if (root.isDirectory())
    {
      File file = root.openNextFile();
      while (file)
      {
        Flist += "<option>" + String(file.name()) + "</option>";
        file = root.openNextFile();
      }
    } else {
      Flist = "<option disabled>No HEX directory on HUB</option>";
    }
  }
  if (Flist == "")
  {
    Flist = "<option disabled>No firmware stored in HUB</option><option disabled>Your must upload HEX file into HUB</option>";
  }
  return Flist;
}

String getHTMLforDeviceLine(uint8_t d)
{
  String blocD = html_device_line;
  Device* dev = hub.getDevice(d);
  blocD.replace("#D#", String(d));
  if (dev != nullptr)
  {
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%CNFNAME%", String(dev->getName()));
    blocD.replace("%CNFMODEL%", String(dev->getModel()));
    blocD.replace("%CNFNAMEVALID%",  strlen(dev->getName()) == 0 ? "aria-invalid='true'" : String(dev->getName()));
    if (dev->getPairing())
    {
      String blocP = html_device_pairing;
      blocP.replace("#D#", String(d));
      blocP.replace("%CNFADDRESS%", String(dev->getAddress()));
      //
      blocD.replace("%GENPAIRING%", blocP);
    } else {
      blocD.replace("%GENPAIRING%", "");
    }
  } else {
    blocD.replace("%CNFADDRESS%", "");
    blocD.replace("%CNFNAME%",  "");
    blocD.replace("%CNFMODEL%",  "");
    blocD.replace("%CNFNAMEVALID%",  "aria-invalid='true'");
    blocD.replace("%GENPAIRING%",  "");
  }
  //
  return blocD;
}

String getHTMLforDevice(uint8_t d, bool withLines)
{
  String blocD = html_device;
  Device* dev = hub.getDevice(d);
  blocD.replace("#D#", String(d));
  blocD.replace("%DEVCOUNT%", String(d + 1));
  if (withLines && dev)
  {
    blocD.replace("%GENLINE%", getHTMLforDeviceLine(d));
  } else {
    blocD.replace("%GENLINE%", "");
  }
  return blocD;
}

String getHTMLforChildLine(uint8_t d, uint8_t c)
{
  String blocC = html_child_line;
  String kcnf;
  Device* dev = hub.getDevice(d);
  if (dev)
  {
    Child* ch = dev->getChild(c);
    blocC.replace("#D#", String(d));
    blocC.replace("#C#", String(c));
    if (ch != nullptr)
    {
      blocC.replace("%CNFC_ID%", String(ch->getId()));
      blocC.replace("%CNFC_DTYPE%", String(ch->getDataType()));
      blocC.replace("%CNFC_LABEL%", String(ch->getLabel()));
      blocC.replace("%CNFC_LABELVALID%", strlen(ch->getLabel()) == 0 ? "aria-invalid='true'" : String(ch->getLabel()));
      const char* elementtype[] = {"Binary sensor", "Numeric sensor", "Switch", "Light", "Cover", "Fan", "HVac", "Select", "Trigger", "Event", "Tag", "Text", "Input", "Custom", "Date", "Time", "Datetime", "Button"};
      blocC.replace("%CNFC_STYPE_INT%", String((int)ch->getElementType()));
      blocC.replace("%CNFC_STYPE_STR%", elementtype[(int)ch->getElementType()]);
      String cnfcDetail = "";
      int cat = ch->getCategory();
      switch (cat) {
        case 1:
          cnfcDetail += "&#9874;";
          break;
        case 2:
          cnfcDetail += "&#128208;";
          break;
        default:
          cnfcDetail += "&#128065;";
          break;
      }
      String habtn = "disabled";
      switch (ch->getElementType())
      {
        case E_BINARYSENSOR:
          blocC.replace("%CNFC_CLASS%", String(ch->getClass()));
          cnfcDetail += "<dfn>" + String(ch->getClass()) + "</dfn>";
          blocC.replace("%CNFC_CATEGORY%", String(cat));
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
          habtn = "";
          break;
        case E_NUMERICSENSOR:
          blocC.replace("%CNFC_CLASS%", String(ch->getClass()));
          cnfcDetail += "<dfn>" + String(ch->getClass()) + "</dfn>";
          blocC.replace("%CNFC_CATEGORY%", String(cat));
          blocC.replace("%CNFC_UNIT%", String(ch->getUnit()));
          cnfcDetail += "<dfn>" + String(ch->getUnit()) + "</dfn>";
          blocC.replace("%CNFC_EXPIRE%", String(ch->getExpire()));
          blocC.replace("%CNFC_OPT%", "");
          blocC.replace("%CNFC_IMIN%", "");
          blocC.replace("%CNFC_IMAX%", "");
          blocC.replace("%CNFC_IDIV%", "");
          blocC.replace("%CNFC_MINI%", ch->getMini() == LONG_MIN ? "" : String(ch->getMini()));
          blocC.replace("%CNFC_MAXI%", ch->getMaxi() == LONG_MAX ? "" : String(ch->getMaxi()));
          blocC.replace("%CNFC_CA%", String(ch->getCoefA()));
          blocC.replace("%CNFC_CB%", String(ch->getCoefB()));
          if (ch->getMini() != LONG_MIN || ch->getMaxi() != LONG_MAX || ch->getCoefA() != 1.0 || ch->getCoefB() != 0.0)
          {
            cnfcDetail += "&#9998;";
          }
          habtn = "";
          break;
        case E_SELECT:
          blocC.replace("%CNFC_CLASS%", "");
          blocC.replace("%CNFC_CATEGORY%", String(cat));
          blocC.replace("%CNFC_UNIT%", "");
          blocC.replace("%CNFC_EXPIRE%", "");
          blocC.replace("%CNFC_OPT%", String(ch->getSelectOptions()));
          blocC.replace("%CNFC_IMIN%", "");
          blocC.replace("%CNFC_IMAX%", "");
          blocC.replace("%CNFC_IDIV%", "");
          blocC.replace("%CNFC_MINI%", "");
          blocC.replace("%CNFC_MAXI%", "");
          blocC.replace("%CNFC_CA%", "");
          blocC.replace("%CNFC_CB%", "");
          habtn = "";
          break;
        case E_INPUTNUMBER:
          blocC.replace("%CNFC_CLASS%", "");
          blocC.replace("%CNFC_CATEGORY%", String(cat));
          blocC.replace("%CNFC_UNIT%", String(ch->getUnit()));
          blocC.replace("%CNFC_EXPIRE%", "");
          blocC.replace("%CNFC_OPT%", "");
          blocC.replace("%CNFC_IMIN%", String(ch->getNumberMin()));
          blocC.replace("%CNFC_IMAX%", String(ch->getNumberMax()));
          blocC.replace("%CNFC_IDIV%", String(ch->getNumberDiv()));
          blocC.replace("%CNFC_MINI%", "");
          blocC.replace("%CNFC_MAXI%", "");
          blocC.replace("%CNFC_CA%", "");
          blocC.replace("%CNFC_CB%", "");
          habtn = "";
          break;
        default:
          blocC.replace("%CNFC_CLASS%", "");
          blocC.replace("%CNFC_CATEGORY%", "0");
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
      blocC.replace("%CNFC_DETAIL%", cnfcDetail);
      blocC.replace("%CNFC_BTN%", habtn);
    } else
    {
      blocC.replace("%CNFC_ID%", "1");
      blocC.replace("%CNFC_LABEL%", "");
      blocC.replace("%CNFC_LABELVALID%", "aria-invalid='true'");
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
      blocC.replace("%CNFC_BTN%", "");
    }
  }
  return blocC;
}

void sendConfigDevices()
{
  String blocD = "";
  if (hub.getNbDevice() > 0)
  {
    for (int d = 0; d < hub.getNbDevice(); d++)
    {
      blocD += getHTMLforDevice(d, true);
    }
  } else
  {
    blocD = "<mark class=\"row\">📌 To add a new device, you must Active pairing below, and press the CFG button when powering ON the module.</mark>";
  }
  docJson.clear();
  docJson["cmd"] = "html";
  docJson["#conf_dev"] = blocD;
  String js;
  serializeJson(docJson, js);
  ws.textAll(js);
}

void sendConfigChilds()
{
  for (uint8_t d = 0; d < hub.getNbDevice(); d++)
  {
    Device* dev = hub.getDevice(d);
    for (uint8_t c = 0; c < dev->getNbChild(); c++)
    {
      docJson.clear();
      docJson["cmd"] = "childnotify";
      docJson["conf_child_" + String(d) + "_" + String(c)] = getHTMLforChildLine(d, c);
      String js;
      serializeJson(docJson, js);
      ws.textAll(js);
    }
  }
}

void notifyConfig()
{
  if (ws.availableForWriteAll())
  {
    sendConfigDevices();
    sendConfigChilds();
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

void notifyDeviceBloc(uint8_t d)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "blocnotify";
    docJson["conf_" + String(d)] = getHTMLforDevice(d, false);
    String js = "";
    serializeJson(docJson, js);
    //DEBUGln(js);
    ws.textAll(js);
  }
}

void notifyDeviceLine(uint8_t d)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "devnotify";
    docJson["conf_dev_" + String(d)] = getHTMLforDeviceLine(d);
    String js = "";
    serializeJson(docJson, js);
    //DEBUGln(js);
    ws.textAll(js);
  }
}

void notifyChildLine(uint8_t d, uint8_t c)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "childnotify";
    docJson["conf_child_" + String(d) + "_" + String(c)] = getHTMLforChildLine(d, c);
    String js = "";
    serializeJson(docJson, js);
    //DEBUGln(js);
    ws.textAll(js);
  }
}

void notifyUploadMessage(String text)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "uplmsg";
    docJson["text"] = text;
    String js = "";
    serializeJson(docJson, js);
    //DEBUGln(js);
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
    String p1 = getValue((char*)data, ';', 1);
    String p2 = getValue((char*)data, ';', 2);
    String p3 = getValue((char*)data, ';', 3);
    if (cmd == "logpacket")
    {
      logPacket = (p1 == "1");
    }
    if (cmd == "pairing")
    {
      pairingActive = (p1 == "1");
    }
    if (cmd == "newadr")
    {
      uint8_t d = p1.toInt();
      uint8_t adr = p2.toInt();
      DEBUGf("New address for device %d = %d\n", d, adr);
      Device* dev = hub.getDevice(d);
      if (dev)
      {
        dev->setNewAddress(adr);
        notifyDeviceLine(d);
      }
    }
    if (cmd == "endpairing")
    {
      uint8_t d = p1.toInt();
      DEBUGf("Stop pairing for device %d\n", d);
      Device* dev = hub.getDevice(d);
      if (dev)
      {
        rl_configParam_t cnfp;
        memset(&cnfp, 0, sizeof(cnfp));
        RLcomm.publishConfig(dev->getAddress(), UIDcode, (rl_configs_t*)&cnfp, C_END);
        dev->setPairing(false);
        notifyDeviceBloc(d);
        notifyDeviceLine(d);
      }
    }
    if (cmd == "uplmodule")
    {
      if (p1 == "open")
      {
        String lst = getFirmwareList(p2); // in firt because it use "docJson"
        docJson.clear();
        docJson["cmd"] = "html";
        docJson["#upllist"] = lst;
        String js;
        serializeJson(docJson, js);
        ws.textAll(js);
      }
      if (p1 == "send" && p2 != "" && p3 != "")
      {
        needUploadFilename = p2 + ";" + p3;
      }
      if (p1 == "del" && p2 != "")
      {
        SPIFFS.remove("/hex/" + p2);
        docJson.clear();
        docJson["cmd"] = "html";
        docJson["#upllist"] = getFirmwareList("0");
        String js;
        serializeJson(docJson, js);
        ws.textAll(js);
      }
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
        html.replace("%CNFDIST%", String(RadioRange));
        html.replace("%LORAOK%", loraOK ? "" : "<i style='color:#FF0000'>Erreur</i>");

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
        html.replace("%CNFPAIR%", pairingActive ? "checked":"");
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
  /* for debug
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
      DEBUGln("Config file is saved");
    } else {
      DEBUGln("** Error saving Config file");
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
    } else if (filename.endsWith(".hex"))
    {
      request->_tempFile = SPIFFS.open("/hex/" + filename, "w");
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
  server.serveStatic("/hex", SPIFFS, "/hex");
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
