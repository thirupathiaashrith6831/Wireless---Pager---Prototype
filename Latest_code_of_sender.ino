#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Keypad.h>
#include <U8g2lib.h>

// -------- OLED --------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// -------- KEYPAD --------
const byte ROWS = 4;

const byte COLS = 3;

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {D1, D2, D3, D4};
byte colPins[COLS] = {D5, D6, D7};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// -------- ESP-NOW --------
uint8_t broadcastMac[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// -------- BUFFER --------
char sendBuffer[5];   // 4 digits + null
int indexPos = 0;

// -------- OLED UPDATE --------
void showOnOLED(const char* text) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_logisoso24_tf);
  u8g2.drawStr(10, 45, text);
  u8g2.sendBuffer();
}

void showReady() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 20, "Sender Ready");
  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);

  // ---- OLED ----
  u8g2.begin();
  showReady();

  // ---- WiFi & ESP-NOW ----
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifi_set_channel(1);

  esp_now_init();
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(broadcastMac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  Serial.println("Sender Ready (OLED + Keypad)");
}

void loop() {
  char key = keypad.getKey();

  if (key) {

    // DIGITS
    if (key >= '0' && key <= '9') {
      if (indexPos < 4) {
        sendBuffer[indexPos++] = key;
        sendBuffer[indexPos] = '\0';

        esp_now_send(broadcastMac, (uint8_t*)&key, 1);
        showOnOLED(sendBuffer);

        Serial.print("Typing: ");
        Serial.println(sendBuffer);
      }
    }

    // SEND & RESET
    else if (key == '#') {
      esp_now_send(broadcastMac, (uint8_t*)&key, 1);

      Serial.print("Sent Final Code: ");
      Serial.println(sendBuffer);

      indexPos = 0;
      memset(sendBuffer, 0, sizeof(sendBuffer));
      showReady();
    }

    // CLEAR ONLY SENDER
    else if (key == '*') {
      indexPos = 0;
      memset(sendBuffer, 0, sizeof(sendBuffer));
      showReady();
      Serial.println("Sender Cleared");
    }
  }
}
