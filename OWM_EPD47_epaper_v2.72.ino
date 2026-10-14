#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif
#include <Arduino.h>            // In-built
#include <esp_task_wdt.h>       // In-built
#include "freertos/FreeRTOS.h"  // In-built
#include "freertos/task.h"      // In-built
#include "epd_driver.h"         // https://github.com/Xinyuan-LilyGO/LilyGo-EPD47
#include "esp_adc_cal.h"        // In-built

#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson
#include <HTTPClient.h>         // In-built

#include <WiFi.h>               // In-built
#include <SPI.h>                // In-built
#include <SD.h>
//#include <time.h>               // In-built
#include <RTClib.h>

#include "owm_credentials.h"
#include "forecast_record.h"
#include "lang.h"
#include "config.h"
#include "loginfo.h"
#include "driver/rtc_io.h"
#include "utilities.h"
#include <Wire.h>
#include <TouchDrvGT911.hpp>
//#include "driver/adc.h"
//#include "esp_adc_cal.h"

/*
Replace in the sdkconfig.h

From:
#define CONFIG_ARDUINO_LOOP_STACK_SIZE 8192

To:
#if defined(CONFIG_ARDUINO_LOOP_STACK_SIZE_OVERRIDE)
    #define CONFIG_ARDUINO_LOOP_STACK_SIZE CONFIG_ARDUINO_LOOP_STACK_SIZE_OVERRIDE
#else
    #define CONFIG_ARDUINO_LOOP_STACK_SIZE 8192
#endif
*/

#if CONFIG_ARDUINO_LOOP_STACK_SIZE <= 8192
#error "Please increase ARDUINO_LOOP_STACK_SIZE to 16384 !!! (.platformio\packages\framework-arduinoespressif32\tools\sdk\esp32\dio_qspi\include\sdkconfig.h)"
#endif

#define SCREEN_WIDTH   EPD_WIDTH
#define SCREEN_HEIGHT  EPD_HEIGHT
// GPIO47 (touch INT) is not an RTC pin. LilyGo documents tying it to GPIO10 so a tap can wake deep sleep.
#define TOUCH_WAKE_PIN GPIO_NUM_10
#define TOUCH_SESSION_MS 15000

#define BTN_UNITS_M        1
#define BTN_UNITS_I        2
#define BTN_SLEEP_DEC      3
#define BTN_SLEEP_INC      4
#define BTN_WAKE_DEC       5
#define BTN_WAKE_INC       6
#define BTN_SLEEPHOUR_DEC  7
#define BTN_SLEEPHOUR_INC  8
#define BTN_DBG_OFF        9
#define BTN_DBG_ON         10
#define BTN_SAVE           11
#define BTN_WEB            12

//################  VERSION  ##################################################
String version = "2.7 / 4.7in";  // Programme version, see change log at end
//################ VARIABLES ##################################################

//enum alignment {LEFT, RIGHT, CENTER};
#define White         0xFF
#define LightGrey     0xBB
#define Grey          0x88
#define DarkGrey      0x44
#define Black         0x00

#define autoscale_on  true
#define autoscale_off false
#define barchart_on   true
#define barchart_off  false

boolean LargeIcon   = true;
boolean SmallIcon   = false;
#define Large  20           // For icon drawing
#define Small  10           // For icon drawing
String  Time_str = "--:--:--";
String  Date_str = "-- --- ----";
int     wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0, EventCnt = 0, vref = 1100;
//################ PROGRAM VARIABLES and OBJECTS ##########################################
#define max_readings 24 // Limited to 3-days here, but could go to 5-days = 40 as the data is issued  

Forecast_record_type  WxConditions[1];
Forecast_record_type  WxForecast[max_readings];

float pressure_readings[max_readings]    = {0};
float temperature_readings[max_readings] = {0};
float humidity_readings[max_readings]    = {0};
float rain_readings[max_readings]        = {0};
float snow_readings[max_readings]        = {0};

//long SleepDuration   = 60; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
//int  WakeupHour      = 8;  // Wakeup after 07:00 to save battery power
//int  SleepHour       = 23; // Sleep  after 23:00 to save battery power
long StartTime       = 0;
long SleepTimer      = 0;
long Delta           = 30; // ESP32 rtc speed compensation, prevents display at xx:59:yy and then xx:00:yy (one minute later) to save power
RTC_DATA_ATTR int boot_count;

// Bottom touch menu selects these. The weather button still steps through them. Timed wake keeps the current one.
#define LAYOUT_FULL     0
#define LAYOUT_MAIN     1
#define LAYOUT_GRAPHS   2
#define LAYOUT_DETAILS  3
#define LAYOUT_DEVICE   4
#define LAYOUT_COUNT    5

RTC_DATA_ATTR uint8_t display_layout = LAYOUT_FULL;
RTC_DATA_ATTR bool display_layout_ready = false;
bool weatherReady = false;
bool menuVisible = false;
String deviceNotice;

const char* layoutName(uint8_t id) {
  switch (id) {
    case LAYOUT_MAIN:    return "Main";
    case LAYOUT_GRAPHS:  return "Graphs";
    case LAYOUT_DETAILS: return "Details";
    case LAYOUT_DEVICE:  return "Device";
    default:             return "Full";
  }
}

String layoutCaption() {
  return String(layoutName(display_layout)) + "  " + String(display_layout + 1) + "/" + String(LAYOUT_COUNT);
}

void loadDisplayLayout() {
  if (!display_layout_ready) {
    preferences.begin("layout", true);
    display_layout = preferences.getUChar("id", LAYOUT_FULL);
    preferences.end();
    display_layout_ready = true;
  }
  if (display_layout >= LAYOUT_COUNT) display_layout = LAYOUT_FULL;
}

void setDisplayLayout(uint8_t id) {
  loadDisplayLayout();
  if (id >= LAYOUT_COUNT) id = LAYOUT_FULL;
  display_layout = id;
  preferences.begin("layout", false);
  preferences.putUChar("id", display_layout);
  preferences.end();
  dbgPrintln("Display layout: " + layoutCaption());
}

void advanceDisplayLayout() {
  loadDisplayLayout();
  setDisplayLayout((display_layout + 1) % LAYOUT_COUNT);
}

//fonts
#include "fonts/opensans5cb_special2.h"
#include "fonts/opensans6b.h"
#include "fonts/opensans8b.h"
#include "fonts/opensans10b.h"
#include "fonts/opensans12b.h"
#include "fonts/opensans18b.h"
#include "fonts/opensans24b.h"
#include "imgs/moon.h"
#include "imgs/sunrise.h"
#include "imgs/sunset.h"
#include "imgs/uvi.h"
#include "imgs/Gauge0.h"
#include "imgs/Gauge1.h"
#include "imgs/Gauge2.h"
#include "imgs/Gauge3.h"
#include "imgs/Gauge4.h"
#include "imgs/AirTree.h"
#include "imgs/TempFL.h"
#include "imgs/TempHi.h"
#include "imgs/TempLo.h"

#include "imgs/imgcat.h"
#include "imgs/imglandscape.h"
#include "imgs/imgminion.h"

GFXfont  currentFont;
uint8_t *framebuffer;

extern LogInfo logInfo;

void run_operating_mode();
void DisplayAirQualitySection(int x, int y);
String dbgPrintln(String _str);
void display_failed_mode_and_sleep();
void deep_sleep_start();

void collectAndWriteLog(int mode, bool is_time_fetched, bool is_weather_fetched, bool is_aq_fetched)
{
    dbgPrintln("InfluxDBClient: collectAndWriteLog");

    logInfo.Mode = mode;
    logInfo.ConfigOk = settings.ConfigOk;
    logInfo.BootCount = boot_count;
    logInfo.UTCTimestamp = is_time_fetched? (datetime_request.response.dt - datetime_request.response.gmt_offset) : 0;
    //logInfo.BatteryPct = DrawBattery(0, 0, true);
    DrawBattery(0, 0, true);
    logInfo.TimeFetchOk = is_time_fetched;
    logInfo.WeatherFetchOk = is_weather_fetched;
    logInfo.AQIFetchOk = is_aq_fetched;

    writeLogInfo();
}

void enable_timed_sleep(int interval_minutes, bool _timeIsSet) {
    // sleep and wake up round minutes, ex every 15 mins
    // will wake up at 7:15, 7:30, 7:45 etc.
    dbgPrintln("enable_timed_sleep (MIN): " + String(interval_minutes));

    if (_timeIsSet == false)
    {
      int fail_cnt = failed_count(false);
      TimeSpan ts = TimeSpan((uint32_t)powf(fail_cnt, 3) * 60);
      
      infoPrintln("DateTime not set, wake up in " 
        + String(ts.days()) + " days "
        + String(ts.hours()) + " hours "
        + String(ts.minutes()) + " minutes (" + String(ts.totalseconds()) + ")");

      esp_sleep_enable_timer_wakeup(((uint64_t)ts.totalseconds())*1000000L);

      return;
    }

    DateTime curTime = DateTime(datetime_request.response.dt);
    DateTime newTime;
    char date_format[] = "YYYY.MM.DD:hh.mm.ss";
    char date_format2[] = "YYYY.MM.DD:hh.mm.ss";

    int idx = (curTime.minute()/interval_minutes) + 1;

    if (interval_minutes * idx >= 60)
    {
        newTime = curTime + TimeSpan(0, 0, 60 - curTime.minute(), 0); //add time to xx.00.00
    }
    else
    {        
        newTime = curTime + TimeSpan(0, 0, interval_minutes * idx - curTime.minute(), 0);
    }

    if (newTime.second() > 0)
    {
        // set time to xx.xx.15
        newTime = newTime - TimeSpan(0, 0, 0, newTime.second()); 
        newTime = newTime + TimeSpan(0, 0, 0, 15); // add 15 sec to not wakeUp before expected min
    }

    if ((settings.WakeupHour > 0 || settings.SleepHour > 0) && settings.WakeupHour < 24 && settings.SleepHour < 24) // sleep/wake hours must be in range 0-23
    {
        int addHrs = 0;
        int loopCnt = 0;
        for (int idx = settings.SleepHour; idx != settings.WakeupHour; idx++)
        {
            if (idx == 24)
            {
                idx = 0;
            }

            if (addHrs > 0 
            || (newTime.hour() == idx && idx == settings.SleepHour && newTime.minute() > 0) //for sleep hour start counting when minutes > 0
            || (newTime.hour() == idx && idx != settings.SleepHour ) ) 
            {                
                addHrs++;        
            } 

            loopCnt++;
            if (loopCnt > 25) //exit loop
            {
                addHrs = 0;
                break;
            }
        }

        if (addHrs > 0) 
        {
            newTime = newTime + TimeSpan(0, addHrs, 0, 0); // add sleep hours
            newTime = newTime - TimeSpan(0, 0, newTime.minute(), 0); // reset minutes to 0    
        }
    }
   
    TimeSpan ts = (newTime - curTime);
    int sleep_time_seconds = ts.totalseconds();
    
    infoPrintln("DateTime Curreent :" + String(curTime.toString(date_format)));
    infoPrintln("DateTime WakeUp at:" + String(newTime.toString(date_format2)));
    char buffer[150];
    sprintf(buffer, "%d hours, %d minutes and %d seconds (total sec: %d)\n", ts.hours(), ts.minutes(), ts.seconds(), sleep_time_seconds);
    infoPrintln("DateTime Sleep for " + String(buffer));

    //sleep_time_micro_sec = sleep_time_seconds;
    //sleep_time_micro_sec = sleep_time_micro_sec * 1000 * 1000;

    esp_sleep_enable_timer_wakeup((uint64_t)sleep_time_seconds*1000000L);
}

void BeginSleep(bool _timeIsSet) {
  epd_poweroff_all();

  if (_timeIsSet)
  {
    UpdateLocalTime();
  }

  enable_timed_sleep(settings.SleepDuration, _timeIsSet);

  deep_sleep_start();  // Sleep for e.g. 30 minutes
}

boolean SetupTime() {
  /*
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
  setenv("TZ", Timezone, 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset(); // Set the TZ environment variable
  */

  WiFiClientSecure client;

  datetime_request.api_key = settings.TimezBBKey;
  datetime_request.handler = datetime_handler;
  datetime_request.make_path(settings.Latitude, settings.Longitude);

  bool is_time_fetched = http_request_data(client, datetime_request);

  return is_time_fetched && UpdateLocalTime();
}

uint8_t StartWiFi() {
  dbgPrintln("Connecting to: " + settings.WiFiSSID);
  IPAddress dns(8, 8, 8, 8); // Use Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(settings.WiFiSSID.c_str(), settings.WiFiPass.c_str());
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    dbgPrintln("STA: Failed!");
    WiFi.disconnect(false);
    delay(500);
    WiFi.begin(settings.WiFiSSID.c_str(), settings.WiFiPass.c_str());
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    infoPrintln("WiFi connected at: " + WiFi.localIP().toString());
  }
  else infoPrintln("WiFi connection *** FAILED ***");
  return WiFi.status();
}

void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  dbgPrintln("WiFi switched Off");
}

bool sdReady = false;
bool sdWriting = false;
bool sdDatedLogReady = false;
String sdLogBuffer;

String sdDayStamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) return "";
  char buf[16];
  strftime(buf, sizeof(buf), "%Y-%m-%d", &timeinfo);
  return String(buf);
}

String sdIsoStamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) return "";
  char buf[24];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  return String(buf);
}

String sdLogPath() {
  String day = sdDayStamp();
  if (day.length()) return "/log/" + day + ".log";
  return "/log/pending.log";
}

void sdEnsureLogFile() {
  if (!sdReady) return;
  String path = sdLogPath();
  if (SD.exists(path)) return;
  File file = SD.open(path, FILE_WRITE);
  if (!file) return;
  file.println("--- " + path + " ---");
  file.close();
}

void sdFlushLog() {
  if (!sdReady || sdWriting || !sdLogBuffer.length()) return;
  sdWriting = true;
  sdEnsureLogFile();
  File file = SD.open(sdLogPath(), FILE_APPEND);
  if (file) {
    file.print(sdLogBuffer);
    file.close();
    sdLogBuffer = "";
  }
  sdWriting = false;
}

void sdCaptureLine(const String& line) {
  if (!sdReady || sdWriting) return;
  if (sdLogBuffer.length() > 16000) return;
  sdLogBuffer += line;
  if (sdLogBuffer.length() >= 2500) sdFlushLog();
}

void sdOnClockReady() {
  if (!sdReady) return;
  if (!sdDatedLogReady && sdDayStamp().length()) {
    sdDatedLogReady = true;
    String dayPath = sdLogPath();
    if (SD.exists("/log/pending.log")) {
      File in = SD.open("/log/pending.log", FILE_READ);
      File out = SD.open(dayPath, FILE_APPEND);
      if (!out) out = SD.open(dayPath, FILE_WRITE);
      if (in && out) {
        out.println("--- earlier boot log ---");
        while (in.available()) out.write(in.read());
      }
      if (in) in.close();
      if (out) out.close();
      SD.remove("/log/pending.log");
    }
  }
  sdEnsureLogFile();
  sdFlushLog();
}

void sdPrepareSleep() {
  sdFlushLog();
  if (!sdReady) return;
  SD.end();
  sdReady = false;
}

bool initSdCard() {
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 4000000)) {
    Serial.println("[D] SD card not found");
    Serial.flush();
    return false;
  }
  sdReady = true;
  SD.mkdir("/log");
  SD.mkdir("/weather");
  sdEnsureLogFile();
  dbgPrintln("SD card mounted, log " + sdLogPath());
  return true;
}

bool appendJsonArray(const String& path, const String& object) {
  if (!SD.exists(path)) {
    File file = SD.open(path, FILE_WRITE);
    if (!file) return false;
    file.print("[\n");
    file.print(object);
    file.print("\n]\n");
    file.close();
    return true;
  }
  File file = SD.open(path, "r+");
  if (!file) return false;
  size_t size = file.size();
  if (size < 3) {
    file.close();
    return false;
  }
  file.seek(size - 3);
  file.print(",\n");
  file.print(object);
  file.print("\n]\n");
  file.close();
  return true;
}

void logWeatherToSd() {
  if (!sdReady) return;
  sdOnClockReady();
  String day = sdDayStamp();
  if (!day.length()) {
    dbgPrintln("SD weather log skipped, clock is not set");
    return;
  }

  JsonDocument doc;
  doc["time"] = sdIsoStamp();
  doc["city"] = settings.City;
  doc["lat"] = settings.Latitude;
  doc["lon"] = settings.Longitude;
  doc["units"] = settings.Units;

  JsonObject current = doc["current"].to<JsonObject>();
  current["dt"] = WxConditions[0].Dt;
  current["temp"] = WxConditions[0].Temperature;
  current["feels_like"] = WxConditions[0].FeelsLike;
  current["humidity"] = WxConditions[0].Humidity;
  current["pressure"] = WxConditions[0].Pressure;
  current["wind_speed"] = WxConditions[0].Windspeed;
  current["wind_deg"] = WxConditions[0].Winddir;
  current["clouds"] = WxConditions[0].Cloudcover;
  current["visibility"] = WxConditions[0].Visibility;
  current["uvi"] = WxConditions[0].UVI;
  current["description"] = WxConditions[0].Forecast0;
  current["icon"] = WxConditions[0].Icon;
  current["sunrise"] = WxConditions[0].Sunrise;
  current["sunset"] = WxConditions[0].Sunset;
  current["high"] = WxConditions[0].High;
  current["low"] = WxConditions[0].Low;

  JsonObject air = doc["air"].to<JsonObject>();
  air["aqi"] = WxAirQ.AQI;
  air["co"] = WxAirQ.CO;
  air["no"] = WxAirQ.NO;
  air["no2"] = WxAirQ.NO2;
  air["o3"] = WxAirQ.O3;
  air["so2"] = WxAirQ.SO2;
  air["pm2_5"] = WxAirQ.PM2_5;
  air["pm10"] = WxAirQ.PM10;
  air["nh3"] = WxAirQ.NH3;

  JsonArray forecast = doc["forecast"].to<JsonArray>();
  for (int i = 0; i < max_readings; i++) {
    if (!WxForecast[i].Dt) continue;
    JsonObject slot = forecast.add<JsonObject>();
    slot["dt"] = WxForecast[i].Dt;
    slot["temp"] = WxForecast[i].Temperature;
    slot["temp_min"] = WxForecast[i].Low;
    slot["temp_max"] = WxForecast[i].High;
    slot["humidity"] = WxForecast[i].Humidity;
    slot["pressure"] = WxForecast[i].Pressure;
    slot["rain"] = WxForecast[i].Rainfall;
    slot["snow"] = WxForecast[i].Snowfall;
    slot["icon"] = WxForecast[i].Icon;
  }

  String body;
  serializeJson(doc, body);
  String path = "/weather/" + day + ".json";
  if (appendJsonArray(path, body)) dbgPrintln("SD weather saved " + path + " (" + String(body.length()) + " bytes)");
  else dbgPrintln("SD weather save failed " + path);
}

void InitialiseSystem() {
  StartTime = millis();
  Serial.begin(115200);
  Serial.setTxBufferSize(1024);
  uint32_t serialWait = millis();
  while (!Serial && millis() - serialWait < 3000) delay(10);
  delay(200);
  dbgPrintln(String(__FILE__) + " Starting...");
  dbgPrintln("USB CDC serial 115200, chip " + String(ESP.getChipModel())
             + " rev " + String(ESP.getChipRevision())
             + ", heap " + String(ESP.getFreeHeap())
             + ", psram " + String(ESP.getFreePsram()));
  epd_init();
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) dbgPrintln("Memory alloc failed!");
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
  initSdCard();
}

void loop() {
  // Nothing to do here
  if (dnsStarted)
  {
    dnsServer.processNextRequest();
  }
}

void run_imgdraw_mode()
{
    //#define WAKEUP_GPIO GPIO_NUM_39
    int img_idx;
    const uint8_t *img_data[3] = {ImgCat_data, ImgLandscape_data, ImgMinion_data};

    preferences.begin("img_key", false);
    img_idx = preferences.getInt("img_idx", 0); 
    preferences.putInt("img_idx", (img_idx + 1) > 2? 0 : img_idx + 1);    
    preferences.end();
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    dbgPrintln("Img_idx: " + String(img_idx));

    epd_poweron();      // Switch on EPD display
    epd_clear();        // Clear the screen

    epd_copy_to_framebuffer(epd_full_screen(), (uint8_t *) img_data[img_idx], framebuffer);
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);

    //edp_update();       // Update the display to show the information
    epd_poweroff();
    epd_poweroff_all(); // Switch off all power to EPD

    deep_sleep_start();
}

void deep_sleep_start()
{
  gpio_set_direction(GPIO_NUM_46, GPIO_MODE_INPUT);

  gpio_set_direction(BUTTON_0, GPIO_MODE_INPUT);
  esp_sleep_enable_ext0_wakeup(BUTTON_0, 0); //wakeup for image draw
  rtc_gpio_pulldown_dis(BUTTON_0);
  rtc_gpio_pullup_en(BUTTON_0); 

  // Leave the touch interrupt as an input. GPIO10 wakes deep sleep when it is tied to GPIO47.
  pinMode(TOUCH_INT, INPUT);
  uint64_t wakeMask = (1ULL << BUTTON_1) | (1ULL << TOUCH_WAKE_PIN);
  esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_LOW);
  rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_1);
  rtc_gpio_pullup_en((gpio_num_t)BUTTON_1);
  rtc_gpio_init(TOUCH_WAKE_PIN);
  rtc_gpio_set_direction(TOUCH_WAKE_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis(TOUCH_WAKE_PIN);
  rtc_gpio_pullup_en(TOUCH_WAKE_PIN);

  dbgPrintln(String("Deep sleep. Wake on timer, GPIO0, GPIO21, or GPIO10 if tied to touch INT GPIO47.")
             + " INT47=" + String(digitalRead(TOUCH_INT))
             + " GPIO10=" + String(gpio_get_level(TOUCH_WAKE_PIN))
             + " layout=" + layoutCaption());
  sdPrepareSleep();
  esp_deep_sleep_start();
}

void setup() {

  InitialiseSystem();

  boot_count++;

  dbgPrintln("\n\n=== WEATHER STATION ===\n\n");

  print_pt();

  uint8_t wakeup = wakeup_reason();
  read_config_from_memory();
  loadDisplayLayout();
  dbgPrintln("Boot " + String(boot_count)
             + ", layout " + layoutCaption()
             + ", wakeup " + String(wakeup)
             + ", ext1 mask 0x" + String((uint32_t)esp_sleep_get_ext1_wakeup_status(), HEX));

  if (!settings.ConfigOk)
  {
      dbgPrintln("MODE: Config (settings config NOK)");      
      run_config_server();
  } 
  else
  {
    switch (wakeup)
    {
      case ESP_SLEEP_WAKEUP_EXT0: // boot button: image pages
        dbgPrintln("MODE: Draw image");
        run_imgdraw_mode();
        break;

      case ESP_SLEEP_WAKEUP_EXT1: { // weather button, or touch INT via GPIO10
        uint64_t pins = esp_sleep_get_ext1_wakeup_status();
        dbgPrintln("EXT1 pins 0x" + String((uint32_t)pins, HEX)
                   + (pins & (1ULL << TOUCH_WAKE_PIN) ? " touch" : "")
                   + (pins & (1ULL << BUTTON_1) ? " button" : ""));
        if (pins & (1ULL << TOUCH_WAKE_PIN)) {
          dbgPrintln("MODE: Touch wake");
          handleWakeTouch();
        } else {
          advanceDisplayLayout();
        }
        dbgPrintln("MODE: Operating");
        run_operating_mode();
        break;
      }

      case ESP_SLEEP_WAKEUP_TIMER:
      case ESP_WAKEUP_SWCPU_RESET: //after validate
        dbgPrintln("MODE: Operating");
        run_operating_mode();
        break;

      default:
        dbgPrintln("MODE: Config");      
        run_config_server();
        break;

    }
  }
}

void run_operating_mode() {
    if (display_layout == LAYOUT_DEVICE) {
      paintScreen();
      serviceTouchThenSleep();
      return;
    }

    if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
      request_render_weather(false);
      failed_count(true);
      if (weatherReady) serviceTouchThenSleep();
      else BeginSleep(true);
    }
    else
    {
      display_failed_mode_and_sleep();
    }

    BeginSleep(false);
}

void request_render_weather(bool _clearDisplay)
{
  byte Attempts = 1;
  bool RxWeather  = false;
  bool RxForecast = false;
  bool RxAqidata = false;

  infoPrintln("Request and render weather");

  if (_clearDisplay)
  {
    dbgPrintln("Clear Display");
    epd_poweron();      // Switch on EPD display
    epd_clear();        // Clear the screen 
    epd_poweroff_all();
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
  }    

  WiFiClientSecure client;   // wifi client object
  while ((RxWeather == false || RxForecast == false) && Attempts <= 2) { // Try up-to 2 time for Weather and Forecast data
    if (RxWeather  == false) RxWeather  = obtainWeatherData(client, "onecall");
    if (RxForecast == false) RxForecast = obtainWeatherData(client, "forecast");
    if (RxAqidata == false) RxAqidata = obtainWeatherData(client, "air_pollution");
    Attempts++;
  }

  collectAndWriteLog(OPERATING_MODE, true, RxWeather, RxAqidata);
  StopWiFi();         // Reduces power consumption

  dbgPrintln("Received all weather data...");
  if (RxWeather && RxForecast && RxAqidata) { // Only if received both Weather or Forecast proceed
    weatherReady = true;
    logWeatherToSd();
    paintScreen();
  }
}

void Convert_Readings_to_Imperial() { // Only the first 3-hours are used
  WxConditions[0].Pressure = hPa_to_inHg(WxConditions[0].Pressure);
  WxForecast[0].Rainfall   = mm_to_inches(WxForecast[0].Rainfall);
  WxForecast[0].Snowfall   = mm_to_inches(WxForecast[0].Snowfall);
}

