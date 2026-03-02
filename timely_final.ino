#define ENABLE_GxEPD2_GFX 1

#include <Arduino.h>
#include "logo.h"
#include <Wire.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
//#include <Fonts/FreeMonoBold9pt7b.h>
#include <Adafruit_BMP280.h>
#include <RTClib.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

const char PRODUCT_NAME[] = "Timely";
const char POWERED_NAME[] = "Powered by Stressbusters";


/**
 * ESP32 C3 as Access Point
 */

// Replace with your network credentials
const char* SSID = "Timely by Stressbusters";
const char* PASSWD = "pwdpassword";
const char* HOSTNAME = "timely.local";

const IPAddress IP(192, 168, 1, 1);
const IPAddress NMASK(255, 255, 255, 0);

const uint16_t AP_ON_TIMER = (60 * 1000); // two minutes
uint64_t ap_last_on = 0;
bool ap_state = false;

// Set web server port number to 80
AsyncWebServer server(80);

DNSServer dnsServer;
const uint8_t DNS_PORT = 53;

const char* PARAM_INPUT_1 = "date";
const char* PARAM_INPUT_2 = "time";

// Variable to store the HTTP request
const char index_html[] PROGMEM = R"rawliteral(
<html>
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">

        <title>Timely configuration</title>
        
        <style>
            * {
                margin: 0;
                padding: 0;
                box-sizing: border-box;
            }

            body {
                background: #262626;
                display: flex;
                justify-content: center;
                align-items: center;
            }

            .container {
                background: #000;
                border-radius: 1rem;
                padding: 2rem;
            }

            form {
                text-align: center;
            }

            label {
                display: block;
                margin: 1rem 0 0.4rem;
                color: #e0e0e0;
                font-size: 1rem;
            }

            input {
                width: 100%;
                padding: 0.6rem;
                margin-bottom: 1rem;
                border: none;
                border-radius: 0.4rem;
                font-size: 1rem;
                background: #4a4a4a;
                color: #e0e0e0;
            }

            button {
                background: #5a5a5a;
                color: #e0e0e0;
                border: none;
                padding: 0.6rem 1.2rem;
                font-size: 1rem;
                border-radius: 0.4rem;
                cursor: pointer;
            }

            button:hover {
                background: #6a6a6a;
            }
            
            .container img {
                display: block;
                margin: 0 auto 1.5rem;
                max-width: 8rem;
                height: auto;
            }
        </style>
    </head>
    <body>
        <div class="container">
            <img src="logo" alt="">
            <form action="/get" method="GET">
                <label>Data: </label>
                <input type="date" min="2026-01-01" name="date" required>

                <label>Tempo: </label>
                <input type="time" name="time" required>

                <button type="submit">Salva</button>
            </form>
        </div>
    </body>
</html>
)rawliteral";


/*===================================================
              E-INK DISPLAY CONFIGURATION
===================================================*/

uint8_t E_INK_ROTATION = 3;

uint8_t E_INK_CS    = 0;
uint8_t E_INK_BUSY  = 5;
uint8_t E_INK_RES   = 7;
uint8_t E_INK_DC    = 1;

uint8_t E_INK_WIDTH = 250;
uint8_t E_INK_HEIGTH = 122;

GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> eink(GxEPD2_213_BN(E_INK_CS, E_INK_DC, E_INK_RES, E_INK_BUSY));
U8G2_FOR_ADAFRUIT_GFX display;

//u8g2_font_7Segments_26x42_mn
//const uint8_t* MAIN_FONT = u8g2_font_inb33_mf;
//const uint8_t* NUMBERS_FONT = u8g2_font_10x20_mf;
//const uint8_t* TEXT_FONT = u8g2_font_inb16_mf;
//const uint8_t* TEXT_FONT = u8g2_font_VCR_OSD_mf;
const uint8_t* MAIN_FONT = u8g2_font_7Segments_26x42_mn;
const uint8_t* TXT_FONT = u8g2_font_9x15B_mf;
const uint8_t* BATT_FONT = u8g2_font_battery19_tn;
const uint8_t* WIFI_FONT = u8g2_font_open_iconic_all_1x_t;

