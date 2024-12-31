#ifndef _api_request_h
#define _api_request_h

#include <WiFi.h>
#include <time.h>
#include "fmt.h"
#include "config.h"
//#include "api_keys.h"
//#include "config.h"

enum class ApiCall : byte
{
    OneCall  = 0,
    Weather  = 1,
    Forecast = 2,
    AQI      = 3
};

constexpr char const *api_names[] = {"onecall", "weather", "forecast", "air_pollution"};

struct Request;
typedef bool (*ResponseHandler) (WiFiClient& resp_stream, Request request);

struct Request {
    String server = "";
    String api_key = "";
    String path = "";
    ResponseHandler handler;
    char const *ROOT_CA = nullptr;    

    void make_path() {}
    
    String get_server_path() {
        return this->server + this->path;
    }

    bool UseHTTPS()
    {
        return (ROOT_CA == nullptr);
    }
} ;


struct TimeZoneDbResponse {
    time_t dt;
    int gmt_offset;
    int dst;
    String formatted;

    void print() {
        struct tm* ti;  // timeinfo
        ti = localtime(&dt);        
        Serial.printf(
            "Date and time:  %d:%d %d, %d-%d-%d \n", 
            ti->tm_hour, ti->tm_min, ti->tm_wday, 
            ti->tm_mday, ti->tm_mon+1, 1900+ti->tm_year
        );
    }
} ;


struct TimeZoneDbRequest: Request {

    explicit TimeZoneDbRequest(): Request() {
        this->server = "api.timezonedb.com";
        //this->api_key = apiKeys.TIMEZDB_KEY;
    }

    explicit TimeZoneDbRequest(String server, String api_key) {
        this->server = server;
        this->api_key = api_key;
    }
    
    void make_path(String _lat, String _lon) {
        this->path = "/v2.1/get-time-zone?key=" + api_key + "&format=json&by=position&lat=" + _lat + "&lng=" + _lon;
    }

    explicit TimeZoneDbRequest(const Request& request): Request(request) { }

    
    TimeZoneDbResponse response;
} ;


struct AirQualityResponse {
    int pm25;
    
    void print() {
        Serial.println("Air quality (PM2.5): " + String(pm25));
        Serial.println("");
    }
} ;


struct AirQualityRequest: Request {
    
    explicit AirQualityRequest(): Request() {
        this->server = "api.waqi.info";
        //this->api_key = apiKeys.WAQI_KEY;
    } 

    explicit AirQualityRequest(String server, String api_key) {
        this->server = server;
        this->api_key = api_key;
    }
    
    explicit AirQualityRequest(const Request& request): Request(request) { }

    void make_path(String _lat, String _lon) {
        this->path = "/feed/geo:" + _lat + ";" + _lon + "/?token=" + api_key;
    }
    
    AirQualityResponse response;
} ;


struct GeocodingNominatimResponse {
    float lat = 0.0f;
    float lon = 0.0f;
    String label = "";
    
    void print() {
        Serial.println("");
        Serial.println("City: (" + String(lat) + ", " + String(lon) + ") " + label);
        Serial.println("");
    }
} ;


struct GeocodingNominatimRequest: Request {
    String name = "";

    explicit GeocodingNominatimRequest(): Request() {
        this->server = "api.positionstack.com";
        //this->api_key = apiKeys.POSITIONSTACK_KEY;
    }

    explicit GeocodingNominatimRequest(String server, String name) {
        this->server = server;
        this->name = name;
        make_path();
    }
    
    explicit GeocodingNominatimRequest(const Request& request): Request(request) { }

    void make_path() {
        this->path = "/v1/forward?access_key=" + api_key + "&query=" + name;
    }
    
    GeocodingNominatimResponse response;
} ;


struct WeatherResponseHourly {  // current and hourly
    int date_ts;
    int sunr_ts; // sunrise
    int suns_ts; // sunset
    int temp;  // round from float
    int feel_t;  // round from float
    int max_t; // [daily] round from float
    int min_t; // [daily] round from float
    int pressure; 
    int clouds;
    int wind_bft; // round from float to bft int
    int wind_deg; // round from float
    String icon;
    String descr;
    float snow;
    float rain;
    int pop; // [hourly] probability of percipitation hourly round to int percent
    