bool DecodeWeather(WiFiClient& json, String Type) {
  dbgPrintln(F("Creating object...and "));
  JsonDocument doc; //(64 * 1024);                      // allocate the JsonDocument
  DeserializationError error = deserializeJson(doc, json); // Deserialize the JSON document
  if (error) {                                             // Test if parsing succeeds.
    infoPrintln("DeserializeJson() failed: " + String(error.c_str()));
    return false;
  }
  else
  {
    infoPrintln(F("DeserializeJson() OK"));
  }
  // convert it to a JsonObject
  JsonObject root = doc.as<JsonObject>();
  dbgPrintln("Decoding " + Type + " data");
  if (Type == "onecall") {
    // All Serial.println statements are for diagnostic purposes and some are not required, remove if not needed with //
    WxConditions[0].High        = -50; // Minimum forecast low
    WxConditions[0].Low         = 50;  // Maximum Forecast High
    WxConditions[0].FTimezone   = doc["timezone_offset"]; // "0"
    JsonObject current = doc["current"];
    WxConditions[0].Sunrise     = current["sunrise"];                              dbgPrintln("SRis: " + String(WxConditions[0].Sunrise));
    WxConditions[0].Sunset      = current["sunset"];                               dbgPrintln("SSet: " + String(WxConditions[0].Sunset));
    WxConditions[0].Temperature = current["temp"];                                 dbgPrintln("Temp: " + String(WxConditions[0].Temperature));
    WxConditions[0].FeelsLike   = current["feels_like"];                           dbgPrintln("FLik: " + String(WxConditions[0].FeelsLike));
    WxConditions[0].Pressure    = current["pressure"];                             dbgPrintln("Pres: " + String(WxConditions[0].Pressure));
    WxConditions[0].Humidity    = current["humidity"];                             dbgPrintln("Humi: " + String(WxConditions[0].Humidity));
    WxConditions[0].DewPoint    = current["dew_point"];                            dbgPrintln("DPoi: " + String(WxConditions[0].DewPoint));
    WxConditions[0].UVI         = current["uvi"];                                  dbgPrintln("UVin: " + String(WxConditions[0].UVI));
    WxConditions[0].Cloudcover  = current["clouds"];                               dbgPrintln("CCov: " + String(WxConditions[0].Cloudcover));
    WxConditions[0].Visibility  = current["visibility"];                           dbgPrintln("Visi: " + String(WxConditions[0].Visibility));
    WxConditions[0].Windspeed   = current["wind_speed"];                           dbgPrintln("WSpd: " + String(WxConditions[0].Windspeed));
    WxConditions[0].Winddir     = current["wind_deg"];                             dbgPrintln("WDir: " + String(WxConditions[0].Winddir));
    JsonObject current_weather  = current["weather"][0];
    String Description = current_weather["description"];                           // "scattered clouds"
    String Icon        = current_weather["icon"];                                  // "01n"
    WxConditions[0].Forecast0   = Description;                                     dbgPrintln("Fore: " + String(WxConditions[0].Forecast0));
    WxConditions[0].Icon        = Icon;                                            dbgPrintln("Icon: " + String(WxConditions[0].Icon));
  }  
  if (Type == "forecast") {
    //Serial.println(json);
    dbgPrintln(F("Receiving Forecast period - ")); //------------------------------------------------
    JsonArray list                    = root["list"];
    for (byte r = 0; r < max_readings; r++) {
      dbgPrintln("Period-" + String(r) + "--------------");
      WxForecast[r].Dt                = list[r]["dt"].as<int>();
      WxForecast[r].Temperature       = list[r]["main"]["temp"].as<float>();       dbgPrintln("Temp: " + String(WxForecast[r].Temperature));
      WxForecast[r].Low               = list[r]["main"]["temp_min"].as<float>();   dbgPrintln("TLow: " + String(WxForecast[r].Low));
      WxForecast[r].High              = list[r]["main"]["temp_max"].as<float>();   dbgPrintln("THig: " + String(WxForecast[r].High));
      WxForecast[r].Pressure          = list[r]["main"]["pressure"].as<float>();   dbgPrintln("Pres: " + String(WxForecast[r].Pressure));
      WxForecast[r].Humidity          = list[r]["main"]["humidity"].as<float>();   dbgPrintln("Humi: " + String(WxForecast[r].Humidity));
      WxForecast[r].Icon              = list[r]["weather"][0]["icon"].as<const char*>(); dbgPrintln("Icon: " + String(WxForecast[r].Icon));
      WxForecast[r].Rainfall          = list[r]["rain"]["3h"].as<float>();         dbgPrintln("Rain: " + String(WxForecast[r].Rainfall));
      WxForecast[r].Snowfall          = list[r]["snow"]["3h"].as<float>();         dbgPrintln("Snow: " + String(WxForecast[r].Snowfall));
      if (r < 8) { // Check next 3 x 8 Hours = 1 day
        if (WxForecast[r].High > WxConditions[0].High) WxConditions[0].High = WxForecast[r].High; // Get Highest temperature for next 24Hrs
        if (WxForecast[r].Low  < WxConditions[0].Low)  WxConditions[0].Low  = WxForecast[r].Low;  // Get Lowest  temperature for next 24Hrs
      }
    }
    //------------------------------------------
    float pressure_trend = WxForecast[0].Pressure - WxForecast[2].Pressure; // Measure pressure slope between ~now and later
    pressure_trend = ((int)(pressure_trend * 10)) / 10.0; // Remove any small variations less than 0.1
    WxConditions[0].Trend = "=";
    if (pressure_trend > 0)  WxConditions[0].Trend = "+";
    if (pressure_trend < 0)  WxConditions[0].Trend = "-";
    if (pressure_trend == 0) WxConditions[0].Trend = "0";

    if (settings.Units == "I") Convert_Readings_to_Imperial();
  }
  if (Type == "air_pollution") {

        //auto& obj = WxAirQ;

      JsonArray list = root["list"];

      auto en = list[0];

      WxAirQ.Dt  = en["dt"];
      WxAirQ.AQI = en["main"]["aqi"];

      auto c = en["components"].as<JsonObject>();

      WxAirQ.CO    = c["co"];
      WxAirQ.NO    = c["no"];
      WxAirQ.NO2   = c["no2"];
      WxAirQ.O3    = c["o3"];
      WxAirQ.SO2   = c["so2"];
      WxAirQ.PM2_5 = c["pm2_5"];
      WxAirQ.PM10  = c["pm10"];
      WxAirQ.NH3   = c["nh3"];
  }
  return true;
}
//#########################################################################################
String ConvertUnixTime(int unix_time) {
  // Returns either '21:12  ' or ' 09:12pm' depending on Units mode
  time_t tm = unix_time;
  struct tm *now_tm = localtime(&tm);
  char output[40];
  if (settings.Units == "M") {
    strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
  }
  else {
    strftime(output, sizeof(output), "%I:%M%P %m/%d/%y", now_tm);
  }
  return output;
}
//#########################################################################################
bool obtainWeatherData(WiFiClientSecure & client, const String & RequestType) {
  const String units = (settings.Units == "M" ? "metric" : "imperial");
  client.stop(); // close connection before sending a new request
  HTTPClient http;  
  //api.openweathermap.org/data/2.5/RequestType?lat={lat}&lon={lon}&appid={API key}
  String apiVer = RequestType == "onecall"? "/data/3.0/":"/data/2.5/";
  String uri = apiVer + RequestType + "?lat=" + settings.Latitude + "&lon=" + settings.Longitude + "&appid=" + settings.OwmApikey + "&mode=json&units=" + units + "&lang=" + Language;
  if (RequestType == "onecall") uri += "&exclude=minutely,hourly,alerts,daily";
  client.setCACert(ROOT_CA_OWM);
  infoPrintln("HTTP connecting to api.openweathermap.org" + uri );
  http.begin(client, "api.openweathermap.org", 443, uri); //http.begin(uri,test_root_ca); //HTTPS example connection
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    infoPrintln("Connection OK");
    if (!DecodeWeather(http.getStream(), RequestType)) return false;
    client.stop();
  }
  else
  {
    infoPrintln("Connection failed, error: %" + http.errorToString(httpCode));
    client.stop();
    http.end();
    return false;
  }
  http.end();
  return true;
}

float mm_to_inches(float value_mm) {
  return 0.0393701 * value_mm;
}

float hPa_to_inHg(float value_hPa) {
  return 0.02953 * value_hPa;
}

int JulianDate(int d, int m, int y) {
  int mm, yy, k1, k2, k3, j;
  yy = y - (int)((12 - m) / 10);
  mm = m + 9;
  if (mm >= 12) mm = mm - 12;
  k1 = (int)(365.25 * (yy + 4712));
  k2 = (int)(30.6001 * mm + 0.5);
  k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
  // 'j' for dates in Julian calendar:
  j = k1 + k2 + d + 59 + 1;
  if (j > 2299160) j = j - k3; // 'j' is the Julian date at 12h UT (Universal Time) For Gregorian calendar:
  return j;
}

float SumOfPrecip(float DataArray[], int readings) {
  float sum = 0;
  for (int i = 0; i <= readings; i++) sum += DataArray[i];
  return sum;
}

String TitleCase(String text) {
  if (text.length() > 0) {
    String temp_text = text.substring(0, 1);
    temp_text.toUpperCase();
    return temp_text + text.substring(1); // Title-case the string
  }
  else return text;
}

TouchDrvGT911 panelTouch;
bool touchReady = false;
uint32_t nextTouchPoll = 0;

struct UiButton {
  int x;
  int y;
  int w;
  int h;
  int id;
};

UiButton deviceButtons[16];
int deviceButtonCount = 0;

bool beginTouch() {
  if (touchReady) return true;
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED) delay(1000);

  Wire.begin(BOARD_SDA, BOARD_SCL);
  pinMode(TOUCH_INT, OUTPUT);
  digitalWrite(TOUCH_INT, HIGH);
  delay(10);

  uint8_t touchAddress = 0;
  Wire.beginTransmission(0x14);
  if (Wire.endTransmission() == 0) touchAddress = 0x14;
  Wire.beginTransmission(0x5D);
  if (Wire.endTransmission() == 0) touchAddress = 0x5D;
  if (touchAddress == 0) {
    dbgPrintln("Touch: GT911 not found on 0x14 or 0x5D (SDA " + String(BOARD_SDA)
               + " SCL " + String(BOARD_SCL) + " INT " + String(TOUCH_INT) + ")");
    return false;
  }
  dbgPrintln("Touch: GT911 address 0x" + String(touchAddress, HEX));

  panelTouch.setPins(-1, TOUCH_INT);
  if (!panelTouch.begin(Wire, touchAddress, BOARD_SDA, BOARD_SCL)) {
    dbgPrintln("Touch: GT911 begin failed");
    return false;
  }
  panelTouch.setMaxCoordinates(EPD_WIDTH, EPD_HEIGHT);
  panelTouch.setSwapXY(true);
  panelTouch.setMirrorXY(false, true);
  panelTouch.setInterruptMode(FALLING);
  touchReady = true;
  dbgPrintln("Touch: GT911 ready, INT level " + String(digitalRead(TOUCH_INT)));
  return true;
}

bool pollTouch(int16_t& x, int16_t& y) {
  if (!touchReady) return false;
  if (millis() < nextTouchPoll) return false;
  nextTouchPoll = millis() + 300;
  return panelTouch.getPoint(&x, &y) > 0;
}

void waitTouchRelease() {
  uint32_t start = millis();
  while (millis() - start < 1500) {
    if (millis() < nextTouchPoll) {
      delay(20);
      continue;
    }
    nextTouchPoll = millis() + 300;
    int16_t x, y;
    if (!touchReady || panelTouch.getPoint(&x, &y) == 0) return;
  }
}

UiButton menuButtons[8];
int menuButtonCount = 0;

void addMenuButton(int x, int y, int w, int h, int id, const char* label) {
  if (menuButtonCount < 8) {
    UiButton& button = menuButtons[menuButtonCount++];
    button.x = x;
    button.y = y;
    button.w = w;
    button.h = h;
    button.id = id;
  }
  bool selected = (id == display_layout);
  drawRect(x, y, w, h, Black);
  if (selected) drawRect(x + 4, y + 4, w - 8, h - 8, Black);
  setFont(OpenSans18B);
  drawString(x + w / 2, y + 18, label, CENTER);
}

void drawOverlayMenu() {
  menuButtonCount = 0;
  const int panelX = 48;
  const int panelY = 145;
  const int panelW = SCREEN_WIDTH - 96;
  const int panelH = 250;
  fillRect(panelX, panelY, panelW, panelH, White);
  drawRect(panelX, panelY, panelW, panelH, Black);
  drawRect(panelX + 4, panelY + 4, panelW - 8, panelH - 8, Black);
  setFont(OpenSans18B);
  drawString(SCREEN_WIDTH / 2, panelY + 18, "Select screen", CENTER);

  const int bw = 250;
  const int bh = 70;
  const int gap = 24;
  const int row1 = 3;
  int rowW = row1 * bw + (row1 - 1) * gap;
  int x0 = (SCREEN_WIDTH - rowW) / 2;
  int y0 = panelY + 70;
  for (int i = 0; i < row1; i++) {
    addMenuButton(x0 + i * (bw + gap), y0, bw, bh, i, layoutName(i));
  }
  int row2 = LAYOUT_COUNT - row1;
  rowW = row2 * bw + (row2 - 1) * gap;
  x0 = (SCREEN_WIDTH - rowW) / 2;
  y0 += bh + 20;
  for (int i = 0; i < row2; i++) {
    addMenuButton(x0 + i * (bw + gap), y0, bw, bh, row1 + i, layoutName(row1 + i));
  }
}

