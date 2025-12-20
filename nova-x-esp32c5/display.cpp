#include "display.h"
#include <memory>
#include <cmath>

void nx::menu::debug_print(const std::string &ctx) {
  Serial.println(("[display] " + ctx).c_str());
}

void nx::menu::init() {
  display.setFont(u8g2_font_6x10_tr);
  fontAscent  = display.getAscent();
  fontDescent = display.getDescent();      
  fontHeight  = fontAscent - fontDescent;   

  Wire.begin(I2C_SDA, I2C_SCL);
  btn.setupButtons(btns, btnCount);
  tx.init();
  debug_print(display.begin() ? "display initialized" : "display init failed");
  adv.init();
}

int nx::menu::calcStartIndex(int sel) {
  if (menuSize > maxVisible && sel >= maxVisible -1) {
    int s = sel - (maxVisible - 1);
    return (s + maxVisible > menuSize) ? (menuSize - maxVisible) : s;
  }
  return 0;
}

int nx::menu::calcRectWidth(int textWidth) {
  int rectWidth = textWidth + padding * 2;
  return constrain(rectWidth, minRectWidth, visibleWidth);
}

int nx::menu::calcRectX(int rectWidth) {
  return baseX + (visibleWidth - rectWidth) / 2;
}

int nx::menu::calcTextX(int rectX, int rectWidth, int textWidth) {
  return rectX + (rectWidth - textWidth) / 2;
}

int nx::menu::calcTextY(int y) {
  int offset = (lineHeight - fontHeight) / 2 - (fontDescent / 2);
  return y + offset + fontAscent - 1;
}

int nx::menu::calcCheckboxRectWidth(const std::string &channel) {
  int channelWidth = display.getStrWidth(channel.c_str());
  int channelX = SCREEN_WIDTH - channelRightMargin - channelWidth;
  int rectWidth = channelX - channelLeftPadding - rectStartX;
  return (rectWidth < minRectWidth) ? minRectWidth : rectWidth;
}

void nx::menu::drawBorder() {
  display.setFontMode(1);
  display.setBitmapMode(1);
  display.drawLine(0, 0, 0, 21);
  display.drawLine(1, 0, 22, 0);
  display.drawLine(1, 1, 1, 20);
  display.drawLine(2, 1, 21, 1);
  display.drawLine(2, 19, 2, 2);
  display.drawLine(3, 2, 20, 2);
  display.drawLine(127, 1, 127, 22);
  display.drawLine(106, 0, 127, 0);
  display.drawLine(126, 2, 126, 21);
  display.drawLine(107, 1, 126, 1);
  display.drawLine(125, 20, 125, 3);
  display.drawLine(108, 2, 125, 2);
  display.drawLine(0, 41, 0, 62);
  display.drawLine(1, 62, 22, 62);
  display.drawLine(1, 42, 1, 61);
  display.drawLine(2, 61, 21, 61);
  display.drawLine(2, 60, 2, 43);
  display.drawLine(3, 60, 20, 60);
  display.drawLine(127, 40, 127, 61);
  display.drawLine(106, 62, 127, 62);
  display.drawLine(126, 42, 126, 61);
  display.drawLine(107, 61, 126, 61);
  display.drawLine(125, 60, 125, 43);
  display.drawLine(108, 60, 125, 60);
  display.drawEllipse(1, 31, 1, 4);
  display.drawEllipse(126, 31, 1, 4);
}

void nx::menu::renderRoundRect(int x, int y, int w, int h) {
  display.setDrawColor(1);
  display.drawRFrame(x, y, w, h, cornerRadius);
  display.drawRFrame(x + 1, y + 1, w - 2, h - 2, cornerRadius - 1);
}

void nx::menu::renderCheckbox(int x, int y, bool checked) {
  int boxY = y + (lineHeight - checkboxSize) / 2;
  display.drawFrame(x, boxY, checkboxSize, checkboxSize);
  if(checked) display.drawBox(x + 1, boxY + 1, checkboxSize - 2, checkboxSize - 2);
}

