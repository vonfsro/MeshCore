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

namespace {

constexpr uint16_t packAscii(char first, char second = 0) {
  return static_cast<uint8_t>(first) |
         (static_cast<uint16_t>(static_cast<uint8_t>(second)) << 8);
}

constexpr uint16_t DROP_CODEPOINT = 0xFFFF;

struct LatinRange {
  uint16_t first;
  uint16_t last;
  char ascii;
  bool alternate_case;
  bool uppercase_is_odd;
};

static const LatinRange LATIN_RANGES[] = {
  // Latin-1 Supplement
  {0x00C0, 0x00C5, 'A', false, false}, {0x00E0, 0x00E5, 'a', false, false},
  {0x00C8, 0x00CB, 'E', false, false}, {0x00E8, 0x00EB, 'e', false, false},
  {0x00CC, 0x00CF, 'I', false, false}, {0x00EC, 0x00EF, 'i', false, false},
  {0x00D2, 0x00D6, 'O', false, false}, {0x00D8, 0x00D8, 'O', false, false},
  {0x00F2, 0x00F6, 'o', false, false}, {0x00F8, 0x00F8, 'o', false, false},
  {0x00D9, 0x00DC, 'U', false, false}, {0x00F9, 0x00FC, 'u', false, false},

  // Latin Extended-A: case normally alternates upper/lower by code point.
  {0x0100, 0x0105, 'A', true, false}, {0x0106, 0x010D, 'C', true, false},
  {0x010E, 0x0111, 'D', true, false}, {0x0112, 0x011B, 'E', true, false},
  {0x011C, 0x0123, 'G', true, false}, {0x0124, 0x0127, 'H', true, false},
  {0x0128, 0x0131, 'I', true, false}, {0x0134, 0x0135, 'J', true, false},
  {0x0136, 0x0137, 'K', true, false}, {0x0139, 0x0142, 'L', true, true},
  {0x0143, 0x0148, 'N', true, true},  {0x014C, 0x0151, 'O', true, false},
  {0x0154, 0x0159, 'R', true, false}, {0x015A, 0x0161, 'S', true, false},
  {0x0162, 0x0167, 'T', true, false}, {0x0168, 0x0173, 'U', true, false},
  {0x0174, 0x0175, 'W', true, false}, {0x0176, 0x0177, 'Y', true, false},
  {0x0179, 0x017E, 'Z', true, true},
};

uint16_t transliterateLatinCodepoint(uint32_t codepoint) {
  // A decomposed accent follows an already emitted base letter, so discard it.
  if (codepoint >= 0x0300 && codepoint <= 0x036F) return DROP_CODEPOINT;

  for (const LatinRange& range : LATIN_RANGES) {
    if (codepoint < range.first || codepoint > range.last) continue;

    char ascii = range.ascii;
    if (range.alternate_case) {
      const bool lowercase = static_cast<bool>(codepoint & 1) ^ range.uppercase_is_odd;
      if (lowercase) ascii += 'a' - 'A';
    }
    return packAscii(ascii);
  }

  // Irregular and multi-letter transliterations.
  switch (codepoint) {
    case 0x00C6: return packAscii('A', 'E'); // Æ
    case 0x00E6: return packAscii('a', 'e'); // æ
    case 0x00C7: return packAscii('C');      // Ç
    case 0x00E7: return packAscii('c');      // ç
    case 0x00D0: return packAscii('D');      // Ð
    case 0x00F0: return packAscii('d');      // ð
    case 0x00D1: return packAscii('N');      // Ñ
    case 0x00F1: return packAscii('n');      // ñ
    case 0x00DD: return packAscii('Y');      // Ý
    case 0x00FD: case 0x00FF: return packAscii('y');
    case 0x00DE: return packAscii('T', 'h'); // Þ
    case 0x00FE: return packAscii('t', 'h'); // þ
    case 0x00DF: return packAscii('s', 's'); // ß
    case 0x0132: return packAscii('I', 'J'); // Ĳ
    case 0x0133: return packAscii('i', 'j'); // ĳ
    case 0x0138: return packAscii('k');      // ĸ
    case 0x0149: case 0x014B: return packAscii('n');
    case 0x014A: return packAscii('N');
    case 0x0152: return packAscii('O', 'E'); // Œ
    case 0x0153: return packAscii('o', 'e'); // œ
    case 0x0178: return packAscii('Y');      // Ÿ
    case 0x017F: return packAscii('s');
    case 0x0218: return packAscii('S');      // Romanian Ș
    case 0x0219: return packAscii('s');
    case 0x021A: return packAscii('T');
    case 0x021B: return packAscii('t');
    default: return 0;
  }
}

} // namespace

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

    const uint16_t replacement = transliterateLatinCodepoint(codepoint);
    if (replacement == DROP_CODEPOINT) {
      // Nothing to emit.
    } else if (replacement != 0) {
      dest[output++] = static_cast<char>(replacement & 0xFF);
      if ((replacement >> 8) != 0 && output < dest_size - 1) {
        dest[output++] = static_cast<char>(replacement >> 8);
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