int menuButtonAt(int x, int y) {
  for (int i = 0; i < menuButtonCount; i++) {
    UiButton& button = menuButtons[i];
    if (x >= button.x && x < button.x + button.w && y >= button.y && y < button.y + button.h) return button.id;
  }
  return -1;
}

void paintScreen() {
  dbgPrintln("Paint layout " + layoutCaption());
  epd_poweron();
  epd_clear();
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
  DisplayWeather();
  edp_update();
  epd_poweroff_all();
}

String keyStatus(const String& key) {
  if (!key.length()) return "empty";
  if (key.length() <= 4) return "set";
  return "set  ..." + key.substring(key.length() - 4);
}

String clipText(const String& text, int limit) {
  if ((int)text.length() <= limit) return text;
  return text.substring(0, limit - 1) + ".";
}

void settingChip(int x, int y, int w, int h, int id, bool selected, const char* label, bool draw) {
  if (deviceButtonCount < 16) {
    UiButton& button = deviceButtons[deviceButtonCount++];
    button.x = x;
    button.y = y;
    button.w = w;
    button.h = h;
    button.id = id;
  }
  if (!draw) return;
  drawRect(x, y, w, h, Black);
  if (selected) drawRect(x + 3, y + 3, w - 6, h - 6, Black);
  setFont(OpenSans12B);
  drawString(x + w / 2, y + 8, label, CENTER);
}

void layoutDeviceControls(bool draw) {
  deviceButtonCount = 0;
  const int x = 500;
  const int chipY = 6;
  int y = 78;
  if (draw) {
    setFont(OpenSans12B);
    drawString(x, y, "Units", LEFT);
  }
  settingChip(x + 150, y - chipY, 110, 36, BTN_UNITS_M, settings.Units == "M", "Metric", draw);
  settingChip(x + 270, y - chipY, 130, 36, BTN_UNITS_I, settings.Units != "M", "Imperial", draw);

  y += 58;
  if (draw) {
    setFont(OpenSans12B);
    drawString(x, y, "Sleep min", LEFT);
    drawString(x + 230, y, String(settings.SleepDuration), CENTER);
  }
  settingChip(x + 150, y - chipY, 52, 36, BTN_SLEEP_DEC, false, "-", draw);
  settingChip(x + 270, y - chipY, 52, 36, BTN_SLEEP_INC, false, "+", draw);

  y += 58;
  if (draw) {
    setFont(OpenSans12B);
    drawString(x, y, "Wake hour", LEFT);
    drawString(x + 230, y, String(settings.WakeupHour), CENTER);
  }
  settingChip(x + 150, y - chipY, 52, 36, BTN_WAKE_DEC, false, "-", draw);
  settingChip(x + 270, y - chipY, 52, 36, BTN_WAKE_INC, false, "+", draw);

  y += 58;
  if (draw) {
    setFont(OpenSans12B);
    drawString(x, y, "Sleep hour", LEFT);
    drawString(x + 230, y, String(settings.SleepHour), CENTER);
  }
  settingChip(x + 150, y - chipY, 52, 36, BTN_SLEEPHOUR_DEC, false, "-", draw);
  settingChip(x + 270, y - chipY, 52, 36, BTN_SLEEPHOUR_INC, false, "+", draw);

  y += 58;
  if (draw) {
    setFont(OpenSans12B);
    drawString(x, y, "Debug log", LEFT);
  }
  settingChip(x + 150, y - chipY, 80, 36, BTN_DBG_OFF, !settings.DbgLogEnable, "Off", draw);
  settingChip(x + 240, y - chipY, 80, 36, BTN_DBG_ON, settings.DbgLogEnable, "On", draw);

  y += 64;
  settingChip(x + 150, y - chipY, 110, 40, BTN_SAVE, false, "Save", draw);
  settingChip(x + 270, y - chipY, 150, 40, BTN_WEB, false, "Web setup", draw);

  if (draw && deviceNotice.length()) {
    setFont(OpenSans10B);
    drawString(x, y + 46, deviceNotice, LEFT);
  }
}

String formatSdBytes(uint64_t bytes) {
  const double gb = 1024.0 * 1024.0 * 1024.0;
  const double mb = 1024.0 * 1024.0;
  if (bytes >= (uint64_t)gb) return String((double)bytes / gb, 2) + " GB";
  if (bytes >= (uint64_t)mb) return String((double)bytes / mb, 1) + " MB";
  return String((double)bytes / 1024.0, 0) + " KB";
}

void DisplayDeviceScreen() {
  DrawBattery(0, 0, true);
  setFont(OpenSans18B);
  drawString(20, 8, "Device", LEFT);
  setFont(OpenSans10B);
  drawString(150, 16, "Touch the screen to open the menu", LEFT);

  setFont(OpenSans12B);
  int y = 56;
  drawString(20, y, "Version   " + version, LEFT); y += 28;
  if (!sdReady) {
    drawString(20, y, "SD card   not found", LEFT); y += 28;
  } else {
    uint64_t card = SD.cardSize();
    uint64_t total = SD.totalBytes();
    uint64_t used = SD.usedBytes();
    uint64_t freeBytes = total > used ? total - used : 0;
    int usedPct = total ? (int)((used * 100ULL) / total) : 0;
    drawString(20, y, "SD card   " + formatSdBytes(card), LEFT); y += 28;
    drawString(20, y, "SD space  " + String(usedPct) + "% used    " + formatSdBytes(freeBytes) + " free", LEFT); y += 28;
  }
  drawString(20, y, "Boots     " + String(boot_count), LEFT); y += 28;
  drawString(20, y, "Battery   " + String(logInfo.BatteryVoltage, 2) + " V    " + String(logInfo.BatteryPct) + "%", LEFT); y += 28;
  String wifiLine = settings.WiFiSSID.length() ? clipText(settings.WiFiSSID, 18) : "(none)";
  if (wifi_signal != 0) wifiLine += "   " + String(wifi_signal) + " dBm";
  drawString(20, y, "WiFi      " + wifiLine, LEFT); y += 28;
  drawString(20, y, "City      " + clipText(settings.City, 22), LEFT); y += 28;
  drawString(20, y, "Lat " + clipText(settings.Latitude, 10) + "   Lon " + clipText(settings.Longitude, 10), LEFT); y += 36;
  drawString(20, y, "OWM key        " + keyStatus(settings.OwmApikey), LEFT); y += 26;
  drawString(20, y, "PositionStack  " + keyStatus(settings.PositionStackKey), LEFT); y += 26;
  drawString(20, y, "TimeZoneDB     " + keyStatus(settings.TimezBBKey), LEFT); y += 26;
  String influx = logSettings.INFLUXDB_URL.length() ? clipText(logSettings.INFLUXDB_BUCKET, 14) : "off";
  if (logSettings.MTLS_CERT.length() && logSettings.MTLS_PKEY.length()) influx += "  mTLS";
  drawString(20, y, "Influx         " + influx, LEFT); y += 36;
  setFont(OpenSans10B);
  drawString(20, y, "City, WiFi and API keys", LEFT); y += 18;
  drawString(20, y, "are changed in Web setup.", LEFT);

  layoutDeviceControls(true);
}

int deviceButtonAt(int x, int y) {
  for (int i = 0; i < deviceButtonCount; i++) {
    UiButton& button = deviceButtons[i];
    if (x >= button.x && x < button.x + button.w && y >= button.y && y < button.y + button.h) return button.id;
  }
  return 0;
}

bool changeDeviceSetting(int id) {
  switch (id) {
    case BTN_UNITS_M:
      if (settings.Units != "M") { settings.Units = "M"; weatherReady = false; deviceNotice = "Save to keep units"; }
      return true;
    case BTN_UNITS_I:
      if (settings.Units != "I") { settings.Units = "I"; weatherReady = false; deviceNotice = "Save to keep units"; }
      return true;
    case BTN_SLEEP_DEC:
      if (settings.SleepDuration > 5) settings.SleepDuration -= 5;
      if (settings.SleepDuration < 5) settings.SleepDuration = 5;
      deviceNotice = "Save to keep sleep time";
      return true;
    case BTN_SLEEP_INC:
      if (settings.SleepDuration < 60) settings.SleepDuration += 5;
      if (settings.SleepDuration > 60) settings.SleepDuration = 60;
      deviceNotice = "Save to keep sleep time";
      return true;
    case BTN_WAKE_DEC:
      settings.WakeupHour = (settings.WakeupHour + 23) % 24;
      deviceNotice = "Save to keep hours";
      return true;
    case BTN_WAKE_INC:
      settings.WakeupHour = (settings.WakeupHour + 1) % 24;
      deviceNotice = "Save to keep hours";
      return true;
    case BTN_SLEEPHOUR_DEC:
      settings.SleepHour = (settings.SleepHour + 23) % 24;
      deviceNotice = "Save to keep hours";
      return true;
    case BTN_SLEEPHOUR_INC:
      settings.SleepHour = (settings.SleepHour + 1) % 24;
      deviceNotice = "Save to keep hours";
      return true;
    case BTN_DBG_OFF:
      settings.DbgLogEnable = false;
      deviceNotice = "Save to keep debug log";
      return true;
    case BTN_DBG_ON:
      settings.DbgLogEnable = true;
      deviceNotice = "Save to keep debug log";
      return true;
    case BTN_SAVE:
      save_config_to_memory();
      deviceNotice = "Saved";
      return true;
    case BTN_WEB:
      run_config_server();
      return false;
    default:
      return false;
  }
}

void showLayout(uint8_t id) {
  menuVisible = false;
  setDisplayLayout(id);
  if (id == LAYOUT_DEVICE || weatherReady) {
    paintScreen();
    return;
  }
  if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
    request_render_weather(false);
    failed_count(true);
    if (!weatherReady) display_failed_mode_and_sleep();
  } else {
    display_failed_mode_and_sleep();
  }
}

void handleScreenTouch(int x, int y) {
  dbgPrintln("Touch x=" + String(x) + " y=" + String(y) + (menuVisible ? " menu-open" : ""));
  if (menuVisible) {
    int id = menuButtonAt(x, y);
    menuVisible = false;
    if (id >= 0) {
      dbgPrintln("Menu select " + String(layoutName(id)));
      if ((uint8_t)id != display_layout) showLayout((uint8_t)id);
      else paintScreen();
    } else {
      dbgPrintln("Menu dismissed");
      paintScreen();
    }
    return;
  }
  if (display_layout == LAYOUT_DEVICE) {
    int button = deviceButtonAt(x, y);
    if (button) {
      dbgPrintln("Device button " + String(button));
      if (changeDeviceSetting(button)) paintScreen();
      return;
    }
  }
  menuVisible = true;
  dbgPrintln("Open layout menu");
  paintScreen();
}

void handleWakeTouch() {
  menuVisible = true;
  dbgPrintln("Wake touch: open layout menu");
}

void serviceTouchThenSleep() {
  uint32_t idleUntil = millis() + TOUCH_SESSION_MS;
  uint32_t nextHeartbeat = millis();
  dbgPrintln("Touch session " + String(TOUCH_SESSION_MS / 1000) + "s, layout " + layoutCaption());
  if (beginTouch()) {
    while ((int32_t)(idleUntil - millis()) > 0) {
      if ((int32_t)(millis() - nextHeartbeat) >= 0) {
        dbgPrintln("Touch listening, " + String((idleUntil - millis()) / 1000) + "s left");
        nextHeartbeat = millis() + 5000;
      }
      int16_t x, y;
      if (!pollTouch(x, y)) {
        delay(20);
        continue;
      }
      handleScreenTouch(x, y);
      waitTouchRelease();
      idleUntil = millis() + TOUCH_SESSION_MS;
    }
  } else {
    dbgPrintln("Touch session skipped, controller not found");
  }
  pinMode(TOUCH_INT, INPUT);
  BeginSleep(true);
}

void DisplayWeather() {
  switch (display_layout) {
    case LAYOUT_MAIN:    DisplayWeatherMain();    break;
    case LAYOUT_GRAPHS:  DisplayWeatherGraphs();  break;
    case LAYOUT_DETAILS: DisplayWeatherDetails(); break;
    case LAYOUT_DEVICE:  DisplayDeviceScreen();   break;
    default:             DisplayWeatherFull();    break;
  }
  if (menuVisible) drawOverlayMenu();
}