void nx::menu::renderLineWithCheckbox(const std::string &ssid, const std::string &channel, int y, bool checked, bool selected) {
  renderCheckbox(checkboxX, y, checked);
  
  int channelWidth = display.getStrWidth(channel.c_str());
  int channelX = SCREEN_WIDTH - channelRightMargin - channelWidth;
  int textY = calcTextY(y);
  
  if(selected) {
    int rectWidth = calcCheckboxRectWidth(channel);
    renderRoundRect(rectStartX, y, rectWidth, lineHeight);
  }
  
  display.drawStr(ssidStartX, textY, ssid.c_str());
  display.drawStr(channelX, textY, channel.c_str());
}

void nx::menu::renderSingleLine(const std::string &text, int y, bool drawRect) {
  int textWidth = display.getStrWidth(text.c_str());
  int rectWidth = calcRectWidth(textWidth);
  int rectX = calcRectX(rectWidth);
  int textX = calcTextX(rectX, rectWidth, textWidth);
  int textY = calcTextY(y);

  if (drawRect) renderRoundRect(rectX, y, rectWidth, lineHeight);
  display.drawStr(textX, textY, text.c_str());
}

void nx::menu::renderText(const std::vector<std::string> &items, int start, int y, int count) {
  for (int i = start; i < (int)items.size() && i < start + count; i++, y += lineHeight) {
    renderSingleLine(items[i], y, false);
  }
}

void nx::menu::drawInitialAnimation(const std::vector<std::string> &items, int idx) {
  int startIdx = calcStartIndex(idx);
  unsigned long startT = millis();
  unsigned long endT = startT + lineAnimDuration + maxVisible * lineDelay;

  while (millis() < endT) {
    unsigned long now = millis();
    display.clearBuffer();
    drawBorder();

    for (int i = 0; i < maxVisible && startIdx + i < menuSize; i++) {
      unsigned long itemStartT = startT + i * lineDelay;
      if (now < itemStartT) continue;

      float t = std::min(1.0f, float(now - itemStartT) / lineAnimDuration);
      float eased = 1.0f - powf(1.0f - t, 3.0f);

      int startY = SCREEN_HEIGHT + textOffsetY + i * lineHeight;
      int finalY = textOffsetY + i * lineHeight;
      int currentY = startY + int((finalY - startY) * eased);

      if (currentY > -lineHeight && currentY < SCREEN_HEIGHT) renderSingleLine(items[startIdx + i], currentY, (startIdx + i) == idx);
    }
    display.sendBuffer();
  }
}

void nx::menu::drawSelectionAnimation(const std::vector<std::string> &items, int newIdx) {
  int oldIdx = (prevSelected < 0 || prevSelected >= menuSize) ? 0 : prevSelected;
  int oldStart = calcStartIndex(oldIdx);
  int newStart = calcStartIndex(newIdx);

  int oldY = textOffsetY + (oldIdx - oldStart) * lineHeight;
  int newY = textOffsetY + (newIdx - newStart) * lineHeight;

  int oldTextW = display.getStrWidth(items[oldIdx].c_str());
  int newTextW = display.getStrWidth(items[newIdx].c_str());
  int oldRectW = calcRectWidth(oldTextW);
  int newRectW = calcRectWidth(newTextW);
  int oldX = calcRectX(oldRectW);
  int newX = calcRectX(newRectW);

  unsigned long startT = millis();
  unsigned long endT = startT + animDuration;

  while (millis() < endT) {
    float t = std::min(1.0f, float(millis() - startT) / animDuration);
    float eased = (t < 0.5f) ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2;

    int curY = oldY + int((newY - oldY) * eased);
    int curX = oldX + int((newX - oldX) * eased);
    int curW = oldRectW + int((newRectW - oldRectW) * eased);

    float startF = oldStart + (newStart - oldStart) * eased;
    int baseIdx = (int)startF;
    int yOffset = textOffsetY - int((startF - baseIdx) * lineHeight + 0.5f);

    display.clearBuffer();
    drawBorder();
    renderText(items, baseIdx, yOffset, maxVisible + 1);
    renderRoundRect(curX, curY, curW, lineHeight);
    display.sendBuffer();
  }

  drawStaticFrame(items, newIdx);
}

