#define ENABLE_GxEPD2_GFX 1

#include <Arduino.h>
#include "logo.h"
#include <Wire.h>
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
//#include <Fonts/FreeMonoBold9pt7b.h>
#include <BMP180.h>
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
bool ap_state_set = false;

// Set web server port number to 80
AsyncWebServer server(80);

DNSServer dnsServer;
const uint8_t DNS_PORT = 53;

const char* PARAM_INPUT_1 = "date";
const char* PARAM_INPUT_2 = "time";
const char* PARAM_INPUT_3 = "giorno";
const char* PARAM_INPUT_4 = "anno";


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
                flex-direction: column;
                align-items: center;
                gap: 1rem;
                padding: 2rem 1rem;
                min-height: 100vh;
                font-family: sans-serif;
            }

            .container {
                background: #000;
                border-radius: 1rem;
                padding: 2rem;
                width: 100%;
                max-width: 420px;
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

            input[type="date"],
            input[type="time"],
            input[type="number"],
            input[type="text"] {
                width: 100%;
                padding: 0.6rem;
                margin-bottom: 1rem;
                border: none;
                border-radius: 0.4rem;
                font-size: 1rem;
                background: #4a4a4a;
                color: #e0e0e0;
            }

            button[type="submit"] {
                background: #5a5a5a;
                color: #e0e0e0;
                border: none;
                padding: 0.6rem 1.2rem;
                font-size: 1rem;
                border-radius: 0.4rem;
                cursor: pointer;
            }

            button[type="submit"]:hover {
                background: #6a6a6a;
            }
            
            .container img {
                display: block;
                margin: 0 auto 1.5rem;
                max-width: 8rem;
                height: auto;
            }

            /* ── Accordion ── */
            .accordion {
                width: 100%;
                max-width: 420px;
            }

            .accordion-item {
                background: #000;
                border-radius: 1rem;
                margin-bottom: 1rem;
                overflow: hidden;
            }

            .accordion-header {
                width: 100%;
                background: #1a1a1a;
                color: #e0e0e0;
                border: none;
                padding: 1rem 1.4rem;
                font-size: 1.05rem;
                font-weight: 600;
                cursor: pointer;
                display: flex;
                justify-content: space-between;
                align-items: center;
                border-radius: 1rem;
                transition: background 0.2s;
            }

            .accordion-header:hover {
                background: #2a2a2a;
            }

            .accordion-header.open {
                border-radius: 1rem 1rem 0 0;
                background: #222;
            }

            .accordion-arrow {
                transition: transform 0.25s;
                font-size: 0.85rem;
            }

            .accordion-header.open .accordion-arrow {
                transform: rotate(180deg);
            }

            .accordion-body {
                display: none;
                padding: 1.2rem 2rem 1.5rem;
            }

            .accordion-body.open {
                display: block;
            }

            /* ── Radio per modulo extra ── */
            .extra-module-radio {
                display: none;
                background: #1a1a1a;
                border-radius: 0.5rem;
                padding: 0.8rem 1rem;
                margin-bottom: 1rem;
                color: #e0e0e0;
                font-size: 0.9rem;
                text-align: left;
            }

            .extra-module-radio.visible {
                display: block;
            }

            .extra-module-radio p {
                margin-bottom: 0.5rem;
                font-weight: 600;
                color: #aaa;
                font-size: 0.85rem;
                text-transform: uppercase;
                letter-spacing: 0.05em;
            }

            .radio-group {
                display: flex;
                gap: 1.5rem;
            }

            .radio-group label {
                display: flex;
                align-items: center;
                gap: 0.4rem;
                margin: 0;
                cursor: pointer;
                color: #e0e0e0;
                font-size: 1rem;
            }

            .radio-group input[type="radio"] {
                width: auto;
                margin: 0;
                accent-color: #888;
            }

            /* modulo 10 nascosto di default */
            .module-extra {
                display: none;
            }
        </style>
    </head>
    <body>

        <!-- Data e ora -->
        <div class="container">
            <img src="logo" alt="">
            <form action="/get" method="GET">
                <label>Data:</label>
                <input type="date" min="2026-01-01" name="date" required>

                <label>Tempo:</label>
                <input type="time" name="time" required>

                <button type="submit">Salva</button>
            </form>
        </div>

        <!-- Classe -->
        <div class="container">
            <form action="/get" method="GET">
                <label>Classe:</label>
                <input type="number" id="classeInput" min="1" max="5" name="anno" required>

                <!-- Radio visibile solo per classe 2 -->
                <div class="extra-module-radio" id="extraRadio">
                    <p>Il 10° modulo è per:</p>
                    <div class="radio-group">
                        <label>
                            <input type="radio" name="giorno_extra" value="Lunedi"> Lunedì
                        </label>
                        <label>
                            <input type="radio" name="giorno_extra" value="Giovedi"> Giovedì
                        </label>
                    </div>
                </div>

                <button type="submit">Salva</button>
            </form>
        </div>

        <!-- Giorni in accordion -->
        <div class="accordion" id="accordionGiorni">

            <!-- Lunedì -->
            <div class="accordion-item">
                <button type="button" class="accordion-header" data-target="lunedi">
                    Lunedì <span class="accordion-arrow">▼</span>
                </button>
                <div class="accordion-body" id="lunedi">
                    <form action="/get" method="GET">
                        <input type="hidden" name="giorno" value="Lunedi">
                        <input type="text" name="M1" placeholder="Modulo 1">
                        <input type="text" name="M2" placeholder="Modulo 2">
                        <input type="text" name="M3" placeholder="Modulo 3">
                        <input type="text" name="M4" placeholder="Modulo 4">
                        <input type="text" name="M5" placeholder="Modulo 5">
                        <input type="text" name="M6" placeholder="Modulo 6">
                        <input type="text" name="M7" placeholder="Modulo 7">
                        <input type="text" name="M8" placeholder="Modulo 8">
                        <input type="text" name="M9" placeholder="Modulo 9">
                        <input type="text" name="M10" placeholder="Modulo 10" class="module-extra" id="lunedi-M10">
                        <button type="submit">Salva</button>
                    </form>
                </div>
            </div>

            <!-- Martedì -->
            <div class="accordion-item">
                <button type="button" class="accordion-header" data-target="martedi">
                    Martedì <span class="accordion-arrow">▼</span>
                </button>
                <div class="accordion-body" id="martedi">
                    <form action="/get" method="GET">
                        <input type="hidden" name="giorno" value="Martedi">
                        <input type="text" name="M1" placeholder="Modulo 1">
                        <input type="text" name="M2" placeholder="Modulo 2">
                        <input type="text" name="M3" placeholder="Modulo 3">
                        <input type="text" name="M4" placeholder="Modulo 4">
                        <input type="text" name="M5" placeholder="Modulo 5">
                        <input type="text" name="M6" placeholder="Modulo 6">
                        <input type="text" name="M7" placeholder="Modulo 7">
                        <input type="text" name="M8" placeholder="Modulo 8">
                        <input type="text" name="M9" placeholder="Modulo 9">
                        <button type="submit">Salva</button>
                    </form>
                </div>
            </div>

            <!-- Mercoledì -->
            <div class="accordion-item">
                <button type="button" class="accordion-header" data-target="mercoledi">
                    Mercoledì <span class="accordion-arrow">▼</span>
                </button>
                <div class="accordion-body" id="mercoledi">
                    <form action="/get" method="GET">
                        <input type="hidden" name="giorno" value="Mercoledi">
                        <input type="text" name="M1" placeholder="Modulo 1">
                        <input type="text" name="M2" placeholder="Modulo 2">
                        <input type="text" name="M3" placeholder="Modulo 3">
                        <input type="text" name="M4" placeholder="Modulo 4">
                        <input type="text" name="M5" placeholder="Modulo 5">
                        <input type="text" name="M6" placeholder="Modulo 6">
                        <input type="text" name="M7" placeholder="Modulo 7">
                        <input type="text" name="M8" placeholder="Modulo 8">
                        <input type="text" name="M9" placeholder="Modulo 9">
                        <button type="submit">Salva</button>
                    </form>
                </div>
            </div>

            <!-- Giovedì -->
            <div class="accordion-item">
                <button type="button" class="accordion-header" data-target="giovedi">
                    Giovedì <span class="accordion-arrow">▼</span>
                </button>
                <div class="accordion-body" id="giovedi">
                    <form action="/get" method="GET">
                        <input type="hidden" name="giorno" value="Giovedi">
                        <input type="text" name="M1" placeholder="Modulo 1">
                        <input type="text" name="M2" placeholder="Modulo 2">
                        <input type="text" name="M3" placeholder="Modulo 3">
                        <input type="text" name="M4" placeholder="Modulo 4">
                        <input type="text" name="M5" placeholder="Modulo 5">
                        <input type="text" name="M6" placeholder="Modulo 6">
                        <input type="text" name="M7" placeholder="Modulo 7">
                        <input type="text" name="M8" placeholder="Modulo 8">
                        <input type="text" name="M9" placeholder="Modulo 9">
                        <input type="text" name="M10" placeholder="Modulo 10" class="module-extra" id="giovedi-M10">
                        <button type="submit">Salva</button>
                    </form>
                </div>
            </div>

            <!-- Venerdì -->
            <div class="accordion-item">
                <button type="button" class="accordion-header" data-target="venerdi">
                    Venerdì <span class="accordion-arrow">▼</span>
                </button>
                <div class="accordion-body" id="venerdi">
                    <form action="/get" method="GET">
                        <input type="hidden" name="giorno" value="Venerdi">
                        <input type="text" name="M1" placeholder="Modulo 1">
                        <input type="text" name="M2" placeholder="Modulo 2">
                        <input type="text" name="M3" placeholder="Modulo 3">
                        <input type="text" name="M4" placeholder="Modulo 4">
                        <input type="text" name="M5" placeholder="Modulo 5">
                        <input type="text" name="M6" placeholder="Modulo 6">
                        <input type="text" name="M7" placeholder="Modulo 7">
                        <input type="text" name="M8" placeholder="Modulo 8">
                        <input type="text" name="M9" placeholder="Modulo 9">
                        <button type="submit">Salva</button>
                    </form>
                </div>
            </div>

        </div><!-- /accordion -->

        <script>
            /* ── Accordion: un solo giorno aperto alla volta ── */
            document.querySelectorAll('.accordion-header').forEach(btn => {
                btn.addEventListener('click', () => {
                    const targetId = btn.dataset.target;
                    const targetBody = document.getElementById(targetId);
                    const isOpen = targetBody.classList.contains('open');

                    // Chiudi tutto
                    document.querySelectorAll('.accordion-body').forEach(b => b.classList.remove('open'));
                    document.querySelectorAll('.accordion-header').forEach(b => b.classList.remove('open'));

                    // Apri quello cliccato solo se era chiuso
                    if (!isOpen) {
                        targetBody.classList.add('open');
                        btn.classList.add('open');
                    }
                });
            });

            /* ── Classe 2: radio + modulo 10 ── */
            const classeInput = document.getElementById('classeInput');
            const extraRadio  = document.getElementById('extraRadio');
            const lunediM10   = document.getElementById('lunedi-M10');
            const giovediM10  = document.getElementById('giovedi-M10');
            const radios      = document.querySelectorAll('input[name="giorno_extra"]');

            function aggiornaMod10() {
                const classe = parseInt(classeInput.value);
                const isSeconda = classe === 2;

                // Mostra/nascondi il radio
                extraRadio.classList.toggle('visible', isSeconda);

                // Nascondi entrambi i M10 di default
                lunediM10.style.display  = 'none';
                giovediM10.style.display = 'none';

                if (isSeconda) {
                    // Mostra il M10 del giorno selezionato dal radio
                    const selezionato = document.querySelector('input[name="giorno_extra"]:checked').value;
                    if (selezionato === 'Lunedi')  lunediM10.style.display  = 'block';
                    if (selezionato === 'Giovedi') giovediM10.style.display = 'block';
                }
            }

            classeInput.addEventListener('input', aggiornaMod10);
            radios.forEach(r => r.addEventListener('change', aggiornaMod10));

            // Stato iniziale
            aggiornaMod10();
        </script>
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

String last_date = "";


/*==============================
              BMP280
==============================*/


BMP180 bmp(BMP180_ULTRAHIGHRES);
//float last_temp = 0;
//float curr_temp = 0;
String last_temp = "";


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

uint8_t last_batt = 0;


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

String last_subj = "";
String last_next_subj = "";
uint8_t last_min_rim = 0;


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
  if (!bmp.begin()) {
    Serial.println(F("Bosch BMP180/BMP085 is not connected or fail to read calibration coefficients"));
    while (1) delay(10);
  }
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

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void ap_web_server_init() {
  if (ap_state) return; // evita reinizializzazioni multiple

  WiFi.mode(WIFI_AP);
  
  // Config PRIMA di softAP
  WiFi.softAPConfig(IP, IP, NMASK);
  
  if (!WiFi.softAP(SSID, PASSWD)) {
    Serial.println("softAP FAILED");
    return;
  }

  delay(100);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());



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
    }/*
    else {
      Serial.println(inputMessage);
      request->send(200, "text/html", "HTTP GET request sent to your ESP on input (" 
                                     + inputDate + ", " + inputTime + ") with values: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
    }*/

    else if (request->hasParam(PARAM_INPUT_4)) {
      uint8_t anno = (request->getParam(PARAM_INPUT_3)->value()).toInt();
      if (anno > 0 && anno < 6) {
        doc["Classe"] = anno;
        saveConfiguration("/orario.json");
      }

    }
    else if (request->hasParam(PARAM_INPUT_3)) {
      Serial.println("Ricevuto");
      int8_t i;
      for (i = 0; i < 7; i++) {
        if (request->getParam(PARAM_INPUT_3)->value() == daysOfTheWeek[i])
          break;
        
        if (i == 7) {
          i = -1;
          break;
        }
      }

      Serial.print("Day: ");
      Serial.println(i);


      if (i >= 0) {
        uint8_t n_moduli;
        bool mod_status = true;
        switch (i) {
          case 1:
            // lunedì;
            // controllo che abbia tutti i parametri M1...M9/M10
            n_moduli = 9;

            for (uint8_t j = 0; j < n_moduli; j++) {
              String mod_test = "M" + String((j+1));
              Serial.print("Checking ");
              Serial.println(mod_test.c_str());
              if (!request->hasParam(mod_test.c_str())) {
                mod_status = false;
                break;
              }
            }

            // controllo se c'è un decimo modulo
            if (request->hasParam("M10") && doc["Classe"] == 2)
              n_moduli = 10;

            if (mod_status) {
              Serial.println("All params OK.");
              // adesso modifico tutti i moduli di lunedì
              // doc["Giorni"]["Lunedi"][0] = "Italiano";

              for (uint8_t j = 0; j < n_moduli; j++) {
                doc["Giorni"][request->getParam(PARAM_INPUT_3)->value()][j] = request->getParam(("M" + String(j+1)).c_str())->value();
              }
              
              saveConfiguration("/orario.json");
            }
            break;

          case 2:
            // martedì
            n_moduli = 6;

            for (uint8_t j = 0; j < n_moduli; j++) {
              String mod_test = "M" + String((j+1));
              Serial.print("Checking ");
              Serial.println(mod_test.c_str());
              if (!request->hasParam(mod_test.c_str())) {
                mod_status = false;
                break;
              }
            }

            if (mod_status) {
              Serial.println("All params OK.");
              // adesso modifico tutti i moduli di lunedì
              // doc["Giorni"]["Lunedi"][0] = "Italiano";

              for (uint8_t j = 0; j < n_moduli; j++) {
                doc["Giorni"][request->getParam(PARAM_INPUT_3)->value()][j] = request->getParam(("M" + String(j+1)).c_str())->value();
              }
              
              saveConfiguration("/orario.json");
            }

            break;

          case 3:
            // mercoledì
            n_moduli = 6;

            for (uint8_t j = 0; j < n_moduli; j++) {
              String mod_test = "M" + String((j+1));
              Serial.print("Checking ");
              Serial.println(mod_test.c_str());
              if (!request->hasParam(mod_test.c_str())) {
                mod_status = false;
                break;
              }
            }

            if (mod_status) {
              Serial.println("All params OK.");
              // adesso modifico tutti i moduli di lunedì
              // doc["Giorni"]["Lunedi"][0] = "Italiano";

              for (uint8_t j = 0; j < n_moduli; j++) {
                doc["Giorni"][request->getParam(PARAM_INPUT_3)->value()][j] = request->getParam(("M" + String(j+1)).c_str())->value();
              }
              
              saveConfiguration("/orario.json");
            }

            break;

          case 4:
            // giovedì
            n_moduli = 9;

            for (uint8_t j = 0; j < n_moduli; j++) {
              String mod_test = "M" + String((j+1));
              Serial.print("Checking ");
              Serial.println(mod_test.c_str());
              if (!request->hasParam(mod_test.c_str())) {
                mod_status = false;
                break;
              }
            }

            // controllo se c'è un decimo modulo
            if (request->hasParam("M10") && doc["Classe"] == 2)
              n_moduli = 10;

            if (mod_status) {
              Serial.println("All params OK.");
              // adesso modifico tutti i moduli di lunedì
              // doc["Giorni"]["Lunedi"][0] = "Italiano";

              for (uint8_t j = 0; j < n_moduli; j++) {
                doc["Giorni"][request->getParam(PARAM_INPUT_3)->value()][j] = request->getParam(("M" + String(j+1)).c_str())->value();
              }
              
              saveConfiguration("/orario.json");
            }

            break;

          case 5:
            // venerdì
            n_moduli = 6;

            for (uint8_t j = 0; j < n_moduli; j++) {
              String mod_test = "M" + String((j+1));
              Serial.print("Checking ");
              Serial.println(mod_test.c_str());
              if (!request->hasParam(mod_test.c_str())) {
                mod_status = false;
                break;
              }
            }

            if (mod_status) {
              Serial.println("All params OK.");
              // adesso modifico tutti i moduli di lunedì
              // doc["Giorni"]["Lunedi"][0] = "Italiano";

              for (uint8_t j = 0; j < n_moduli; j++) {
                doc["Giorni"][request->getParam(PARAM_INPUT_3)->value()][j] = request->getParam(("M" + String(j+1)).c_str())->value();
              }
              
              saveConfiguration("/orario.json");
            }

            break;
        }
      }

      request->send(200, "text/html", "HTTP GET request sent to your ESP for MODULI");

    }
    else {
      request->send(200, "text/html", "HTTP GET request sent to your ESP. ERROR malformed.");
    }
    
    Serial.println("Access Point activated");
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

