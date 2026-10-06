#include <Wire.h>                           //i2c
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>               //
#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include "web_server.h"
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

#define SDA_PIN             8
#define SCL_PIN             9
#define LED_PIN             4
#define Buzzer_PIN          18

// NÚT NHẤN VẬT LÝ
#define BUTTON_PIN          15

#define HIGH_HR_THRESHOLD   100     // Nguong BAT canh bao
#define HIGH_HR_RESET       90      // Nguong TAT canh bao (hysteresis, phai thap hon nguong bat)
#define ALARM_MIN_HOLD_MS   3000UL  // Thoi gian toi thieu giu canh bao truoc khi duoc phep tat (chong nhap nhay)
#define ALARM_BEEP_PERIOD_MS 250UL  // Chu ky nhap nhay coi/den khi canh bao (250ms = 2 lan/giay)

// Che do do OFFLINE (khong can WiFi)
#define WIFI_CONNECT_TIMEOUT_MS 8000UL   // Thoi gian toi da cho ket noi WiFi 
#define OFFLINE_BOOT_HOLD_MS    2000UL   // Giu nut vat ly trong luc khoi dong -> bo qua WiFi luon

#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_RESET          -1
#define OLED_ADDRESS        0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "TEN_WiFi";
const char* password = "password";

unsigned long previousMillisResultHB = 0;
const unsigned long intervalResultHB = 1000;        // capnhat manhinh

unsigned long lastSSE = 0;
const unsigned long SSE_INTERVAL = 80;       //tansuat gui qua web

bool lastButtonState = HIGH;
bool buttonState = HIGH;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

const byte rateSize = 8;        //8 lan do nhip tim

byte rates[rateSize];
byte rateSpot = 0;

long lastBeat = 0;

float beatsPerMinute = 0;
int beatAvg = 0;
int BPMval = 0;

int x = 0;
int y = 0;
int lastx = 0;
int lasty = 20;

bool get_BPM = false;

// Trang thai canh bao (LED/coi) va WiFi/Offline
bool alarmActive = false;
unsigned long alarmActiveSince = 0;

bool wifiConnected = false;
bool offlineMode = false;

byte graphSampleCount = 0;

byte tSecond = 0;
byte tMinute = 0;
byte tHour = 0;

char tTime[10];

const unsigned char logo_POEM [] PROGMEM = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x93, 0xfb, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbb, 0xfb, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xfb, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbd, 0xf7, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbc, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbe, 0xf7, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xde, 0xf7, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xde, 0x77, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x77, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x77, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xee, 0x23, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf6, 0x23, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf6, 0x23, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf6, 0x03, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x01, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x78, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x78, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x78, 0x0f, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xb8, 0xff, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xbb, 0xff, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xd3, 0xff, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0xc1, 0xfc, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xc0, 0xc0, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xc0, 0x07, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x80, 0x1f, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x3f, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0x0f, 0xff, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x0f, 0xff, 0xff, 0xff, 0xf0, 0x1f, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3, 0x87, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xc3, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc7, 0xff, 0xff, 
  0xff, 0xff, 0xcf, 0x30, 0x33, 0xf8, 0x3c, 0x7c, 0x60, 0xfc, 0x1e, 0x79, 0xff, 0x87, 0xff, 0xff, 
  0xff, 0xff, 0xce, 0x32, 0x67, 0xe3, 0x1c, 0x7c, 0x64, 0x31, 0x8e, 0x71, 0xff, 0x8f, 0xff, 0xff, 
  0xff, 0xff, 0xcc, 0xe7, 0xe7, 0xe7, 0x8c, 0x78, 0x67, 0x33, 0xc6, 0x67, 0xff, 0x0f, 0xff, 0xff, 
  0xff, 0xff, 0x99, 0xe7, 0xe7, 0xcf, 0xcc, 0x38, 0xe7, 0x27, 0xe6, 0x4f, 0xfe, 0x1f, 0xff, 0xff, 
  0xff, 0xff, 0x93, 0xe7, 0xe7, 0xcf, 0xcd, 0x30, 0xce, 0x27, 0xe6, 0x1f, 0xfc, 0x7f, 0xff, 0xff, 
  0xff, 0xff, 0x83, 0xe0, 0xe7, 0xcf, 0xcd, 0x34, 0xcc, 0x67, 0xe6, 0x1f, 0xf8, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0x93, 0xe7, 0xe7, 0x8f, 0xc9, 0x24, 0xc0, 0xc7, 0xe4, 0x8f, 0xf1, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0x99, 0xe7, 0xe7, 0xcf, 0x99, 0x8c, 0xcf, 0xe7, 0xcc, 0xcf, 0xe3, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0x98, 0xe7, 0xe7, 0xcf, 0x99, 0x8c, 0xcf, 0xe7, 0xcc, 0xe7, 0xc7, 0x83, 0xff, 0xff, 
  0xff, 0xff, 0x9c, 0x64, 0x64, 0x47, 0x39, 0x9c, 0xcf, 0xe3, 0x9c, 0xe3, 0x80, 0x03, 0xff, 0xff, 
  0xff, 0xff, 0x9e, 0x60, 0x60, 0xe0, 0x79, 0xdc, 0xcf, 0xf0, 0x3c, 0xf1, 0x80, 0x03, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