void nx::menu::drawStaticFrame(const std::vector<std::string> &items, int idx) {
  int startIdx = calcStartIndex(idx);
  int y = textOffsetY;

  display.clearBuffer();
  drawBorder();
  renderText(items, startIdx, y, maxVisible);

  for (int i = 0; i < maxVisible && startIdx + i < menuSize; i++) {
    if (startIdx + i == idx) {
      int textWidth = display.getStrWidth(items[idx].c_str());
      int rectWidth = calcRectWidth(textWidth);
      int rectX = calcRectX(rectWidth);
      renderRoundRect(rectX, y, rectWidth, lineHeight);
    }
    y += lineHeight;
  }

  display.sendBuffer();
}

void nx::menu::drawCheckboxInitialAnimation(const std::vector<std::string> &ssids, const std::vector<std::string> &channels, const std::vector<bool> &checked, int idx) {
  int startIdx = calcStartIndex(idx);
  unsigned long startT = millis();
  unsigned long endT = startT + lineAnimDuration + maxVisible * lineDelay;

  while (millis() < endT) {
    unsigned long now = millis();
    display.clearBuffer();

    for (int i = 0; i < maxVisible && startIdx + i < menuSize; i++) {
      unsigned long itemStartT = startT + i * lineDelay;
      if (now < itemStartT) continue;

      float t = std::min(1.0f, float(now - itemStartT) / lineAnimDuration);
      float eased = 1.0f - powf(1.0f - t, 3.0f);

      int startY = SCREEN_HEIGHT + textOffsetY + i * lineHeight;
      int finalY = textOffsetY + i * lineHeight;
      int currentY = startY + int((finalY - startY) * eased);

      if (currentY > -lineHeight && currentY < SCREEN_HEIGHT) {
        bool isSelected = (startIdx + i == idx);
        bool isChecked = (startIdx + i < (int)checked.size()) ?  checked[startIdx + i] :  false;
        renderLineWithCheckbox(ssids[startIdx + i], channels[startIdx + i], currentY, isChecked, isSelected);
      }
    }
    display.sendBuffer();
  }
}

void nx::menu::drawCheckboxSelectionAnimation(const std::vector<std::string> &ssids, const std::vector<std::string> &channels, const std::vector<bool> &checked, int newIdx) {
  int oldIdx = (prevSelected < 0 || prevSelected >= menuSize) ? 0 : prevSelected;
  int oldStart = calcStartIndex(oldIdx);
  int newStart = calcStartIndex(newIdx);

  int oldY = textOffsetY + (oldIdx - oldStart) * lineHeight;
  int newY = textOffsetY + (newIdx - newStart) * lineHeight;

  int oldRectW = calcCheckboxRectWidth(channels[oldIdx]);
  int newRectW = calcCheckboxRectWidth(channels[newIdx]);

  unsigned long startT = millis();
  unsigned long endT = startT + animDuration;

  while (millis() < endT) {
    float t = std::min(1.0f, float(millis() - startT) / animDuration);
    float eased = (t < 0.5f) ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2;

    int curY = oldY + int((newY - oldY) * eased);
    int curW = oldRectW + int((newRectW - oldRectW) * eased);

    float startF = oldStart + (newStart - oldStart) * eased;
    int baseIdx = (int)startF;
    int yOffset = textOffsetY - int((startF - baseIdx) * lineHeight + 0.5f);

    display.clearBuffer();
    
    for (int i = 0; i < maxVisible + 1 && baseIdx + i < menuSize; i++) {
      int itemY = yOffset + i * lineHeight;
      bool isChecked = (baseIdx + i < (int)checked.size()) ? checked[baseIdx + i] :  false;
      renderLineWithCheckbox(ssids[baseIdx + i], channels[baseIdx + i], itemY, isChecked, false);
    }
    
    renderRoundRect(rectStartX, curY, curW, lineHeight);
    display.sendBuffer();
  }

  drawCheckboxFrame(ssids, channels, checked, newIdx);
}

