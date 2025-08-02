# WiFi Scanning and Configuration

## Overview
The Deauth Auto system includes a powerful WiFi scanning feature that allows users to discover nearby networks and automatically configure target SSID and MAC addresses.

## Features

### 🔍 WiFi Network Scanning
- **Scan Button**: Click "🔍 Scan WiFi Networks" to discover all available WiFi networks
- **Real-time Results**: Shows SSID, MAC address, signal strength (RSSI), and channel information
- **Signal Quality**: Automatically categorizes networks as "Strong" (>-50 dBm) or "Weak" (≤-50 dBm)

### 📋 Network Selection
- **SSID Dropdown**: Select target network SSID from scanned results
- **MAC Dropdown**: Select corresponding MAC address from the same scan
- **Auto-fill**: Selected values automatically populate the configuration fields
- **Visual Feedback**: Clear status messages confirm selections

### 💾 Configuration Saving
- **Settings.json**: All selections are saved to the `settings.json` file
- **Persistent Storage**: Configuration persists across device reboots
- **Validation**: Form validation ensures all required fields are completed

## How to Use

1. **Access the Web Interface**
   - Connect to the device's WiFi network
   - Open browser and navigate to the device IP (default: 192.168.1.254)
   - Login with admin credentials

2. **Scan for Networks**
   - Click "🔍 Scan WiFi Networks" button
   - Wait for scan to complete (usually 5-10 seconds)
   - Review the scan results showing total networks found

3. **Select Target Network**
   - From the SSID dropdown, select your target network
   - From the MAC dropdown, select the corresponding MAC address
   - Both fields will auto-fill with your selection

4. **Save Configuration**
   - Click "💾 Save Configuration" button
   - Configuration is saved to `settings.json`
   - Success message confirms the save operation

## Technical Details

### API Endpoints
- `GET /scan-wifi`: Returns JSON array of discovered networks
  ```json
  {
    "networks": [
      {
        "ssid": "NetworkName",
        "mac": "AA:BB:CC:DD:EE:FF",
        "rssi": -45,
        "channel": 6,
        "encryption": 3
      }
    ]
  }
  ```

### Configuration File Structure
```json
{
  "ip": "192.168.1.254",
  "target_ssid": "SelectedNetwork",
  "target_mac": "AA:BB:CC:DD:EE:FF",
  "login_page_name": "login.html",
  "total_send_pkt": 100,
  "username": "admin",
  "password": "password123"
}
```

### Signal Strength Guidelines
- **Excellent**: -30 to -50 dBm (Strong networks)
- **Good**: -50 to -60 dBm
- **Fair**: -60 to -70 dBm
- **Poor**: Below -70 dBm (Weak networks)

## Troubleshooting

### Scan Not Working
- Ensure device is in WiFi scanning mode
- Check if WiFi is enabled on the device
- Try refreshing the page and scanning again

### No Networks Found
- Move closer to WiFi networks
- Check if networks are broadcasting SSID
- Ensure device has proper WiFi permissions

### Save Fails
- Check if all required fields are filled
- Verify device has write permissions to SPIFFS
- Check available storage space

## Security Notes
- Only scan networks you have permission to access
- MAC addresses are used for legitimate network management
- All configuration is stored locally on the device 