/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2024 Intel Corporation. All rights reserved.
 *
 * Author: Tobiasz Dryjanski <tobiaszx.dryjanski@intel.com>
 */

#ifndef __SOF_PERFORMANCE_MONITOR_H__
#define __SOF_PERFORMANCE_MONITOR_H__

#include <zephyr/sys/bitarray.h>
#include <ipc4/base_fw.h>

/* to be moved to Zephyr */
#define WIN3_MBASE DT_REG_ADDR(DT_PHANDLE(DT_NODELABEL(mem_window3), memory))
#define ADSP_PMW ((volatile uint32_t *) \
		 (sys_cache_uncached_ptr_get((__sparse_force void __sparse_cache *) \
				     (WIN3_MBASE + WIN3_OFFSET))))

/**
 * Initializer for struct perf_data_item_comp
 *
 * @param[out] perf Struct to be initialized
 * @param[in] resource_id
 * @param[in] power_mode
 */
void perf_data_item_comp_init(struct perf_data_item_comp *perf, uint32_t resource_id,
			      uint32_t power_mode);

/**
 * Get next free performance data slot from Memory Window 3
 *
 * @return performance data record
 */
struct perf_data_item_comp *perf_data_getnext(void);

/**
 * Free a performance data slot in Memory Window 3
 *
 * @return 0 if succeeded, in other case the slot is already free
 */
int free_performance_data(struct perf_data_item_comp *item);

/**
 * Set performance measurements state
 *
 * @param[in] state Value to be set.
 */
void perf_meas_set_state(enum ipc4_perf_measurements_state_set state);

/**
 * Get performance measurements state
 *
 * @return performance measurements state
 */
enum ipc4_perf_measurements_state_set perf_meas_get_state(void);

/**
 * Get global performance data entries.
 *
 * @param[out] global_perf_data Struct to be filled with data
 * @return 0 if succeeded, error code otherwise.
 */
int get_performance_data(struct global_perf_data * const global_perf_data);

/**
 * Get extended global performance data entries.
 *
 * @param[out] ext_global_perf_data Struct to be filled with data
 * @return 0 if succeeded, error code otherwise.
 */
int get_extended_performance_data(struct extended_global_perf_data * const ext_global_perf_data);

/**
 * Reset performance data values for all records.
 *
 * @return 0 if succeeded, error code otherwise.
 */
int reset_performance_counters(void);

/**
 * Reinitialize performance data values for all created components;
 *
 * @return 0 if succeeded, error code otherwise.
 */
int enable_performance_counters(void);

/**
 * Unregister performance data records marked for removal.
 */
void disable_performance_counters(void);

#ifdef CONFIG_SOF_TELEMETRY_IO_PERFORMANCE_MEASUREMENTS

struct io_perf_data_item {
	/* ID of interface */
	uint32_t id : 8;
	/*  Instance of interface / bus */
	uint32_t instance : 8;
	/*  I/O direction from ACE perspective: 0 - Input, 1 - Output */
	uint32_t direction : 1;
	/*  I/O state: 0 - powered down / disabled, 1 - powered up / enabled */
	uint32_t state : 1;
	/* Power Mode:  0 - D0, 1 - D0ix (clock gating enabled), */
	uint32_t power_mode : 2;
	uint32_t rsvd : 11;
	/* The component still exists (0) or has been already deleted (1) */
	uint32_t is_removed : 1;
	/* Performance data */
	/*
	 * I/O (id)  - ID - Units - Description
	 *
	 * Host IPC  - 0  - Count - Counter of Host IPC messages incoming and outcoming
	 * IDC       - 1  - Count - Counter of IDC messages incoming and outcoming per DSP core
	 * DMIC      - 2 - Bytes - Counter of bytes transferred over DMIC interface
	 * I2S       - 3 - Bytes - Counter of bytes transferred over I2S interface
	 * SoundWire - 4 - Bytes - Counter of bytes transferred over SoundWire interface
	 * HD/A      - 5 - Bytes - Counter of bytes transferred over HD/A interface
	 * USB       - 6 - Bytes - Counter of bytes transferred over USB interface
	 * GPIO      - 7 - Count - Counter of GPIO interrupts or triggers
	 * I2C       - 8 - Bytes - Counter of bytes transferred over I2C interface
	 * I3C       - 9 - Bytes - Counter of bytes transferred over I3C interface
	 * I3C interrupt - 10 - Bytes - Counter of I3C interrupts
	 * UART      - 11 - Bytes - Counter of bytes transferred over UART interface
	 * SPI       - 12 - Bytes - Counter of bytes transferred over SPI interface
	 * CSI-2     - 13 - Bytes - Counter of bytes transferred over CSI-2 interface
	 * DTF       - 14 - Bytes - Counter of bytes transferred over DTF interface
	 */
	uint64_t data;
} __packed;

