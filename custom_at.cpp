/**
 * @file custom_at.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief
 * @version 0.1
 * @date 2022-05-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"

// Forward declarations
int interval_send_handler(SERIAL_PORT port, char *cmd, stParam *param);
int timeout_handler(SERIAL_PORT port, char *cmd, stParam *param);
int status_handler(SERIAL_PORT port, char *cmd, stParam *param);

/** Time interval to send packets in milliseconds */
uint32_t g_send_repeat_time = 60000;
/** Timeout for machine inactive detection */
uint32_t g_timeout = 30000;

/**
 * @brief Add send interval AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_interval_at(void)
{
	return api.system.atMode.add((char *)"SENDINT",
								 (char *)"Set/Get interval sending time",
								 (char *)"SENDINT", interval_send_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for send interval AT commands
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int interval_send_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		Serial.print(cmd);
		Serial.printf("=%ld\r\n", g_send_repeat_time / 1000);
	}
	else if (param->argc == 1)
	{
		// MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		for (int i = 0; i < strlen(param->argv[0]); i++)
		{
			if (!isdigit(*(param->argv[0] + i)))
			{
				// MYLOG("AT_CMD", "%d is no digit", i);
				return AT_PARAM_ERROR;
			}
		}

		uint32_t new_send_freq = strtoul(param->argv[0], NULL, 10);

		// MYLOG("AT_CMD", "Requested interval %ld", new_send_freq);

		g_send_repeat_time = new_send_freq * 1000;

		// MYLOG("AT_CMD", "New interval %ld", g_send_repeat_time);
		// Stop the timer
		api.system.timer.stop(RAK_TIMER_0);
		if (g_send_repeat_time != 0)
		{
			// Restart the timer
			api.system.timer.start(RAK_TIMER_0, g_send_repeat_time, NULL);
		}
		// Save custom settings
		save_at_setting(true);
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add timeout AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_timeout_at(void)
{
	return api.system.atMode.add((char *)"TOUT",
								 (char *)"Set/Get timeout time",
								 (char *)"TOUT", timeout_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for timeout AT commands
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int timeout_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		Serial.print(cmd);
		Serial.printf("=%ld\r\n", g_timeout / 1000);
	}
	else if (param->argc == 1)
	{
		// MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		for (int i = 0; i < strlen(param->argv[0]); i++)
		{
			if (!isdigit(*(param->argv[0] + i)))
			{
				// MYLOG("AT_CMD", "%d is no digit", i);
				return AT_PARAM_ERROR;
			}
		}

		uint32_t new_timeout = strtoul(param->argv[0], NULL, 10);

		// MYLOG("AT_CMD", "Requested interval %ld", new_send_freq);

		g_timeout = new_timeout * 1000;

		// MYLOG("AT_CMD", "New interval %ld", g_send_repeat_time);
		// Stop the timer
		api.system.timer.stop(RAK_TIMER_1);
		// Save custom settings
		save_at_setting(false);
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add custom Status AT commands
 *
 * @return true AT commands were added
 * @return false AT commands couldn't be added
 */
bool init_status_at(void)
{
	return api.system.atMode.add((char *)"STATUS",
								 (char *)"Get device status",
								 (char *)"STATUS", status_handler,
								 RAK_ATCMD_PERM_READ);
}

/** Network modes as text array*/
char *nwm_list[] = {"P2P", "LoRaWAN", "FSK"};

/**
 * @brief Print device status over Serial
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int status_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	String value_str = "";
	int nw_mode = 0;
	int region_set = 0;
	uint8_t key_eui[16] = {0}; // efadff29c77b4829acf71e1a6e76f713

	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		Serial.println("Device Status:");
		value_str = api.system.hwModel.get();
		value_str.toUpperCase();
		Serial.printf("%s\r\n", value_str.c_str());
		Serial.printf("%s\r\n", api.system.firmwareVer.get().c_str());
		Serial.printf("Interval: %d s\r\n", g_send_repeat_time / 1000);
		nw_mode = api.lorawan.nwm.get();
		Serial.printf("%s\r\n", nwm_list[nw_mode]);
		if (nw_mode == 1)
		{
			Serial.printf("%s\r\n", api.lorawan.njs.get() ? "Joined" : "Not joined");
			region_set = api.lorawan.band.get();
			Serial.printf("Band: %d\r\n", region_set);
			if (api.lorawan.njm.get())
			{
				Serial.println("OTAA");
				api.lorawan.deui.get(key_eui, 8);
				Serial.printf("DevEUI %02X%02X%02X%02X%02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
							  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
				api.lorawan.appeui.get(key_eui, 8);
				Serial.printf("AppEUI %02X%02X%02X%02X%02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
							  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
				api.lorawan.appkey.get(key_eui, 16);
				Serial.printf("AppKey %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
							  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
							  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
							  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
			}
			else
			{
				Serial.println("ABP");
				api.lorawan.appskey.get(key_eui, 16);
				Serial.printf("AppsKey %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
							  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
							  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
							  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
				api.lorawan.nwkskey.get(key_eui, 16);
				Serial.printf("NwsKey %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
							  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
							  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
							  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
				api.lorawan.daddr.set(key_eui, 4);
				Serial.printf("DevAddr %02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3]);
			}
		}
		else if (nw_mode == 0)
		{
			Serial.printf("Freq = %d\r\n", api.lorawan.pfreq.get());
			Serial.printf("SF = %d\r\n", api.lorawan.psf.get());
			Serial.printf("BW = %d\r\n", api.lorawan.pbw.get());
			Serial.printf("CR = %d\r\n", api.lorawan.pcr.get());
			Serial.printf("PL = %d\r\n", api.lorawan.ppl.get());
			Serial.printf("TX pwr = %d\r\n", api.lorawan.ptp.get());
		}
		else
		{
			Serial.printf("Freq = %d\r\n", api.lorawan.pfreq.get());
			Serial.printf("BR = %d\r\n", api.lorawan.pbr.get());
			Serial.printf("Dev = %d\r\n", api.lorawan.pfdev.get());
		}
	}
	else
	{
		return AT_PARAM_ERROR;
	}
	return AT_OK;
}

/**
 * @brief Get setting from flash
 *
 * @return false read from flash failed or invalid settings type
 */
