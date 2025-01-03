#ifndef _config_h
#define _config_h

#include <Arduino.h> 
#include <WiFi.h> 
#include <Preferences.h>
#include <WiFiManager.h> 
#include <rom/rtc.h> 
#include "owm_credentials.h"
#include "epd_driver.h"  
#include "api_request.h" 
#include <ArduinoJson.h> 
#include <HTTPClient.h>
#include "units.h"
#include "aqi_metric.h"

#include "fonts/opensans8b.h"
#include "fonts/opensans10b.h"
#include "fonts/opensans12b.h"
#include "fonts/opensans18b.h"
#include "fonts/opensans24b.h"

#define MEMORY_ID "mem"

#define NOT_SET_MODE 0
#define CONFIG_MODE 1
#define VALIDATING_MODE 2
#define OPERATING_MODE 3

enum alignment {LEFT, RIGHT, CENTER};

int SLEEP_INTERVAL_MIN;
//flag for saving data
bool shouldSaveConfig = false;
bool configOk = false;
bool dnsStarted = false;
extern uint8_t *framebuffer;

Preferences preferences;
DNSServer dnsServer;
Settings settings;
struct GeocodingNominatimRequest location_request;
struct TimeZoneDbRequest datetime_request;
struct WeatherRequest weather_request;


int get_mode();
extern int drawString(int x, int y, String text, alignment align);
uint8_t StartWiFi();
JsonDocument deserialize(WiFiClient& resp_stream, bool is_embeded=false);
void set_mode(int mode);
void set_mode_and_reboot(int mode);
void edp_update();
void setFont(GFXfont const & font);
boolean SetupTime();
void request_render_weather(bool _clearDisplay = false);
void BeginSleep();
//boolean UpdateLocalTime();
//extern bool DecodeWeather(WiFiClient& json, String Type);
void DisplayStatusSection(int x, int y, int rssi);

template<typename T>
T value_or_default(JsonObject jobj, String key, T default_value) {
    if (!jobj[key].is<T>()) {
        return default_value;
    } else {
        return jobj[key];
    }
}


template<typename T>
T nested_value_or_default(JsonObject parent_jobj, String key, String nested_key, T default_value) {
    if (!parent_jobj[key].is<T>()) {
        return default_value;
    } else {
        return parent_jobj[key][nested_key];
    }
}

void print_pt()
{
  printf("\n\nESP32 Partition table:\n\n");

  printf("| Type | Sub |  Offset  |   Size   | Size (b) |       Label      |\n");
  printf("| ---- | --- | -------- | -------- | -------- | ---------------- |\n");
  
  esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (pi != NULL) {
    do {
      const esp_partition_t* p = esp_partition_get(pi);
      printf("|  %02x  | %02x  | 0x%06X | 0x%06X | %8d | %-16s |\r\n", 
        p->type, p->subtype, p->address, p->size, p->size, p->label);
    } while (pi = (esp_partition_next(pi)));
  }

  uint32_t program_size = ESP.getSketchSize();
  uint32_t free_size = ESP.getFlashChipSize();
  uint32_t psram_size = ESP.getPsramSize();
  uint32_t free_sketch_space = ESP.getFreeSketchSpace();

  Serial.println("\nSketch size: " + String(program_size) + "\nFree sketch space: " + String(free_sketch_space) 
    + "\nFlash chip size: " + String(free_size) + "\nPsram size: " + String(psram_size) +"\n\n");
}

String dbgPrintln(String _str = "") {
    String ret = _str == ""? "" : "=== DBG: " + _str;
    Serial.println(ret);
    return ret + "\n";
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  dbgPrintln("Should save config");
  shouldSaveConfig = true;
}

void update_location(GeocodingNominatimResponse& location_resp, JsonObject& jobj) {
    location_resp.lat = jobj["data"][0]["latitude"].as<float>();
    location_resp.lon = jobj["data"][0]["longitude"].as<float>();
    location_resp.label = String(jobj["data"][0]["label"].as<String>()).substring(0, 25); //char*
}