/* those below are used for bits in io_perf_data_item, it just uses 32bit vars specifically */
enum io_perf_data_item_dir {
	INPUT_DIRECTION = 0,
	OUTPUT_DIRECTION = 1,
};

enum io_perf_data_item_state {
	POWERED_DOWN_DISABLED = 0,
	POWERED_UP_ENABLED = 1,
};

enum io_perf_data_item_power_mode {
	D0_POWER_MODE = 0,
	D0IX_POWER_MODE = 1,
};

enum io_perf_data_item_id {
	IPC_ID = 0,
	IDC_ID = 1,
	DMIC_ID = 2,
	I2S_ID = 3,
	SOUND_WIRE_ID = 4,
	HDA_ID = 5,
	USB_ID = 6,
	GPIO_ID = 7,
	I2C_ID = 8,
	I3C_ID = 9,
	I3C_INTERRUPT_ID = 10,
	UART_ID = 11,
	SPI_ID = 12,
	CSI_2_ID = 13,
	DTF_ID = 14,
	INVALID_ID = 0xFF
};

struct io_global_perf_data {
	/* Number of statistics */
	uint32_t perf_item_count;
	/*  Performance statistics per I/O */
	struct io_perf_data_item perf_items[0];
};

/* I/O Performance Monitor initialization. */
/*
 * @return ErrorCode
 */
int io_perf_monitor_init(void);

/* Release slot */
/*
 * @param [in] slot_id pointer to io_perf_data_item
 * @return ErrorCode
 */
int io_perf_monitor_release_slot(struct io_perf_data_item *slot_id);

/* Get I/O performance data */
/*
 * @param [in] io_global_perf_data point to io_global_perf_data
 * @return ErrorCode
 */
int
io_perf_monitor_get_performance_data(struct io_global_perf_data *io_global_perf_data);

/* Set control state of I/o performance measurements process */
/*
 * @param [in] state a state to set
 * @return ErrorCode
 */
int
io_perf_monitor_set_state(enum ipc4_perf_measurements_state_set state);

/* Get control state of I/o performance measurements process */
/*
 * @return ipc4_perf_measurements_state_set
 */
enum ipc4_perf_measurements_state_set io_perf_monitor_get_state(void);

/* Initialization of I/O performance data */
/*
 * @param [out] slot_id pointer to io_perf_data_item
 * @param [in]  init_data pointer to init data
 */
int io_perf_monitor_init_data(struct io_perf_data_item **slot_id,
			      struct io_perf_data_item *init_data);

/* Update of I/O performance data */
/*
 * @param [in] slot_id pointer to io_perf_data_item
 * @param [in] increment
 */
void io_perf_monitor_update_data(struct io_perf_data_item *slot_id,
				 uint32_t increment);

/* Update of I/0 state */
/*
 * @param [in] slot_id pointer to io_perf_data_item
 * @param [in] power_up 0 - powered down / disabled, 1 - powered up / enabled
 */
void io_perf_monitor_update_io_state(struct io_perf_data_item *slot_id,
				     bool const power_up);

/* Update of I/0 power mode */
/*
 * @param [in] slot_id pointer to io_perf_data_item
 * @param [in] power_mode 0 - D0, 1 - D0ix (clock gating enabled)
 */
void io_perf_monitor_update_power_mode(struct io_perf_data_item *slot_id,
				       bool const power_mode);

#endif //SUPPORTED(IO_PERFORMANCE_MEASUREMENTS)

#endif
