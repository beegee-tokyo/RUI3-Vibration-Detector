/**
 * @file RAK1904_acc.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialize and read values from the LIS3DH sensor
 * @version 0.1
 * @date 2022-04-11
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"

//******************************************************************//
// RAK1904 INT1_PIN
//******************************************************************//
// Slot A      WB_IO1
// Slot B      WB_IO2 ( not recommended, pin conflict with IO2)
// Slot C      WB_IO3
// Slot D      WB_IO5
// Slot E      WB_IO4
// Slot F      WB_IO6
//******************************************************************//

/** Interrupt pin, depends on slot */
uint8_t acc_int_pin = WB_IO5;

// Forward declarations
void int_callback_rak1904(void);

/** Sensor instance using Wire */
LIS3DH myIMU(I2C_MODE, 0x18);

/**
 * @brief Timer callback for machine timeout
 * 		Called if no motion was detected for 5 minutes
 */
void machine_timeout(void *)
{
	MYLOG("ACC","Timeout");
	last_status = wm_finished;
	sensor_handler(NULL);
}

/**
 * @brief Initialize LIS3DH 3-axis
 * acceleration sensor
 *
 * @return true If sensor was found and is initialized
 * @return false If sensor initialization failed
 */
bool init_rak1904(void)
{
	// Setup interrupt pin
	pinMode(acc_int_pin, INPUT);

	Wire.begin();

	myIMU.settings.accelSampleRate = 10; // Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
	myIMU.settings.accelRange = 2;		 // Max G force readable.  Can be: 2, 4, 8, 16

	myIMU.settings.adcEnabled = 0;
	myIMU.settings.tempEnabled = 0;
	myIMU.settings.xAccelEnabled = 1;
	myIMU.settings.yAccelEnabled = 1;
	myIMU.settings.zAccelEnabled = 1;

	// Call .begin() to configure the IMU
	status_t result = myIMU.begin();

	if (result != IMU_SUCCESS)
	{
		return false;
	}

	uint8_t data_to_write = 0;
	data_to_write |= 0x20;								 // Z high
	data_to_write |= 0x08;								 // Y high
	data_to_write |= 0x02;								 // X high
	myIMU.writeRegister(LIS3DH_INT1_CFG, data_to_write); // Enable interrupts on high tresholds for x, y and z

	// Set interrupt trigger range
	data_to_write = 0;
	data_to_write |= 0x2; // A lower threshold for vibration detection
	myIMU.writeRegister(LIS3DH_INT1_THS, data_to_write); // 1/8th range

	// Set interrupt signal length
	data_to_write = 0;
	data_to_write |= 0x01; // 1 * 1/50 s = 20ms
	myIMU.writeRegister(LIS3DH_INT1_DURATION, data_to_write);

	myIMU.readRegister(&data_to_write, LIS3DH_CTRL_REG5);
	data_to_write &= 0xF3;								  // Clear bits of interest
	data_to_write |= 0x08;								  // Latch interrupt (Cleared by reading int1_src)
	myIMU.writeRegister(LIS3DH_CTRL_REG5, data_to_write); // Set interrupt to latching

	// Select interrupt pin 1
	data_to_write = 0;
	data_to_write |= 0x40; // AOI1 event (Generator 1 interrupt on pin 1)
	data_to_write |= 0x20; // AOI2 event ()
	myIMU.writeRegister(LIS3DH_CTRL_REG3, data_to_write);

	// No interrupt on pin 2
	myIMU.writeRegister(LIS3DH_CTRL_REG6, 0x00);

	// Enable high pass filter
	myIMU.writeRegister(LIS3DH_CTRL_REG2, 0x01);

	// Set low power mode
	data_to_write = 0;
	myIMU.readRegister(&data_to_write, LIS3DH_CTRL_REG1);
	data_to_write |= 0x08;
	myIMU.writeRegister(LIS3DH_CTRL_REG1, data_to_write);
	delay(100);
	data_to_write = 0;
	myIMU.readRegister(&data_to_write, 0x1E);
	data_to_write |= 0x90;
	myIMU.writeRegister(0x1E, data_to_write);
	delay(100);

	clear_int_rak1904();

	// Set the interrupt callback function
	attachInterrupt(acc_int_pin, int_callback_rak1904, RISING);

	// Create a timer to detect end of washing cycle.
	api.system.timer.create(RAK_TIMER_1, machine_timeout, RAK_TIMER_ONESHOT);

	// Get the saved machine timeout
	get_at_setting(false);

	return true;
}

/**
 * @brief Reads the values from the LIS3DH sensor
 *
 */
void read_rak1904(void)
{
	MYLOG("ACC", "x %f y %f z %f g", myIMU.readFloatAccelX(), myIMU.readFloatAccelY(), myIMU.readFloatAccelZ());
}

/**
 * @brief ACC interrupt handler
 * @note gives semaphore to wake up main loop
 *
 */
void int_callback_rak1904(void)
{
	MYLOG("ACC", "IRQ");
	read_rak1904();

	if (last_status == wm_off)
	{
		last_status = wm_started;
		// Start the timer.
		api.system.timer.start(RAK_TIMER_1, g_timeout, NULL);

		// Report start of washing cycle
		sensor_handler(NULL);
	}
	else if (last_status == wm_started)
	{
		// restart the timer
		api.system.timer.stop(RAK_TIMER_1);
		api.system.timer.start(RAK_TIMER_1, g_timeout, NULL);
		MYLOG("ACC","Timeout reset");
	}
	clear_int_rak1904();
}

/**
 * @brief Clear ACC interrupt register to enable next wakeup
 *
 */
void clear_int_rak1904(void)
{
	uint8_t dataRead;
	myIMU.readRegister(&dataRead, LIS3DH_INT1_SRC); // cleared by reading
	// attachInterrupt(acc_int_pin, int_callback_rak1904, RISING);
}