const uint8_t MAX_PARTIAL_REFRESH = 30;
const uint32_t PARTIAL_REFRESH_RATE = (30 * 1000); 
uint64_t last_partial_refresh = 0;
uint16_t count_partial_refresh = 0;
bool is_full_refresh = false;

const uint16_t BLACK = GxEPD_BLACK;
const uint16_t WHITE  = GxEPD_WHITE;


bool wifi_forced_refresh = false;

bool time_change = true;


/*=======================================
              REAL TIME CLOCK
=======================================*/

RTC_DS1307 rtc;
String last_time = "";
String curr_time = "";

String curr_date = "";
String last_date = "";


/*==============================
              BMP280
==============================*/

Adafruit_BMP280 bmp;
//float last_temp = 0;
//float curr_temp = 0;
uint16_t temp_read_rate = 5000; // ms
uint64_t last_temp_read = 0;


/*===================================
              ISR BUTTONS
===================================*/

const uint8_t WIFI_BTN_PIN   = 10;
volatile bool left_btn_state   = false;


/*=======================================
              BATTERY MONITOR
=======================================*/

const uint8_t BATT_PIN    = 3;
const float OPERATING_V     = 3.3;

const uint16_t R1 = 2220;
const uint16_t R2 = 5100;
const float BATT_FULL_VOLTAGE = 4.2;

const float CENTRAL_NODE_VOLTAGE = (BATT_FULL_VOLTAGE * R2) / (R1 + R2);
const uint16_t BATT_FULL_READ    = 3600;

uint8_t curr_perc = 255;
uint8_t last_perc = 255;


/*==============================
              BUZZER
==============================*/

const uint8_t BUZZ_PIN = 2;

/**
 * 
 * ORARIO SCOLASTICO
 * 
**/

JsonDocument doc;

char daysOfTheWeek[7][10] = {"Domenica" ,"Lunedi", "Martedi", "Mercoledi", "Giovedi", "Venerdi", "Sabato"};

const TimeSpan MODULO = TimeSpan(0, 0, 50, 0);
const TimeSpan INTERVALLO_M = TimeSpan(0, 0, 10, 0);
const TimeSpan INTERVALLO_P = TimeSpan(0, 0, 5, 0);

String classe; // esempio per 5

enum materie {
  M1 = 0,
  M2,
  M3,
  M4,
  M5,
  M6,
  M7,
  M8,
  M9,
  M10
};


// stable
void displayInit() {
  eink.init(115200, true, 50, false);
  eink.setRotation(E_INK_ROTATION);
  display.begin(eink);
}

void clearScreen(uint16_t color) {
  eink.setFullWindow();
  eink.firstPage();
  do {
    eink.fillScreen(color);
  } while (eink.nextPage());
}

void drawLogo() {
  display.setFontMode(1);                 // use u8g2 transparent mode (this is default)
  display.setFontDirection(0);            // left to right (this is default)
  display.setForegroundColor(WHITE);         // apply Adafruit GFX color
  display.setBackgroundColor(BLACK);         // apply Adafruit GFX color
  display.setFont(TXT_FONT);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  int16_t tw = display.getUTF8Width(PRODUCT_NAME); // text box width
  int16_t ta = display.getFontAscent(); // positive
  int16_t td = display.getFontDescent(); // negative; in mathematicians view
  int16_t th = ta - td; // text box height
  //Serial.print("ascent, descent ("); Serial.print(u8g2Fonts.getFontAscent()); Serial.print(", "); Serial.print(u8g2Fonts.getFontDescent()); Serial.println(")");
  // center bounding box by transposition of origin:
  // y is base line for u8g2Fonts, like for Adafruit_GFX True Type fonts
  uint16_t x = (eink.width() - tw) / 2;
  uint16_t y = (eink.height() - th) / 2 + ta;


  //eink.setFullWindow();
  eink.firstPage();
  do {
    eink.fillScreen(BLACK);
    eink.drawXBitmap((eink.width() - logo_width) / 2, (eink.height() - logo_height) / 2 - 10, logo_bits, logo_width, logo_height, WHITE);
    display.setCursor(x, eink.height() - 10); // start writing at this position
    display.print(PRODUCT_NAME);
  } while (eink.nextPage());
}