const unsigned char Heart_Icon[] PROGMEM = {
  0x00, 0x00, 0x18, 0x30, 0x3c, 0x78, 0x7e, 0xfc,
  0xff, 0xfe, 0xff, 0xfe, 0xee, 0xee, 0xd5, 0x56,
  0x7b, 0xbc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0,
  0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00
};

const char* PARAM_INPUT_1 = "BTN_Start_Get_BPM";

String BTN_Start_Get_BPM = "";

JSONVar JSON_All_Data;

AsyncWebServer server(80);
AsyncEventSource events("/events");   //gui du lieu lien tuc

// RESET DỮ LIỆU ĐO
void ResetMeasurementData() {

  BPMval = 0;
  beatsPerMinute = 0;
  beatAvg = 0;

  x = 0;
  y = 20;

  lastx = 0;
  lasty = 20;

  tSecond = 0;
  tMinute = 0;
  tHour = 0;

  rateSpot = 0;
  graphSampleCount = 0;

  lastBeat = millis();

  for (byte i = 0; i < rateSize; i++) {
    rates[i] = 0;
  }

  sprintf(tTime, "%02d:%02d:%02d",
          tHour,
          tMinute,
          tSecond);
}

// VẼ ĐỒ THỊ TÍN HIỆU IR
void DrawGraph(long irValue) {

  if (x >= 128) {

    display.fillRect(0, 0, 128, 42, BLACK);

    x = 0;
    lastx = 0;
    lasty = 20;
  }

  int ySignalMap = map(                     //chuyen ir thanh toa do
    irValue,
    30000,
    120000,
    2,
    40
  );

  ySignalMap = constrain(               //giơi han toạ do
    ySignalMap,
    2,
    40
  );

  y = 42 - ySignalMap;

  if (x == 0) {
    lastx = 0;
    lasty = y;
  }

  display.drawLine(
    lastx,
    lasty,
    x,
    y,
    WHITE
  );

  lastx = x;
  lasty = y;

  x++;
}

// HIỂN THỊ MÀN HÌNH START
void ShowStartScreen() {

  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(20, 0);
  display.print("Bat dau do BPM");

  display.setTextSize(3);
  display.setCursor(55, 25);
  display.print("3");

  display.display();

  delay(1000);

  display.fillRect(
    55,
    25,
    25,
    30,
    BLACK
  );

  display.setCursor(55, 25);
  display.print("2");
  display.display();
  delay(1000);
  display.fillRect(
    55,
    25,
    25,
    30,
    BLACK
  );

  display.setCursor(55, 25);
  display.print("1");
  display.display();
  delay(1000);
  display.clearDisplay();
  display.drawLine(
    0,
    43,
    127,
    43,
    WHITE
  );

  display.drawBitmap(
    0,
    47,
    Heart_Icon,
    16,
    16,
    WHITE
  );

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(20, 48);
  display.print(": 0");
  display.setCursor(92, 48);
  display.print("BPM");
  display.display();
}

// HIỂN THỊ MÀN HÌNH STOP
void ShowStopScreen() {

  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(2);

  display.setCursor(40, 25);
  display.print("STOP");

  display.display();

  delay(800);

  display.clearDisplay();

  display.drawLine(
    0,
    43,
    127,
    43,
    WHITE
  );

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(10, 48);
  display.print("Nhan START de do");

  display.display();
}