void update_datetime(TimeZoneDbResponse& datetime_resp, JsonObject& jobj) {
    datetime_resp.dt = jobj["timestamp"].as<int>();
    datetime_resp.gmt_offset = jobj["gmtOffset"].as<int>();
    datetime_resp.dst = jobj["dst"].as<int>();
    datetime_resp.formatted = jobj["formatted"].as<String>();
}
/*
void update_current_weather(WeatherResponseHourly& hourly, JsonObject& root) {
    hourly.date_ts = root["current"]["dt"].as<int>();
    hourly.sunr_ts = root["current"]["sunrise"].as<int>();
    hourly.suns_ts = root["current"]["sunset"].as<int>();
    hourly.temp = kelv2cels(root["current"]["temp"].as<float>());
    hourly.feel_t = kelv2cels(root["current"]["feels_like"].as<float>());
    hourly.max_t = kelv2cels(root["daily"][0]["temp"]["max"].as<float>());
    hourly.min_t = kelv2cels(root["daily"][0]["temp"]["min"].as<float>());
    hourly.pressure = root["current"]["pressure"].as<int>();
    hourly.clouds = root["current"]["clouds"].as<int>();
    hourly.wind_bft = wind_ms2bft(root["current"]["wind_speed"].as<float>());
    hourly.wind_deg = root["current"]["wind_deg"].as<int>();
    hourly.icon = String(root["current"]["weather"][0]["icon"].as<String>()).substring(0, 2); //char*
    hourly.descr = root["current"]["weather"][0]["description"].as<String>(); //char*
    hourly.pop = round(root["hourly"][1]["pop"].as<float>() * 100);
    hourly.snow = value_or_default(root["current"], "snow", 0.0f);
    hourly.rain = value_or_default(root["current"], "rain", 0.0f);
}


void update_forecast_weather(WeatherResponseDaily& daily, JsonObject& root, const int day_offset) {
    daily.date_ts = root["daily"][day_offset]["dt"].as<int>();
    daily.max_t = kelv2cels(root["daily"][day_offset]["temp"]["max"].as<float>());
    daily.min_t = kelv2cels(root["daily"][day_offset]["temp"]["min"].as<float>());
    daily.wind_bft = wind_ms2bft(root["daily"][day_offset]["wind_speed"].as<float>());
    daily.wind_deg = root["daily"][day_offset]["wind_deg"].as<int>();
    daily.pop = round(root["daily"][day_offset]["pop"].as<float>() * 100);
    daily.snow = value_or_default(root["daily"][day_offset], "snow", 0.0f);
    daily.rain = value_or_default(root["daily"][day_offset], "rain", 0.0f);
}


void update_percip_forecast(WeatherResponseRainHourly& percip, JsonObject& root, const int hour_offset) {
    percip.date_ts = root["hourly"][hour_offset]["dt"].as<int>();
    percip.pop = round(root["hourly"][hour_offset]["pop"].as<float>() * 100);
    percip.snow = nested_value_or_default(root["hourly"][hour_offset], "snow", "1h", 0.0f);
    percip.rain = nested_value_or_default(root["hourly"][hour_offset], "rain", "1h", 0.0f);
    percip.feel_t = kelv2cels1(root["hourly"][hour_offset]["feels_like"].as<float>());
    percip.icon = String(root["hourly"][hour_offset]["weather"][0]["icon"].as<String>()).substring(0, 2); //char*
}
*/
bool location_handler(WiFiClient& resp_stream, Request request) {
    //const int json_size = 20 * 1024;
    JsonDocument doc = deserialize(resp_stream, true);
    JsonObject api_resp = doc.as<JsonObject>();

    if (api_resp.isNull()) {
        return false;
    }
    location_request = GeocodingNominatimRequest(request);
    GeocodingNominatimResponse& location_resp = location_request.response;
    Serial.print("Geocoding...");
    update_location(location_resp, api_resp);
    location_resp.print();
    return true;
}

