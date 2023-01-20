/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include <stddef.h>

#include "hal/wdt_types.h"
#include "hal/wdt_hal.h"
#include "hal/mwdt_ll.h"

#if CONFIG_HAL_WDT_USE_ROM_IMPL

#if CONFIG_IDF_TARGET_ESP32C6
void wdt_hal_init(wdt_hal_context_t *hal, wdt_inst_t wdt_inst, uint32_t prescaler, bool enable_intr)
{
    //Initialize HAL context
    memset(hal, 0, sizeof(wdt_hal_context_t));
    if (wdt_inst == WDT_MWDT0) {
        hal->mwdt_dev = &TIMERG0;
    }
#if SOC_TIMER_GROUPS >= 2
    else if (wdt_inst == WDT_MWDT1) {
        hal->mwdt_dev = &TIMERG1;
    }
#endif
    else {
#if CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2      // ESP32C6-TODO, ESP32H2-TODO: IDF-5653
        hal->rwdt_dev = &LP_WDT;
#else
        hal->rwdt_dev = &RTCCNTL;
#endif
    }
    hal->inst = wdt_inst;

    if (hal->inst == WDT_RWDT) {
        //Unlock RTC WDT
        rwdt_ll_write_protect_disable(hal->rwdt_dev);
        //Disable RTC WDT, all stages, and all interrupts.
        rwdt_ll_disable(hal->rwdt_dev);
        rwdt_ll_disable_stage(hal->rwdt_dev, WDT_STAGE0);
        rwdt_ll_disable_stage(hal->rwdt_dev, WDT_STAGE1);
        rwdt_ll_disable_stage(hal->rwdt_dev, WDT_STAGE2);
        rwdt_ll_disable_stage(hal->rwdt_dev, WDT_STAGE3);
#ifdef CONFIG_IDF_TARGET_ESP32
        //Enable or disable level interrupt. Edge interrupt is always disabled.
        rwdt_ll_set_edge_intr(hal->rwdt_dev, false);
        rwdt_ll_set_level_intr(hal->rwdt_dev, enable_intr);
#else
        //Enable or disable chip reset on timeout, and length of chip reset signal
        rwdt_ll_set_chip_reset_width(hal->rwdt_dev, 0);
        rwdt_ll_set_chip_reset_en(hal->rwdt_dev, false);
#endif
        rwdt_ll_clear_intr_status(hal->rwdt_dev);
        rwdt_ll_set_intr_enable(hal->rwdt_dev, enable_intr);
        //Set default values
#if SOC_CPU_CORES_NUM > 1
        rwdt_ll_set_appcpu_reset_en(hal->rwdt_dev, true);
#endif
        rwdt_ll_set_procpu_reset_en(hal->rwdt_dev, true);
        rwdt_ll_set_pause_in_sleep_en(hal->rwdt_dev, true);
        rwdt_ll_set_cpu_reset_length(hal->rwdt_dev, WDT_RESET_SIG_LENGTH_3_2us);
        rwdt_ll_set_sys_reset_length(hal->rwdt_dev, WDT_RESET_SIG_LENGTH_3_2us);
        //Lock RTC WDT
        rwdt_ll_write_protect_enable(hal->rwdt_dev);
    } else {
        //Unlock WDT
        mwdt_ll_write_protect_disable(hal->mwdt_dev);
        //Disable WDT and stages.
        mwdt_ll_disable(hal->mwdt_dev);
        mwdt_ll_disable_stage(hal->mwdt_dev, 0);
        mwdt_ll_disable_stage(hal->mwdt_dev, 1);
        mwdt_ll_disable_stage(hal->mwdt_dev, 2);
        mwdt_ll_disable_stage(hal->mwdt_dev, 3);
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
        //Enable or disable level interrupt. Edge interrupt is always disabled.
        mwdt_ll_set_edge_intr(hal->mwdt_dev, false);
        mwdt_ll_set_level_intr(hal->mwdt_dev, enable_intr);
#endif
        mwdt_ll_clear_intr_status(hal->mwdt_dev);
        mwdt_ll_set_intr_enable(hal->mwdt_dev, enable_intr);
        //Set default values
        mwdt_ll_set_cpu_reset_length(hal->mwdt_dev, WDT_RESET_SIG_LENGTH_3_2us);
        mwdt_ll_set_sys_reset_length(hal->mwdt_dev, WDT_RESET_SIG_LENGTH_3_2us);
        mwdt_ll_set_clock_source(hal->mwdt_dev, MWDT_CLK_SRC_DEFAULT);
        mwdt_ll_enable_clock(hal->mwdt_dev, true);
        //Set tick period
        mwdt_ll_set_prescaler(hal->mwdt_dev, prescaler);
        //Lock WDT
        mwdt_ll_write_protect_enable(hal->mwdt_dev);
    }
}

void wdt_hal_deinit(wdt_hal_context_t *hal)
{
    if (hal->inst == WDT_RWDT) {
        //Unlock WDT
        rwdt_ll_write_protect_disable(hal->rwdt_dev);
        //Disable WDT and clear any interrupts
        rwdt_ll_feed(hal->rwdt_dev);
        rwdt_ll_disable(hal->rwdt_dev);
        rwdt_ll_clear_intr_status(hal->rwdt_dev);
        rwdt_ll_set_intr_enable(hal->rwdt_dev, false);
        //Lock WDT
        rwdt_ll_write_protect_enable(hal->rwdt_dev);
    } else {
        //Unlock WDT
        mwdt_ll_write_protect_disable(hal->mwdt_dev);
        //Disable WDT and clear/disable any interrupts
        mwdt_ll_feed(hal->mwdt_dev);
        mwdt_ll_disable(hal->mwdt_dev);
        mwdt_ll_clear_intr_status(hal->mwdt_dev);
        mwdt_ll_set_intr_enable(hal->mwdt_dev, false);
        mwdt_ll_enable_clock(hal->mwdt_dev, false);
        //Lock WDT
        mwdt_ll_write_protect_enable(hal->mwdt_dev);
    }
    //Deinit HAL context
    hal->mwdt_dev = NULL;
}
#endif // CONFIG_IDF_TARGET_ESP32C6

#endif // CONFIG_HAL_WDT_USE_ROM_IMPL