bool rtcInit() {
  return rtc.begin();
}

void bmpInit() {
  if (!bmp.begin(BMP280_ADDRESS_ALT, 0x60)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}

String getTime() {
  DateTime now = rtc.now();

  String hour   = String(now.hour());
  String minute = String(now.minute());

  if (hour.length() != 2)
    hour = "0" + hour;
  if (minute.length() != 2)
    minute = "0" + minute;

  return hour + ':' + minute;
}

void littleFSInit() {
  if(!LittleFS.begin()){
    //Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  //Serial.println("LittleFS mounted successfully");
}



// testing

//const char HelloWorld[] = "Hello World!";
//const char HelloWeACtStudio[] = "WeAct Studio";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void ap_web_server_init() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, PASSWD);
  //Serial.println("Wait 100 ms for AP_START...");
  delay(100);
  
  //Serial.println("Set softAPConfig");
  WiFi.softAPConfig(IP, IP, NMASK);
  
  //IPAddress myIP = WiFi.softAPIP();
  //Serial.print("AP IP address: ");
  //Serial.println(WiFi.softAPIP());

  //activating dns to use custom hostname
  dnsServer.start(DNS_PORT, HOSTNAME, IP);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });
  
  server.on("/logo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/logo0.webp", "/");
  });

  //avoiding android errors
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("/");
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage = "Dati inviati non validi";
    String inputDate;
    String inputTime;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputDate = request->getParam(PARAM_INPUT_1)->value();

      inputMessage += " and " + request->getParam(PARAM_INPUT_2)->value();
      inputTime = request->getParam(PARAM_INPUT_2)->value();

      // split date and time
      //2026-02-12 and 20:03
      /*
       *
       * YYYY-MM-DD --> 2026-02-12
       * HH:MM --> 20:03
       *
       */
      //Serial.println(inputDate  + "test");

      uint8_t firstDash = inputDate.indexOf("-");
      uint8_t secondDash = inputDate.indexOf("-", firstDash + 1);

      /*
      //String year  = inputDate.substring(0, firstDash);
      //String month = inputDate.substring(firstDash + 1, secondDash);
      //String day   = inputDate.substring(secondDash + 1);

      Serial.print("YEAR: ");
      Serial.println((uint16_t)inputDate.substring(0, firstDash).toInt());
      Serial.print("MONTH: ");
      Serial.println(inputDate.substring(firstDash + 1, secondDash).toInt());
      Serial.print("DAY: ");
      Serial.println(inputDate.substring(secondDash + 1).toInt());
      */

      uint8_t timeDividerPos = inputTime.indexOf(":");

      /*
      String hour = inputTime.substring(0, timeDividerPos);

      Serial.print("Hour. ");
      Serial.println(hour);
      */

      // This line sets the RTC with an explicit date & time, for example to set
      // January 21, 2014 at 3am you would call:
      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

      rtc.adjust(DateTime(inputDate.substring(0, firstDash).toInt(), inputDate.substring(firstDash + 1, secondDash).toInt(), inputDate.substring(secondDash + 1).toInt(), inputTime.substring(0, timeDividerPos).toInt(), inputTime.substring(timeDividerPos + 1).toInt(), 0));

      request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputDate + " and " + inputTime + ") with values: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
    }
    else {
      Serial.println(inputMessage);
      request->send(200, "text/html", "HTTP GET request sent to your ESP on input (" 
                                     + inputDate + ", " + inputTime + ") with values: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
    }
    
  });
  server.onNotFound(notFound);
  // Start server
  server.begin();
  
}

void ap_web_server_off() {
  WiFi.softAPdisconnect(true);
}