bool get_at_setting(bool get_timeout)
{
	uint8_t flash_value[16];
	uint32_t offset = SEND_FREQ_OFFSET;
	if (get_timeout)
	{
		offset = SEND_FREQ_OFFSET;
	}
	else
	{
		offset = TIMEOUT_OFFSET;
	}

	if (!api.system.flash.get(offset, flash_value, 5))
	{
		// MYLOG("AT_CMD", "Failed to read send interval from Flash");
		return false;
	}
	if (flash_value[4] != 0xAA)
	{
		// MYLOG("AT_CMD", "No valid send interval found, set to default, read 0X%02X 0X%02X 0X%02X 0X%02X",
		// 	  flash_value[0], flash_value[1],
		// 	  flash_value[2], flash_value[3]);
		if (get_timeout)
		{
			g_send_repeat_time = 0;
		}
		else
		{
			g_timeout = 30000;
		}
		save_at_setting(get_timeout);
		return false;
	}
	MYLOG("AT_CMD", "Read send interval 0X%02X 0X%02X 0X%02X 0X%02X from %d",
		  flash_value[0], flash_value[1],
		  flash_value[2], flash_value[3], offset);
	if (get_timeout)
	{
		g_send_repeat_time = 0;
		g_send_repeat_time |= flash_value[0] << 0;
		g_send_repeat_time |= flash_value[1] << 8;
		g_send_repeat_time |= flash_value[2] << 16;
		g_send_repeat_time |= flash_value[3] << 24;
		MYLOG("AT_CMD", "Send interval found %ld", g_send_repeat_time);
	}
	else
	{
		g_timeout = 0;
		g_timeout |= flash_value[0] << 0;
		g_timeout |= flash_value[1] << 8;
		g_timeout |= flash_value[2] << 16;
		g_timeout |= flash_value[3] << 24;
		MYLOG("AT_CMD", "Timeout found %ld", g_timeout);
	}
	return true;
}

/**
 * @brief Save setting to flash
 *
 * @return true write to flash was successful
 * @return false write to flash failed or invalid settings type
 */
bool save_at_setting(bool set_timeout)
{
	uint8_t flash_value[16] = {0};
	bool wr_result = false;
	uint32_t offset = SEND_FREQ_OFFSET;
	if (set_timeout)
	{
		offset = SEND_FREQ_OFFSET;
		flash_value[0] = (uint8_t)(g_send_repeat_time >> 0);
		flash_value[1] = (uint8_t)(g_send_repeat_time >> 8);
		flash_value[2] = (uint8_t)(g_send_repeat_time >> 16);
		flash_value[3] = (uint8_t)(g_send_repeat_time >> 24);
		flash_value[4] = 0xAA;
	}
	else
	{
		offset = TIMEOUT_OFFSET;
		flash_value[0] = (uint8_t)(g_timeout >> 0);
		flash_value[1] = (uint8_t)(g_timeout >> 8);
		flash_value[2] = (uint8_t)(g_timeout >> 16);
		flash_value[3] = (uint8_t)(g_timeout >> 24);
		flash_value[4] = 0xAA;
	}

	MYLOG("AT_CMD", "Writing time 0X%02X 0X%02X 0X%02X 0X%02X to %d",
		  flash_value[0], flash_value[1],
		  flash_value[2], flash_value[3], offset);
	wr_result = api.system.flash.set(offset, flash_value, 5);
	if (!wr_result)
	{
		// Retry
		wr_result = api.system.flash.set(offset, flash_value, 5);
	}
	wr_result = true;
	return wr_result;
}