void nx::menu::drawCheckboxFrame(const std::vector<std::string> &ssids, const std::vector<std::string> &channels, const std::vector<bool> &checked, int idx) {
  int startIdx = calcStartIndex(idx);
  int y = textOffsetY;

  display.clearBuffer();

  for (int i = 0; i < maxVisible && startIdx + i < menuSize; i++) {
    bool isSelected = (startIdx + i == idx);
    bool isChecked = (startIdx + i < (int)checked.size()) ? checked[startIdx + i] : false;
    renderLineWithCheckbox(ssids[startIdx + i], channels[startIdx + i], y, isChecked, isSelected);
    y += lineHeight;
  }
  display.sendBuffer();
}

void nx::menu::drawMenuWithCheckbox(const std::vector<std::string> &ssids, const std::vector<std::string> &channels, const std::vector<bool> &checked, int idx) {
  menuSize = ssids.size();
  if (menuSize == 0) return;

  idx = constrain(idx, 0, menuSize - 1);

  bool menuChanged = (ssids != prevMenuName);
  bool firstDraw = (prevSelected == -1);
  bool selectionChanged = (prevSelected != idx);

  if (firstDraw || menuChanged) drawCheckboxInitialAnimation(ssids, channels, checked, idx);
  else if (selectionChanged) drawCheckboxSelectionAnimation(ssids, channels, checked, idx);
  else drawCheckboxFrame(ssids, channels, checked, idx);
  
  prevMenuName = ssids;
  prevSelected = idx;
}

void nx::menu::drawMenu(const std::vector<std::string> &items, int idx) {
  menuSize = items.size();
  if (menuSize == 0) return;

  idx = constrain(idx, 0, menuSize - 1);
  visibleWidth = SCREEN_WIDTH - 2 * baseX - ((menuSize > maxVisible) ? scrollbarW : 0);

  bool menuChanged = (items != prevMenuName);
  bool firstDraw = (prevSelected == -1);
  bool selectionChanged = (prevSelected != idx);

  if (firstDraw || menuChanged) drawInitialAnimation(items, idx);
  else if (selectionChanged) drawSelectionAnimation(items, idx);
  else drawStaticFrame(items, idx);

  prevMenuName = items;
  prevSelected = idx;
}

void nx::menu::renderProgressEffect(int progress){
  if (progress == 0) return;
  display.drawXBMP(15, 41, 18, 16, image_folder_open_file_bits);
  display.drawXBMP(97, 41, 17, 16, image_folder_explorer_bits);
  int t = constrain(progress, 0, 100);
  int currentX = startX + ((endX - startX) * t) / 100;
  int offsetY = (4 * arcHeight * t * (100 - t)) / 10000;
  int currentY = startY - offsetY;
  display.drawXBMP(currentX, currentY, 18, 16, image_folder_file_bits);
}

void nx::menu::renderScanEffect(int progress) {
  display.setBitmapMode(1);
  const unsigned char* currentImage;
  if (progress < 20) currentImage = image_wifi_bits;
  else if (progress < 40) currentImage = image_wifi_25_bits;
  else if (progress < 60) currentImage = image_wifi_50_bits;
  else if (progress < 80) currentImage = image_wifi_75_bits;
  else currentImage = image_wifi_full_bits;
  
  display.drawXBMP((SCREEN_WIDTH - 19) / 2, ((SCREEN_HEIGHT - 16) / 2) + 2, 19, 16, currentImage);
}

void nx::menu::renderPopup(const std::string& ctx){
  int textW = display.getStrWidth(ctx.c_str());
  int popupW = textW + padding * 3;
  int popupH = lineHeight + 4;

  int popupX = (SCREEN_WIDTH - popupW) / 2;
  int popupY = (SCREEN_HEIGHT - popupH) / 2;

  display.setDrawColor(0);
  display.drawBox(popupX - 2, popupY - 2, popupW + 4, popupH + 4);

  display.setDrawColor(1);
  display.drawRFrame(popupX, popupY, popupW, popupH, cornerRadius);
  display.drawRFrame(popupX + 1, popupY + 1, popupW - 2, popupH - 2, cornerRadius - 1);

  int textX = popupX + (popupW - textW) / 2;
  int textY = popupY + calcTextY(0) + 2;
  display.drawStr(textX, textY, ctx.c_str());
  display.sendBuffer();
  delay(500);
}

