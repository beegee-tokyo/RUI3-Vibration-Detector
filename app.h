/**
 * @file app.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Application includes, defines and globals
 * @version 0.1
 * @date 2023-04-06
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <Arduino.h>
#include <SparkFunLIS3DH.h>
#include <CayenneLPP.h>

// Debug
// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 1
#endif

#if MY_DEBUG > 0
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
	} while (0);                         \
	delay(100)
#else
#define MYLOG(...)
#endif

// Cayenne LPP channel numbers
#define LPP_CHANNEL_BATT 1 // Base Board
#define LPP_CHANNEL_ACC 48 // RAK1904 ACC interrupt

// Forward declarations
void sensor_handler(void *);
void send_packet(void);
bool init_rak1904(void);
void clear_int_rak1904(void);
extern uint32_t g_send_repeat_time;
extern uint32_t g_timeout;

// State
enum status_val
{
	wm_off = 0,
	wm_started = 1,
	wm_finished = 2
};

// Custom AT commands
bool init_interval_at(void);
bool init_timeout_at(void);
bool init_status_at(void);
bool get_at_setting(bool get_timeout);
bool save_at_setting(bool set_timeout);

/** Send Interval offset in flash */
#define SEND_FREQ_OFFSET 0x00000002 // length 4 bytes
/** Timeout offset in flash */
#define TIMEOUT_OFFSET 0x00000008 // length 4 bytes

extern const char *sw_version;

extern volatile status_val last_status;