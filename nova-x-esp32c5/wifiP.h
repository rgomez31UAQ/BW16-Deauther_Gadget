#pragma once
#include <WiFi.h>
#include <cstdint>
#include <string>
#include <array>

#define STORE_LEN 64
#define PER_PKT 3

struct macAddr {
  uint8_t addr[6];
};

struct BSSIDInfo {
  std::array<uint8_t, 6> bssid;
  std::string ssid;
  bool encrypted; 
  
  BSSIDInfo(const uint8_t* mac, const char* name, bool enc = false) : ssid(name), encrypted(enc) {
    memcpy(bssid. data(), mac, 6);
  }
  
  bool operator==(const BSSIDInfo& other) const {
    return bssid == other.bssid;
  }
};

namespace nx {
  class wifi {
  private:
    uint8_t currentChannel = 0;
    
    // Processes scan results by filtering duplicates and updating channel AP map
    void processScanResults(int scanCount, uint8_t channel, int& totalScan, bool verbose = false);
    
  public:
    static const uint8_t channelList[37];
    static const size_t channelCount;
    
    // Scan variables
    size_t channelIndex = 0;
    unsigned long lastScan = 0;
    unsigned long scanInterval = 300;
    bool scanComplete = false;
    
    void init();
    void debug_print(const char* context);
    
    // MAC management
    void storeMac(const uint8_t* mac);
    bool checkedMac(const uint8_t* mac);
    void clearMacStored();
    
    // Channel management
    void setBandChannel(uint8_t channel);
    
    // Scan functions
    int scanNetwork(bool all = true, uint8_t channel = 0);
    void performProgressiveScan();
    
    // Deauth
    void txDeauthFrameAll();
    void txDeauthFrameChannel(uint8_t channel);
    void txDeauthFrameBSSID(const uint8_t* bssid, uint8_t channel);
    
    // Beacon
    void txBeaconFrame(const char* ssid, uint8_t channel, const uint8_t* bssid, bool encrypted = false);
    
    
    // Authentication 
    void txAuthFrame(const uint8_t* bssid, uint8_t channel);
    void txAuthFlood();
    
    // Association 
    void txAssocFrame(const uint8_t* bssid, const char* ssid, uint8_t channel);
    void txAssocFlood();
  };
}