void DisplayWeatherFull() {                      // 4.7" e-paper display is 960x540 resolution
  DisplayStatusSection(525, 27, wifi_signal);    // Wi-Fi signal strength and Battery voltage
  DisplayGeneralInfoSection();                   // Top line of the display
  DisplayDisplayWindSection(137, 155, WxConditions[0].Winddir, WxConditions[0].Windspeed, 100);
  DisplayAstronomySection(5, 252);               // Astronomy section Sun rise/set, Moon phase and Moon icon
  DisplayMainWeatherSection(320, 110);           // Centre section of display for Location, temperature, Weather report, current Wx Symbol
  DisplayWeatherIcon(830, 120);                  // Display weather icon scale = Large;
  DisplayForecastSection(285, 220, 82);          // 3hr forecast boxes
  DisplayGraphSection(320, 220);                 // Graphs of pressure, temperature, humidity and rain or snowfall
  setFont(OpenSans8B);
  drawString(610, 6, layoutCaption(), LEFT);
}

void DisplayGeneralInfoSection() {
  setFont(OpenSans18B);
  drawString(5, 3, settings.City, LEFT);
  setFont(OpenSans12B);
  drawString(385, 5, Date_str + " @ " + Time_str, LEFT);
}

void DisplayWeatherIcon(int x, int y) {
  DisplayConditionsSection(x, y, WxConditions[0].Icon, LargeIcon);
  DisplayForecastTextSection(x-100, y+75);
}

void DisplayMainWeatherSection(int x, int y) {
  setFont(OpenSans8B);
  DisplayTempHumiPressSection(x+5, y - 45);
  //DisplayForecastTextSection(x - 55, y + 45);
  DisplayVisiCCoverUVISection(x - 18, y + 45);
  DisplayAirQualitySection(x - 25, y + 85);
}

void DisplayDisplayWindSection(int x, int y, float angle, float windspeed, int Cradius) {
  arrow(x, y, Cradius - 22, angle, 18, 33); // Show wind direction on outer circle of width and length
  setFont(OpenSans8B);
  int dxo, dyo, dxi, dyi;
  drawCircle(x, y, Cradius, Black);       // Draw compass circle
  drawCircle(x, y, Cradius + 1, Black);   // Draw compass circle
  drawCircle(x, y, Cradius * 0.7, Black); // Draw compass inner circle
  for (float a = 0; a < 360; a = a + 22.5) {
    dxo = Cradius * cos((a - 90) * PI / 180);
    dyo = Cradius * sin((a - 90) * PI / 180);
    if (a == 45)  drawString(dxo + x + 15, dyo + y - 18, TXT_NE, CENTER);
    if (a == 135) drawString(dxo + x + 20, dyo + y - 2,  TXT_SE, CENTER);
    if (a == 225) drawString(dxo + x - 20, dyo + y - 2,  TXT_SW, CENTER);
    if (a == 315) drawString(dxo + x - 15, dyo + y - 18, TXT_NW, CENTER);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    drawLine(dxo + x, dyo + y, dxi + x, dyi + y, Black);
    dxo = dxo * 0.7;
    dyo = dyo * 0.7;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    drawLine(dxo + x, dyo + y, dxi + x, dyi + y, Black);
  }
  drawString(x, y - Cradius - 20,     TXT_N, CENTER);
  drawString(x, y + Cradius + 10,     TXT_S, CENTER);
  drawString(x - Cradius - 15, y - 5, TXT_W, CENTER);
  drawString(x + Cradius + 10, y - 5, TXT_E, CENTER);
  drawString(x + 3, y + 50, String(angle, 0) + "°", CENTER);
  setFont(OpenSans12B);
  drawString(x, y - 50, WindDegToOrdinalDirection(angle), CENTER);
  setFont(OpenSans24B);
  drawString(x + 3, y - 18, String(windspeed, 1), CENTER);
  setFont(OpenSans12B);
  drawString(x, y + 25, (settings.Units == "M" ? "m/s" : "mph"), CENTER);
}

String WindDegToOrdinalDirection(float winddirection) {
  if (winddirection >= 348.75 || winddirection < 11.25)  return TXT_N;
  if (winddirection >=  11.25 && winddirection < 33.75)  return TXT_NNE;
  if (winddirection >=  33.75 && winddirection < 56.25)  return TXT_NE;
  if (winddirection >=  56.25 && winddirection < 78.75)  return TXT_ENE;
  if (winddirection >=  78.75 && winddirection < 101.25) return TXT_E;
  if (winddirection >= 101.25 && winddirection < 123.75) return TXT_ESE;
  if (winddirection >= 123.75 && winddirection < 146.25) return TXT_SE;
  if (winddirection >= 146.25 && winddirection < 168.75) return TXT_SSE;
  if (winddirection >= 168.75 && winddirection < 191.25) return TXT_S;
  if (winddirection >= 191.25 && winddirection < 213.75) return TXT_SSW;
  if (winddirection >= 213.75 && winddirection < 236.25) return TXT_SW;
  if (winddirection >= 236.25 && winddirection < 258.75) return TXT_WSW;
  if (winddirection >= 258.75 && winddirection < 281.25) return TXT_W;
  if (winddirection >= 281.25 && winddirection < 303.75) return TXT_WNW;
  if (winddirection >= 303.75 && winddirection < 326.25) return TXT_NW;
  if (winddirection >= 326.25 && winddirection < 348.75) return TXT_NNW;
  return "?";
}

void DisplayTempHumiPressSection(int x, int y) {
  setFont(OpenSans18B);
  drawString(x - 30, y-10, String(WxConditions[0].Temperature, 1) + "°   " + String(WxConditions[0].Humidity, 0) + "%", LEFT);
  setFont(OpenSans12B);
  DrawPressureAndTrend(x + 195, y + 3, WxConditions[0].Pressure, WxConditions[0].Trend);
  int Yoffset = 47;
  if (WxConditions[0].Windspeed > 0) {
    drawString(x + 5, y + Yoffset, String(WxConditions[0].FeelsLike, 1), LEFT);   // Show FeelsLike temperature if windspeed > 0
    drawGrayscaleImage(ImgTempFL_info(x - 30, y + Yoffset - 5));
    //Yoffset += 30;
  }
  //drawString(x + 75, y + Yoffset - 7, String(WxConditions[0].High, 0) + "° | " + String(WxConditions[0].Low, 0) + "° Hi/Lo", LEFT); // Show forecast high and Low

  drawGrayscaleImage(ImgTempHi_info(x + 70, y + Yoffset -5));
  drawString(x + 95, y + Yoffset, String(WxConditions[0].High, 1), LEFT); // Show forecast high and Low
  
  drawGrayscaleImage(ImgTempLo_info(x + 160, y + Yoffset - 5));
  drawString(x + 185, y + Yoffset, String(WxConditions[0].Low, 1), LEFT);
}

void DisplayForecastTextSection(int x, int y) {
#define lineWidth 34
  setFont(OpenSans12B);
  String Wx_Description = WxConditions[0].Forecast0;
  Wx_Description.replace(".", ""); // remove any '.'
  int spaceRemaining = 0, p = 0, charCount = 0, Width = lineWidth;
  while (p < Wx_Description.length()) {
    if (Wx_Description.substring(p, p + 1) == " ") spaceRemaining = p;
    if (charCount > Width - 1) { // '~' is the end of line marker
      Wx_Description = Wx_Description.substring(0, spaceRemaining) + "~" + Wx_Description.substring(spaceRemaining + 1);
      charCount = 0;
    }
    p++;
    charCount++;
  }
  if (WxForecast[0].Rainfall > 0) Wx_Description += " (" + String(WxForecast[0].Rainfall, 1) + String((settings.Units == "M" ? "mm" : "in")) + ")";
  String Line1 = Wx_Description.substring(0, Wx_Description.indexOf("~"));
  String Line2 = Wx_Description.substring(Wx_Description.indexOf("~") + 1);
  drawString(x + 30, y + 5, TitleCase(Line1), LEFT);
  if (Line1 != Line2) drawString(x + 30, y + 30, Line2, LEFT);
}

void DisplayVisiCCoverUVISection(int x, int y) {
  setFont(OpenSans12B);
  dbgPrintln("==========================" + String(WxConditions[0].Visibility));
  Visibility(x + 5, y, String(WxConditions[0].Visibility) + "M");
  CloudCover(x + 155, y, WxConditions[0].Cloudcover);
  Display_UVIndexLevel(x + 265, y, WxConditions[0].UVI);
}

void Display_UVIndexLevel(int x, int y, float UVI) {
  String Level = "";
  if (UVI <= 2)              Level = " (L)";
  if (UVI >= 3 && UVI <= 5)  Level = " (M)";
  if (UVI >= 6 && UVI <= 7)  Level = " (H)";
  if (UVI >= 8 && UVI <= 10) Level = " (VH)";
  if (UVI >= 11)             Level = " (EX)";
  drawString(x + 20, y - 5, String(UVI, (UVI < 0 ? 1 : 0)) + Level, LEFT);
  DrawUVI(x - 10, y - 5);
}

void DisplayForecastWeather(int x, int y, int index, int fwidth) {
  x = x + fwidth * index;
  DisplayConditionsSection(x + fwidth / 2 - 5, y + 85, WxForecast[index].Icon, SmallIcon);
  setFont(OpenSans10B);
  drawString(x + fwidth / 2, y + 30, String(ConvertUnixTime(WxForecast[index].Dt + WxConditions[0].FTimezone).substring(0, 5)), CENTER);
  drawString(x + fwidth / 2, y + 130, String(WxForecast[index].High, 0) + "°/" + String(WxForecast[index].Low, 0) + "°", CENTER);
}

double NormalizedMoonPhase(int d, int m, int y) {
  int j = JulianDate(d, m, y);
  //Calculate approximate moon phase
  double Phase = (j + 4.867) / 29.53059;
  return (Phase - (int) Phase);
}

void DisplayAstronomySection(int x, int y) {

  String Hemisphere = atof(settings.Latitude.c_str()) > 0? "north" : "south";

  setFont(OpenSans10B);
  time_t now = time(NULL);
  struct tm * now_utc  = gmtime(&now);
  drawString(x + 5, y + 102, MoonPhase(now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900, Hemisphere), LEFT);
  DrawMoonImage(x + 10, y + 23); // Different references!
  DrawMoon(x - 28, y - 15, 75, now_utc->tm_mday, now_utc->tm_mon + 1, now_utc->tm_year + 1900, Hemisphere); // Spaced at 1/2 moon size, so 10 - 75/2 = -28
  drawString(x + 115, y + 40, ConvertUnixTime(WxConditions[0].Sunrise).substring(0, 5), LEFT); // Sunrise
  drawString(x + 115, y + 80, ConvertUnixTime(WxConditions[0].Sunset).substring(0, 5), LEFT);  // Sunset
  DrawSunriseImage(x + 180, y + 20);
  DrawSunsetImage(x + 180, y + 60);
}

void DrawMoon(int x, int y, int diameter, int dd, int mm, int yy, String hemisphere) {
  double Phase = NormalizedMoonPhase(dd, mm, yy);
  hemisphere.toLowerCase();
  if (hemisphere == "south") Phase = 1 - Phase;
  // Draw dark part of moon
  fillCircle(x + diameter - 1, y + diameter, diameter / 2 + 1, DarkGrey);
  const int number_of_lines = 90;
  for (double Ypos = 0; Ypos <= number_of_lines / 2; Ypos++) {
    double Xpos = sqrt(number_of_lines / 2 * number_of_lines / 2 - Ypos * Ypos);
    // Determine the edges of the lighted part of the moon
    double Rpos = 2 * Xpos;
    double Xpos1, Xpos2;
    if (Phase < 0.5) {
      Xpos1 = -Xpos;
      Xpos2 = Rpos - 2 * Phase * Rpos - Xpos;
    }
    else {
      Xpos1 = Xpos;
      Xpos2 = Xpos - 2 * Phase * Rpos + Rpos;
    }
    // Draw light part of moon
    double pW1x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW1y = (number_of_lines - Ypos)  / number_of_lines * diameter + y;
    double pW2x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW2y = (number_of_lines - Ypos)  / number_of_lines * diameter + y;
    double pW3x = (Xpos1 + number_of_lines) / number_of_lines * diameter + x;
    double pW3y = (Ypos + number_of_lines)  / number_of_lines * diameter + y;
    double pW4x = (Xpos2 + number_of_lines) / number_of_lines * diameter + x;
    double pW4y = (Ypos + number_of_lines)  / number_of_lines * diameter + y;
    drawLine(pW1x, pW1y, pW2x, pW2y, White);
    drawLine(pW3x, pW3y, pW4x, pW4y, White);
  }
  drawCircle(x + diameter - 1, y + diameter, diameter / 2, Black);
}