// BẮT ĐẦU ĐO
void StartMeasurement() {

  // Reset dữ liệu trước khi đo
  ResetMeasurementData();

  get_BPM = true;

  lastBeat = millis();
  previousMillisResultHB = millis();

  digitalWrite(LED_PIN, LOW);
  digitalWrite(Buzzer_PIN, LOW);

  Serial.println("=== BAT DAU DO BPM ===");

  ShowStartScreen();
}

// DỪNG ĐO
void StopMeasurement() {

  get_BPM = false;

  digitalWrite(LED_PIN, LOW);
  digitalWrite(Buzzer_PIN, LOW);

  Serial.println("=== DUNG DO BPM ===");

  ShowStopScreen();
}

// XỬ LÝ LỆNH START / STOP
void ProcessCommand(String command) {

  command.trim();

  if (command != "START" && command != "STOP") {
    return;
  }

  Serial.print("Xu ly lenh: ");
  Serial.println(command);

  if (command == "START") {

    // Nếu đang đo rồi thì không START lại
    if (!get_BPM) {
      StartMeasurement();
    }

  } else if (command == "STOP") {

    // Nếu đang dừng rồi thì không STOP lại
    if (get_BPM) {
      StopMeasurement();
    }
  }
}

void HandlePhysicalButton() {

  bool reading = digitalRead(BUTTON_PIN);

  // Có thay đổi trạng thái nút
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Đợi hết thời gian chống dội
  if ((millis() - lastDebounceTime) > debounceDelay) {

    if (reading != buttonState) {

      buttonState = reading;

      if (buttonState == LOW) {

        Serial.println("=== NHAN NUT VAT LY ===");

        if (get_BPM) {
          ProcessCommand("STOP");
        } else {
          ProcessCommand("START");
        }
      }
    }
  }

  lastButtonState = reading;
}

void GetHeartRate() {

  long irValue = particleSensor.getIR();

  if (irValue < 35000) {

    beatsPerMinute = 0;
    beatAvg = 0;
    BPMval = 0;

  } else {

    if (get_BPM && checkForBeat(irValue)) {

      long delta = millis() - lastBeat;     //thời gian 2 nhịp đap

      lastBeat = millis();

      if (delta > 0) {

        beatsPerMinute =
          60.0 / (delta / 1000.0);      //BPM tuc thời

        if (
          beatsPerMinute < 255 &&
          beatsPerMinute > 30
        ) {

          rates[rateSpot++] =
            (byte)beatsPerMinute;

          rateSpot %= rateSize;

          beatAvg = 0;

          for (byte i = 0; i < rateSize; i++) {
            beatAvg += rates[i];                //trung bình
          }

          beatAvg /= rateSize;
        }
      }
    }

    BPMval = beatAvg;
  }

  if (get_BPM) {

    graphSampleCount++;

    if (graphSampleCount >= 2) {            //trên 2 lần lấy mẫu vẽ đồ thị

      graphSampleCount = 0;

      DrawGraph(irValue);
    }
  }

  if (wifiConnected && millis() - lastSSE >= SSE_INTERVAL) {              // 80ms gủi 1lan

    lastSSE = millis();

    JSON_All_Data["heartbeat_Signal"] = irValue;
    JSON_All_Data["BPM_TimeStamp"] = tTime;
    JSON_All_Data["BPM_Val"] = BPMval;
    JSON_All_Data["BPM_State"] = get_BPM;

    String JSON_All_Data_Send =
      JSON.stringify(JSON_All_Data);

    events.send(
      JSON_All_Data_Send.c_str(),
      "allDataJSON",
      millis()
    );
  }

  unsigned long currentMillisResultHB = millis();   //bô đếm thời gian

  if (
    currentMillisResultHB -
    previousMillisResultHB >=
    intervalResultHB
  ) {

    previousMillisResultHB =
      currentMillisResultHB;

    if (get_BPM) {

      tSecond++;

      if (tSecond >= 60) {
        tSecond = 0;
        tMinute++;
      }

      if (tMinute >= 60) {
        tMinute = 0;
        tHour++;
      }

      sprintf(
        tTime,
        "%02d:%02d:%02d",
        tHour,
        tMinute,
        tSecond
      );

      Serial.print("IR = ");
      Serial.print(irValue);

      Serial.print("   BPM = ");
      Serial.println(BPMval);

      display.fillRect(
        0,
        43,
        128,
        21,
        BLACK
      );

      display.drawLine(
        0,
        43,
        127,
        43,
        WHITE
      );

      display.drawBitmap(
        0,
        47,
        Heart_Icon,
        16,
        16,
        WHITE
      );

      display.setTextSize(2);
      display.setTextColor(WHITE);

      display.setCursor(20, 48);
      display.print(": ");
      display.print(BPMval);

      display.setCursor(92, 48);
      display.print("BPM");

      display.display();
    }
  }

  // ==== CANH BAO NHIP TIM CAO (LED + COI), CO HYSTERESIS + GIU TOI THIEU ====
  if (!get_BPM) {

    // Dung do -> tat canh bao ngay lap tuc
    alarmActive = false;

  } else if (!alarmActive && BPMval > HIGH_HR_THRESHOLD) {

    // Vuot nguong tren -> bat canh bao
    alarmActive = true;
    alarmActiveSince = millis();

  } else if (
    alarmActive &&
    BPMval <= HIGH_HR_RESET &&
    (millis() - alarmActiveSince) >= ALARM_MIN_HOLD_MS        //chỉ báo 3s
  ) {

    // Chi tat khi da xuong duoi nguong RESET (thap hon nguong bat)
    // VA da canh bao du lau (chong nhap nhay khi BPM dao dong quanh 100)
    alarmActive = false;
  }

  if (alarmActive) {

    digitalWrite(LED_PIN, HIGH);

    if ((millis() / ALARM_BEEP_PERIOD_MS) % 2 == 0) {
      digitalWrite(Buzzer_PIN, HIGH);
    } else {
      digitalWrite(Buzzer_PIN, LOW);
    }

  } else {

    digitalWrite(LED_PIN, LOW);
    digitalWrite(Buzzer_PIN, LOW);
  }
}