void IRAM_ATTR leftBtnChange() {
  left_btn_state = true;
}

void attachBtnInterrupts() {
  attachInterrupt(digitalPinToInterrupt(WIFI_BTN_PIN), leftBtnChange, FALLING);
}

void dettachBtnInterrupts() {
  detachInterrupt(WIFI_BTN_PIN);
}

void initializeButtons() {
  pinMode(WIFI_BTN_PIN, INPUT_PULLUP);

  attachBtnInterrupts();
}

void initializeBuzzer() {
  pinMode(BUZZ_PIN, OUTPUT);
}

uint8_t isrBtnChange() {
  if (left_btn_state) {
    left_btn_state = false;
    return 1;
  }

  return 0;
}

void checkRefreshState() {
  if (count_partial_refresh >= MAX_PARTIAL_REFRESH) {
    //eink.display(false);
    clearScreen(WHITE);
    count_partial_refresh = 0;
    is_full_refresh = true;
  }
}

void printText(String text, const uint8_t* font) {
  dettachBtnInterrupts();
  delay(10);

  eink.setRotation(E_INK_ROTATION);

  display.setFontMode(1);
  display.setFontDirection(0); // left to right (this is default)
  display.setForegroundColor(BLACK);
  display.setBackgroundColor(WHITE);
  display.setFont(font);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  int16_t tw = display.getUTF8Width(text.c_str()); // text box width
  int16_t ta = display.getFontAscent();
  int16_t td = display.getFontDescent();
  int16_t th = ta - td; // text box height

  // center bounding box by transposition of origin:
  // y is base line for u8g2Fonts, like for Adafruit_GFX True Type fonts
  uint16_t x = (eink.width() - tw) / 2;
  uint16_t y = (eink.height() - th) / 2 + ta;

  // partial window
  uint16_t wx = (eink.width()  - tw) / 2;
  uint16_t wy = (eink.height() - th) / 2;

  eink.setPartialWindow(wx, wy, tw, th);
  //eink.setPartialWindow(0, 0, eink.width(), eink.height());

  /*
  eink.firstPage();
  do {
    eink.fillRect(wx, wy, tw, th, GxEPD_WHITE);
  }
  while (eink.nextPage());
  count_partial_refresh++;*/

  //delay(500);

  eink.firstPage();
  do {
    eink.fillRect(wx, wy, tw, th, WHITE);
    //display.fillScreen(GxEPD_BLACK);
    //eink.fillRect(0, 0, eink.width(), eink.height(), GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(text);
  }
  while (eink.nextPage());
  count_partial_refresh++;

  attachBtnInterrupts();
}

void printText(String text, const uint8_t* font, int16_t offset_x, int16_t offset_y) {
  dettachBtnInterrupts();
  delay(10);

  eink.setRotation(E_INK_ROTATION);

  display.setFontMode(1);
  display.setFontDirection(0); // left to right (this is default)
  display.setForegroundColor(BLACK);
  display.setBackgroundColor(WHITE);
  display.setFont(font);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  int16_t tw = display.getUTF8Width(text.c_str()); // text box width
  int16_t ta = display.getFontAscent();
  int16_t td = display.getFontDescent();
  int16_t th = ta - td; // text box height
  
  eink.setPartialWindow(offset_x, offset_y, tw, th);

  //delay(500);

  eink.firstPage();
  do {
    //eink.fillRect(wx, wy, tbw, tbh, BLACK);
    //display.fillScreen(GxEPD_BLACK);
    eink.fillRect(offset_x, offset_y, tw, th, GxEPD_WHITE);
    display.setCursor(offset_x, offset_y + ta);
    display.print(text);
  }
  while (eink.nextPage());
  count_partial_refresh++;

  attachBtnInterrupts();
}

