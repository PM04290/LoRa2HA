#pragma once
#include "include_html.hpp"  // inside file

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
//AsyncEventSource events("/events");

bool logPacket = false;
bool pairingActive = false;

size_t content_len;


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
      if (error)
      {
        DEBUGln("failed to deserialize config file");
      } else
      {
        for (JsonVariant obj : docJson.as<JsonArray>())
        {
          String n = obj["name"].as<String>();
          Flist += "<option>" + n + "</option>";
        }
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

String getHTML_DeviceHeader(uint8_t idx)
{
  String blocD = html_device_header;
  CDevice* dev = hub.getDeviceByAddress(idx);
  blocD.replace("#D#", String(idx));
  if (dev)
  {
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%CNFNAME%", String(dev->getName()));
    blocD.replace("%CNFMODEL%", String(dev->getModel()));
  } else {
    blocD.replace("%CNFADDRESS%", "");
    blocD.replace("%CNFNAME%", "");
    blocD.replace("%CNFMODEL%", "");
  }
  //
  return blocD;
}

String getHTML_DevicePairing(uint8_t idx)
{
  String blocD = "";
  CDevice* dev = hub.getDeviceByAddress(idx);
  if (dev && dev->getPairing())
  {
    int newIdx = idx + 1;
    CDevice* newdev = hub.getDeviceByAddress(newIdx);
    while (newdev) {
      newIdx++;
      newdev = hub.getDeviceByAddress(newIdx);
    }
    blocD = html_device_pairing;
    blocD.replace("#D#", String(idx));
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%CNFNEWADR%", String(newIdx));
    //
  }
  //
  return blocD;
}

String getHTML_DeviceForm(uint8_t idx)
{
  String blocD = html_device_form;
  CDevice* dev = hub.getDeviceByAddress(idx);
  blocD.replace("#D#", String(idx));
  if (dev)
  {
    blocD.replace("%CNFMODEL%", dev->getModel());
    blocD.replace("%CNFNAME%", dev->getName());
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%GENHEADER%", getHTML_DeviceHeader(idx));
    blocD.replace("%GENPAIRING%", getHTML_DevicePairing(idx));
  }
  return blocD;
}

String getHTMLforChildLine(uint8_t address, uint8_t id)
{
  String blocC = html_child_line;
  String kcnf;
  CDevice* dev = hub.getDeviceByAddress(address);
  if (dev)
  {
    CEntity* ent = dev->getEntityById(id);
    blocC.replace("#D#", String(address));
    blocC.replace("#C#", String(id));
    if (ent)
    {
      blocC.replace("%CNFC_ID%", String(ent->getId()));
      blocC.replace("%CNFC_DTYPE%", String(ent->getDataType()));
      blocC.replace("%CNFC_LABEL%", String(ent->getLabel()));
      blocC.replace("%CNFC_LABELVALID%", strlen(ent->getLabel()) == 0 ? "aria-invalid='true'" : String(ent->getLabel()));
      const char* elementtype[] = {"Binary sensor", "Numeric sensor", "Switch", "Light", "Cover", "Fan", "HVac", "Select", "Trigger", "Event", "Tag", "Text", "Input", "Custom", "Date", "Time", "Datetime", "Button"};
      blocC.replace("%CNFC_STYPE_INT%", String((int)ent->getElementType()));
      blocC.replace("%CNFC_STYPE_STR%", elementtype[(int)ent->getElementType()]);
      String cnfcDetail = "";
      int cat = ent->getCategory();
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
      switch (ent->getElementType())
      {
        case E_BINARYSENSOR:
          blocC.replace("%CNFC_CLASS%", String(ent->getClass()));
          cnfcDetail += "<dfn>" + String(ent->getClass()) + "</dfn>";
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
          blocC.replace("%CNFC_CLASS%", String(ent->getClass()));
          cnfcDetail += "<dfn>" + String(ent->getClass()) + "</dfn>";
          blocC.replace("%CNFC_CATEGORY%", String(cat));
          blocC.replace("%CNFC_UNIT%", String(ent->getUnit()));
          cnfcDetail += "<dfn>" + String(ent->getUnit()) + "</dfn>";
          blocC.replace("%CNFC_EXPIRE%", String(ent->getExpire()));
          blocC.replace("%CNFC_OPT%", "");
          blocC.replace("%CNFC_IMIN%", "");
          blocC.replace("%CNFC_IMAX%", "");
          blocC.replace("%CNFC_IDIV%", "");
          blocC.replace("%CNFC_MINI%", ent->getMini() == LONG_MIN ? "" : String(ent->getMini()));
          blocC.replace("%CNFC_MAXI%", ent->getMaxi() == LONG_MAX ? "" : String(ent->getMaxi()));
          blocC.replace("%CNFC_CA%", String(ent->getCoefA()));
          blocC.replace("%CNFC_CB%", String(ent->getCoefB()));
          if (ent->getMini() != LONG_MIN || ent->getMaxi() != LONG_MAX || ent->getCoefA() != 1.0 || ent->getCoefB() != 0.0)
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
          blocC.replace("%CNFC_OPT%", String(ent->getSelectOptions()));
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
          blocC.replace("%CNFC_UNIT%", String(ent->getUnit()));
          blocC.replace("%CNFC_EXPIRE%", "");
          blocC.replace("%CNFC_OPT%", "");
          blocC.replace("%CNFC_IMIN%", String(ent->getNumberMin()));
          blocC.replace("%CNFC_IMAX%", String(ent->getNumberMax()));
          blocC.replace("%CNFC_IDIV%", String(ent->getNumberDiv()));
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
  CDevice* dev = nullptr;
  while ((dev = hub.walkDevice(dev)))
  {
    blocD += getHTML_DeviceForm(dev->getAddress());
  }
  if (blocD == "") {
    blocD = "<mark class=\"row\">📌 To add a new device, you must active pairing below, and press the CFG button when powering ON the module.</mark>";
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
  CDevice* dev = nullptr;
  while ((dev = hub.walkDevice(dev)))
  {
    CEntity* ent = nullptr;
    while ((ent = dev->walkEntity(ent)))
    {
      uint8_t address = dev->getAddress();
      uint8_t id = ent->getId();
      docJson.clear();
      docJson["cmd"] = "childnotify";
      docJson["conf_child_" + String(address) + "_" + String(id)] = getHTMLforChildLine(address, id);
      String js;
      serializeJson(docJson, js);
      ws.textAll(js);
    }
  }
}

void notifyAllConfig()
{
  if (ws.availableForWriteAll())
  {
    sendConfigDevices();
    sendConfigChilds();
    ws.textAll("{\"cmd\":\"loadselect\"}");
  }
}

void notifyDeviceForm(uint8_t d)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "formnotify";
    docJson["form_" + String(d)] = getHTML_DeviceForm(d);
    String js = "";
    serializeJson(docJson, js);
    //DEBUGln(js);
    ws.textAll(js);
  }
}

void notifyDeviceHeader(uint8_t d)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "html";
    docJson["#header_" + String(d)] = getHTML_DeviceHeader(d);
    String js = "";
    serializeJson(docJson, js);
    //DEBUGln(js);
    ws.textAll(js);
  }
}

void notifyDevicePairing(uint8_t d)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "html";
    docJson["#pairing_" + String(d)] = getHTML_DevicePairing(d);
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

void notifyUploadMessage(String text)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "uplmsg";
    docJson["text"] = text;
    String js;
    serializeJson(docJson, js);
    //DEBUGln(js);
    ws.textAll(js);
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  String js;
  DEBUGln("handleWebSocketMessage");
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    bool needreload = false;
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
    if (cmd == "deldev")
    {
      hub.delDevice(p1.toInt());
      docJson.clear();
      docJson["cmd"] = "remove";
      docJson["#form_"] = p1;
      serializeJson(docJson, js);
      ws.textAll(js);
    }
    if (cmd == "pairing")
    {
      pairingActive = (p1 == "1");
    }
    if (cmd == "newadr")
    {
      uint8_t oldAdr = p1.toInt();
      uint8_t newAdr = p2.toInt();
      DEBUGf("New address for device %d = %d\n", oldAdr, newAdr);
      CDevice* dev = hub.getDeviceByAddress(oldAdr);
      if (dev && (oldAdr != newAdr))
      {
        docJson.clear();
        docJson["cmd"] = "remove";
        docJson["#form_"] = String(oldAdr);
        serializeJson(docJson, js);
        ws.textAll(js);
        //
        dev->setAddress(oldAdr, newAdr);
        //
        notifyDeviceForm(newAdr);
        notifyDeviceHeader(newAdr);
        notifyDevicePairing(newAdr);
        CEntity* ent = nullptr;
        while ((ent = dev->walkEntity(ent)))
        {
          uint8_t id = ent->getId();
          docJson.clear();
          docJson["cmd"] = "childnotify";
          docJson["conf_child_" + String(newAdr) + "_" + String(id)] = getHTMLforChildLine(newAdr, id);
          serializeJson(docJson, js);
          ws.textAll(js);
        }
      }
    }
    if (cmd == "endpairing")
    {
      uint8_t address = p1.toInt();
      DEBUGf("Stop pairing for device %d\n", address);
      if (hub.endPairing(address))
      {
        notifyDevicePairing(address);
      }
    }
    if (cmd == "uplmodule")
    {
      if (p1 == "open")
      {
        String lst = getFirmwareList(p2); // in first because it use "docJson"
        docJson.clear();
        docJson["cmd"] = "html";
        docJson["#upllist"] = lst;
        serializeJson(docJson, js);
        ws.textAll(js);
      }
      if (p1 == "send" && p2 != "" && p3 != "")
      {
        needUploadFilename = p2 +  ";" + p3;
      }
      if (p1 == "del" && p2 != "")
      {
        SPIFFS.remove("/hex/" + p2);
        docJson.clear();
        docJson["cmd"] = "html";
        docJson["#upllist"] = getFirmwareList("0");
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
      notifyAllConfig();
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
        html.replace("%CNFPAIR%", pairingActive ? "checked" : "");
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
  if (request->hasParam("cnfdev", true))
  {
    // ! ! ! POST data must contain ONLY 1 Device
    JsonDocument docJSon;
    //JsonObject Jconfig = docJSon.to<JsonObject>();
    int params = request->params();
    CDevice* dev = nullptr;
    CEntity* ent = nullptr;
    for (int i = 0; i < params; i++)
    {
      AsyncWebParameter* p = request->getParam(i);
      String str = p->name();
      if (str == "cnfdev") continue;
      // exemple : dev_0_address
      //      or : dev_1_child_2_label
      String valdev = getValue(str, '_', 0);
      String keydev = getValue(str, '_', 1);
      String attrdev = getValue(str, '_', 2);
      String keychild = getValue(str, '_', 3);
      String attrchild = getValue(str, '_', 4);
      String sval(p->value().c_str());

      DEBUGf("[%s][%s][%s][%s][%s] = %s\n", valdev.c_str(), keydev.c_str(), attrdev.c_str(), keychild.c_str(), attrchild.c_str(), sval.c_str());
      if (valdev == "dev" && keydev.toInt() > 0)
      {
        dev = hub.getDeviceByAddress(keydev.toInt());
        if (dev) {
          if (attrdev == "childs") {
            ent = hub.getEntityById(keydev.toInt(), keychild.toInt());
            if (ent) {
              ent->setAttr(attrchild, sval, 0);
            }
          }
        }
      }
    }
    //
    if (dev) {
      dev->publishConfig();
    }
    hub.saveConfig();
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
    if (request->hasParam("cnfdist", true))
    {
      RadioRange = request->getParam("cnfdist", true)->value().toInt();
    }
    vpnNeeded = false;
    if (request->hasParam("cnfvpn", true))
    {
      vpnNeeded = request->getParam("cnfvpn", true)->value().toInt() > 0;
    }

    DEBUGln(AP_ssid);
    DEBUGln(RadioFreq);
    DEBUGln(RadioRange);
    DEBUGln(vpnNeeded);
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
    EEPROM.writeByte(EEPROM_DATA_RANGE, RadioRange);
    EEPROM.writeByte(EEPROM_DATA_NVPN, !vpnNeeded);
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
  //
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  //
  /*
    events.onConnect([](AsyncEventSourceClient * client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hi!", NULL, millis(), 10000);
    });
    server.addHandler(&events);*/
  //
  server.begin();

  Update.onProgress(updateProgress);
  DEBUGln("HTTP server started");
}
