// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2023 Intel Corporation. All rights reserved.
//
// Author: Tobiasz Dryjanski <tobiaszx.dryjanski@intel.com>

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <adsp_debug_window.h>
#include <mem_window.h>
#include <zephyr/debug/sparse.h>
#include <sof/debug/telemetry/telemetry.h>
#include <sof/audio/module_adapter/module/generic.h>
#include "adsp_debug_window.h"

/* systic vars */
int systick_counter;
int prev_ccount;
int perf_period_sum_;
int perf_period_cnt_;
struct perf_queue perf_queue = {0};

static void perf_queue_append(struct perf_queue *q, size_t element)
{
	if (!q->full) {
		q->elements[q->index] = element;
		q->sum += element;
		q->index++;
		q->size++;
		if (q->index >= AVG_PERF_MEAS_DEPTH) {
			q->index = 0;
			q->size = AVG_PERF_MEAS_DEPTH;
			q->full = true;
		}
	} else {
		/* no space, pop tail */
		q->sum -= q->elements[q->index];
		/* replace tail */
		q->elements[q->index] = element;
		q->sum += element;
		/* move tail */
		q->index++;
		if (q->index >= AVG_PERF_MEAS_DEPTH)
			q->index = 0;
	}
}

static size_t perf_queue_avg(struct perf_queue *q)
{
	if (!q->size)
		return 0;
	return q->sum / q->size;
}

int telemetry_init(void)
{
	/* systick_init */
	uint8_t slot_num = DW_TELEMETRY_SLOT;
	volatile struct adsp_debug_window *window = ADSP_DW;
//	struct adsp_debug_slot *slot = (struct adsp_debug_slot *)(ADSP_DW->slots[slot_num]);
//	uint8_t *data = slot->data;
//	struct telemetry_wnd_data *wnd_data = (struct telemetry_wnd_data *)slot->data;
	struct telemetry_wnd_data *wnd_data = (struct telemetry_wnd_data *)ADSP_DW->slots[slot_num];
	struct system_tick_info *systick_info =
			(struct system_tick_info *)wnd_data->system_tick_info;

	window->descs[slot_num].type = ADSP_DW_SLOT_TELEMETRY;
	window->descs[slot_num].resource_id = 0;
	wnd_data->separator_1 = 0x0000C0DE;

	/* Zero values per core */
	for (int i = 0; i < CONFIG_MAX_CORE_COUNT; i++) {
		systick_info[i].count = 0;
		systick_info[i].last_sys_tick_count = 0;
		systick_info[i].max_sys_tick_count = 0;
		systick_info[i].last_ccount = 0;
		systick_info[i].avg_utilization = 0;
		systick_info[i].peak_utilization = 0;
		systick_info[i].peak_utilization_4k = 0;
		systick_info[i].peak_utilization_8k = 0;
	}
	return 0;
}

void update_telemetry(uint32_t begin_ccount, uint32_t current_ccount)
{
	++systick_counter;
	int prid = cpu_get_id();

	//second, debug slot, is there any define for that?
	struct telemetry_wnd_data *wnd_data =
		(struct telemetry_wnd_data *)ADSP_DW->slots[DW_TELEMETRY_SLOT];
	struct system_tick_info *systick_info =
		(struct system_tick_info *)wnd_data->system_tick_info;

	systick_info[prid].count = systick_counter;
	systick_info[prid].last_sys_tick_count = current_ccount - begin_ccount;
	systick_info[prid].max_sys_tick_count =
			MAX(current_ccount - begin_ccount,
			    systick_info[prid].max_sys_tick_count);
	systick_info[prid].last_ccount = current_ccount;

	#ifdef PERFORMANCE_MEASUREMENTS
	const size_t measured_systick = begin_ccount - prev_ccount;

	prev_ccount = begin_ccount;
	if (systick_counter > 2) {
		perf_period_sum_ += measured_systick;
		perf_period_cnt_ = (perf_period_cnt_ + 1) % AVG_PERF_MEAS_PERIOD;
		if (perf_period_cnt_ == 0) {
			// Append average of last AVG_PERF_MEAS_PERIOD runs
			perf_queue_append(&perf_queue, perf_period_sum_ / AVG_PERF_MEAS_PERIOD);
			perf_period_sum_ = 0;
			// Calculate average from all buckets
			systick_info[prid].avg_utilization = perf_queue_avg(&perf_queue);
		}
		if (systick_counter > 1) {
			systick_info[prid].peak_utilization =
				MAX(systick_info[prid].peak_utilization,
				    measured_systick);
			systick_info[prid].peak_utilization_4k =
				MAX(systick_info[prid].peak_utilization_4k,
				    measured_systick);
			systick_info[prid].peak_utilization_8k =
				MAX(systick_info[prid].peak_utilization_8k,
				    measured_systick);
		}
		if ((systick_counter % 0x1000) == 0)
			systick_info[prid].peak_utilization_4k = 0;
		if ((systick_counter % 0x2000) == 0)
			systick_info[prid].peak_utilization_8k = 0;
	}
	#endif
}

/* init telemetry using Zephyr*/
SYS_INIT(telemetry_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