void printText(String text, const uint8_t* font, uint8_t rotation, int16_t offset_x, int16_t offset_y) {
  dettachBtnInterrupts();
  delay(10);

  eink.setRotation(rotation);

  display.setFontMode(1);
  display.setFontDirection(0); // left to right (this is default)
  display.setForegroundColor(BLACK);
  display.setBackgroundColor(WHITE);
  display.setFont(font);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  int16_t tw = display.getUTF8Width(text.c_str()); // text box width
  int16_t ta = display.getFontAscent();
  int16_t td = display.getFontDescent();
  int16_t th = ta - td; // text box height
  
  eink.setPartialWindow(offset_x, offset_y, tw, th);

  //delay(500);

  eink.firstPage();
  do {
    //eink.fillRect(wx, wy, tbw, tbh, BLACK);
    //display.fillScreen(GxEPD_BLACK);
    eink.fillRect(offset_x, offset_y, tw, th, GxEPD_WHITE);
    display.setCursor(offset_x, offset_y + ta);
    display.print(text);
  }
  while (eink.nextPage());
  count_partial_refresh++;

  attachBtnInterrupts();
}

void printXText(String text, const uint8_t* font, int16_t offset_y) {
  dettachBtnInterrupts();
  delay(10);

  

  int16_t offset_x = 0;

  eink.setRotation(E_INK_ROTATION);

  display.setFontMode(1);
  display.setFontDirection(0); // left to right (this is default)
  display.setForegroundColor(BLACK);
  display.setBackgroundColor(WHITE);
  display.setFont(font);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall

  int16_t tw = display.getUTF8Width(text.c_str()); // text box width
  int16_t ta = display.getFontAscent();
  int16_t td = display.getFontDescent();
  int16_t th = ta - td; // text box height
  // center bounding box by transposition of origin:
  // y is base line for u8g2Fonts, like for Adafruit_GFX True Type fonts
  uint16_t x = (eink.width() - tw) / 2;
  uint16_t y = offset_y + ta;

  // partial window
  eink.setPartialWindow(x, offset_y, tw, th);

  eink.firstPage();
  do {
    eink.fillRect(x, offset_y, tw, th, WHITE);
    //display.fillScreen(GxEPD_BLACK);
    //eink.fillRect(0, 0, eink.width(), eink.height(), GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(text);
  } while (eink.nextPage());
  count_partial_refresh++;

  attachBtnInterrupts();
}

void deleteXText(String text, const uint8_t* font, int16_t offset_y) {
  dettachBtnInterrupts();
  delay(10);

  

  int16_t offset_x = 0;

  eink.setRotation(E_INK_ROTATION);

  display.setFontMode(1);
  display.setFontDirection(0); // left to right (this is default)
  display.setForegroundColor(BLACK);
  display.setBackgroundColor(WHITE);
  display.setFont(font);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall

  int16_t tw = display.getUTF8Width(text.c_str()); // text box width
  int16_t ta = display.getFontAscent();
  int16_t td = display.getFontDescent();
  int16_t th = ta - td; // text box height
  // center bounding box by transposition of origin:
  // y is base line for u8g2Fonts, like for Adafruit_GFX True Type fonts
  uint16_t x = (eink.width() - tw) / 2;
  uint16_t y = offset_y + ta;

  // partial window
  eink.setPartialWindow(x, offset_y, tw, th);

  eink.firstPage();
  do {
    eink.fillRect(x, offset_y, tw, th, WHITE);
    //display.fillScreen(GxEPD_BLACK);
    //eink.fillRect(0, 0, eink.width(), eink.height(), GxEPD_WHITE);
    //display.setCursor(x, y);
    //display.print(text);
  } while (eink.nextPage());
  count_partial_refresh++;

  attachBtnInterrupts();
}

uint8_t getBatteryPercentage() {
  return (analogRead(BATT_PIN) * 100) / 3900;
}

uint8_t getBatteryTicks() {
  uint8_t perc = getBatteryPercentage();

  if (perc >= 80)
    return 5;
  
  if (perc >= 60)
    return 4;

  if (perc >= 40)
    return 3;

  if (perc >= 20)
    return 2;

  if (perc > 0)
    return 1;

  return 0;
}