String MoonPhase(int d, int m, int y, String hemisphere) {
  int c, e;
  double jd;
  int b;
  if (m < 3) {
    y--;
    m += 12;
  }
  ++m;
  c   = 365.25 * y;
  e   = 30.6  * m;
  jd  = c + e + d - 694039.09;     /* jd is total days elapsed */
  jd /= 29.53059;                        /* divide by the moon cycle (29.53 days) */
  b   = jd;                              /* int(jd) -> b, take integer part of jd */
  jd -= b;                               /* subtract integer part to leave fractional part of original jd */
  b   = jd * 8 + 0.5;                /* scale fraction from 0-8 and round by adding 0.5 */
  b   = b & 7;                           /* 0 and 8 are the same phase so modulo 8 for 0 */
  if (hemisphere == "south") b = 7 - b;
  if (b == 0) return TXT_MOON_NEW;              // New;              0%  illuminated
  if (b == 1) return TXT_MOON_WAXING_CRESCENT;  // Waxing crescent; 25%  illuminated
  if (b == 2) return TXT_MOON_FIRST_QUARTER;    // First quarter;   50%  illuminated
  if (b == 3) return TXT_MOON_WAXING_GIBBOUS;   // Waxing gibbous;  75%  illuminated
  if (b == 4) return TXT_MOON_FULL;             // Full;            100% illuminated
  if (b == 5) return TXT_MOON_WANING_GIBBOUS;   // Waning gibbous;  75%  illuminated
  if (b == 6) return TXT_MOON_THIRD_QUARTER;    // Third quarter;   50%  illuminated
  if (b == 7) return TXT_MOON_WANING_CRESCENT;  // Waning crescent; 25%  illuminated
  return "";
}

void DisplayForecastSection(int x, int y, int fwidth) {
  int f = 0;
  do {
    DisplayForecastWeather(x, y, f, fwidth);
    f++;
  } while (f < 8);
}

void loadGraphReadings() {
  int r = 0;
  do { // Pre-load temporary arrays with with data - because C parses by reference and remember that[1] has already been converted to I units
    if (settings.Units == "I") pressure_readings[r] = WxForecast[r].Pressure * 0.02953;   else pressure_readings[r] = WxForecast[r].Pressure;
    if (settings.Units == "I") rain_readings[r]     = WxForecast[r].Rainfall * 0.0393701; else rain_readings[r]     = WxForecast[r].Rainfall;
    if (settings.Units == "I") snow_readings[r]     = WxForecast[r].Snowfall * 0.0393701; else snow_readings[r]     = WxForecast[r].Snowfall;
    temperature_readings[r]                = WxForecast[r].Temperature;
    humidity_readings[r]                   = WxForecast[r].Humidity;
    r++;
  } while (r < max_readings);
}

void drawPrecipGraph(int x, int y, int gwidth, int gheight) {
  if (SumOfPrecip(rain_readings, max_readings) >= SumOfPrecip(snow_readings, max_readings))
    DrawGraph(x, y, gwidth, gheight, 0, 30, settings.Units == "M" ? TXT_RAINFALL_MM : TXT_RAINFALL_IN, rain_readings, max_readings, autoscale_on, barchart_on);
  else
    DrawGraph(x, y, gwidth, gheight, 0, 30, settings.Units == "M" ? TXT_SNOWFALL_MM : TXT_SNOWFALL_IN, snow_readings, max_readings, autoscale_on, barchart_on);
}

void drawAllGraphs(int x0, int y0, int x1, int y1, int gwidth, int gheight) {
  loadGraphReadings();
  DrawGraph(x0, y0, gwidth, gheight, 900, 1050, settings.Units == "M" ? TXT_PRESSURE_HPA : TXT_PRESSURE_IN, pressure_readings, max_readings, autoscale_on, barchart_off);
  DrawGraph(x1, y0, gwidth, gheight, 10, 30,    settings.Units == "M" ? TXT_TEMPERATURE_C : TXT_TEMPERATURE_F, temperature_readings, max_readings, autoscale_on, barchart_off);
  DrawGraph(x0, y1, gwidth, gheight, 0, 100,    TXT_HUMIDITY_PERCENT, humidity_readings, max_readings, autoscale_off, barchart_off);
  drawPrecipGraph(x1, y1, gwidth, gheight);
}

void DisplayGraphSection(int x, int y) {
  (void)x;
  (void)y;
  int gwidth = 175, gheight = 100;
  int gx = (SCREEN_WIDTH - gwidth * 4) / 5 + 8;
  int gy = (SCREEN_HEIGHT - gheight - 30);
  int gap = gwidth + gx;
  loadGraphReadings();
  DrawGraph(gx + 0 * gap, gy, gwidth, gheight, 900, 1050, settings.Units == "M" ? TXT_PRESSURE_HPA : TXT_PRESSURE_IN, pressure_readings, max_readings, autoscale_on, barchart_off);
  DrawGraph(gx + 1 * gap, gy, gwidth, gheight, 10, 30,    settings.Units == "M" ? TXT_TEMPERATURE_C : TXT_TEMPERATURE_F, temperature_readings, max_readings, autoscale_on, barchart_off);
  DrawGraph(gx + 2 * gap, gy, gwidth, gheight, 0, 100,   TXT_HUMIDITY_PERCENT, humidity_readings, max_readings, autoscale_off, barchart_off);
  drawPrecipGraph(gx + 3 * gap + 5, gy, gwidth, gheight);
}

void DisplayConditionsSection(int x, int y, String IconName, bool IconSize) {
  dbgPrintln("Icon name: " + IconName);
  if      (IconName == "01d" || IconName == "01n") ClearSky(x, y, IconSize, IconName);
  else if (IconName == "02d" || IconName == "02n") FewClouds(x, y, IconSize, IconName);
  else if (IconName == "03d" || IconName == "03n") ScatteredClouds(x, y, IconSize, IconName);
  else if (IconName == "04d" || IconName == "04n") BrokenClouds(x, y, IconSize, IconName);
  else if (IconName == "09d" || IconName == "09n") ChanceRain(x, y, IconSize, IconName);
  else if (IconName == "10d" || IconName == "10n") Rain(x, y, IconSize, IconName);
  else if (IconName == "11d" || IconName == "11n") Thunderstorms(x, y, IconSize, IconName);
  else if (IconName == "13d" || IconName == "13n") Snow(x, y, IconSize, IconName);
  else if (IconName == "50d" || IconName == "50n") Mist(x, y, IconSize, IconName);
  else                                             Nodata(x, y, IconSize, IconName);
}

void arrow(int x, int y, int asize, float aangle, int pwidth, int plength) {
  float dx = (asize - 10) * cos((aangle - 90) * PI / 180) + x; // calculate X position
  float dy = (asize - 10) * sin((aangle - 90) * PI / 180) + y; // calculate Y position
  float x1 = 0;         float y1 = plength;
  float x2 = pwidth / 2;  float y2 = pwidth / 2;
  float x3 = -pwidth / 2; float y3 = pwidth / 2;
  float angle = aangle * PI / 180 - 135;
  float xx1 = x1 * cos(angle) - y1 * sin(angle) + dx;
  float yy1 = y1 * cos(angle) + x1 * sin(angle) + dy;
  float xx2 = x2 * cos(angle) - y2 * sin(angle) + dx;
  float yy2 = y2 * cos(angle) + x2 * sin(angle) + dy;
  float xx3 = x3 * cos(angle) - y3 * sin(angle) + dx;
  float yy3 = y3 * cos(angle) + x3 * sin(angle) + dy;
  fillTriangle(xx1, yy1, xx3, yy3, xx2, yy2, Black);
}

void DrawSegment(int x, int y, int o1, int o2, int o3, int o4, int o11, int o12, int o13, int o14) {
  drawLine(x + o1,  y + o2,  x + o3,  y + o4,  Black);
  drawLine(x + o11, y + o12, x + o13, y + o14, Black);
}

void DrawPressureTrendArrow(int x, int y, String slope) {
  if      (slope == "+") {
    DrawSegment(x, y, 0, 0, 8, -8, 8, -8, 16, 0);
    DrawSegment(x - 1, y, 0, 0, 8, -8, 8, -8, 16, 0);
  }
  else if (slope == "0") {
    DrawSegment(x, y, 8, -8, 16, 0, 8, 8, 16, 0);
    DrawSegment(x - 1, y, 8, -8, 16, 0, 8, 8, 16, 0);
  }
  else if (slope == "-") {
    DrawSegment(x, y, 0, 0, 8, 8, 8, 8, 16, 0);
    DrawSegment(x - 1, y, 0, 0, 8, 8, 8, 8, 16, 0);
  }
}

void DrawPressureAndTrend(int x, int y, float pressure, String slope) {
  drawString(x + 25, y - 10, String(pressure, (settings.Units == "M" ? 0 : 1)) + (settings.Units == "M" ? "hPa" : "in"), LEFT);
  DrawPressureTrendArrow(x, y, slope);
}

void DisplayPageHeader(const String& caption) {
  setFont(OpenSans18B);
  drawString(16, 8, settings.City, LEFT);
  setFont(OpenSans12B);
  drawString(300, 12, Date_str + "  " + Time_str, LEFT);
  setFont(OpenSans10B);
  drawString(16, 42, caption, LEFT);
  DisplayStatusSection(525, 28, wifi_signal);
  drawLine(16, 68, SCREEN_WIDTH - 16, 68, Grey);
}

String iconPhrase(const String& icon) {
  String code = icon.substring(0, 2);
  if (code == "01") return "Clear";
  if (code == "02") return "Partly Cloudy";
  if (code == "03") return "Mostly Cloudy";
  if (code == "04") return "Cloudy";
  if (code == "09") return "Showers";
  if (code == "10") return "Rain";
  if (code == "11") return "Storm";
  if (code == "13") return "Snow";
  if (code == "50") return "Mist";
  return "";
}

struct tm shiftedLocal(time_t utc) {
  time_t shifted = utc + (time_t)WxConditions[0].FTimezone;
  return *localtime(&shifted);
}

int collectDaySlots(DaySlot* days, int maxDays) {
  int count = 0;
  for (int r = 0; r < max_readings; r++) {
    if (WxForecast[r].Dt <= 0) continue;
    struct tm tm = shiftedLocal(WxForecast[r].Dt);
    int key = (tm.tm_year << 9) | tm.tm_yday;
    int idx = -1;
    for (int i = 0; i < count; i++) if (days[i].key == key) idx = i;
    if (idx < 0) {
      if (count >= maxDays) continue;
      idx = count++;
      days[idx].key = key;
      days[idx].labelDt = WxForecast[r].Dt;
      days[idx].high = WxForecast[r].High;
      days[idx].low = WxForecast[r].Low;
      days[idx].icon[0] = 0;
      days[idx].iconScore = -1;
    }
    if (WxForecast[r].High > days[idx].high) days[idx].high = WxForecast[r].High;
    if (WxForecast[r].Low < days[idx].low) days[idx].low = WxForecast[r].Low;
    int score = 1;
    if (tm.tm_hour >= 8 && tm.tm_hour <= 18) score = 2;
    if (tm.tm_hour >= 11 && tm.tm_hour <= 16) score = 3;
    if (WxForecast[r].Icon.endsWith("d")) score++;
    if (score > days[idx].iconScore) {
      days[idx].iconScore = score;
      strncpy(days[idx].icon, WxForecast[r].Icon.c_str(), sizeof(days[idx].icon) - 1);
      days[idx].icon[sizeof(days[idx].icon) - 1] = 0;
    }
  }
  return count;
}

String daySlotLabel(const DaySlot& day, int todayKey) {
  if (day.key == todayKey) return "Today";
  struct tm tm = shiftedLocal(day.labelDt);
  char buf[24];
  snprintf(buf, sizeof(buf), "%s, %s %d", weekday_D[tm.tm_wday], month_M[tm.tm_mon], tm.tm_mday);
  return String(buf);
}

void drawStatIcon(int kind, int x, int y) {
  switch (kind) {
    case 0:
      drawLine(x + 8, y + 1, x + 8, y + 12, Black);
      drawCircle(x + 8, y + 15, 3, Black);
      break;
    case 1:
      fillCircle(x + 8, y + 12, 5, Black);
      fillCircle(x + 8, y + 8, 3, White);
      break;
    case 2:
      drawLine(x + 1, y + 4, x + 16, y + 4, Black);
      drawLine(x + 1, y + 9, x + 11, y + 9, Black);
      drawLine(x + 1, y + 14, x + 16, y + 14, Black);
      break;
    case 3:
      drawCircle(x + 8, y + 9, 7, Black);
      drawLine(x + 8, y + 9, x + 8, y + 5, Black);
      drawLine(x + 8, y + 9, x + 12, y + 11, Black);
      break;
    default:
      drawCircle(x + 8, y + 9, 3, Black);
      drawLine(x + 8, y + 1, x + 8, y + 4, Black);
      drawLine(x + 8, y + 14, x + 8, y + 17, Black);
      drawLine(x + 1, y + 9, x + 4, y + 9, Black);
      drawLine(x + 12, y + 9, x + 15, y + 9, Black);
      break;
  }
}

void drawMainStatRow(int panelX, int rowY, int rowW, int rowH, int kind, const String& label, const String& value, const String& value2) {
  drawStatIcon(kind, panelX + 16, rowY + (rowH - 18) / 2);
  setFont(OpenSans12B);
  int textY = value2.length() ? rowY + 6 : rowY + 16;
  drawString(panelX + 48, rowY + 16, label, LEFT);
  drawString(panelX + rowW - 8, textY, value, RIGHT);
  if (value2.length()) drawString(panelX + rowW - 8, textY + 20, value2, RIGHT);
}

