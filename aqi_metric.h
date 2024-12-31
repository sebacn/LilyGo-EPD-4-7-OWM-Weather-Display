/*
 * Copyright (c) 2024 zahical. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * @file Definition of OWM's Air Quality Index components.
 */
#ifndef _aqi_metric_h
#define _aqi_metric_h

#include <cstdint>
#include <Arduino.h>

/// Air Quality Index components
enum class AQIndicator : uint8_t
{
    /// Sulphur dioxide
    SO2,
    // Nitrogen dioxide
    NO2,
    /// Coarse particulate matter
    PM10,
    /// Fine particles matter
    PM2_5,
    /// Ozone
    O3,
    /// Carbon monoxide
    CO,
    /// Ammonia (not part of OWM's AQI)
    NH3,
    /// Nitrogen monoxide (not part of OWM's AQI)
    NO,
};

typedef struct
{
    int Dt;  // Date and time, Unix timestamp, UTC
    int AQI; // Air Quality Index; 1=Good...5=Very poor

    // all amounts in ug/m^3
    float CO;    // Carbon monoxide
    float NO;    // Nitrogen monoxide
    float NO2;   // Nitrogen dioxide
    float O3;    // Ozone
    float SO2;   // Sulphur dioxide
    float PM2_5; // Fine particles matter
    float PM10;  // Coarse particulate matter
    float NH3;   // Ammonia
} Air_quality_record;

constexpr auto aiq_scale_levels = 4;
AQIndicator AQIComponents[3] = {AQIndicator::PM2_5, AQIndicator::PM10, AQIndicator::NO2};
Air_quality_record WxAirQ;

struct IndicatorSpec
{
    size_t offset;                     // offset of member in Air_quality_record struct
    uint16_t levels[aiq_scale_levels]; // gauge level thresholds
    char const *main;                  // main text
    char const *sub;                   // subscript text
#ifdef __PLATFORMIO_BUILD_DEBUG__
    AQIndicator e_val; // used to check whether Indicators[] and AQIndicator have the same order
#endif
};

#ifdef __PLATFORMIO_BUILD_DEBUG__
#define CVAL(v) .e_val = v
#else
#define CVAL(v)
#endif

// clang-format off
constexpr IndicatorSpec const Indicators[] =
{
    {
      .offset = offsetof(Air_quality_record, SO2),
      .levels = {20, 80, 250 , 350},
      .main = "SO",
      .sub = "2",
      CVAL(AQIndicator::SO2)
    },
    {
      .offset = offsetof(Air_quality_record, NO2),
      .levels = {40, 70, 150, 200},
      .main = "NO",
      .sub = "2",
      CVAL(AQIndicator::NO2)
    },
    {
      .offset = offsetof(Air_quality_record, PM10),
      .levels = {20, 50, 100, 200},
      .main = "PM",
      .sub = "10",
      CVAL(AQIndicator::PM10)
    },
    {
      .offset = offsetof(Air_quality_record, PM2_5),
      .levels = {10, 25, 50, 75},
      .main = "PM",
      .sub = "2.5",
      CVAL(AQIndicator::PM2_5)
    },
    {
      .offset = offsetof(Air_quality_record, O3),
      .levels = {60, 100, 140, 180},
      .main = "O",
      .sub = "3",
      CVAL(AQIndicator::O3)
    },
    {
      .offset = offsetof(Air_quality_record, CO),
      .levels = {4400, 9400, 12400, 15400},
      .main = "CO",
      CVAL(AQIndicator::CO)
    },
    {
      .offset = offsetof(Air_quality_record, NH3),
      .levels = {999, 999, 999, 999},
      .main = "NH",
      .sub = "3",
      CVAL(AQIndicator::NH3)
    },
    {
      .offset = offsetof(Air_quality_record, NO),
      .levels = {999, 999, 999, 999},
      .main = "NO",
      CVAL(AQIndicator::NO)
    },
};

#endif