    void print() {
        char buffer[150];
        Serial.println("Weather currently: " + descr);        
        // 15 * 8 char strings
        sprintf(
            buffer, 
            "%8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s", 
            "date_ts", "sunr_ts", "suns_ts", "temp", "feel_t", 
            "max_t", "min_t", "pressure", "clouds", "wind_bft", 
            "wind_deg", "icon", "snow", "rain", "pop"
        );
        Serial.println(buffer);
        sprintf(
            buffer, 
            "%8s %8s %8s %8d %8d %8d %8d %8d %8d %8d %8d %8s %8.1f %8.1f %8d",
            ts2HM(date_ts), ts2HM(sunr_ts), ts2HM(suns_ts), 
            temp, feel_t, max_t, min_t,
            pressure, clouds, wind_bft, wind_deg,
            icon, snow, rain, pop
        );
        Serial.println(buffer);
    }
} ;


struct WeatherResponseDaily {
    int date_ts;
    int max_t;
    int min_t;
    int wind_bft;
    int wind_deg;
    int pop;
    float snow;
    float rain;

    void print() {
        char buffer[100];
        Serial.print("Forecast: " + ts2dm(date_ts));
        sprintf(
            buffer, 
            "%8s %8s %8s %8s %8s %8s %8s",
            "max_t", "min_t", "wind_bft", 
            "wind_deg", "pop", "snow", "rain"
        );
        Serial.println(buffer);
        Serial.printf("%15s", "");
        sprintf(
            buffer, 
            "%8d %8d %8d %8d %8d %8s %8s",
            max_t, min_t, wind_bft, wind_deg, pop,
            snow? "yes" : "no", rain? "yes" : "no"
        );
        Serial.println(buffer);
    }

} ;


struct WeatherResponseRainHourly {
    int date_ts;
    int pop;
    float feel_t;
    float snow;
    float rain;
    String icon;

    void print() {
        char buffer[60];
        Serial.print("Rain: " + ts2date(date_ts));
        sprintf(
            buffer, 
            "%8s %8s %8s %8s %8s",
            "pop", "snow", "rain", "feel", "icon"
        );
        Serial.println(buffer);
        Serial.printf("%25s", "");
        sprintf(
            buffer, 
            "%8d %8.1f %8.1f %8.1f %8s",
            pop, snow, rain, feel_t, icon
        );
        Serial.println(buffer);
    }
} ;


struct WeatherRequest: Request {
    ApiCall apiCall;

    explicit WeatherRequest(): Request() {
        this->server = "api.openweathermap.org";
        //this->api_key = apiKeys.OPENWEATHER_KEY;
    }
    
    explicit WeatherRequest(String server, String api_key) {
        this->server = server;
        this->api_key = api_key;
    }
    
    explicit WeatherRequest(const Request& request): Request(request) { }

    void make_path(String _lat, String _lon, String _units) {
        
        this->path = "/data/2.5/" + String(api_names[(byte)apiCall]) + "?lat=" + _lat + "&lon=" + _lon + "&appid=" + api_key + "&lang=" + LANGS[LANG]; 

        if (apiCall != ApiCall::AQI){
            this->path += "&mode=json&units=";
            this->path += _units == "M"? "metric" : "imperial" ;
        }

        if (apiCall == ApiCall::OneCall){
            this->path += "&exclude=minutely,hourly,alerts,daily";
        }
    }
    
    WeatherResponseHourly hourly[1];
    WeatherResponseDaily daily[2];
    WeatherResponseRainHourly rain[5];
} ;


String openweather_icons[9] = {
    "01",   // 0 clear sky
    "02",   // 1 few clouds    
    "03",   // 2 scattered clouds
    "04",   // 3 broken clouds
    "09",   // 4 shower rain
    "10",   // 5 rain
    "11",   // 6 thunderstorm
    "13",   // 7 snow
    "50"    // 8 mist
};

constexpr char const *OWM_ROOT_CA =
    R"(-----BEGIN CERTIFICATE-----
MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw
MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV
BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU
aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy
dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK
AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B
3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY
tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/
Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2
VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT
79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6
c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT
Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l
c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee
UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE
Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd
BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G
A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF
Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO
VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3
ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs
8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR
iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze
Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ
XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/
qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB
VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB
L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG
jjxDah2nGN59PRbxYvnKkKj9
-----END CERTIFICATE-----)";

#endif
