#include "SSD1306Display.h"

bool SSD1306Display::i2c_probe(TwoWire& wire, uint8_t addr) {
  wire.beginTransmission(addr);
  uint8_t error = wire.endTransmission();
  return (error == 0);
}

// Color scheme
ColorVal UIColor::window_bkg = SSD1306_BLACK;
ColorVal UIColor::title_bkg = SSD1306_BLACK;
ColorVal UIColor::title_txt = SSD1306_WHITE;
ColorVal UIColor::primary_txt = SSD1306_WHITE;
ColorVal UIColor::secondary_txt = SSD1306_WHITE;
ColorVal UIColor::warning_txt = SSD1306_WHITE;
ColorVal UIColor::popup_bkg = SSD1306_BLACK;
ColorVal UIColor::popup_txt = SSD1306_WHITE;
ColorVal UIColor::corp_blue = SSD1306_WHITE;

bool SSD1306Display::begin() {
  if (!_isOn) {
    if (_peripher_power) _peripher_power->claim();
    _isOn = true;
  }
  #ifdef DISPLAY_ROTATION
  display.setRotation(DISPLAY_ROTATION);
  #endif
  return display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS, true, false) && i2c_probe(Wire, DISPLAY_ADDRESS);
}

void SSD1306Display::turnOn() {
  if (!_isOn) {
    if (_peripher_power) _peripher_power->claim();
    _isOn = true;  // set before begin() to prevent double claim
    if (_peripher_power) begin();  // re-init display after power was cut
  }
  display.ssd1306_command(SSD1306_DISPLAYON);
}

void SSD1306Display::turnOff() {
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  if (_isOn) {
    if (_peripher_power) {
#if PIN_OLED_RESET >= 0
      digitalWrite(PIN_OLED_RESET, LOW);
#endif
      _peripher_power->release();
    }
    _isOn = false;
  }
}

void SSD1306Display::clear() {
  display.clearDisplay();
  display.display();
}

void SSD1306Display::startFrame(ColorVal bkg) {
  display.clearDisplay();  // TODO: apply 'bkg'
  _color = SSD1306_WHITE;
  display.setTextColor(_color);
  display.setTextSize(1);
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
}

void SSD1306Display::setTextSize(int sz) {
  display.setTextSize(sz);
}

void SSD1306Display::setColor(ColorVal c) {
  _color = c;
  display.setTextColor(_color);
}

void SSD1306Display::setCursor(int x, int y) {
  display.setCursor(x, y);
}

void SSD1306Display::print(const char* str) {
  display.print(str);
}

void SSD1306Display::fillRect(int x, int y, int w, int h) {
  display.fillRect(x, y, w, h, _color);
}

void SSD1306Display::drawRect(int x, int y, int w, int h) {
  display.drawRect(x, y, w, h, _color);
}

void SSD1306Display::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  display.drawBitmap(x, y, bits, w, h, _color);
}

uint16_t SSD1306Display::getTextWidth(const char* str) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
  return w;
}