void nx::menu::drawSubMenu(const std::string& title, bool progressFlag, bool scanFlag){
  progress += 5;
  if(progress > 100) progress = 1;
  display.clearBuffer();
  renderSingleLine(title, 5, true);
  if(scanFlag) renderScanEffect(progress);
  drawBorder();
  if(progressFlag) renderProgressEffect(progress);
  display.sendBuffer();
}

uint8_t nx::menu::getRandomChannel() {
  if(random(0, 2) == 0) return random(1, 12);
  return channels5G[random(0, 9)];
}

void nx::menu::generateRandomBSSID(uint8_t* bssid) {
  for(int i = 0; i < 6; i++) bssid[i] = random(0, 256);
  bssid[0] &= 0xFE;
}

std::string nx::menu::modifySSIDWithSpaces(const std::string& ssid, int cloneCount) {
  int spaceCount = (cloneCount % 25) + 1;
  std::string modifiedSSID = ssid.empty() ? "Hidden_AP" : ssid;
  for(int i = 0; i < spaceCount; i++) modifiedSSID += " ";
  if(modifiedSSID.length() > 32) modifiedSSID = modifiedSSID.substr(0, 32);
  return modifiedSSID;
}

void nx::menu::sendBeaconForAP(const BSSIDInfo* ap, int& cloneCount) {
  std::string modifiedSSID = modifySSIDWithSpaces(ap->ssid, cloneCount);
  uint8_t fakeBSSID[6];
  generateRandomBSSID(fakeBSSID);
  tx.txBeaconFrame(modifiedSSID.c_str(), getRandomChannel(), fakeBSSID, ap->encrypted);
  cloneCount++;
}

void nx::menu::executeChannelAttack(const char* attackType, std::function<void(uint8_t)> attackFunc) {
  if(channelAPMap.empty()) {
    renderPopup("Scan First");
    return;
  }
  
  std::vector<std::string> channelList = getChannelList();
  std::vector<uint8_t> channelNumbers;
  channelNumbers.reserve(channelAPMap.size());
  
  for(auto& entry : channelAPMap) channelNumbers.push_back(entry.first);

  int selectedIdx = 0;
  drawMenu(channelList, selectedIdx);
  
  while(true) {
    if(btn.btnPress(btnUp)) {
      selectedIdx = (selectedIdx > 0) ? selectedIdx - 1 : (int)channelList.size() - 1;
      drawMenu(channelList, selectedIdx);
    }
    if(btn.btnPress(btnDown)) {
      selectedIdx = (selectedIdx < (int)channelList.size() - 1) ? selectedIdx + 1 :  0;
      drawMenu(channelList, selectedIdx);
    }
    if(btn.btnPress(btnOk)) {
      if(selectedIdx < (int)channelNumbers.size()) {
        uint8_t targetChannel = channelNumbers[selectedIdx];
        char title[32];
        snprintf(title, sizeof(title), "Ch %d %s", targetChannel, attackType);
        
        while(true) {
          drawSubMenu(std::string(title));
          attackFunc(targetChannel);
          if(btn.btnPress(btnBack)) break;
        }
        drawMenu(channelList, selectedIdx);
      }
    }
    if(btn.btnPress(btnBack)) break;
  }
}