void loadConfiguration(const char* filename) {
  // Open file for reading
  File file = LittleFS.open(filename);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  /*
  JsonArray lunedi = doc["Giorni"]["Lunedi"];

  for (int i = 0; i < lunedi.size(); i++) {
    const char* materia = lunedi[i];
    Serial.println(materia);
  }
  */
  classe = doc["Classe"].as<String>();
  
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}

String getDate() {
  DateTime now = rtc.now();

  String day = String(now.day());
  String month = String(now.month());
  String year = String(now.year());

  if (day.length() != 2)
    day = "0" + day;

  if (month.length() != 2)
    month = "0" + month;

  return day + "/" + month + "/" + year;
}

String getTemperature() {
  //return String(bmp.readTemperature(), 1) + "°C";
  return "22.4°C";
}

void printTextRightAligned(String text, const uint8_t* font, int16_t yOffset) {

  dettachBtnInterrupts();
  delay(10);

  eink.setRotation(E_INK_ROTATION);
  display.setFontMode(1);
  display.setFontDirection(0);
  display.setForegroundColor(BLACK);
  display.setBackgroundColor(WHITE);
  display.setFont(font);

  int16_t tw = display.getUTF8Width(text.c_str());
  int16_t ta = display.getFontAscent();
  int16_t td = display.getFontDescent();
  int16_t th = ta - td;

  // Calcolo posizione X allineata a destra
  int16_t x = eink.width() - tw;

  // Protezione: se il testo è più largo dello schermo
  if (x < 0) {
    x = 0;
    tw = eink.width();   // limitiamo l’area al display
  }

  // Protezione Y (non uscire in basso)
  if (yOffset + th > eink.height()) {
    yOffset = eink.height() - th;
  }
  if (yOffset < 0) {
    yOffset = 0;
  }

  int16_t y = yOffset + ta;  // baseline corretta

  eink.setPartialWindow(x, yOffset, tw, th);

  eink.firstPage();
  do {
    eink.fillRect(x, yOffset, tw, th, WHITE);
    display.setCursor(x, y);
    display.print(text);
  }
  while (eink.nextPage());

  count_partial_refresh++;

  attachBtnInterrupts();
}

bool isSchoolDay() {
  return (rtc.now().dayOfTheWeek() != 0) && (rtc.now().dayOfTheWeek() != 6);
}

bool isReentryDay() {
  return (rtc.now().dayOfTheWeek() == 1) || (rtc.now().dayOfTheWeek() == 4);
}

uint8_t getHour() {
  return rtc.now().hour();
}

uint8_t getMinute() {
  return rtc.now().minute();
}





void setup() {
  Serial.begin(9600);

  displayInit();

  rtcInit();

  //bmpInit();

  initializeButtons();

  littleFSInit();

  loadConfiguration("/orario.json");

  //clearScreen(WHITE);
  //delay(2000);
  
  drawLogo();
  delay(2000);
  clearScreen(WHITE);
  
  for (uint8_t i = 0; i < 8; i++) {
    printText(POWERED_NAME, TXT_FONT);
  }

  clearScreen(WHITE);

  //Serial.println(index_html);

  //fullScreenStartup();

  //printText("NIZAR", TXT_FONT);
}


String last_subj = "";
String last_next_subj = "";
uint8_t last_min_rim = 0;


