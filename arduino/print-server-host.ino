#include <M5Stack.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "WebServer.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include "Adafruit_Thermal.h"
#include "app.h"

Adafruit_Thermal printer(&Serial);     // Pass addr to printer constructor

const IPAddress apIP(192, 168, 4, 1);
const char* apSSID = "M5STACK_M";
const char* apMPD = "321!321!";
boolean settingMode;
String ssidList;
String wifi_ssid;
String wifi_password;

// DNSServer dnsServer;
WebServer webServer(80);

// wifi config store
Preferences preferences;

void setup() {
  m5.begin();
  preferences.begin("wifi-config");

  delay(10);
  if (restoreConfig()) {
    if (checkConnection()) {
      settingMode = false;
      startWebServer();
      return;
    }
  }
  settingMode = true;
  setupMode();
  
  int heatTime = 125;
  int heatInterval = 40;
  char printDensity = 20; 
  char printBreakTime = 2;
  
  // fil vert 2: fil jaune : 5 fil noir : gnd
  Serial.begin(19200, SERIAL_8N1, 2, 5);
  printer.begin();
  initPrinter(heatTime, heatInterval, printDensity, printBreakTime);
}

void loop() {
  if (settingMode) {
  }
  webServer.handleClient();
}

boolean restoreConfig() {
  wifi_ssid = preferences.getString("");
  wifi_password = preferences.getString("");
  Serial.print("WIFI-SSID: ");
  M5.Lcd.print("WIFI-SSID: ");
  Serial.println(wifi_ssid);
  M5.Lcd.println(wifi_ssid);
  Serial.print("WIFI-PASSWD: ");
  M5.Lcd.print("WIFI-PASSWD: ");
  Serial.println(wifi_password);
  M5.Lcd.println(wifi_password);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  if(wifi_ssid.length() > 0) {
    return true;
} else {
    return false;
  }
}

boolean checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  M5.Lcd.print("Waiting for Wi-Fi connection");
  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      M5.Lcd.println();
      Serial.println("Connected!");
      M5.Lcd.println("Connected!");
      return (true);
    }
    delay(500);
    Serial.print(".");
    M5.Lcd.print(".");
    count++;
  }
  Serial.println("Timed out.");
  M5.Lcd.println("Timed out.");
  return false;
}

void startWebServer() {
  if (settingMode) {
    Serial.print("Starting Web Server at ");
    M5.Lcd.print("Starting Web Server at ");
    Serial.println(WiFi.softAPIP());
    M5.Lcd.println(WiFi.softAPIP());

     webServer.on("/on", handleRoot);

  webServer.on("/", handleRoot);

  webServer.on("/print", handlePrint);
  
  webServer.on("/config", handleConfig);

  webServer.onNotFound(handleNotFound);

  }
  else {
    Serial.print("Starting Web Server at ");
    M5.Lcd.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
    M5.Lcd.println(WiFi.localIP());
    webServer.on("/", []() {
      String s = "<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("STA mode", s));
    });
    webServer.on("/reset", []() {
      // reset the wifi config
      preferences.remove("WIFI_SSID");
      preferences.remove("WIFI_PASSWD");
      String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
  }
  webServer.begin();
}

void setupMode() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("");
  M5.Lcd.println("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID,apMPD);
  WiFi.mode(WIFI_MODE_AP);
  // WiFi.softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet);
  // WiFi.softAP(const char* ssid, const char* passphrase = NULL, int channel = 1, int ssid_hidden = 0);
  // dnsServer.start(53, "*", apIP);
  startWebServer();
  Serial.print("Starting Access Point at \"");
  M5.Lcd.print("Starting Access Point at \"");
  Serial.print(apSSID);
  M5.Lcd.print(apSSID);
  Serial.println("\"");
  M5.Lcd.println("\"");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}

void handlePrint(){
  if (webServer.hasArg("data")== false){ //Check if content received
    webServer.send(400, "text/plain", "{\"code\": 400}");
  } else {
    String content = webServer.arg("data");
    const size_t bufferSize = JSON_ARRAY_SIZE(480) + JSON_OBJECT_SIZE(1) + 1850;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject&  parsed= jsonBuffer.parseObject(content);
    int size = 480;
    uint8_t adalogo_data[size];
    for(int i=0;i<size;i++){
      adalogo_data[i] = (uint8_t)parsed["data"][i];
    }
    if( 1 ) {
      printer.printBitmap(384, 10, adalogo_data, false);
      delay(200);
      webServer.send(200, "text/plain", "{\"code\": 200}"); 
    } else {
      webServer.send(503, "text/plain", "{\"code\": 503}");
    }
  }
}
void handleConfig(){
  if (webServer.hasArg("data")== false){ //Check if content received
    webServer.send(400, "text/plain", "No config...");
  } else {
    String content = webServer.arg("data");
    const size_t bufferSize = JSON_OBJECT_SIZE(4) + 70;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject&  parsed= jsonBuffer.parseObject(content);

    int heatTime = parsed["heatTime"];
    int heatInterval = parsed["heatInterval"];
    char printDensity = parsed["printDensity"];
    char printBreakTime = parsed["printBreakTime"];

    initPrinter(heatTime, heatInterval, printDensity, printBreakTime);
    
    webServer.send(200, "text/plain", "Done");
  }
 
}
void handleRoot() {
  
  String content = "";
  content += FPSTR(app_html);
  webServer.send(200, "text/html", content);
  
}

void handleNotFound(){

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";
  for (uint8_t i=0; i<webServer.args(); i++){
    message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
  }
  webServer.send(404, "text/plain", message);
  
}
void initPrinter(int &heatTime, int &heatInterval, char &printDensity, char &printBreakTime){
 //Modify the print speed and heat
 printer.write(27);
 printer.write(55);
 printer.write(7); //Default 64 dots = 8*('7'+1)
 printer.write(heatTime); //Default 80 or 800us
 printer.write(heatInterval); //Default 2 or 20us
 //Modify the print density and timeout
 printer.write(18);
 printer.write(35);
 int printSetting = (printDensity<<4) | printBreakTime;
 printer.write(printSetting); //Combination of printDensity and printBreakTime
}