void setup() {

  Serial.begin(115200);

  Serial.println();

  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  pinMode(Buzzer_PIN, OUTPUT);

  // NÚT NHẤN VẬT LÝ
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(Buzzer_PIN, LOW);

  // === KIEM TRA GIU NUT VAT LY KHI KHOI DONG -> BO QUA WIFI, VAO THANG OFFLINE ===
  if (digitalRead(BUTTON_PIN) == LOW) {

    Serial.println("Phat hien giu nut khi khoi dong...");

    unsigned long holdStart = millis();
    bool stillHeld = true;

    while (millis() - holdStart < OFFLINE_BOOT_HOLD_MS) {
      if (digitalRead(BUTTON_PIN) != LOW) {
        stillHeld = false;
        break;
      }
      delay(20);
    }

    if (stillHeld) {
      offlineMode = true;
      Serial.println("=== VAO CHE DO OFFLINE (khong dung WiFi) ===");
    }
  }

  sprintf(
    tTime,
    "%02d:%02d:%02d",
    tHour,
    tMinute,
    tSecond
  );

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  Wire.setClock(100000);

  Serial.println("Khoi tao I2C...");

  Serial.print("SDA = GPIO");
  Serial.println(SDA_PIN);

  Serial.print("SCL = GPIO");
  Serial.println(SCL_PIN);

  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
    )
  ) {

    Serial.println(
      F("Khong tim thay OLED SSD1306 tai 0x3C!")
    );

    while (1) {

      digitalWrite(
        LED_PIN,
        !digitalRead(LED_PIN)
      );

      delay(500);
    }
  }

  display.clearDisplay();

  display.drawBitmap(
    0,
    0,
    logo_POEM,
    128,
    64,
    WHITE
  );

  display.display();

  delay(3000);

  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("OLED OK - I2C 0x3C");

  display.setCursor(0, 12);
  display.println("Dang khoi tao MAX30102...");

  display.display();

  delay(800);

  if (
    !particleSensor.begin(
      Wire,
      I2C_SPEED_STANDARD
    )
  ) {

    Serial.println(
      "Khong tim thay cam bien MAX30102 tai 0x57!"
    );

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.println("MAX30102 ERROR");

    display.println("Khong tim thay 0x57");
    display.println("Kiem tra SDA/SCL");

    display.display();

    while (1) {

      digitalWrite(
        Buzzer_PIN,
        HIGH
      );

      delay(100);

      digitalWrite(
        Buzzer_PIN,
        LOW
      );

      delay(900);
    }
  }

  Serial.println(
    "MAX30102 OK - tim thay tai 0x57"
  );

  byte ledBrightness = 0x1F;        //cuong do sáng hong ngoại
  byte sampleAverage = 4;           //lay 4 mẫu để giảm nhiêu
  byte ledMode = 2;                 //chế độ 2 led(hồng ngoại + đỏ)
  byte sampleRate = 100;            //toc do lay mau(Hz)
  int pulseWidth = 411;             //độ rộng xung
  int adcRange = 4096;              //dải đo adc

  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );

  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);

  Serial.println(
    "MAX30102 da cau hinh xong."
  );

  Serial.println();

  if (offlineMode) {

    // Da chon offline ngay tu luc khoi dong -> khong dung radio WiFi luon
    Serial.println("Bo qua WiFi (che do OFFLINE do nguoi dung chon).");
    WiFi.mode(WIFI_OFF);

  } else {

    Serial.println("WIFI mode : STA");

    WiFi.mode(WIFI_STA);

    delay(100);

    display.clearDisplay();

    display.setTextColor(WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.print("Connecting WiFi...");

    display.display();

    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(
      ssid,
      password
    );

    unsigned long wifiStartAttempt = millis();

    while (
      WiFi.status() != WL_CONNECTED &&
      (millis() - wifiStartAttempt) < WIFI_CONNECT_TIMEOUT_MS
    ) {

      Serial.print(".");

      delay(300);
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {

      wifiConnected = true;

      Serial.println("WiFi connected");

      Serial.print("ESP32 IP address : ");
      Serial.println(WiFi.localIP());

    } else {

      // Khong ket noi duoc trong thoi gian cho phep -> KHONG restart,
      // chuyen sang che do OFFLINE va tiep tuc chay binh thuong.
      Serial.println("Khong ket noi duoc WiFi - chuyen sang che do OFFLINE.");

      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);

      offlineMode = true;
    }
  }

  if (wifiConnected) {

    server.on(
      "/",
      HTTP_GET,
      [](AsyncWebServerRequest *request) {

        request->send_P(
          200,
          "text/html",
          MAIN_page
        );
      }
    );

    events.onConnect(
      [](AsyncEventSourceClient *client) {

        if (client->lastId()) {

          Serial.printf(
            "Client reconnected! Last message ID: %u\n",
            client->lastId()
          );
        }

        client->send(
          "hello!",
          NULL,
          millis(),
          10000
        );
      }
    );

    server.on(
      "/BTN_Comd",
      HTTP_GET,
      [](AsyncWebServerRequest *request) {

        if (
          request->hasParam(
            PARAM_INPUT_1
          )
        ) {

          BTN_Start_Get_BPM =
            request
            ->getParam(PARAM_INPUT_1)
            ->value();

          Serial.print(
            "BTN_Start_Get_BPM : "
          );

          Serial.println(
            BTN_Start_Get_BPM
          );
        }

        request->send(
          200,
          "text/plain",
          "OK"
        );
      }
    );

    server.addHandler(&events);

    server.begin();

    display.clearDisplay();

    display.setTextColor(WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("MAX30102: OK");

    display.println("OLED: OK");

    display.println();

    display.println("ESP32 IP:");

    display.println(WiFi.localIP());

    display.display();

    delay(2500);

  } else {

    // CHE DO OFFLINE: khong co web server, chi dung nut vat ly + OLED + coi/den
    display.clearDisplay();

    display.setTextColor(WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("MAX30102: OK");
    display.println("OLED: OK");
    display.println();
    display.println("CHE DO: OFFLINE");
    display.println("(Khong dung WiFi)");
    display.println("Dieu khien bang");
    display.println("nut vat ly GPIO15");

    display.display();

    delay(2500);
  }

  display.clearDisplay();

  display.drawLine(
    0,
    43,
    127,
    43,
    WHITE
  );

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(10, 48);
  display.print("Nhan START de do");

  display.display();

  ResetMeasurementData();

  Serial.println();
  Serial.println("==============================");
  Serial.println("SAN SANG DO NHIP TIM");
  Serial.println("Nut vat ly: GPIO15");
  Serial.println("==============================");
}

void loop() {

  HandlePhysicalButton();

  if (
    BTN_Start_Get_BPM == "START" ||
    BTN_Start_Get_BPM == "STOP"
  ) {

    String command =
      BTN_Start_Get_BPM;

    BTN_Start_Get_BPM = "";

    ProcessCommand(command);
  }

  GetHeartRate();
}