void loop() {
  if (((millis() - last_partial_refresh) > PARTIAL_REFRESH_RATE) || wifi_forced_refresh || time_change) {
    checkRefreshState();
    
    //printXText(getTime(), MAIN_FONT, 30);

    printText(String(getBatteryTicks()), BATT_FONT, 4, 1, eink.width() - 19);

    if (!ap_state)
      printXText(getDate(), TXT_FONT, 0);
    else
      printXText("WiFi  mode", TXT_FONT, 0);
      //printXText("8", WIFI_FONT, 0);
    

    printTextRightAligned(getTemperature(), TXT_FONT, 0);



    if(isSchoolDay()) {
      //Serial.println(daysOfTheWeek[now.dayOfTheWeek()]);
      String next_subj = "";
      String act_subj = "";
      char* intervallo = "Intervallo";
      char* pranzo = "Pausa pranzo";
      uint8_t act_min = getMinute();
      uint8_t min_rimanenti = 0;
      char* day = daysOfTheWeek[rtc.now().dayOfTheWeek()];

      // tutti i giorni scolastici
      switch (getHour()) {
        case 7:
          if (act_min >= 50)
            next_subj = doc["Giorni"][day][M1].as<String>();
            //Serial.println(next_subj);
          break;

        case 8:
          if (act_min < 50) {
            // primo modulo
            act_subj = doc["Giorni"][day][M1].as<String>();

            if (act_min >= 45) {
              // next subject
              next_subj = doc["Giorni"][day][M2].as<String>();
            }

            min_rimanenti = 50 - act_min;
          } else {
            // secondo modulo
            act_subj = doc["Giorni"][day][M2].as<String>();

            min_rimanenti = 40 + (60 - act_min);
          }
          break;
          
        case 9:
          if (act_min < 40) {
            // secondo modulo
            act_subj = doc["Giorni"][day][M2].as<String>();

            if (act_min >= 35) {
              // intervallo fra 10 min
              next_subj = intervallo;
            }

            min_rimanenti = 40 - act_min;
          } else if (act_min < 50) {
            // intervallo
            act_subj = intervallo;

            // next subj
            next_subj = doc["Giorni"][day][M3].as<String>();

            min_rimanenti = 50 - act_min;
          } else {
            // terzo modulo
            act_subj = doc["Giorni"][day][M3].as<String>();
            min_rimanenti = 40 + (60 - act_min);
          }
          break;
          
        case 10:
          if (act_min < 40) {
            // terzo modulo
            act_subj = doc["Giorni"][day][M3].as<String>();

            if (act_min >= 35) {
              // next subject
              next_subj = doc["Giorni"][day][M4].as<String>();
            }

            min_rimanenti = 40 - act_min;
          } else {
            // quarto modulo
            act_subj = doc["Giorni"][day][M4].as<String>();
            min_rimanenti = 30 + (60 - act_min);
          }
          break;
          
        case 11:
          if (act_min < 30) {
            // quarto modulo
            act_subj = doc["Giorni"][day][M4].as<String>();

            if (act_min >= 25) {
              // next subj
              next_subj = intervallo;
            }

            min_rimanenti = 30 - act_min;
          } else if (act_min < 40) {
            act_subj = intervallo;

            next_subj = doc["Giorni"][day][M5].as<String>();

            min_rimanenti = 40 - act_min;
          } else {
            // quinto modulo
            act_subj = doc["Giorni"][day][M5].as<String>();

            min_rimanenti = 30 + (60 - act_min);
          }
          break;
          
        case 12:
          if (act_min < 30) {
            // quinto modulo
            act_subj = doc["Giorni"][day][M5].as<String>();

            if (act_min >= 25) {
              // next subj
              next_subj = doc["Giorni"][day][M6].as<String>();
            }

            min_rimanenti = 30 - act_min;
          } else {
            // sesto modulo
            act_subj = doc["Giorni"][day][M6].as<String>();

            min_rimanenti = 20 + (60 - act_min);
          }
          break;
          
        case 13:
          if (act_min < 20) {
            act_subj = doc["Giorni"][day][M6].as<String>();

            min_rimanenti = 20 - act_min;

          }
          break;

        default:
          break;
      }

      // giorni con rientro
      if (isReentryDay()) {
        switch (getHour()) {
          case 13:
            if (act_min < 20) {
              act_subj = doc["Giorni"][day][M6].as<String>();
              next_subj = pranzo;
            } else {
              act_subj = pranzo;
              min_rimanenti = 20 + (60 - act_min);
            }
            
            break;
          case 14:
            if (act_min < 20) {
              act_subj = pranzo;

              // next subj
              if (act_min > 10) {
                next_subj = doc["Giorni"][day][M7].as<String>();
              }

              min_rimanenti = 20 - act_min;
            } else {
              act_subj = doc["Giorni"][day][M7].as<String>();

              min_rimanenti = 10 + (60 - act_min);
            }
            break;
          case 15:
            if (act_min < 10) {
              act_subj = doc["Giorni"][day][M7].as<String>();
              next_subj = doc["Giorni"][day][M8].as<String>();

              min_rimanenti = 10 - act_min;
            } else if (act_min >= 55) {
              act_subj = doc["Giorni"][day][M8].as<String>();
              next_subj = intervallo;

              min_rimanenti = 60 - act_min;
            } else {
              act_subj = doc["Giorni"][day][M8].as<String>();

              min_rimanenti = 60 - act_min;
            }
            break;
          case 16:
            if (act_min < 5) {
              act_subj = intervallo;
              next_subj = doc["Giorni"][day][M9].as<String>();

              min_rimanenti = 5 - act_min;
            } else if (act_min < 55) {
              act_subj = doc["Giorni"][day][M9].as<String>();

              min_rimanenti = 55 - act_min;
            }
            break;
          default:
            break;
        }

        if (doc["Classe"].as<int>() == 2 && doc["GiornoRientro"].as<String>() == day) {
          switch (getHour()) {
            case 16:
              if (act_min < 55) {
                act_subj = doc["Giorni"][day][M9].as<String>();
                next_subj = doc["Giorni"][day][M10].as<String>();
              } else {
                act_subj = doc["Giorni"][day][M10].as<String>();

                min_rimanenti = 45 + (60 - act_min);
              }

              break;

            case 17:
              if (act_min < 45) {
                act_subj = doc["Giorni"][day][M10].as<String>();

                min_rimanenti = 45 - act_min;
              }
              break;

            default:
              break;
          }
        }
      }

      char* min_rim_prefix = "Minuti rimanenti: ";
      
      if (act_subj != "") {
        //Serial.println("test");
        //printXText(act_subj, TXT_FONT, 85);
        if (act_subj != last_subj)
          deleteXText(last_subj, TXT_FONT, 72);
        printXText(act_subj, TXT_FONT, 72);

        

        if (last_min_rim != min_rimanenti)
          deleteXText(min_rim_prefix + String(last_min_rim), TXT_FONT, 90);
        printXText(min_rim_prefix + String(min_rimanenti), TXT_FONT, 90);

        last_subj = act_subj;
        last_min_rim = min_rimanenti;
      } else {
        deleteXText(last_subj, TXT_FONT, 72);
        deleteXText(min_rim_prefix + String(last_min_rim), TXT_FONT, 90);
      }

      if (next_subj != "") {
        next_subj = "Next: " + next_subj;

        if (next_subj != last_next_subj)
          deleteXText(last_next_subj, TXT_FONT, 108);

        printXText(next_subj, TXT_FONT, 108);
        last_next_subj = next_subj;
      } else {
        deleteXText(last_next_subj, TXT_FONT, 108);
      }

    } // if

    last_partial_refresh = millis();
    wifi_forced_refresh = false;
    time_change = false;
  }

  // l'ora viene aggiornata appena cambia
  curr_time = getTime();
  if (curr_time != last_time || is_full_refresh) {
    printXText(getTime(), MAIN_FONT, 20);
    last_time = curr_time;
    time_change = true;
    //fillRect();
  }

  // gestione WiFi mode
  if (isrBtnChange() != 0) {
    left_btn_state = false;
    ap_web_server_init();
    ap_last_on = millis();
    ap_state = true;
    wifi_forced_refresh = true;
  }

  if (millis() - ap_last_on > AP_ON_TIMER && ap_state) {
    if (WiFi.softAPgetStationNum() < 1) {
        ap_web_server_off();
        //Serial.println("AP off");
        ap_state = false;
    }
    ap_last_on = millis();
  }

  if (ap_state) {
    dnsServer.processNextRequest();
  }

  // ALWAST THE LAST IN THE LOOP
  if (is_full_refresh) {
    is_full_refresh = false;
  }
}