bool datetime_handler(WiFiClient& resp_stream, Request request) {
    //const int json_size = 10 * 1024;
    JsonDocument doc = deserialize(resp_stream, true);
    JsonObject api_resp = doc.as<JsonObject>();

    if (api_resp.isNull()) {
        return false;
    }
    datetime_request = TimeZoneDbRequest(request);
    TimeZoneDbResponse& datetime_response = datetime_request.response;
    update_datetime(datetime_response, api_resp);
    datetime_response.print();
    return true;
}

JsonDocument deserialize(WiFiClient& resp_stream, bool is_embeded) {
    // https://arduinojson.org/v6/assistant/
    Serial.print("\nDeserializing json, size: x bytes...");
    JsonDocument doc;//(size);
    DeserializationError error;
    
    if (is_embeded) {
        String stream_as_string = resp_stream.readString();
        int begin = stream_as_string.indexOf('{');
        int end = stream_as_string.lastIndexOf('}');
        Serial.print("\nEmbeded json algorithm obtained document...\n");
        String trimmed_json = stream_as_string.substring(begin, end+1);
        dbgPrintln(trimmed_json);
        error = deserializeJson(doc, trimmed_json);
    } else {
        error = deserializeJson(doc, resp_stream);
    }
    if (error) {
        Serial.print(F("\ndeserialization error:"));
        dbgPrintln(error.c_str());
    } else {
        dbgPrintln("deserialized.");
    }
    dbgPrintln("");
    return doc;
}

bool weather_handler(WiFiClient& resp_stream, Request request) {

    JsonDocument doc = deserialize(resp_stream);
    JsonObject api_resp = doc.as<JsonObject>();

    if (api_resp.isNull()) {
        return false;
    }

    return true;
}

/*
bool weather_handler(WiFiClient& resp_stream, Request request) {


    const int json_size = 35 * 1024;
    JsonDocument doc = deserialize(resp_stream, json_size);
    JsonObject api_resp = doc.as<JsonObject>();

    if (api_resp.isNull()) {
        return false;
    }
    weather_request = WeatherRequest(request);
    WeatherResponseHourly& hourly = weather_request.hourly[0];
    WeatherResponseDaily& next_day = weather_request.daily[0];
    WeatherResponseDaily& second_next_day = weather_request.daily[1];

    update_current_weather(hourly, api_resp);
    hourly.print();
    update_forecast_weather(next_day, api_resp, 1);
    next_day.print();
    update_forecast_weather(second_next_day, api_resp, 2);
    second_next_day.print();

    for (int hour = 0; hour < 5; hour++) {
        int offset = hour + 1;
        update_percip_forecast(weather_request.rain[hour], api_resp, offset);
        weather_request.rain[hour].print();
    }
    
    return true;
}
*/
bool http_request_data(WiFiClientSecure& client, Request request, unsigned int retry=3) {
    
    bool ret_val = false;

    while (!ret_val && retry--) {
        ret_val = true;
        client.stop();
        HTTPClient http;
        Serial.printf("\nHTTP connecting to %s%s [retry left: %s]", request.server.c_str(), request.path.c_str(), String(retry).c_str());
        client.setCACert(request.ROOT_CA);
        http.begin(client, request.server, 443, request.path);
        int http_code = http.GET();
        
        if(http_code == HTTP_CODE_OK) {
            dbgPrintln("\nHTTP connection established");
            if (!request.handler(http.getStream(), request)) {
                ret_val = false;
            }
        } else {
            Serial.printf("\nHTTP connection failed %s, error: %s \n\n", String(http_code).c_str(), http.errorToString(http_code).c_str());
            ret_val = false;
        }
        client.stop();
        http.end();
    }
    return ret_val;
}
/*
bool http_request_dataV2(Request request, unsigned int retry=3) {
    
    bool ret_val = false;
    WiFiClient *transport = nullptr;

    if (request.UseHTTPS())
    {
        auto c = new WiFiClientSecure();
        c->setCACert(request.ROOT_CA);
        transport = c;
    }
    else 
    { 
        transport = new WiFiClient{}; 
    }

    request.make_path();

    while (!ret_val && retry--) {
        ret_val = true;
        transport->stop();
        HTTPClient http;
        Serial.printf("\nHTTP (HTTPS: %s) connecting to %s%s [retry left: %s]", String(request.UseHTTPS()).c_str(), request.server.c_str(), request.path.c_str(), String(retry).c_str());
        http.begin(*transport, request.server, request.UseHTTPS()? 443 : 80, request.path);
        int http_code = http.GET();
        
        if(http_code == HTTP_CODE_OK) {
            dbgPrintln("\nHTTP connection established");
            if (!request.handler(http.getStream(), request)) {
                ret_val = false;
            }
        } else {
            Serial.printf("\nHTTP connection failed %s, error: %s \n\n", String(http_code).c_str(), http.errorToString(http_code).c_str());
            ret_val = false;
        }
        transport->stop();
        http.end();
    }
    return ret_val;
}


void disconnect_from_wifi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

bool connect_to_wifi(unsigned int retry=5) {

    int wifi_conn_status = WL_IDLE_STATUS;
    WiFi.mode(WIFI_STA); // Access Point mode off
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);

    while(wifi_conn_status != WL_CONNECTED && retry--) {
        dbgPrintln("\nConnecting to: " + wifiCred.ssid + " [retry left: " + retry +"]");
        unsigned long start = millis();
        wifi_conn_status = WiFi.begin(wifiCred.ssid.c_str(), wifiCred.pass.c_str());
        
        while (true) {
            if (millis() > start + 10000) { // 10s
                break;
            }
            delay(100);
    
            wifi_conn_status = WiFi.status();
            
            if (wifi_conn_status == WL_CONNECTED) {
                dbgPrintln("Wifi connected. IP: " + WiFi.localIP().toString());    
                break;
            } else if(wifi_conn_status == WL_CONNECT_FAILED) {
                dbgPrintln("Wifi failed to connect.");
                break;
            }
        }
        if (wifi_conn_status == WL_CONNECTED) {
            return true;
        }
        delay(2000); // 2sec
    }
    return false;
}
*/

