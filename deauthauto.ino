#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include "webfile.h"  // Include the generated web files header

// Configuration structure
struct Config {
  String ip;
  String targetSSID;
  String targetMAC;
  String loginPageName;
  int totalSendPkt;
};

Config config;
ESP8266WebServer server(80);

// Function to parse IP address string to IPAddress
bool parseIPAddress(const String& ipString, IPAddress& ip) {
  int parts[4];
  int partIndex = 0;
  int startIndex = 0;
  
  for (int i = 0; i < ipString.length() && partIndex < 4; i++) {
    if (ipString.charAt(i) == '.') {
      parts[partIndex] = ipString.substring(startIndex, i).toInt();
      partIndex++;
      startIndex = i + 1;
    }
  }
  
  if (partIndex == 3) {
    parts[partIndex] = ipString.substring(startIndex).toInt();
    partIndex++;
  }
  
  if (partIndex == 4) {
    for (int i = 0; i < 4; i++) {
      if (parts[i] < 0 || parts[i] > 255) {
        return false;
      }
    }
    ip = IPAddress(parts[0], parts[1], parts[2], parts[3]);
    return true;
  }
  
  return false;
}

// Function to load configuration from settings.json
bool loadConfig() {
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount SPIFFS");
    return false;
  }
  
  if (!SPIFFS.exists("/settings.json")) {
    Serial.println("settings.json not found, creating default config");
    createDefaultConfig();
    return false;
  }
  
  File file = SPIFFS.open("/settings.json", "r");
  if (!file) {
    Serial.println("Failed to open settings.json");
    return false;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("Failed to parse settings.json");
    return false;
  }
  
  // Load configuration values
  config.ip = doc["ip"].as<String>();
  config.targetSSID = doc["target_ssid"].as<String>();
  config.targetMAC = doc["target_mac"].as<String>();
  config.loginPageName = doc["login_page_name"].as<String>();
  config.totalSendPkt = doc["total_send_pkt"].as<int>();
  
  Serial.println("Configuration loaded successfully:");
  Serial.println("IP: " + config.ip);
  Serial.println("Target SSID: " + config.targetSSID);
  Serial.println("Target MAC: " + config.targetMAC);
  Serial.println("Login Page Name: " + config.loginPageName);
  Serial.println("Total Send Packets: " + String(config.totalSendPkt));
  
  return true;
}

// Function to create default configuration
void createDefaultConfig() {
  DynamicJsonDocument doc(1024);
  
  doc["ip"] = "192.168.1.254";
  doc["target_ssid"] = "Redmi 9a";
  doc["target_mac"] = "AA:BB:CC:DD:EE:FF";
  doc["login_page_name"] = "login.html";
  doc["total_send_pkt"] = 100;
  
  File file = SPIFFS.open("/settings.json", "w");
  if (!file) {
    Serial.println("Failed to create settings.json");
    return;
  }
  
  serializeJson(doc, file);
  file.close();
  Serial.println("Default settings.json created");
}

// Function to save configuration
bool saveConfig() {
  DynamicJsonDocument doc(1024);
  
  doc["ip"] = config.ip;
  doc["target_ssid"] = config.targetSSID;
  doc["target_mac"] = config.targetMAC;
  doc["login_page_name"] = config.loginPageName;
  doc["total_send_pkt"] = config.totalSendPkt;
  
  File file = SPIFFS.open("/settings.json", "w");
  if (!file) {
    Serial.println("Failed to open settings.json for writing");
    return false;
  }
  
  serializeJson(doc, file);
  file.close();
  Serial.println("Configuration saved successfully");
  return true;
}

// Web server handlers
void sendProgmem(const char* ptr, size_t size, const char* type) {
    server.sendHeader("Content-Length", String(size));
    server.sendHeader("Cache-Control", "max-age=3600");
    server.send_P(200, type, ptr, size);
}



void handleSave() {
  Serial.println("Save request received");
  
  // Debug: Print all arguments
  for (int i = 0; i < server.args(); i++) {
    Serial.print("Arg ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(server.argName(i));
    Serial.print(" = ");
    Serial.println(server.arg(i));
  }
  
  // Update configuration
  if (server.hasArg("ip")) {
    config.ip = server.arg("ip");
    Serial.println("IP updated: " + config.ip);
  }
  if (server.hasArg("ssid")) {
    config.targetSSID = server.arg("ssid");
    Serial.println("SSID updated: " + config.targetSSID);
  }
  if (server.hasArg("mac")) {
    config.targetMAC = server.arg("mac");
    Serial.println("MAC updated: " + config.targetMAC);
  }
  if (server.hasArg("login")) {
    config.loginPageName = server.arg("login");
    Serial.println("Login page updated: " + config.loginPageName);
  }
  if (server.hasArg("packets")) {
    config.totalSendPkt = server.arg("packets").toInt();
    Serial.println("Packets updated: " + String(config.totalSendPkt));
  }
  
  // Save configuration
  if (saveConfig()) {
    Serial.println("Configuration saved successfully");
    server.send(200, "text/plain", "OK");
    
    // Reboot after successful save
    Serial.println("Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();
  } else {
    Serial.println("Failed to save configuration");
    server.send(500, "text/plain", "ERROR");
  }
}

void handleConfig() {
  DynamicJsonDocument doc(1024);
  doc["ip"] = config.ip;
  doc["target_ssid"] = config.targetSSID;
  doc["target_mac"] = config.targetMAC;
  doc["login_page_name"] = config.loginPageName;
  doc["total_send_pkt"] = config.totalSendPkt;
  
  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Deauth Auto Starting...");
  
  // Load configuration
  if (!loadConfig()) {
    Serial.println("Using default configuration");
  }
  
  // Parse IP address from config
  IPAddress local_IP, gateway, subnet;
  if (parseIPAddress(config.ip, local_IP)) {
    gateway = local_IP;
    subnet = IPAddress(255, 255, 255, 0);
  } else {
    // Fallback to default IP if parsing fails
    local_IP = IPAddress(192, 168, 1, 254);
    gateway = IPAddress(192, 168, 1, 254);
    subnet = IPAddress(255, 255, 255, 0);
    Serial.println("Invalid IP in config, using default: 192.168.1.254");
  }
  
  // Setup WiFi
  WiFi.mode(WIFI_AP_STA);
  if (WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("AP IP configured successfully");
  } else {
    Serial.println("AP IP configuration failed");
  }
  
  // Use SSID from config or default to "Redmi 9a"
  String apSSID = config.targetSSID;
  if (apSSID.length() == 0) {
    apSSID = "Redmi 9a";
    Serial.println("No SSID in config, using default: Redmi 9a");
  }
  WiFi.softAP(apSSID.c_str());
  
  // Setup web server
  server.on("/", HTTP_GET, []() {
      sendProgmem((char*)index_html, index_html_len, "text/html");
  });
  server.on("/index.html", HTTP_GET, []() {
      sendProgmem((char*)index_html, index_html_len, "text/html");
  });
  server.on("/style.css", HTTP_GET, []() {
      sendProgmem((char*)style_css, style_css_len, "text/css");
  });
  server.on("/script.js", HTTP_GET, []() {
      sendProgmem((char*)script_js, script_js_len, "text/javascript");
  });
  server.on("/save", HTTP_POST, handleSave);
  server.on("/config", HTTP_GET, handleConfig);
  server.begin();
  
  Serial.println("Web server started");
  Serial.print("Access point: ");
  Serial.println(apSSID);
  Serial.println("Password:");
  Serial.print("Web interface: http://");
  Serial.println(local_IP.toString());
}

void loop() {
  server.handleClient();
  delay(10);
}