static const char* transliterateLatinCodepoint(uint32_t codepoint) {
  // A decomposed accent follows an already emitted base letter, so discard it.
  if (codepoint >= 0x0300 && codepoint <= 0x036F) return "";

  // Latin-1 Supplement. Multi-letter forms retain useful distinctions while
  // still fitting into the same number of bytes as their UTF-8 source.
  if (codepoint >= 0x00C0 && codepoint <= 0x00C5) return "A"; // À-Å
  if (codepoint >= 0x00E0 && codepoint <= 0x00E5) return "a"; // à-å
  if (codepoint >= 0x00C8 && codepoint <= 0x00CB) return "E"; // È-Ë
  if (codepoint >= 0x00E8 && codepoint <= 0x00EB) return "e"; // è-ë
  if (codepoint >= 0x00CC && codepoint <= 0x00CF) return "I"; // Ì-Ï
  if (codepoint >= 0x00EC && codepoint <= 0x00EF) return "i"; // ì-ï
  if ((codepoint >= 0x00D2 && codepoint <= 0x00D6) || codepoint == 0x00D8) return "O";
  if ((codepoint >= 0x00F2 && codepoint <= 0x00F6) || codepoint == 0x00F8) return "o";
  if (codepoint >= 0x00D9 && codepoint <= 0x00DC) return "U"; // Ù-Ü
  if (codepoint >= 0x00F9 && codepoint <= 0x00FC) return "u"; // ù-ü

  switch (codepoint) {
    case 0x00C6: return "AE"; // Æ
    case 0x00E6: return "ae"; // æ
    case 0x00C7: return "C";  // Ç
    case 0x00E7: return "c";  // ç
    case 0x00D0: return "D";  // Ð
    case 0x00F0: return "d";  // ð
    case 0x00D1: return "N";  // Ñ
    case 0x00F1: return "n";  // ñ
    case 0x00DD: return "Y";  // Ý
    case 0x00FD: case 0x00FF: return "y"; // ý ÿ
    case 0x00DE: return "Th"; // Þ
    case 0x00FE: return "th"; // þ
    case 0x00DF: return "ss"; // ß
    default: break;
  }

  // Latin Extended-A covers the accented alphabets used by most European
  // languages, including Polish, Czech, Slovak, Hungarian and Baltic ones.
  if (codepoint >= 0x0100 && codepoint <= 0x0105) return (codepoint & 1) ? "a" : "A";
  if (codepoint >= 0x0106 && codepoint <= 0x010D) return (codepoint & 1) ? "c" : "C";
  if (codepoint >= 0x010E && codepoint <= 0x0111) return (codepoint & 1) ? "d" : "D";
  if (codepoint >= 0x0112 && codepoint <= 0x011B) return (codepoint & 1) ? "e" : "E";
  if (codepoint >= 0x011C && codepoint <= 0x0123) return (codepoint & 1) ? "g" : "G";
  if (codepoint >= 0x0124 && codepoint <= 0x0127) return (codepoint & 1) ? "h" : "H";
  if (codepoint >= 0x0128 && codepoint <= 0x0131) return (codepoint & 1) ? "i" : "I";
  if (codepoint >= 0x0134 && codepoint <= 0x0135) return (codepoint & 1) ? "j" : "J";
  if (codepoint >= 0x0136 && codepoint <= 0x0137) return (codepoint & 1) ? "k" : "K";
  if (codepoint == 0x0138) return "k";
  if (codepoint >= 0x0139 && codepoint <= 0x0142) return (codepoint & 1) ? "L" : "l";
  if (codepoint >= 0x0143 && codepoint <= 0x0148) return (codepoint & 1) ? "N" : "n";
  if (codepoint == 0x0149 || codepoint == 0x014B) return "n";
  if (codepoint == 0x014A) return "N";
  if (codepoint >= 0x014C && codepoint <= 0x0151) return (codepoint & 1) ? "o" : "O";
  if (codepoint >= 0x0154 && codepoint <= 0x0159) return (codepoint & 1) ? "r" : "R";
  if (codepoint >= 0x015A && codepoint <= 0x0161) return (codepoint & 1) ? "s" : "S";
  if (codepoint >= 0x0162 && codepoint <= 0x0167) return (codepoint & 1) ? "t" : "T";
  if (codepoint >= 0x0168 && codepoint <= 0x0173) return (codepoint & 1) ? "u" : "U";
  if (codepoint >= 0x0174 && codepoint <= 0x0175) return (codepoint & 1) ? "w" : "W";
  if (codepoint >= 0x0176 && codepoint <= 0x0177) return (codepoint & 1) ? "y" : "Y";
  if (codepoint >= 0x0179 && codepoint <= 0x017E) return (codepoint & 1) ? "Z" : "z";
  if (codepoint >= 0x0300 && codepoint <= 0x036F) return ""; // combining diacritics

  switch (codepoint) {
    case 0x0132: return "IJ"; // Ĳ
    case 0x0133: return "ij"; // ĳ
    case 0x0152: return "OE"; // Œ
    case 0x0153: return "oe"; // œ
    case 0x0178: return "Y";  // Ÿ
    case 0x017F: return "s";  // long s
    case 0x0218: return "S";  // Ș (Romanian)
    case 0x0219: return "s";  // ș
    case 0x021A: return "T";  // Ț
    case 0x021B: return "t";  // ț
    default: return nullptr;
  }
}

void SSD1306Display::translateUTF8ToBlocks(char* dest, const char* src, size_t dest_size) {
  if (dest == nullptr || dest_size == 0) return;
  if (src == nullptr) {
    dest[0] = 0;
    return;
  }

  size_t input = 0;
  size_t output = 0;
  while (src[input] != 0 && output < dest_size - 1) {
    const uint8_t first = static_cast<uint8_t>(src[input]);
    if (first >= 32 && first <= 126) {
      dest[output++] = static_cast<char>(first);
      input++;
      continue;
    }
    if (first < 0x80) {
      input++;  // Ignore non-printable ASCII control characters.
      continue;
    }

    uint32_t codepoint = 0;
    size_t sequence_length = 1;
    const uint8_t second = static_cast<uint8_t>(src[input + 1]);
    if (first >= 0xC2 && first <= 0xDF && (second & 0xC0) == 0x80) {
      codepoint = ((first & 0x1F) << 6) | (second & 0x3F);
      sequence_length = 2;
    } else if (first >= 0x80) {
      // Skip the complete unsupported UTF-8 character, not each byte.
      while (src[input + sequence_length] != 0 &&
             (static_cast<uint8_t>(src[input + sequence_length]) & 0xC0) == 0x80) {
        sequence_length++;
      }
    }

    const char* replacement = transliterateLatinCodepoint(codepoint);
    if (replacement != nullptr) {
      while (*replacement != 0 && output < dest_size - 1) {
        dest[output++] = *replacement++;
      }
    } else {
      dest[output++] = '\xDB';
    }
    input += sequence_length;
  }
  dest[output] = 0;
}

void SSD1306Display::endFrame() {
  display.display();
}