void print_reset_reason(RESET_REASON reason) {
    switch ( reason) {
        case 1 : Serial.print("POWERON_RESET"); break;
        case 3 : Serial.print("SW_RESET"); break;
        case 4 : Serial.print("OWDT_RESET"); break;
        case 5 : Serial.print("DEEPSLEEP_RESET"); break;
        case 6 : Serial.print("SDIO_RESET"); break; 
        case 7 : Serial.print("TG0WDT_SYS_RESET"); break;
        case 8 : Serial.print("TG1WDT_SYS_RESET"); break;
        case 9 : Serial.print("RTCWDT_SYS_RESET"); break;
        case 10 : Serial.print("INTRUSION_RESET"); break;
        case 11 : Serial.print("TGWDT_CPU_RESET"); break;
        case 12 : Serial.print("SW_CPU_RESET"); break;
        case 13 : Serial.print("RTCWDT_CPU_RESET"); break;
        case 14 : Serial.print("EXT_CPU_RESET"); break;
        case 15 : Serial.print("RTCWDT_BROWN_OUT_RESET"); break;
        case 16 : Serial.print("RTCWDT_RTC_RESET"); break;
        default : Serial.print("UNKNOWN");
    }
}


void wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    Serial.print("CPU0 reset reason: ");
    print_reset_reason(rtc_get_reset_reason(0));
    Serial.print(",  CPU1 reset reason: ");
    print_reset_reason(rtc_get_reset_reason(1));
    dbgPrintln();
    
    switch(wakeup_reason){
        //dbgPrintln("Location variable: " + String(curr_loc));
        
        case ESP_SLEEP_WAKEUP_EXT0 : 
            dbgPrintln("\nWakeup by ext signal RTC_IO -> GPIO39"); 
            if (get_mode() == OPERATING_MODE) {
                // Toggle between 2 screens caused by button press WAKE_BTN_PIN
            }        
            break;
            
        case ESP_SLEEP_WAKEUP_EXT1 : 
            dbgPrintln("Wakeup by ext signal RTC_CNTL -> GPIO34"); 
            get_mode();
            set_mode_and_reboot(CONFIG_MODE);
            break;
            
        case ESP_SLEEP_WAKEUP_TIMER : dbgPrintln("Wakeup by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : dbgPrintln("Wakeup by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : dbgPrintln("Wakeup by ULP program"); break;
        default : Serial.printf("Wakeup not caused by deep sleep: %d\n", wakeup_reason); 
            if (rtc_get_reset_reason(0) == POWERON_RESET && rtc_get_reset_reason(1) == EXT_CPU_RESET)
            {
                get_mode();
                set_mode_and_reboot(CONFIG_MODE);
            }
        break;
    }
}

