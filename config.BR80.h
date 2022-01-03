
#define DC_LOCO_ID 1
#define FWD_MOTOR_PIN 22
#define REV_MOTOR_PIN 23
#define FWD_LIGHT_PIN 25
#define REV_LIGHT_PIN 26

#define MARKLIN_LOCO F("MARKLIN"),				\
    NULL ,  \
    new MotorDriver(18        , 19, UNUSED_PIN, UNUSED_PIN, UNUSED_PIN, 2.00, 2000, UNUSED_PIN, TIMER_PROG)

#define MOTOR_SHIELD_TYPE MARKLIN_LOCO

#define IO_NO_HAL // need all pins for other stuff
  
/////////////////////////////////////////////////////////////////////////////////////
//
// The IP port to talk to a WIFI or Ethernet shield.
//
#define IP_PORT 2560

/////////////////////////////////////////////////////////////////////////////////////
//
// NOTE: Only supported on Arduino Mega
// Set to false if you not even want it on the Arduino Mega
//
#define ENABLE_WIFI true

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE WiFi Parameters (only in effect if WIFI is on)
//
//#define DONT_TOUCH_WIFI_CONF
//
// if DONT_TOUCH_WIFI_CONF is set, all WIFI config will be done with
// the <+> commands and this sketch will not change anything over
// AT commands.
//
//#define WIFI_SSID "KTHOPEN"
//#define WIFI_PASSWORD ""
//#define WIFI_SSID "Your network name"
#include "config.WIFI.h"
//#define WIFI_PASSWORD ""
#define WIFI_HOSTNAME "dccex"
#define WIFI_CONNECT_TIMEOUT 14000


//#define ENABLE_ETHERNET true

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE STATIC IP ADDRESS *OR* COMMENT OUT TO USE DHCP
//
//#define IP_ADDRESS { 192, 168, 1, 200 }

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE MAC ADDRESS ARRAY FOR ETHERNET COMMUNICATIONS INTERFACE
//
// Uncomment to use with Ethernet Shields
//
// NOTE: This is not used with ESP8266 WiFi modules.
// 
// #define MAC_ADDRESS {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF }

/////////////////////////////////////////////////////////////////////////////////////
//
// DEFINE LCD SCREEN USAGE BY THE BASE STATION
//
// Note: This feature requires an I2C enabled LCD screen using a PCF8574 based chipset.
//       or one using a Hitachi  HD44780.
//
// To enable, uncomment the line below and make sure only the correct LIB_TYPE line
// is uncommented below to select the library used for your LCD backpack

//#define ENABLE_LCD

#ifdef ENABLE_LCD
    #define LIB_TYPE_PCF8574
	//#define LIB_TYPE_I2C
	// This defines the I2C address for the LCD device
	#define LCD_ADDRESS 0x27 //common defaults are 0x27 and 0x3F

	// This defines the number of columns the LCD device has
	#define LCD_COLUMNS 16

	// This defines the number of lines the LCD device has
	#define LCD_LINES 2
#endif


//#define OLED_DRIVER 128,64


/////////////////////////////////////////////////////////////////////////////////////
//
// Enable custom command filtering
#define ENABLE_CUSTOM_FILTER false

/////////////////////////////////////////////////////////////////////////////////////
//
// Enable custom command filtering
#define ENABLE_CUSTOM_CALLBACK false

/////////////////////////////////////////////////////////////////////////////////////
//
// Enable custom command filtering
#define ENABLE_FREE_MEM_WARNING false