void nx::menu::executeSelectedAttack(const char* attackType, std::function<void(const BSSIDInfo&, uint8_t)> attackFunc) {
  if(channelAPMap.empty()) {
    renderPopup("Scan First");
    return;
  }
  if(selectedAPs.empty()) {
    renderPopup("No APs Selected");
    return;
  }
  
  int selectedCount = 0;
  for(bool sel : selectedAPs) if(sel) selectedCount++;
  
  if(selectedCount == 0) {
    renderPopup("No APs Selected");
    return;
  }
  
  char title[32];
  snprintf(title, sizeof(title), "%s %d APs", attackType, selectedCount);
  
  while(true) {
    drawSubMenu(std::string(title));
    size_t idx = 0;
    for(auto& entry : channelAPMap) {
      uint8_t channel = entry.first;
      for(auto& apInfo : entry.second) {
        if(idx < selectedAPs.size() && selectedAPs[idx]) {
          attackFunc(apInfo, channel);
        }
        idx++;
      }
    }
    if(btn.btnPress(btnBack)) break;
  }
}

void nx::menu::deauthAttack(){
  if(channelAPMap.empty()){
    renderPopup("Scan First");
    return;
  }
  while(true){
    drawSubMenu("Deauth All");
    tx.txDeauthFrameAll();
    if(btn.btnPress(btnBack)) break;
  }
}

std::vector<std::string> nx::menu::getChannelList() {
  std::vector<std::string> channels;
  for(auto& entry : channelAPMap) {
    uint8_t ch = entry.first;
    int apCount = entry.second.size();
    char buf[32];
    snprintf(buf, sizeof(buf), "Ch %d (%d APs)", ch, apCount);
    channels.push_back(std::string(buf));
  }
  
  if(channels.empty()) channels.push_back("No APs Found");
  return channels;
}

std::string nx::menu::formatBSSID(const uint8_t* bssid) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  return std::string(buf);
}

std::string nx::menu::truncateSSID(const std::string& ssid, size_t maxLen) {
  if(ssid.length() <= maxLen) return ssid;
  return ssid.substr(0, maxLen) + "...";
}

void nx::menu::getSSIDAndChannelLists(std::vector<std::string> &ssids, std::vector<std::string> &channels) {
  ssids.clear();
  channels.clear();
  
  for(auto& entry : channelAPMap) {
    for(auto& apInfo : entry.second) {
      std::string ssid = (apInfo.ssid.empty() || apInfo.ssid == "") ? "<Hidden>" : truncateSSID(apInfo.ssid, 10);
      char chStr[8];
      snprintf(chStr, sizeof(chStr), "Ch%d", entry.first);
      ssids.push_back(ssid);
      channels.push_back(std::string(chStr));
    }
  }
  
  if(ssids.empty()) {
    ssids.push_back("No APs Found");
    channels.push_back("");
  }
}

void nx::menu::deauthByChannel() {
  executeChannelAttack("Attack", [this](uint8_t ch) {tx.txDeauthFrameChannel(ch);});
}

void nx::menu::drawSelectMenu() {
  if(channelAPMap.empty()) {
    renderPopup("Scan First");
    return;
  }
  
  std::vector<std::string> ssidList;
  std::vector<std::string> channelList;
  getSSIDAndChannelLists(ssidList, channelList);
  
  if(selectedAPs.size() != ssidList.size()) {
    selectedAPs.clear();
    selectedAPs.resize(ssidList.size(), false);
  }
  
  int selectedIdx = 0;
  drawMenuWithCheckbox(ssidList, channelList, selectedAPs, selectedIdx);
  
  while(true) {
    if(btn.btnPress(btnUp)){
      selectedIdx = (selectedIdx > 0) ? selectedIdx - 1 : (int)ssidList.size() - 1;
      drawMenuWithCheckbox(ssidList, channelList, selectedAPs, selectedIdx);
    }
    if(btn.btnPress(btnDown)){
      selectedIdx = (selectedIdx < (int)ssidList.size() - 1) ? selectedIdx + 1 : 0;
      drawMenuWithCheckbox(ssidList, channelList, selectedAPs, selectedIdx);
    }
    if(btn.btnPress(btnOk)){
      if(selectedIdx < (int)selectedAPs.size()) {
        selectedAPs[selectedIdx] = ! selectedAPs[selectedIdx];
        drawMenuWithCheckbox(ssidList, channelList, selectedAPs, selectedIdx);
      }
    }
    if(btn.btnPress(btnBack)) break;
  }
}