void read_config_from_memory() {
    dbgPrintln("Read config from memory...");

    preferences.begin(MEMORY_ID, true);  // first param true means 'read only'

    settings.WiFiSSID = preferences.getString("WiFiSSID","");
    settings.WiFiPass = preferences.getString("WiFiPass","");
    settings.OwmApikey = preferences.getString("OwmApikey","");     
    settings.City = preferences.getString("City","");                         
    settings.Latitude = preferences.getString("Latitude","");                        
    settings.Longitude = preferences.getString("Longitude","");  
    settings.TimezBBKey = preferences.getString("TimezBBKey","");
    settings.PositionStackKey = preferences.getString("PosStackKey","");
    settings.SleepDuration = preferences.getLong("SleepDuration", 60);
    settings.WakeupHour = preferences.getInt("WakeupHour", 8);
    settings.SleepHour = preferences.getInt("SleepHour", 23); 
    settings.Units = preferences.getString("Units", "M");
    
    preferences.end();

    delay(1000);
    settings.print();
}

void save_config_to_memory() {
    dbgPrintln("Save config to memory.");
    
    preferences.begin(MEMORY_ID, false);  // first param false means 'read/write'

    preferences.putString("WiFiSSID", settings.WiFiSSID);
    preferences.putString("WiFiPass", settings.WiFiPass);
    preferences.putString("OwmApikey", settings.OwmApikey);     
    preferences.putString("City", settings.City);                         
    preferences.putString("Latitude", settings.Latitude);                        
    preferences.putString("Longitude", settings.Longitude);  
    preferences.putString("TimezBBKey", settings.TimezBBKey);
    preferences.putString("PosStackKey", settings.PositionStackKey);
    preferences.putLong("SleepDuration", settings.SleepDuration);
    preferences.putInt("WakeupHour", settings.WakeupHour);
    preferences.putInt("SleepHour", settings.SleepHour); 
    preferences.putString("Units", settings.Units); 

    preferences.end();

    delay(1000);
    settings.print();  
}

void display_validating_mode() {
  
    epd_poweron();      // Switch on EPD display
    epd_clear();        // Clear the screen

    setFont(OpenSans24B);
    drawString(250, 25, "LilyGo T5 4.7 EPD47", LEFT);
    setFont(OpenSans18B);
    drawString(15, 110, "Validate settings..", LEFT);

    DisplayStatusSection(575, 25, -100); 

    edp_update();       // Update the display to show the information
    epd_poweroff_all(); // Switch off all power to EPD
}

void display_config_mode(String network, String pass, String ip) {
  
    epd_poweron();      // Switch on EPD display
    epd_clear();        // Clear the screen

    setFont(OpenSans24B);
    drawString(250, 25, "LilyGo T5 4.7 EPD47", LEFT);
    setFont(OpenSans18B);
    drawString(15, 110, "WiFi Configuration..", LEFT);
    setFont(OpenSans12B);
    drawString(15, 170, "Connect to weather station...\nSSID: " + network + "\nPass: " + pass + "\nURL: http://" + ip , LEFT);

    DisplayStatusSection(575, 25, -100); 

    edp_update();       // Update the display to show the information
    epd_poweroff_all(); // Switch off all power to EPD
}