void printXTextFull(String text, const uint8_t* font, int16_t offset_y) {
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
  eink.setPartialWindow(0, offset_y, eink.width(), th);

  eink.firstPage();
  do {
    eink.fillRect(0, offset_y, eink.width(), th, WHITE);
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

  //doc["Giorni"]["Lunedi"][0] = "Telecomunicazioni";
}

void saveConfiguration(const char* filename) {
  // Delete existing file, otherwise the configuration is appended to the file
  LittleFS.remove(filename);

  // Open file for writing
  File file = LittleFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
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
  return String(bmp.getTemperature(), 1) + "°C";
  //return "22.4°C";
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


void printSchoolUpdates() {
  if(isSchoolDay()) {
    //Serial.println(daysOfTheWeek[now.dayOfTheWeek()]);
    String next_subj = "";
    String act_subj = "";
    char* intervallo = "Intervallo";
    char* pranzo = "Pausa pranzo";
    uint8_t act_min = getMinute();
    int8_t min_rimanenti = -1;
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
    
    // funziona ma si può fare meglio, più ottimizzato per lo sche

    if (act_subj != last_subj || is_full_refresh) {
      printXTextFull(act_subj, TXT_FONT, 72);

      last_subj = act_subj;
    }

    if (min_rimanenti !=  -1) {
      if (last_min_rim != min_rimanenti || is_full_refresh) {
        printXTextFull(min_rim_prefix + String(min_rimanenti), TXT_FONT, 90);
        last_min_rim = min_rimanenti;
      }
    } else if (last_min_rim != -1) {
      printXTextFull("", TXT_FONT, 90);
      last_min_rim = -1;
    }
    
    
    
    if (next_subj != last_next_subj || is_full_refresh) {
      if (next_subj != "")
        printXTextFull("Next: " + next_subj, TXT_FONT, 108);
      else
        printXTextFull("", TXT_FONT, 108);
      last_next_subj = next_subj;
    }

  } // if
}

void printBatteryState() {
  uint8_t act_batt = getBatteryTicks();

  if (act_batt != last_batt || is_full_refresh) {
    printText(String(act_batt), BATT_FONT, 4, 1, eink.width() - 19);
    last_batt = act_batt;
  }
}

void printDateWifi() {
  String act_date = getDate();

  if (!ap_state) {
    if (act_date != last_date || ap_state_set || is_full_refresh) {
      printXText(act_date, TXT_FONT, 0);
      last_date = act_date;
      ap_state_set = false;
    }

  }
  else if (!ap_state_set || is_full_refresh) {
    printXText("WiFi  mode", TXT_FONT, 0);
    //printXText("8", WIFI_FONT, 0);
    ap_state_set = true;
  }
}

void printTemperature() {
  String act_temp = getTemperature();

  if (last_temp != act_temp || is_full_refresh) {
    printTextRightAligned(act_temp, TXT_FONT, 0);
    last_temp = act_temp;
  }
}











void setup() {
  Serial.begin(9600);

  displayInit();

  rtcInit();

  bmpInit();

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

void loop() {
  if (((millis() - last_partial_refresh) > PARTIAL_REFRESH_RATE) || wifi_forced_refresh || time_change) {
    checkRefreshState();
    
    //printXText(getTime(), MAIN_FONT, 30);

    printBatteryState();
    
    printDateWifi();
    
    printTemperature();
    
    printSchoolUpdates();
    
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


  // ALWAST THE LAST IN THE LOOP
  if (is_full_refresh) {
    is_full_refresh = false;
  }
}