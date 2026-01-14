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
  startAnimation();
  setBrightness(12);
}
// IMPORT brightness range 0 ~ 15
void nx::menu::setBrightness(uint8_t brightness){
  if (brightness > 15) brightness = 15;
	brightness = brightness | (brightness << 4);
	uint8_t VMOCH = brightness >> 5;
	VMOCH <<= 4;
	uint8_t preCharge = brightness >> 4;
	preCharge |= preCharge << 4;

	Wire.beginTransmission(0x3C);
	Wire.write(0);
	Wire.write(0xd9); Wire.write(preCharge);
	Wire.write(0xdb); Wire.write(VMOCH);
	Wire.write(0x81); Wire.write(brightness);
	Wire.endTransmission();
}
// for Fade in impact
uint8_t gamma16(uint8_t x){
  return (x * x + 7) / 15;
}

void nx::menu::startAnimation(){
  uint8_t level = 0;
  for (uint8_t i = 0; i < startAnimeallArray_LEN; i++){
    display.clearBuffer();
    display.drawXBMP(0,0,128,64,startAnimeallArray[i]);
    display.sendBuffer();
    if(i % framesPerStep == 0 && level < maxBrLevel){
      level++;
      uint8_t gLevel = gamma16(level);
      setBrightness(gLevel);
    }
    delay(25);
  }
  setBrightness(maxBrLevel);
  delay(600);
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
  display.drawLine(0, 2, 0, 22);
  display.drawLine(2, 0, 22, 0);
  display.drawLine(1, 1, 1, 20);
  display.drawLine(2, 1, 21, 1);
  display.drawLine(2, 19, 2, 2);
  display.drawLine(3, 2, 20, 2);
  display.drawLine(127, 2, 127, 22);
  display.drawLine(106, 0, 125, 0);
  display.drawLine(126, 2, 126, 21);
  display.drawLine(107, 1, 126, 1);
  display.drawLine(125, 19, 125, 2);
  display.drawLine(108, 2, 125, 2);
  display.drawLine(0, 42, 0, 61);
  display.drawLine(2, 63, 22, 63);
  display.drawLine(1, 43, 1, 62);
  display.drawLine(2, 62, 21, 62);
  display.drawLine(2, 61, 2, 44);
  display.drawLine(3, 61, 20, 61);
  display.drawLine(127, 42, 127, 61);
  display.drawLine(106, 63, 125, 63);
  display.drawLine(126, 43, 126, 61);
  display.drawLine(107, 62, 126, 62);
  display.drawLine(125, 61, 125, 44);
  display.drawLine(108, 61, 125, 61);
  display.drawLine(1, 28, 1, 36);
  display.drawLine(2, 29, 2, 35);
  display.drawLine(126, 28, 126, 36);
  display.drawLine(125, 29, 125, 35);
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


void nx::menu::renderTyping(const char* const* texts, const int* yPositions, int textCount, unsigned long displayDuration, std::function<void()> drawBackground){
  while(true){
    if(btn.btnPress(btnBack)) break;
    
    for(int t = 0; t < textCount; t++){
      int len = strlen(texts[t]);
      for(int i = 0; i <= len; i++){
        if(btn.btnPress(btnBack)) return;
        display.clearBuffer();
        if(drawBackground) drawBackground();
        for(int prev = 0; prev < t; prev++){
          int w = display.getStrWidth(texts[prev]);
          display.drawStr((SCREEN_WIDTH - w) / 2, yPositions[prev], texts[prev]);
        }
        std::string temp = std::string(texts[t], i);
        int w = display.getStrWidth(temp.c_str());
        display.drawStr((SCREEN_WIDTH - w) / 2, yPositions[t], temp.c_str());
        display.sendBuffer();
        delay(50);
      }
      delay(200);
    }
    unsigned long startTime = millis();
    while(millis() - startTime < displayDuration){
      if(btn.btnPress(btnBack)) return;
      delay(50);
    }
    for(int t = textCount - 1; t >= 0; t--){
      int len = strlen(texts[t]);
      for(int i = len; i >= 0; i--){
        if(btn.btnPress(btnBack)) return;
        display.clearBuffer();
        if(drawBackground) drawBackground();
        for(int remaining = 0; remaining < t; remaining++){
          int w = display.getStrWidth(texts[remaining]);
          display.drawStr((SCREEN_WIDTH - w) / 2, yPositions[remaining], texts[remaining]);
        }
        std::string temp = std::string(texts[t], i);
        int w = display.getStrWidth(temp.c_str());
        display.drawStr((SCREEN_WIDTH - w) / 2, yPositions[t], temp.c_str());
        display.sendBuffer();
        delay(30);
      }
      delay(100);
    }
    delay(500);
  }
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
  return random(1, 12);
}

void nx::menu::generateRandomBSSID(uint8_t* bssid) {
  for(int i = 0; i < 6; i++) bssid[i] = random(0, 256);
  bssid[0] &= 0xFE;
}

std::vector<std::array<uint8_t,6>> nx::menu::generateRandomBSSIDList(){
  std::vector<std::array<uint8_t,6>> bssidList;
  bssidList.reserve(MAX_SSID);
  for( int i=0; i<MAX_SSID;i++){
    std::array<uint8_t,6> bssid;
    generateRandomBSSID(bssid.data());
    bssidList.push_back(bssid);
  }
  return bssidList;
}
void nx::menu::executeAttackAll(const std::string& title,std::function<void()> attackFunc){
  if(channelAPMap.empty()){
    renderPopup("Scan First");
    return;
  }

  while(true){
    drawSubMenu(title);
    attackFunc();
    if(btn.btnPress(btnBack)) break;
  }
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
  executeAttackAll("Deauth All",[this](){tx.txDeauthFrameAll();});
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
  executeAttackAll("Auth Flood",[this](){tx.txAuthFlood();});
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
  executeAttackAll("Assoc Flood",[this](){tx.txAuthFlood();});
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
  for(auto& entry :channelAPMap) {
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
  executeBeaconAttack(allAPs, std::string(title));
}

void nx::menu::beaconSSIDDupe() {
  if(channelAPMap.empty()) {
    renderPopup("Scan First");
    return;
  }
  if(selectedAPs.empty() || std::count(selectedAPs. begin(), selectedAPs.end(), true) == 0) {
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
  
  executeBeaconAttack(selectedAPList, std::string(title));
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
    if(btn. btnPress(btnOk)) {
      if(selectedIdx < (int)channelNumbers.size()) {
        uint8_t targetChannel = channelNumbers[selectedIdx];
        std::vector<const BSSIDInfo*> channelAPs;
        if(channelAPMap.find(targetChannel) != channelAPMap.end()) {
          for(auto& apInfo : channelAPMap[targetChannel]) {
            channelAPs.push_back(&apInfo);
          }
        }
        
        char title[32];
        snprintf(title, sizeof(title), "Ch %d Beacon", targetChannel);
        
        executeBeaconAttack(channelAPs, std::string(title));
        drawMenu(channelList, selectedIdx);
      }
    }
    if(btn.btnPress(btnBack)) break;
  }
}
void nx::menu::beaconCustomPrefix(const std::string& prefix){
  std::vector<std::string> ssidList;
  std::vector<std::array<uint8_t,6>> bssidList;
  std::vector<uint8_t> channelList;
  std::vector<bool> encryptedList;
  
  ssidList.reserve(MAX_SSID);
  bssidList.reserve(MAX_SSID);
  channelList.reserve(MAX_SSID);
  encryptedList.reserve(MAX_SSID);

  for(int i = 0; i < MAX_SSID; i++) {
    char ssidBuf[33];
    snprintf(ssidBuf, sizeof(ssidBuf), "%s %d", prefix.c_str(), i + 1);
    ssidList.push_back(std::string(ssidBuf));

    std::array<uint8_t,6> bssid;
    generateRandomBSSID(bssid.data());
    bssidList.push_back(bssid);

    channelList.push_back(getRandomChannel());
    encryptedList.push_back(random(0, 2));
  }
  char title[32];
  snprintf(title, sizeof(title), "%s Beacon", prefix.c_str());
  
  while(true){
    drawSubMenu(std::string(title));

    for (size_t i = 0; i < ssidList.size(); i++){
      tx.txBeaconFrame(ssidList[i].c_str(),channelList[i],bssidList[i].data(),encryptedList[i]);
      if(btn.btnPress(btnBack)) return;
    }
    if(btn.btnPress(btnBack)) break;
  }

}
void nx::menu::executeBeaconAttack(const std::vector<const BSSIDInfo*>& apList, const std::string& title){
  if(apList.empty()){
    renderPopup("No APs to Attack");
    return;
  }
  std::vector<std::string> allSSIDs;
  std::vector<std::array<uint8_t,6>> allBSSIDs;
  std::vector<uint8_t> allChannels;
  std::vector<bool> allEncrypted;

  for(const auto* ap : apList){
    std::string baseSSID = ap->ssid.empty() ? "Hidden_AP" : ap->ssid;
    
    for (int i = 0; i < CLONES_PER_AP; i++){
      int spaceCount = (i % 25) + 1;
      std::string modifiedSSID = baseSSID;
      for (int j=0; j<spaceCount;j++) modifiedSSID += " ";
      if (modifiedSSID.length() > 32) modifiedSSID = modifiedSSID.substr(0,32);

      std::array<uint8_t, 6> bssid;
      generateRandomBSSID(bssid. data());

      allSSIDs.push_back(modifiedSSID);
      allBSSIDs.push_back(bssid);
      allChannels.push_back(getRandomChannel());
      allEncrypted.push_back(ap->encrypted);
    }
  }
  while (true){
    drawSubMenu(title);

    for(size_t i = 0; i < allSSIDs.size();i++){
      tx.txBeaconFrame(allSSIDs[i].c_str(),allChannels[i],allBSSIDs[i].data(),allEncrypted[i]);
      if(btn.btnPress(btnBack)) return;
    }
  }
}
std::vector<std::string> nx::menu::getRandomSSID(){
  std::vector<std::string> ssidList;
  ssidList.reserve(MAX_SSID);
  for (int i = 0; i<MAX_SSID; i++){
    char randomSSID[33]; // ssid max len 32 + null terminator
    int ssidLen = random(8,MAX_SSID_LEN);
    for (int j = 0; j<ssidLen; j++) randomSSID[j] = charset[random(0,strlen(charset))];
    randomSSID[ssidLen] = '\0';
    ssidList.push_back(std::string(randomSSID));
  }
  return ssidList;
}

void nx::menu::beaconRandom() {
  while(true) {
    drawSubMenu("Random Beacon");
    
    std::vector<std::string> ssidList = getRandomSSID();
    std::vector<std::array<uint8_t,6>> bssidList = generateRandomBSSIDList();

    for (int i = 0; i < ssidList.size(); i++){
      tx.txBeaconFrame(ssidList[i].c_str(), getRandomChannel(),bssidList[i].data(), random(0, 2));
      if(btn.btnPress(btnBack)) return;
    }
    
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
  tx.scanComplete = false;
  while(!tx.scanComplete){
    drawSubMenu("Scan", false, true);
    tx.performProgressiveScan();
    if(btn.btnPress(btnBack)) break;
  }
}

void nx::menu::drawAbout(){
  auto drawBg = [this](){
    renderSingleLine("Nova-X", 5, true);
    drawBorder();
  };
  renderTyping(info, yOffsetTyping, 3, displayDuration, drawBg);
}

void nx::menu::drawPacketMonitor(){
  tx.setUseDFS(false);
  tx.startPacketMonitor();
  tx.setChannelHopping(true);
  uint32_t lastUpdate = 0;
  uint8_t ch = 0;
  while (true){
    tx.updatePacketMonitor();
    if(millis() - lastUpdate > PACKET_BAR_INTERVAL){
      lastUpdate = millis();
      display.clearBuffer();
      char header[32];
      ch = tx.getCurrentChannel();
      const char* band = (ch > 14) ? "5G" : "2G";
      const char* hop = tx.getChannelHopping() ? "HOP" : "FIX";

      snprintf(header,sizeof(header),"%s CH%d %s",band,ch,hop);
      display.drawStr(0,8,header);

      char rate[16];
      snprintf(rate,sizeof(rate),"%lupps",tx.getPacketRate());
      display.drawStr(90,8,rate);

      uint16_t maxPacket = tx.getMaxPacket();
      if(maxPacket > 0){
        double scale = tx.getScaleFactor(SCREEN_HEIGHT - 12);

      // BAR

      //   int x = 0;
      //   for (int i = 0; i < SCAN_PACKET_LIST_SIZE && x < SCREEN_WIDTH; i++){
      //     uint16_t pktCount = tx.getPacketAtIdx(i);
      //     int y = 63 - (pktCount * scale);

      //     if(pktCount > 0){
      //       display.drawLine(x,63,x,y);
      //       x++;
      //       display.drawLine(x,63,x,y);
      //       x++;
      //     }
      //     else x += 2;
      //   }

      // LINE

        int prevX = -1;
        int prevY = -1;

        for (int i = 0; i < SCAN_PACKET_LIST_SIZE; i++){
          uint16_t pktCount = tx.getPacketAtIdx(i);
          int x =  i * 2; // 2 pixel distance
          int y = 63 - (pktCount * scale);

          if(x >= SCREEN_WIDTH) break;

          display.drawPixel(x,y);

          if(prevX >= 0 && prevY >= 0) display.drawLine(prevX,prevY,x,y);
          prevX = x;
          prevY = y;
        }
      }
      display.sendBuffer();
    }
    if(btn.btnPress(btnUp)) {
      tx.setChannelHopping(false);
      tx.nextChannel();
      Serial.printf("[PKT] Manual Channel: %d\n",ch);
    }
    if(btn.btnPress(btnDown)){
      tx.setChannelHopping(false);
      tx.prevChannel();
      Serial.printf("[PKT] Manual Channel: %d\n",ch);
    }
    if(btn.btnPress(btnBack)) break;
  }
  tx.stopPacketMonitor();
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