void display_print_text(GFXfont const & font, int _x, int _y, String _str, bool _clear = false) {
  
    epd_poweron();      // Switch on EPD display

    if (_clear)
    {
       epd_clear();        // Clear the screen 
    }    

    setFont(font);
    drawString(_x, _y, _str, LEFT);

    edp_update();       // Update the display to show the information
    epd_poweroff_all(); // Switch off all power to EPD
}

void run_config_server() {
    struct tm timeinfo;
    String network = "LilyGo-T5-4.7-weather-wifi";
    String pass = String(abs((int)esp_random())).substring(0, 4) + "0000";

    print_pt();

    read_config_from_memory();

    IPAddress localIp(192, 168, 4, 1);
    IPAddress localMask(255, 255, 255, 0);
    WiFi.softAPConfig(localIp, localIp, localMask);

    dnsStarted = dnsServer.start(53, "*", WiFi.softAPIP());
    
    Serial.printf("\nStart config server on ssid: %s, pass: %s, ip: %s \n\n", network.c_str(), pass.c_str(), WiFi.softAPIP().toString());

    display_config_mode(network, pass, WiFi.softAPIP().toString());

    WiFi.mode(WIFI_STA); 

    WiFiManagerParameter parmCity("parmCity", "City", settings.City.c_str(), 15);
    WiFiManagerParameter parmOwmApikey("parmOwmApikey", "Openweathermap api key", settings.OwmApikey.c_str(), 40);
    WiFiManagerParameter parmPositionStackKey("parmPositionStackKey", "PositionStack api key", settings.PositionStackKey.c_str(), 40);    
    WiFiManagerParameter parmTimezdbKey("parmTimezdbKey", "Timezdb key", settings.TimezBBKey.c_str(), 20);
    WiFiManagerParameter parmUnits("parmUnits", "Units (M/I)", settings.Units.c_str(), 3);
    WiFiManagerParameter parmSleepDuration("parmSleepDuration", "Sleep duration (5-60 min)", String(settings.SleepDuration).c_str(), 10);
    WiFiManagerParameter parmWakeupHour("parmWakeupHour", "Wakeup hour", String(settings.WakeupHour).c_str(), 10);
    WiFiManagerParameter parmSleepHour("parmSleepHour", "Sleep hour", String(settings.SleepHour).c_str(), 10);    

    WiFiManager wm;

    wm.addParameter(&parmCity);
    wm.addParameter(&parmOwmApikey);
    wm.addParameter(&parmPositionStackKey);
    wm.addParameter(&parmTimezdbKey);
    wm.addParameter(&parmUnits);
    wm.addParameter(&parmSleepDuration);
    wm.addParameter(&parmWakeupHour);
    wm.addParameter(&parmSleepHour);       

    //wm.setTimeout(120);
    wm.setConfigPortalTimeout(60*5); //5 min
    wm.setSaveConfigCallback(saveConfigCallback);

    bool res = wm.startConfigPortal(network.c_str(), pass.c_str());

    if (!res && !configOk) {

        dbgPrintln("Config: WiFi timeout..");

        if (StartWiFi() == WL_CONNECTED) {
            configTime(0, 0, "0.uk.pool.ntp.org", "time.nist.gov");

            if (getLocalTime(&timeinfo, 5000)) { // Wait for 5-sec for time to synchronise

                char buffer[80];
                strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", &timeinfo);
                display_print_text(OpenSans12B, 15, 330, "UTC Date and time: " + String(buffer));
            }
        }

        display_print_text(OpenSans12B, 15, 370, "Wifi connection timeout..\nConfig validation failed..\nPower off");

        dbgPrintln("Config: power off..");

        epd_poweroff_all();
        //uint64_t sleep_time_micro_sec = 86400000000;//24* 60 * 1000 * 1000 * 60; //24h
        esp_sleep_enable_timer_wakeup(86400000000*30);
        esp_deep_sleep_start(); 

        delay(5000);
        dbgPrintln("Config: Sleep..");
    } 
    else 
    {
        dbgPrintln("Config: WiFi manager exit..");

        display_print_text(OpenSans12B, 15, 330, "Wifi connected OK!");
        delay(2000);

        if (shouldSaveConfig)
        {
            dbgPrintln("Config: WiFi manager save config..");

            settings.WiFiPass = wm.getWiFiPass();
            settings.WiFiSSID = wm.getWiFiSSID();
            settings.City = String(parmCity.getValue());
            settings.OwmApikey = String(parmOwmApikey.getValue());
            settings.PositionStackKey = String(parmPositionStackKey.getValue());
            settings.TimezBBKey = String(parmTimezdbKey.getValue());
            settings.SleepDuration = atoi(parmSleepDuration.getValue());
            settings.WakeupHour = atoi(parmWakeupHour.getValue());
            settings.SleepHour = atoi(parmSleepHour.getValue());            
            
            save_config_to_memory();

            configOk = false; //validate after save
        }   
        
        if (configOk)
        {
            set_mode(OPERATING_MODE);
            display_print_text(OpenSans12B, 15, 350, "Restarting to operation mode (config Ok)..");
            delay(2000);

            if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
                SetupTime();
                request_render_weather(true);
            }
        }
        else
        {
            set_mode(VALIDATING_MODE);
            display_print_text(OpenSans12B, 15, 350, "Restarting to validation mode..");
        } 

        delay(3000);
        ESP.restart();
    }
}