void DisplayWeatherMain() {
  const String degree = settings.Units == "M" ? "°C" : "°F";

  fillCircle(22, 24, 7, Black);
  fillCircle(22, 22, 3, White);
  setFont(OpenSans18B);
  drawString(40, 10, settings.City, LEFT);

  String clockText = Time_str;
  if (settings.Units == "M" && clockText.length() >= 5) clockText = clockText.substring(0, 5);
  setFont(OpenSans12B);
  drawString(SCREEN_WIDTH - 16, 16, Date_str + "  |  " + clockText, RIGHT);

  const int ruleX = 490;
  const int panelX = 506;
  const int panelY = 68;
  const int panelW = SCREEN_WIDTH - 16 - panelX;
  const int panelH = 250;
  const int rowH = panelH / 5;
  drawLine(ruleX, panelY, ruleX, panelY + panelH, Black);
  for (int row = 1; row < 5; row++) {
    drawLine(panelX, panelY + row * rowH, panelX + panelW, panelY + row * rowH, Grey);
  }

  DisplayConditionsSection(145, 180, WxConditions[0].Icon, LargeIcon);
  setFont(OpenSans24B);
  drawString(270, 108, String(WxConditions[0].Temperature, 0) + degree, LEFT);
  setFont(OpenSans18B);
  drawString(270, 172, iconPhrase(WxConditions[0].Icon), LEFT);
  setFont(OpenSans12B);
  drawString(270, 214, "Feels like " + String(WxConditions[0].FeelsLike, 0) + degree, LEFT);

  String pressureText = String(WxConditions[0].Pressure, settings.Units == "M" ? 0 : 1)
                        + (settings.Units == "M" ? " hPa" : " in");
  String sunText = ConvertUnixTime(WxConditions[0].Sunrise).substring(0, 5) + " / "
                   + ConvertUnixTime(WxConditions[0].Sunset).substring(0, 5);
  drawMainStatRow(panelX, panelY + 0 * rowH, panelW, rowH, 0, "High / Low",
                  String(WxConditions[0].High, 0) + "° / " + String(WxConditions[0].Low, 0) + "°", "");
  drawMainStatRow(panelX, panelY + 1 * rowH, panelW, rowH, 1, "Humidity",
                  String(WxConditions[0].Humidity, 0) + "%", "");
  drawMainStatRow(panelX, panelY + 2 * rowH, panelW, rowH, 2, "Wind",
                  String(WxConditions[0].Windspeed, 1) + (settings.Units == "M" ? " m/s" : " mph"),
                  WindDegToOrdinalDirection(WxConditions[0].Winddir));
  drawMainStatRow(panelX, panelY + 3 * rowH, panelW, rowH, 3, "Pressure", pressureText, "");
  drawMainStatRow(panelX, panelY + 4 * rowH, panelW, rowH, 4, "Sunrise / Sunset", sunText, "");

  DaySlot days[5];
  int dayCount = collectDaySlots(days, 5);
  if (dayCount < 1) return;
  struct tm nowTm = shiftedLocal(time(NULL));
  int todayKey = (nowTm.tm_year << 9) | nowTm.tm_yday;

  const int stripX = 16;
  const int stripY = 336;
  const int stripW = SCREEN_WIDTH - 32;
  const int stripH = 188;
  const int colW = stripW / dayCount;
  drawRect(stripX, stripY, stripW, stripH, Black);
  for (int i = 0; i < dayCount; i++) {
    int colX = stripX + i * colW;
    int colRight = (i == dayCount - 1) ? stripX + stripW : colX + colW;
    if (i > 0) drawLine(colX, stripY + 10, colX, stripY + stripH - 10, Grey);
    if (days[i].key == todayKey) {
      drawRect(colX + 6, stripY + 6, (colRight - colX) - 12, stripH - 12, Black);
      drawRect(colX + 7, stripY + 7, (colRight - colX) - 14, stripH - 14, Black);
    }
    int mid = (colX + colRight) / 2;
    setFont(OpenSans12B);
    drawString(mid, stripY + 16, daySlotLabel(days[i], todayKey), CENTER);
    DisplayConditionsSection(mid, stripY + 82, days[i].icon, SmallIcon);
    setFont(OpenSans18B);
    drawString(mid, stripY + 124, String(days[i].high, 0) + "° / " + String(days[i].low, 0) + "°", CENTER);
    setFont(OpenSans10B);
    drawString(mid, stripY + 158, iconPhrase(days[i].icon), CENTER);
  }
}

void DisplayWeatherGraphs() {
  DisplayPageHeader(layoutCaption());
  setFont(OpenSans12B);
  drawString(250, 42, String(WxConditions[0].Temperature, 1) + "°    "
             + String(WxConditions[0].Humidity, 0) + "%", LEFT);

  const int gwidth = 380;
  const int gheight = 150;
  drawAllGraphs(56, 118, 506, 348, gwidth, gheight);
}

void DisplayWeatherDetails() {
  DisplayPageHeader(layoutCaption());
  setFont(OpenSans12B);
  drawString(250, 42, String(WxConditions[0].Temperature, 1) + "°    "
             + String(WxConditions[0].Humidity, 0) + "%", LEFT);

  DisplayDisplayWindSection(170, 200, WxConditions[0].Winddir, WxConditions[0].Windspeed, 95);
  DisplayAstronomySection(430, 78);
  DisplayVisiCCoverUVISection(420, 230);
  DisplayAirQualitySection(420, 285);
  DisplayForecastSection(16, 368, 116);
}

void DisplayStatusSection(int x, int y, int rssi) {
  setFont(OpenSans12B);
  DrawRSSI(x + 385, y, rssi);
  DrawBattery(x + 180, y, false);
}

void DrawRSSI(int x, int y, int rssi) {
  int WIFIsignal = 0;
  int xpos = 1;
  for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
    if (_rssi <= -20)  WIFIsignal = 30; //            <-20dbm displays 5-bars
    if (_rssi <= -40)  WIFIsignal = 24; //  -40dbm to  -21dbm displays 4-bars
    if (_rssi <= -60)  WIFIsignal = 18; //  -60dbm to  -41dbm displays 3-bars
    if (_rssi <= -80)  WIFIsignal = 12; //  -80dbm to  -61dbm displays 2-bars
    if (_rssi <= -100) WIFIsignal = 6;  // -100dbm to  -81dbm displays 1-bar
    fillRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
    xpos++;
  }
}

boolean UpdateLocalTime() {
  struct tm timeinfo;
  char   time_output[30], day_output[30], update_time[30];
  
  while (!getLocalTime(&timeinfo)) { // Wait for 5-sec for time to synchronise
    Serial.println("Failed to obtain time");
    return false;
  }

  //struct tm timeinfo = *localtime(&datetime_request.response.dt);

  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
  //See http://www.cplusplus.com/reference/ctime/strftime/
  //Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");      // Displays: Saturday, June 24 2017 14:05:49
  char buffer[30];
  strftime(buffer, 7, "%a %b %d %Y   %H:%M:%S", &timeinfo);
  dbgPrintln("UpdateLocalTime: " + String(buffer));
  if (settings.Units == "M") {
    sprintf(day_output, "%s, %02u %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
    strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);  // Creates: '@ 14:05:49'   and change from 30 to 8 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    sprintf(time_output, "%s", update_time);
  }
  else
  {
    strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo); // Creates  'Sat May-31-2019'
    strftime(update_time, sizeof(update_time), "%r", &timeinfo);        // Creates: '@ 02:05:49pm'
    sprintf(time_output, "%s", update_time);
  }
  Date_str = day_output;
  Time_str = time_output;
  sdOnClockReady();
  return true;
}

void DrawBattery(int x, int y, bool _skipDraw) {
  
  uint8_t percentage = 100;

  float voltage = ((float)(2*analogReadMilliVolts(BATT_PIN)))/1000.0f + 0.07f;

  if (voltage > 1 ) { // Only display if there is a valid reading

    percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;   

    percentage = voltage < 3.5f ? 0 : percentage;
    percentage = voltage > 4.2f ? 100 : percentage;

    dbgPrintln("Voltage = " + String(voltage) + ", pct: " + String(percentage) + ", wifistat: " + String(WiFi.status()));

    if (_skipDraw)
    {
      logInfo.BatteryPct = percentage;
      logInfo.BatteryVoltage = voltage;
      return;
    }
    y -= 3;
    drawRect(x + 25, y - 14, 40, 15, Black);
    fillRect(x + 65, y - 10, 4, 7, Black);
    fillRect(x + 27, y - 12, 36 * percentage / 100.0, 11, Black);
    y +=3;
    drawString(x + 80, y - 17, String(percentage) + "% " + String(voltage, 2) + "v", LEFT);
  }
}

// Symbols are drawn on a relative 10x10grid and 1 scale unit = 1 drawing unit
void addcloud(int x, int y, int scale, int linesize) {
  fillCircle(x - scale * 3, y, scale, Black);                                                              // Left most circle
  fillCircle(x + scale * 3, y, scale, Black);                                                              // Right most circle
  fillCircle(x - scale, y - scale, scale * 1.4, Black);                                                    // left middle upper circle
  fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75, Black);                                       // Right middle upper circle
  fillRect(x - scale * 3 - 1, y - scale, scale * 6, scale * 2 + 1, Black);                                 // Upper and lower lines
  fillCircle(x - scale * 3, y, scale - linesize, White);                                                   // Clear left most circle
  fillCircle(x + scale * 3, y, scale - linesize, White);                                                   // Clear right most circle
  fillCircle(x - scale, y - scale, scale * 1.4 - linesize, White);                                         // left middle upper circle
  fillCircle(x + scale * 1.5, y - scale * 1.3, scale * 1.75 - linesize, White);                            // Right middle upper circle
  fillRect(x - scale * 3 + 2, y - scale + linesize - 1, scale * 5.9, scale * 2 - linesize * 2 + 2, White); // Upper and lower lines
}

void addrain(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) {
    setFont(OpenSans8B);
    drawString(x - 25, y + 12, "///////", LEFT);
  }
  else
  {
    setFont(OpenSans18B);
    drawString(x - 60, y + 25, "///////", LEFT);
  }
}

void addsnow(int x, int y, int scale, bool IconSize) {
  if (IconSize == SmallIcon) {
    setFont(OpenSans8B);
    drawString(x - 25, y + 15, "* * * *", LEFT);
  }
  else
  {
    setFont(OpenSans18B);
    drawString(x - 60, y + 30, "* * * *", LEFT);
  }
}

void addtstorm(int x, int y, int scale) {
  y = y + scale / 2;
  for (int i = 1; i < 5; i++) {
    drawLine(x - scale * 4 + scale * i * 1.5 + 0, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 0, y + scale, Black);
    drawLine(x - scale * 4 + scale * i * 1.5 + 1, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 1, y + scale, Black);
    drawLine(x - scale * 4 + scale * i * 1.5 + 2, y + scale * 1.5, x - scale * 3.5 + scale * i * 1.5 + 2, y + scale, Black);
    drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 0, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 0, Black);
    drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 1, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 1, Black);
    drawLine(x - scale * 4 + scale * i * 1.5, y + scale * 1.5 + 2, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5 + 2, Black);
    drawLine(x - scale * 3.5 + scale * i * 1.4 + 0, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 0, y + scale * 1.5, Black);
    drawLine(x - scale * 3.5 + scale * i * 1.4 + 1, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 1, y + scale * 1.5, Black);
    drawLine(x - scale * 3.5 + scale * i * 1.4 + 2, y + scale * 2.5, x - scale * 3 + scale * i * 1.5 + 2, y + scale * 1.5, Black);
  }
}

void addsun(int x, int y, int scale, bool IconSize) {
  int linesize = 5;
  fillRect(x - scale * 2, y, scale * 4, linesize, Black);
  fillRect(x, y - scale * 2, linesize, scale * 4, Black);
  DrawAngledLine(x + scale * 1.4, y + scale * 1.4, (x - scale * 1.4), (y - scale * 1.4), linesize * 1.5, Black); // Actually sqrt(2) but 1.4 is good enough
  DrawAngledLine(x - scale * 1.4, y + scale * 1.4, (x + scale * 1.4), (y - scale * 1.4), linesize * 1.5, Black);
  fillCircle(x, y, scale * 1.3, White);
  fillCircle(x, y, scale, Black);
  fillCircle(x, y, scale - linesize, White);
}

void addfog(int x, int y, int scale, int linesize, bool IconSize) {
  if (IconSize == SmallIcon) linesize = 3;
  for (int i = 0; i < 6; i++) {
    fillRect(x - scale * 3, y + scale * 1.5, scale * 6, linesize, Black);
    fillRect(x - scale * 3, y + scale * 2.0, scale * 6, linesize, Black);
    fillRect(x - scale * 3, y + scale * 2.5, scale * 6, linesize, Black);
  }
}

void DrawAngledLine(int x, int y, int x1, int y1, int size, int color) {
  int dx = (size / 2.0) * (x - x1) / sqrt(sq(x - x1) + sq(y - y1));
  int dy = (size / 2.0) * (y - y1) / sqrt(sq(x - x1) + sq(y - y1));
  fillTriangle(x + dx, y - dy, x - dx,  y + dy,  x1 + dx, y1 - dy, color);
  fillTriangle(x - dx, y + dy, x1 - dx, y1 + dy, x1 + dx, y1 - dy, color);
}