void nx::menu::deauthSelected() {
  executeSelectedAttack("Attack", [this](const BSSIDInfo& apInfo, uint8_t channel){tx.txDeauthFrameBSSID(apInfo.bssid.data(), channel);});
}

void nx::menu::authAttack(){
  if(channelAPMap.empty()){
    renderPopup("Scan First");
    return;
  }
  while(true){
    drawSubMenu("Auth Flood");
    tx.txAuthFlood();
    if(btn.btnPress(btnBack)) break;
  }
}

void nx::menu::authByChannel() {
  executeChannelAttack("Auth", [this](uint8_t ch) {
    if(channelAPMap.find(ch) != channelAPMap.end()){
      for(auto& apInfo : channelAPMap[ch]) tx.txAuthFrame(apInfo.bssid.data(), ch);
    }
  });
}

void nx::menu::authSelected() {
  executeSelectedAttack("Auth", [this](const BSSIDInfo& apInfo, uint8_t channel) {tx.txAuthFrame(apInfo.bssid.data(), channel);});
}

void nx::menu::assocAttack(){
  if(channelAPMap.empty()){
    renderPopup("Scan First");
    return;
  }
  while(true){
    drawSubMenu("Assoc Flood");
    tx.txAssocFlood();
    if(btn.btnPress(btnBack)) break;
  }
}

void nx::menu::assocByChannel() {
  executeChannelAttack("Assoc", [this](uint8_t ch) {
    if(channelAPMap.find(ch) != channelAPMap.end()){
      for(auto& apInfo : channelAPMap[ch]){
        const char* ssid = apInfo.ssid.empty() ? "Network" : apInfo.ssid.c_str();
        tx.txAssocFrame(apInfo.bssid.data(), ssid, ch);
      }
    }
  });
}

void nx::menu::assocSelected() {
  executeSelectedAttack("Assoc", [this](const BSSIDInfo& apInfo, uint8_t channel) {
    const char* ssid = apInfo.ssid.empty() ? "Network" : apInfo.ssid.c_str();
    tx.txAssocFrame(apInfo.bssid.data(), ssid, channel);
  });
}
void nx::menu::beaconAllSSID() {
  if(channelAPMap.empty()) {
    renderPopup("Scan First");
    return;
  }
  
  std::vector<const BSSIDInfo*> allAPs;
  for(auto& entry : channelAPMap) {
    for(auto& apInfo : entry.second) {
      allAPs.push_back(&apInfo);
    }
  }
  
  if(allAPs.empty()) {
    renderPopup("No SSIDs Found");
    return;
  }
  
  char title[32];
  snprintf(title, sizeof(title), "Beacon %d SSIDs", (int)allAPs.size());
  
  int cloneCount = 0;
  
  while(true) {
    drawSubMenu(std::string(title));
    
    for(const auto* ap : allAPs) {
      sendBeaconForAP(ap, cloneCount);
    }
    
    if(btn.btnPress(btnBack)) break;
  }
}

void nx::menu::beaconSSIDDupe() {
  if(channelAPMap.empty()) {
    renderPopup("Scan First");
    return;
  }
  if(selectedAPs.empty() || std::count(selectedAPs.begin(), selectedAPs.end(), true) == 0) {
    renderPopup("No APs Selected");
    return;
  }
  
  std::vector<const BSSIDInfo*> selectedAPList;
  size_t idx = 0;
  for(auto& entry : channelAPMap) {
    for(auto& apInfo : entry.second) {
      if(idx < selectedAPs.size() && selectedAPs[idx]) {
        selectedAPList.push_back(&apInfo);
      }
      idx++;
    }
  }
  
  char title[32];
  snprintf(title, sizeof(title), "Beacon %d SSIDs", (int)selectedAPList.size());
  
  int cloneCount = 0;
  
  while(true) {
    drawSubMenu(std::string(title));
    
    for(const auto* ap : selectedAPList) {
      sendBeaconForAP(ap, cloneCount);
    }
    
    if(btn.btnPress(btnBack)) break;
  }
}