void run_validating_mode() {
    //server.end();
    String keyErrMsg;
    bool checkFailed = false;
        
    read_config_from_memory();
    
    dbgPrintln("Validating mode..");
    display_validating_mode();
    delay(2000);

    configOk = false;
    
    if (StartWiFi() == WL_CONNECTED) {
        
        WiFiClientSecure client;
        //bool is_location_fetched = false;

        dbgPrintln("Wifi connected, validate settings...");

        if (settings.City == "")
        {
          keyErrMsg = "City,";
        }

        if (settings.OwmApikey == "")
        {
          keyErrMsg += "OwmApikey,";
        } 

        if (settings.PositionStackKey == "")
        {
          keyErrMsg += "PositionStackKey,";
        } 

        if (settings.TimezBBKey == "")
        {
          keyErrMsg += "TimezBBKey,";
        }

        if (settings.SleepDuration < 5 || settings.SleepDuration > 60)
        {
          keyErrMsg += "SleepDuration,";
        }

        if (settings.WakeupHour == 0 || settings.WakeupHour > 23)
        {
          keyErrMsg += "WakeupHour,";
        }

        if (settings.SleepHour > 23)
        {
          keyErrMsg += "SleepHour";
        }

        dbgPrintln("Validate key, missing keys: " + (keyErrMsg == ""? "No" : keyErrMsg));

        if (keyErrMsg != "")
        {

          display_print_text(OpenSans12B, 15, 170, "Parameter(s) is/are not configured:\n" + keyErrMsg + "\n" + "Restarting in 10 sec");  

          set_mode(CONFIG_MODE);
          delay(15000);
          ESP.restart();
          return;
        }
        
        dbgPrintln("Validate: get locations by names");
        
        location_request.handler = location_handler;
        location_request.name = settings.City;
        location_request.api_key = settings.PositionStackKey;
        location_request.make_path();

        if (http_request_data(client, location_request))
        {
            settings.Latitude = location_request.response.lat;
            settings.Longitude = location_request.response.lon;

            if (atof(settings.Latitude.c_str()) == 0 
             && atof(settings.Longitude.c_str()) == 0 )
            {
                keyErrMsg += dbgPrintln("Validate: location for " + settings.City + " fetch FAILED (lat:0,lon:0)");
                checkFailed = true;
            }
            else
            {
                dbgPrintln("Validate: location for " + settings.City + " fetched Ok");
            }
            
        }
        else
        {
            keyErrMsg += dbgPrintln("Validate: location " + settings.City + " fetch FAILED");
            checkFailed = true;
        }   

        if (!checkFailed)
        {
            datetime_request.api_key = settings.TimezBBKey;            
            datetime_request.handler = datetime_handler;
            datetime_request.make_path(settings.Latitude, settings.Longitude);

            weather_request.api_key = settings.OwmApikey;
            weather_request.handler = weather_handler;
            weather_request.make_path(settings.Latitude, settings.Longitude, settings.Units);

            bool is_time_fetched = http_request_data(client, datetime_request);
            bool is_weather_fetched = http_request_data(client, weather_request);

            if (!is_time_fetched)
            {
                keyErrMsg += dbgPrintln("Time fetch error\nTIMEZDB_KEY not valid");
                checkFailed = true;
            }

            if (!is_weather_fetched)
            {
                keyErrMsg += dbgPrintln("Weather fetch error\nOPENWEATHER_KEY not valid");
                checkFailed = true;
            }
        } 

        if (checkFailed)
        {
            display_print_text(OpenSans12B, 15, 170, "Validation failed:\n" + keyErrMsg);

            delay(15000);
            set_mode(CONFIG_MODE);
            ESP.restart();
            return;
        } 
    }
    else
    {
        dbgPrintln("Wifi connection failed, reboot to config...");

        display_print_text(OpenSans12B, 15, 170, "Wifi connection failed\nReboot to config...");

        set_mode(CONFIG_MODE);
        delay(15000);
        ESP.restart();
    }

    configOk = true;
    set_mode(OPERATING_MODE);
    save_config_to_memory();
    delay(1000);
    display_print_text(OpenSans12B, 15, 170, "Validation OK!\nReboot to operating mode....");
    delay(3000);

    if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
        SetupTime();
        request_render_weather(true);
    }
    BeginSleep();

    delay(15000);
    dbgPrintln("Deep sleep..");

    //set_mode_and_reboot(OPERATING_MODE);
}

