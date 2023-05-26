/**
 * @file RUI3-Modular.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 based code for low power practice
 * @version 0.1
 * @date 2023-03-29
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app.h"

/** Packet is confirmed/unconfirmed (Set with AT commands) */
bool g_confirmed_mode = false;
/** If confirmed packet, number or retries (Set with AT commands) */
uint8_t g_confirmed_retry = 0;
/** Data rate  (Set with AT commands) */
uint8_t g_data_rate = 3;

/** Flag if transmit is active, used by some sensors */
volatile bool tx_active = false;

/** fPort to send packages */
uint8_t set_fPort = 2;

/** Payload buffer */
CayenneLPP g_solution_data(255);

volatile status_val last_status = wm_off;

/**
 * @brief Callback after join request cycle
 *
 * @param status Join result
 */
void joinCallback(int32_t status)
{
	// MYLOG("JOIN-CB", "Join result %d", status);
	if (status != 0)
	{
		// MYLOG("JOIN-CB", "LoRaWan OTAA - join fail! \r\n");
		// To be checked if this makes sense
		// api.lorawan.join();
	}
	else
	{
		// MYLOG("JOIN-CB", "LoRaWan OTAA - joined! \r\n");
		digitalWrite(LED_BLUE, LOW);
	}
}

/**
 * @brief LoRaWAN callback after packet was received
 *
 * @param data Structure with the received data
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	// MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
	// for (int i = 0; i < data->BufferSize; i++)
	// {
	// 	Serial.printf("%02X", data->Buffer[i]);
	// }
	// Serial.print("\r\n");
	tx_active = false;
}

/**
 * @brief LoRaWAN callback after TX is finished
 *
 * @param status TX status
 */
void sendCallback(int32_t status)
{
	// MYLOG("TX-CB", "TX status %d", status);
	digitalWrite(LED_BLUE, LOW);
	tx_active = false;
}

/**
 * @brief LoRa P2P callback if a packet was received
 *
 * @param data
 */
void recv_cb(rui_lora_p2p_recv_t data)
{
	// MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
	// for (int i = 0; i < data.BufferSize; i++)
	// {
	// 	Serial.printf("%02X", data.Buffer[i]);
	// }
	// Serial.print("\r\n");
	tx_active = false;
}

/**
 * @brief LoRa P2P callback if a packet was sent
 *
 */
void send_cb(void)
{
	// MYLOG("TX-P2P-CB", "P2P TX finished");
	digitalWrite(LED_BLUE, LOW);
	tx_active = false;
}

/**
 * @brief Arduino setup, called once after reboot/power-up
 *
 */
void setup()
{
	// Setup for LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		g_confirmed_mode = api.lorawan.cfm.get();

		g_confirmed_retry = api.lorawan.rety.get();

		g_data_rate = api.lorawan.dr.get();

		// Setup the callbacks for joined and send finished
		api.lorawan.registerRecvCallback(receiveCallback);
		api.lorawan.registerSendCallback(sendCallback);
		api.lorawan.registerJoinCallback(joinCallback);
	}
	else // Setup for LoRa P2P
	{
		api.lorawan.registerPRecvCallback(recv_cb);
		api.lorawan.registerPSendCallback(send_cb);
	}

	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);

	// Start Serial
	Serial.begin(115200);

	// Delay for 5 seconds to give the chance for AT+BOOT
	delay(5000);

	// Initialize custom AT commands
	init_interval_at();
	init_status_at();
	init_timeout_at();

	// Get saved send interval settings
	get_at_setting(true);

	// Bug in RUI3 keeps old firmware version from user settings
	api.system.firmwareVersion.set(String(sw_version));

	Serial.println("RAKwireless Node");
	Serial.println("------------------------------------------------------");
	Serial.printf("Ver %s\n", api.system.firmwareVersion.get().c_str());
	Serial.println("------------------------------------------------------");

	// Initialize module
	Wire.begin();
	digitalWrite(LED_GREEN, LOW);

	// Initialize the accelerometer
	if (init_rak1904())
	{
		Serial.println("+EVT: RAK1904 OK");
	}

	uint8_t rbuff[18];

	// if (service_nvm_get_sn_from_nvm(rbuff, 18) == UDRV_RETURN_OK)
	// {
	// 	for (int i = 0; i < 18; i++)
	// 	{
	// 		Serial.printf("%c", rbuff[i]);
	// 	}
	// }
	// Serial.println();

	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	if (g_send_repeat_time != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, g_send_repeat_time, NULL);
	}

	// Check if it is LoRa P2P
	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);

		sensor_handler(NULL);
	}

	// if (api.lorawan.nwm.get() == 1)
	// {
	// 	if (g_confirmed_mode)
	// 	{
	// 		MYLOG("SETUP", "Confirmed");
	// 	}
	// 	else
	// 	{
	// 		MYLOG("SETUP", "Unconfirmed");
	// 	}

	// 	MYLOG("SETUP", "Retry = %d", g_confirmed_retry);

	// 	MYLOG("SETUP", "DR = %d", g_data_rate);
	// }

	api.system.lpm.set(1);
}

/**
 * @brief sensor_handler is a timer function called every
 * g_send_repeat_time milliseconds. Default is 120000. Can be
 * changed in main.h
 *
 */
void sensor_handler(void *)
{
	if (last_status == wm_started)
	{
		digitalWrite(LED_GREEN, HIGH);
		MYLOG("UPLINK", "Vibration Start");
		api.system.timer.stop(RAK_TIMER_0);
	}

	if (last_status == wm_finished)
	{
		last_status = wm_off;
		api.system.timer.stop(RAK_TIMER_1);
		digitalWrite(LED_GREEN, LOW);
		MYLOG("UPLINK", "Vibration Finished");
		if (g_send_repeat_time != 0)
		{
			// Start a timer.
			api.system.timer.start(RAK_TIMER_0, g_send_repeat_time, NULL);
		}
	}

	MYLOG("UPLINK", "Start");
	digitalWrite(LED_BLUE, HIGH);

	if (api.lorawan.nwm.get() == 1)
	{
		// Check if the node has joined the network
		if (!api.lorawan.njs.get())
		{
			MYLOG("UPLINK", "Skip");
			digitalWrite(LED_BLUE, LOW);
			return;
		}
	}

	// Clear payload
	g_solution_data.reset();

	// Add battery voltage
	g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());

	g_solution_data.addPresence(LPP_CHANNEL_ACC, last_status == wm_started ? true : false);

	// Send the packet
	send_packet();
}

/**
 * @brief This example is complete timer
 * driven. The loop() does nothing than
 * sleep.
 *
 */
void loop()
{
	api.system.sleep.all();
	// api.system.scheduler.task.destroy();
}

/**
 * @brief Send the data packet that was prepared in
 * Cayenne LPP format by the different sensor and location
 * aqcuision functions
 *
 */
void send_packet(void)
{
	// Check if it is LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		// Send the packet
		if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), set_fPort, g_confirmed_mode, g_confirmed_retry))
		{
			MYLOG("LPW", "Enqueued");
			tx_active = true;
		}
		else
		{
			MYLOG("LPW", "Failed");
			tx_active = false;
		}
	}
	// It is P2P
	else
	{
		digitalWrite(LED_BLUE, LOW);

		if (api.lorawan.psend(g_solution_data.getSize(), g_solution_data.getBuffer()))
		{
			MYLOG("P2P", "Enqueued");
		}
		else
		{
			MYLOG("P2P", "Failed");
		}
	}
}
