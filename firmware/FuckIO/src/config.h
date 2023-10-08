#pragma once

// General Settings:
#define HOSTNAME "fuckio"      // Hostname used for DNS
#define WIFI_AP_NAME "FuckIO"  // Name of the Wifi the AP creates
#define AP_PASSWORD "12345678" // Default password of the AP
#define SERIAL_BAUDS 115200    // Default baud rate for serial communication over USB
#define RSSI_UPDATES 500       // Intervall in ms at which the RSSI value should be send as MQTT message

/**************************************************************************/
/*!
  @brief  Struct to reference all needed pins
*/
/**************************************************************************/
typedef struct
{
    int servoPul;
    int servoDir;
    int servoEna;
    int servoAlm;
    int servoPed;
    int sw;
    int lmt1;
    int rgb;
    int rs232txd;
    int rs232rxd;
} OSSMPins;

/**************************************************************************/
/*!
  @brief  struct to reference other config parameters
*/
/**************************************************************************/
typedef struct
{
    String description;
    OSSMPins pin;
    bool useIHSV57V6Driver;
} OSSMConfig;

/**************************************************************************/
/*!
  @brief  Pin definitions for OSSM Ref Board V2
*/
/**************************************************************************/
static const OSSMPins OSSMv2{
    .servoPul = 14,
    .servoDir = 27,
    .servoEna = 26,
    .servoAlm = 13,
    .servoPed = 4,
    .sw = 23,
    .lmt1 = 12,
    .rgb = 25,
    .rs232txd = 17,
    .rs232rxd = 16};

/**************************************************************************/
/*!
  @brief  Pin definitions for OSSM Ref Board V1
*/
/**************************************************************************/
static const OSSMPins OSSMv1{
    .servoPul = 14,
    .servoDir = 27,
    .servoEna = 26,
    .servoAlm = 39,
    .servoPed = 36,
    .sw = 23,
    .lmt1 = 12,
    .rgb = 25,
    .rs232txd = 17,
    .rs232rxd = 16};

/**************************************************************************/
/*!
  @brief  OSSM configurations with V1 and V2 board and generic stepper or
  IHSV57 servo with firmware v6 for modbus control.
*/
/**************************************************************************/
static const OSSMConfig genericOSSMv2{
    .description = "OSSM Ref Board V2 with Generic Stepper",
    .pin = OSSMv2,
    .useIHSV57V6Driver = false};

static const OSSMConfig genericOSSMv1{
    .description = "OSSM Ref Board V1 with Generic Stepper",
    .pin = OSSMv1,
    .useIHSV57V6Driver = false};

static const OSSMConfig ihsv57v6OSSMv2{
    .description = "OSSM Ref Board V2 with IHSV57 v6 Servo",
    .pin = OSSMv2,
    .useIHSV57V6Driver = true};

static const OSSMConfig ihsv57v6OSSMv1{
    .description = "OSSM Ref Board V1 with IHSV57 v6 Servo",
    .pin = OSSMv1,
    .useIHSV57V6Driver = true};

/**************************************************************************/
/*!
  @brief  Array holding all OSSM configurations
*/
/**************************************************************************/
static const OSSMConfig OSSM[] = {
    genericOSSMv2,
    genericOSSMv1,
    ihsv57v6OSSMv2,
    ihsv57v6OSSMv1};

static const unsigned int sizeOfOSSM = sizeof(OSSM) / sizeof(OSSM[0]);

// // Pin Definitions OSSM Ref Board V2
// #define SERVO_PUL         14
// #define SERVO_DIR         27
// #define SERVO_ENA         26
// #define SERVO_ALM         13  // 39 on OSSM Ref Board V1
// #define SERVO_PED         4   // 36 on OSSM Ref Board V1
// #define SW                23
// #define LMT1              12
// #define RGB               25  //WS2813
// #define RS232_TXD         17
// #define RS232_RXD         16