void ClearSky(int x, int y, bool IconSize, String IconName) {
  int scale = Small;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  y += (IconSize ? 0 : 10);
  addsun(x, y, scale * (IconSize ? 1.7 : 1.2), IconSize);
}

void BrokenClouds(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
}

void FewClouds(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x + (IconSize ? 10 : 0), y, scale * (IconSize ? 0.9 : 0.8), linesize);
  addsun((x + (IconSize ? 10 : 0)) - scale * 1.8, y - scale * 1.6, scale, IconSize);
}

void ScatteredClouds(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x - (IconSize ? 35 : 0), y * (IconSize ? 0.75 : 0.93), scale / 2, linesize); // Cloud top left
  addcloud(x, y, scale * 0.9, linesize);                                         // Main cloud
}

void Rain(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  y += 15;
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
  addrain(x, y, scale, IconSize);
}

void ChanceRain(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  y += 15;
  addsun(x - scale * 1.8, y - scale * 1.8, scale, IconSize);
  addcloud(x, y, scale * (IconSize ? 1 : 0.65), linesize);
  addrain(x, y, scale, IconSize);
}

void Thunderstorms(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  y += 5;
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
  addtstorm(x, y, scale);
}

void Snow(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  addcloud(x, y, scale * (IconSize ? 1 : 0.75), linesize);
  addsnow(x, y, scale, IconSize);
}

void Mist(int x, int y, bool IconSize, String IconName) {
  int scale = Small, linesize = 5;
  if (IconName.endsWith("n")) addmoon(x, y, IconSize);
  if (IconSize == LargeIcon) scale = Large;
  addsun(x, y, scale * (IconSize ? 1 : 0.75), linesize);
  addfog(x, y, scale, linesize, IconSize);
}

void CloudCover(int x, int y, int CloudCover) {
  addcloud(x - 9, y,     Small * 0.3, 2); // Cloud top left
  addcloud(x + 3, y - 2, Small * 0.3, 2); // Cloud top right
  addcloud(x, y + 15,    Small * 0.6, 2); // Main cloud
  drawString(x + 30, y, String(CloudCover) + "%", LEFT);
}

void Visibility(int x, int y, String Visibility) {
  float start_angle = 0.52, end_angle = 2.61, Offset = 10;
  int r = 14;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    drawPixel(x + r * cos(i), y - r / 2 + r * sin(i) + Offset, Black);
    drawPixel(x + r * cos(i), 1 + y - r / 2 + r * sin(i) + Offset, Black);
  }
  start_angle = 3.61; end_angle = 5.78;
  for (float i = start_angle; i < end_angle; i = i + 0.05) {
    drawPixel(x + r * cos(i), y + r / 2 + r * sin(i) + Offset, Black);
    drawPixel(x + r * cos(i), 1 + y + r / 2 + r * sin(i) + Offset, Black);
  }
  fillCircle(x, y + Offset, r / 4, Black);
  drawString(x + 20, y, Visibility, LEFT);
}

void addmoon(int x, int y, bool IconSize) {
  int xOffset = 65;
  int yOffset = 12;
  if (IconSize == LargeIcon) {
    xOffset = 130;
    yOffset = -40;
  }
  fillCircle(x - 28 + xOffset, y - 37 + yOffset, uint16_t(Small * 1.0), Black);
  fillCircle(x - 16 + xOffset, y - 37 + yOffset, uint16_t(Small * 1.6), White);
}

void Nodata(int x, int y, bool IconSize, String IconName) {
  if (IconSize == LargeIcon) setFont(OpenSans24B); else setFont(OpenSans12B);
  drawString(x - 3, y - 10, "?", CENTER);
}

void DrawMoonImage(int x, int y) {
  Rect_t area = {
    .x = x, .y = y, .width  = moon_width, .height =  moon_height
  };
  epd_draw_grayscale_image(area, (uint8_t *) moon_data);
}

void DrawSunriseImage(int x, int y) {
  Rect_t area = {
    .x = x, .y = y, .width  = sunrise_width, .height =  sunrise_height
  };
  epd_draw_grayscale_image(area, (uint8_t *) sunrise_data);
}

void DrawSunsetImage(int x, int y) {
  Rect_t area = {
    .x = x, .y = y, .width  = sunset_width, .height =  sunset_height
  };
  epd_draw_grayscale_image(area, (uint8_t *) sunset_data);
}

void DrawUVI(int x, int y) {
  Rect_t area = {
    .x = x, .y = y, .width  = uvi_width, .height = uvi_height
  };
  epd_draw_grayscale_image(area, (uint8_t *) uvi_data);
}

/* (C) D L BIRD
    This function will draw a graph on a ePaper/TFT/LCD display using data from an array containing data to be graphed.
    The variable 'max_readings' determines the maximum number of data elements for each array. Call it with the following parametric data:
    x_pos-the x axis top-left position of the graph
    y_pos-the y-axis top-left position of the graph, e.g. 100, 200 would draw the graph 100 pixels along and 200 pixels down from the top-left of the screen
    width-the width of the graph in pixels
    height-height of the graph in pixels
    Y1_Max-sets the scale of plotted data, for example 5000 would scale all data to a Y-axis of 5000 maximum
    data_array1 is parsed by value, externally they can be called anything else, e.g. within the routine it is called data_array1, but externally could be temperature_readings
    auto_scale-a logical value (TRUE or FALSE) that switches the Y-axis autoscale On or Off
    barchart_on-a logical value (TRUE or FALSE) that switches the drawing mode between barhcart and line graph
    barchart_colour-a sets the title and graph plotting colour
    If called with Y!_Max value of 500 and the data never goes above 500, then autoscale will retain a 0-500 Y scale, if on, the scale increases/decreases to match the data.
    auto_scale_margin, e.g. if set to 1000 then autoscale increments the scale by 1000 steps.
*/
void DrawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, float DataArray[], int readings, boolean auto_scale, boolean barchart_mode) {
#define auto_scale_margin 0 // Sets the autoscale increment, so axis steps up fter a change of e.g. 3
#define y_minor_axis 5      // 5 y-axis division markers
  setFont(OpenSans10B);
  int maxYscale = -10000;
  int minYscale =  10000;
  int last_x, last_y;
  float x2, y2;
  if (auto_scale == true) {
    for (int i = 1; i < readings; i++ ) {
      if (DataArray[i] >= maxYscale) maxYscale = DataArray[i];
      if (DataArray[i] <= minYscale) minYscale = DataArray[i];
    }
    maxYscale = round(maxYscale + auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Max
    Y1Max = round(maxYscale + 0.5);
    if (minYscale != 0) minYscale = round(minYscale - auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Min
    Y1Min = round(minYscale);
  }
  // Draw the graph
  last_x = x_pos + 1;
  last_y = y_pos + (Y1Max - constrain(DataArray[1], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
  drawRect(x_pos, y_pos, gwidth + 3, gheight + 2, Grey);
  drawString(x_pos - 20 + gwidth / 2, y_pos - 28, title, CENTER);
  for (int gx = 0; gx < readings; gx++) {
    x2 = x_pos + gx * gwidth / (readings - 1) - 1 ; // max_readings is the global variable that sets the maximum data that can be plotted
    y2 = y_pos + (Y1Max - constrain(DataArray[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1;
    if (barchart_mode) {
      fillRect(last_x + 2, y2, (gwidth / readings) - 1, y_pos + gheight - y2 + 2, Black);
    } else {
      drawLine(last_x, last_y - 1, x2, y2 - 1, Black); // Two lines for hi-res display
      drawLine(last_x, last_y, x2, y2, Black);
    }
    last_x = x2;
    last_y = y2;
  }
  //Draw the Y-axis scale
#define number_of_dashes 20
  for (int spacing = 0; spacing <= y_minor_axis; spacing++) {
    for (int j = 0; j < number_of_dashes; j++) { // Draw dashed graph grid lines
      if (spacing < y_minor_axis) drawFastHLine((x_pos + 3 + j * gwidth / number_of_dashes), y_pos + (gheight * spacing / y_minor_axis), gwidth / (2 * number_of_dashes), Grey);
    }
    if ((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing) < 5 || title == TXT_PRESSURE_IN) {
      drawString(x_pos - 10, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
    }
    else
    {
      if (Y1Min < 1 && Y1Max < 10) {
        drawString(x_pos - 3, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 1), RIGHT);
      }
      else {
        drawString(x_pos - 7, y_pos + gheight * spacing / y_minor_axis - 5, String((Y1Max - (float)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01), 0), RIGHT);
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    drawString(20 + x_pos + gwidth / 3 * i, y_pos + gheight + 10, String(i) + "d", LEFT);
    if (i < 2) drawFastVLine(x_pos + gwidth / 3 * i + gwidth / 3, y_pos, gheight, LightGrey);
  }
}

int drawString(int x, int y, String text, alignment align) {
  char * data  = const_cast<char*>(text.c_str());
  int  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  int w, h;
  int xx = x, yy = y;
  get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);
  if (align == RIGHT)  x = x - w;
  if (align == CENTER) x = x - w / 2;
  int cursor_y = y + h;
  write_string(&currentFont, data, &x, &cursor_y, framebuffer);
  return w;
}

void fillCircle(int x, int y, int r, uint8_t color) {
  epd_fill_circle(x, y, r, color, framebuffer);
}

void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_hline(x0, y0, length, color, framebuffer);
}

void drawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_vline(x0, y0, length, color, framebuffer);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  epd_write_line(x0, y0, x1, y1, color, framebuffer);
}

void drawCircle(int x0, int y0, int r, uint8_t color) {
  epd_draw_circle(x0, y0, r, color, framebuffer);
}

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_draw_rect(x, y, w, h, color, framebuffer);
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_fill_rect(x, y, w, h, color, framebuffer);
}

void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2, uint16_t color) {
  epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, framebuffer);
}

void drawPixel(int x, int y, uint8_t color) {
  epd_draw_pixel(x, y, color, framebuffer);
}

void setFont(GFXfont const & font) {
  currentFont = font;
}

void edp_update() {
  epd_draw_grayscale_image(epd_full_screen(), framebuffer); // Update the screen
}
/*
   1071 lines of code 03-03-2021
*/

void drawGrayscaleImage(Rect_t const& area, uint8_t const * data) {
  // epd_draw_grayscale_image(area, (uint8_t *) data);

  // unlike the original code, this method does not flush the image the screen,
  // but only copies it to the framebuffer
  epd_copy_to_framebuffer(area, (uint8_t *)data, framebuffer);
}

void DrawGauge(int x, int y, AQIndicator ind)
{
    IndicatorSpec const& spec = Indicators[(uint8_t)ind];
    auto val_ptr              = ((uint8_t const *)&WxAirQ) + spec.offset;
    float val                 = *(float const *)(val_ptr);

    int level = 0;
    while (level < aiq_scale_levels && val >= spec.levels[level])
        level++;

    switch (level)
    {
        case 0: drawGrayscaleImage(ImgGaugeFrame0_info(x, y)); break;
        case 1: drawGrayscaleImage(ImgGaugeFrame1_info(x, y)); break;
        case 2: drawGrayscaleImage(ImgGaugeFrame2_info(x, y)); break;
        case 3: drawGrayscaleImage(ImgGaugeFrame3_info(x, y)); break;
        case 4: drawGrayscaleImage(ImgGaugeFrame4_info(x, y)); break;
        default: break;
    }

    constexpr auto gauge_center_offset_x = ImgGaugeFrame0_width / 2;
    constexpr auto gauge_txt_baseline_y  = 17 + 10;
    constexpr auto txt_subscript_shift_y = 8;

    setFont(OpenSans8B);
    int dw = drawString(x + gauge_center_offset_x, y + gauge_txt_baseline_y, spec.main, CENTER);
    if (spec.sub)
    {
        setFont(OpenSans6B);
        drawString(x + gauge_center_offset_x + dw / 2, y + gauge_txt_baseline_y + txt_subscript_shift_y, spec.sub,
                   LEFT);
    }

    setFont(OpenSans10B);
    // all other indicators range < 1000, CO reaches 20 000, so scale it so it fits in 3 digits
    if (ind == AQIndicator::CO) val /= 100.0;
    int32_t disp_val = round(val);
    if (disp_val > 999) disp_val = 999;
    drawString(x + ImgGaugeFrame0_width + 3, y + 23 - 12, String(disp_val), LEFT);
}

void DisplayAQI(int x, int y)
{
    drawGrayscaleImage(ImgAIQ_info(x, y));
    setFont(OpenSans10B);
    // OWM's api uses 1=best...5=worst and we use x/5 scale, so we need to reverse the output
    int q = 6 - WxAirQ.AQI;
    drawString(x + ImgAIQ_width + 3, y + 10, String(q) + "/5", LEFT);
}

void DisplayAirQualitySection(int x, int y)
{
    constexpr auto aiq_w   = 85;
    constexpr auto gauge_w = 100;

    DisplayAQI(x, y + 1);

    DrawGauge(x + +aiq_w, y, AQIComponents[0]);
    DrawGauge(x + aiq_w + gauge_w, y, AQIComponents[1]);
    DrawGauge(x + aiq_w + 2 * gauge_w, y, AQIComponents[2]);
}