void nx::menu::beaconDupeByChannel() {
  if(channelAPMap.empty()) {
    renderPopup("Scan First");
    return;
  }
  
  std::vector<std::string> channelList = getChannelList();
  std::vector<uint8_t> channelNumbers;
  channelNumbers.reserve(channelAPMap.size());
  
  for(auto& entry : channelAPMap) channelNumbers.push_back(entry.first);

  int selectedIdx = 0;
  drawMenu(channelList, selectedIdx);
  
  while(true) {
    if(btn.btnPress(btnUp)) {
      selectedIdx = (selectedIdx > 0) ? selectedIdx - 1 : (int)channelList.size() - 1;
      drawMenu(channelList, selectedIdx);
    }
    if(btn.btnPress(btnDown)) {
      selectedIdx = (selectedIdx < (int)channelList.size() - 1) ? selectedIdx + 1 :  0;
      drawMenu(channelList, selectedIdx);
    }
    if(btn.btnPress(btnOk)) {
      if(selectedIdx < (int)channelNumbers.size()) {
        uint8_t targetChannel = channelNumbers[selectedIdx];
        char title[32];
        snprintf(title, sizeof(title), "Ch %d Beacon", targetChannel);
        
        int cloneCount = 0;
        
        while(true) {
          drawSubMenu(std::string(title));
          if(channelAPMap.find(targetChannel) != channelAPMap.end()) {
            for(auto& apInfo : channelAPMap[targetChannel]) {
              sendBeaconForAP(&apInfo, cloneCount);
            }
          }
          if(btn.btnPress(btnBack)) break;
        }
        drawMenu(channelList, selectedIdx);
      }
    }
    if(btn.btnPress(btnBack)) break;
  }
}
void nx::menu::beaconRandom() {
  while(true) {
    drawSubMenu("Random Beacon");
    char randomSSID[33];
    int ssidLen = random(8, 33);
    
    for(int i = 0; i < ssidLen; i++) {
      randomSSID[i] = charset[random(0, strlen(charset))];
    }
    randomSSID[ssidLen] = '\0';
    
    uint8_t fakeBSSID[6];
    generateRandomBSSID(fakeBSSID);
    
    tx.txBeaconFrame(randomSSID, getRandomChannel(),fakeBSSID, random(0, 2));
    if(btn.btnPress(btnBack)) break;
  }
}
void nx::menu::runBle(std::function<void()> func, const std::string& title){
  while(true){
    if(btn.btnPress(btnBack)) break;
    drawSubMenu(title);
    func();
  }
}

void nx::menu::drawIosAdv(){
  runBle([this](){adv.iosAdv();}, "Ios");
}

void nx::menu::drawSamsungAdv(){
  runBle([this](){adv.samsungAdv();}, "Samsung");
}

void nx::menu::scanWiFi(){
  while(! tx.scanComplete){
    drawSubMenu("Scan", false, true);
    tx.performProgressiveScan();
    if(btn.btnPress(btnBack)) break;
  }
}

void nx::menu::drawAbout(){
  display.clearBuffer();
  display.sendBuffer();
  while(true){
    if(btn.btnPress(btnBack)) break;
  }
}

void nx::menu::menuHandler(std::vector<menuItem> &menu, int index) {
  std::vector<std::string> menuNames;
  for (const auto &m : menu) menuNames.push_back(m.name);

  drawMenu(menuNames, index);

  while (true) {
    menuItem &item = menu[index];
    bool indexChanged = false;

    if (btn.btnPress(btnUp)) {
      index = (index > 0) ? index - 1 : (int)menu.size() - 1;
      indexChanged = true;
    }
    if (btn.btnPress(btnDown)) {
      index = (index < (int)menu.size() - 1) ? index + 1 : 0;
      indexChanged = true;
    }
    if (btn.btnPress(btnOk)) {
      if (!item.subMenu.empty()) {
        int subIndex = 0;
        menuHandler(item.subMenu, subIndex);
        drawMenu(menuNames, index);
      } else if (item.action) {
        item.action();
        drawMenu(menuNames, index);
      }
    }
    if (btn.btnPress(btnBack)) break;
    if (indexChanged) drawMenu(menuNames, index);
  }
}