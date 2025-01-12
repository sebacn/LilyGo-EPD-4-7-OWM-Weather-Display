#ifndef _logInfo_h
#define _logInfo_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <InfluxDbClient.h>
#include "ca_cert.h"
//#include "view.h"
#include "api_request.h"
#include "forecast_record.h"

#define NOT_SET_MODE 0
#define CONFIG_MODE 1
#define VALIDATING_MODE 2
#define OPERATING_MODE 3

extern String dbgPrintln(String _str);
extern String infoPrintln(String _str);
#define max_readings 24 
extern Forecast_record_type  WxConditions[1];
extern Forecast_record_type  WxForecast[max_readings];

// Server certificate in PEM format, placed in the program (flash) memory to save RAM
constexpr char const *ROOT_CA_INFLUXDB = ROOT_CA_POSITIONSTACK;

const char MTLS_CERT[] PROGMEM =  R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----)EOF";

const char MTLS_PKEY[] PROGMEM =  R"EOF(
-----BEGIN PRIVATE KEY-----
-----END PRIVATE KEY-----)EOF";

struct LogSettings {
    // InfluxDB 2 server or cloud url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
    String INFLUXDB_URL; // = "influxdb-url"
    // InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
    String INFLUXDB_TOKEN; // "token"
    // InfluxDB 2 organization name or id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
    String INFLUXDB_ORG; // "org"
    // InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
    String INFLUXDB_BUCKET; // "bucket"

    void print()
    {
        dbgPrintln("INFLUXDB_URL: " + INFLUXDB_URL);
        dbgPrintln("INFLUXDB_BUCKET: " + INFLUXDB_BUCKET);
        dbgPrintln("INFLUXDB_ORG: " + INFLUXDB_ORG);
        dbgPrintln("INFLUXDB_TOKEN: " + INFLUXDB_TOKEN);
    }
        
};

struct LogInfo {
    int BootCount;
    long UTCTimestamp;
    int BatteryPct;
    float BatteryVoltage;
    //int BatteryVref;
    bool ConfigOk;
    int Mode;
    bool TimeFetchOk; 
    bool WeatherFetchOk; 
    bool AQIFetchOk; 

    String mode2String()
    {
        String ret;

        switch (Mode)
        {
            case CONFIG_MODE:
            ret = "CONFIG_MODE";
            break;

            case VALIDATING_MODE:
            ret = "VALIDATING_MODE";
            break;

            case OPERATING_MODE:
            ret = "OPERATING_MODE";
            break;
        
        default:
            ret = "NOT_SET_MODE";
            break;
        }

        return ret;
    }
} ;

LogSettings logSettings;
LogInfo logInfo;

InfluxDBClient client;
// Single InfluxDB instance

void writeLogInfo(){

    dbgPrintln("InfluxDBClient: writeLogInfo");

    if (   logSettings.INFLUXDB_URL == ""
        || logSettings.INFLUXDB_ORG == ""
        || logSettings.INFLUXDB_BUCKET == ""
        || logSettings.INFLUXDB_TOKEN == "")
    {
        dbgPrintln("InfluxDBClient: missing configuration");
        return;
    }

    client.setConnectionParams(logSettings.INFLUXDB_URL, logSettings.INFLUXDB_ORG, logSettings.INFLUXDB_BUCKET, logSettings.INFLUXDB_TOKEN, ROOT_CA_INFLUXDB, MTLS_CERT, MTLS_PKEY);

    Point pointDevice("LilyGo-T5-4.7-weather-wifi");
    pointDevice.addTag("Mode", logInfo.mode2String());
    pointDevice.addTag("Config_Ok", String(logInfo.ConfigOk));
    pointDevice.addTag("Time_OK", String(logInfo.TimeFetchOk));
    pointDevice.addTag("OWM_Ok", String(logInfo.WeatherFetchOk));
    pointDevice.addTag("AQI_OK", String(logInfo.AQIFetchOk));

    dbgPrintln("InfluxDBClient UTCTimestamp (s): " + String(logInfo.UTCTimestamp));
    if (logInfo.UTCTimestamp > 0)
    {        
        pointDevice.setTime(logInfo.UTCTimestamp); //Unix timestamp WritePrecision::S
    }
    
    pointDevice.addField("DEV_BootCnt", logInfo.BootCount);
    pointDevice.addField("DEV_BatteryPct", logInfo.BatteryPct);
    pointDevice.addField("DEV_BatteryVoltage", logInfo.BatteryVoltage);   
    //pointDevice.addField("DEV_BatteryVref", logInfo.BatteryVref); 

    if (logInfo.WeatherFetchOk)
    {
        pointDevice.addField("OWM_TempC", WxConditions[0].Temperature);
        pointDevice.addField("OWM_TempFeelC", WxConditions[0].FeelsLike); 
        pointDevice.addField("OWM_Pressure", WxConditions[0].Pressure); 
        pointDevice.addField("OWM_WindSpeed", WxConditions[0].Windspeed); 
        pointDevice.addField("OWM_WindDir", WxConditions[0].Winddir); 
        pointDevice.addField("OWM_Humidity", WxConditions[0].Humidity);
        pointDevice.addField("OWM_DewPoint", WxConditions[0].DewPoint);
        pointDevice.addField("OWM_Cloudcover", WxConditions[0].Cloudcover);
        pointDevice.addField("OWM_Visibility", WxConditions[0].Visibility);
        pointDevice.addField("OWM_Rainfall", WxForecast[0].Rainfall);
        pointDevice.addField("OWM_Snowfall", WxForecast[0].Snowfall);
    }

    if (logInfo.AQIFetchOk)
    {
        pointDevice.addField("AQI_PM25", WxAirQ.PM2_5); 
        pointDevice.addField("AQI_PM10", WxAirQ.PM10); 
        pointDevice.addField("AQI_NO2", WxAirQ.NO2); 
        pointDevice.addField("AQI_O3", WxAirQ.O3);
        pointDevice.addField("AQI_SO2", WxAirQ.SO2);
        pointDevice.addField("AQI_CO", WxAirQ.CO);
    }

    client.setInsecure(false);

    // Check server connection
    if (client.validateConnection()) 
    {
        infoPrintln("InfluxDBClient Connected OK to: " + client.getServerUrl());

        client.setWriteOptions(WriteOptions().useServerTimestamp(logInfo.UTCTimestamp == 0).writePrecision(WritePrecision::S));

        // Write point
        if (client.writePoint(pointDevice)) 
        {
            if (!client.isBufferEmpty()) {
                dbgPrintln("InfluxDBClient flushBuffer"); //Write all remaining points to db
                client.flushBuffer();
            }
            infoPrintln("InfluxDBClient write OK");            
        }
        else
        {
            infoPrintln("InfluxDBClient write failed: " + client.getLastErrorMessage());
        }
    } 
    else 
    {
        infoPrintln("InfluxDB connection failed: " + client.getLastErrorMessage());
    }
}

#endif