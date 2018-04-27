/*
 * LPS22HB.hpp
 *
 *  Created on: Apr 26, 2018
 *      Author: dagar
 */

#pragma once

#include <drivers/device/CDev.hpp>
#include <drivers/device/Device.hpp>
#include <px4_config.h>
#include <px4_workqueue.h>

#include <systemlib/perf_counter.h>

#include <drivers/drv_baro.h>
#include <drivers/drv_hrt.h>
#include <drivers/device/ringbuffer.h>
#include <drivers/drv_device.h>

#include <uORB/uORB.h>

#include <float.h>
#include <getopt.h>

static constexpr uint8_t WHO_AM_I = 0x0F;
static constexpr uint8_t LPS22HB_ID_WHO_AM_I = 0xB1;

static constexpr uint8_t CTRL_REG1 = 0x10;
static constexpr uint8_t CTRL_REG2 = 0x11;
static constexpr uint8_t CTRL_REG3 = 0x12;

#define BOOT		(1 << 7)
#define FIFO_EN		(1 << 6)
#define STOP_ON_FTH	(1 << 5)
#define IF_ADD_INC	(1 << 4)
#define I2C_DIS		(1 << 3)
#define SWRESET		(1 << 2)
#define ONE_SHOT	(1 << 0)

static constexpr uint8_t STATUS = 0x27;

#define T_OR (1 << 5) // Temperature data overrun.
#define P_OR (1 << 4) // Pressure data overrun.
#define T_DA (1 << 1) // Temperature data available.
#define P_DA (1 << 0) // Pressure data available.

static constexpr uint8_t PRESS_OUT_XL = 0x28;
static constexpr uint8_t PRESS_OUT_L = 0x29;
static constexpr uint8_t PRESS_OUT_H = 0x2A;

static constexpr uint8_t TEMP_OUT_L = 0x2B;
static constexpr uint8_t TEMP_OUT_H = 0x2C;

/* interface factories */
extern device::Device *LPS22HB_SPI_interface(int bus);
extern device::Device *LPS22HB_I2C_interface(int bus);
typedef device::Device *(*LPS22HB_constructor)(int);

class LPS22HB : public device::CDev
{
public:
	LPS22HB(device::Device *interface, const char *path);
	virtual ~LPS22HB();

	virtual int		init();

	virtual int		ioctl(struct file *filp, int cmd, unsigned long arg);

	/**
	 * Diagnostics - print some basic information about the driver.
	 */
	void			print_info();

protected:
	Device			*_interface;

private:
	work_s			_work{};
	unsigned		_measure_ticks{0};

	bool			_collect_phase{false};

	orb_advert_t		_baro_topic{nullptr};

	int			_orb_class_instance{-1};
	int			_class_instance{-1};

	perf_counter_t		_sample_perf;
	perf_counter_t		_comms_errors;

	baro_report	_last_report{};           /**< used for info() */

	/**
	 * Initialise the automatic measurement state machine and start it.
	 *
	 * @note This function is called at open and error time.  It might make sense
	 *       to make it more aggressive about resetting the bus in case of errors.
	 */
	void			start();

	/**
	 * Stop the automatic measurement state machine.
	 */
	void			stop();

	/**
	 * Reset the device
	 */
	int			reset();

	/**
	 * Perform a poll cycle; collect from the previous measurement
	 * and start a new one.
	 *
	 * This is the heart of the measurement state machine.  This function
	 * alternately starts a measurement, or collects the data from the
	 * previous measurement.
	 *
	 * When the interval between measurements is greater than the minimum
	 * measurement interval, a gap is inserted between collection
	 * and measurement to provide the most recent measurement possible
	 * at the next interval.
	 */
	void			cycle();

	/**
	 * Static trampoline from the workq context; because we don't have a
	 * generic workq wrapper yet.
	 *
	 * @param arg		Instance pointer for the driver that is polling.
	 */
	static void		cycle_trampoline(void *arg);

	/**
	 * Write a register.
	 *
	 * @param reg		The register to write.
	 * @param val		The value to write.
	 * @return		OK on write success.
	 */
	int			write_reg(uint8_t reg, uint8_t val);

	/**
	 * Read a register.
	 *
	 * @param reg		The register to read.
	 * @param val		The value read.
	 * @return		OK on read success.
	 */
	int			read_reg(uint8_t reg, uint8_t &val);

	/**
	 * Issue a measurement command.
	 *
	 * @return		OK if the measurement command was successful.
	 */
	int			measure();

	/**
	 * Collect the result of the most recent measurement.
	 */
	int			collect();

	/* this class has pointer data members, do not allow copying it */
	LPS22HB(const LPS22HB &);
	LPS22HB operator=(const LPS22HB &);
};