/*
void run_operating_mode() {

    read_config_from_memory();
    curr_loc = read_location_from_memory();
    wakeup_reason();

    if (connect_to_wifi()) {
        WiFiClient client;

        datetime_request.api_key = apiKeys.TIMEZDB_KEY;
        datetime_request.make_path(location[curr_loc]);
        datetime_request.handler = datetime_handler;

        weather_request.api_key = apiKeys.OPENWEATHER_KEY;
        weather_request.make_path(location[curr_loc]);
        weather_request.handler = weather_handler;

        airquality_request.api_key = apiKeys.WAQI_KEY;
        airquality_request.make_path(location[curr_loc]);
        airquality_request.handler = air_quality_handler;

        bool is_time_fetched = http_request_data(client, datetime_request);
        bool is_weather_fetched = http_request_data(client, weather_request);
        bool is_aq_fetched = http_request_data(client, airquality_request);

        view = View();

        update_header_view(view, is_time_fetched); 
        update_weather_view(view, is_weather_fetched);
        update_air_quality_view(view, is_aq_fetched);
            
        dbgPrintln("\nUpdate display.");
        display_header(view);
        display_weather(view);
        display_air_quality(view);
    }

    display.update();
    delay(100); // too fast display powerDown displays blank (white)??

    // deep sleep stuff
    enable_timed_sleep(SLEEP_INTERVAL_MIN);
    begin_deep_sleep();
}
*/

void set_mode(int mode) {

    dbgPrintln("Set mode");

    preferences.begin(MEMORY_ID, false);
    preferences.putInt("mode", mode);    
    preferences.putBool("configOk", configOk);  // global variable
    preferences.end();   

    dbgPrintln("Config OK: " + String(configOk) + ", Mode: " + String(mode)); 

    delay(3000);
}


void set_mode_and_reboot(int mode) {
    set_mode(mode);
    ESP.restart();
}

int get_mode() {

    dbgPrintln("Get mode");

    preferences.begin(MEMORY_ID, true);
    int mode = preferences.getInt("mode", NOT_SET_MODE);
    configOk = preferences.getBool("configOk", false);  // global variable   
    preferences.end();

    dbgPrintln("Config OK: " + String(configOk) + ", Mode: " + String(mode));

    return mode;
}


#endif