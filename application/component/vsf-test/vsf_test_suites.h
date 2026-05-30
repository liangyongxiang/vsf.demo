/******************************************************************************
 *   Copyright(C)2009-2024 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and       *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

// Single header that aggregates all vsf_test peripheral suite headers.
// Every main() / entry point that wants to run all tests should include this
// instead of listing individual peripheral headers.

#ifndef __VSF_TEST_SUITES_H__
#define __VSF_TEST_SUITES_H__

#include "test/vsf_test/usart/vsf_test_usart.h"
#include "test/vsf_test/gpio/vsf_test_gpio.h"
#include "test/vsf_test/i2c/vsf_test_i2c.h"
#include "test/vsf_test/rtc/vsf_test_rtc.h"
#include "test/vsf_test/flash/vsf_test_flash.h"
#include "test/vsf_test/wdt/vsf_test_wdt.h"
#include "test/vsf_test/adc/vsf_test_adc.h"
#include "test/vsf_test/pwm/vsf_test_pwm.h"
#include "test/vsf_test/timer/vsf_test_timer.h"
#include "test/vsf_test/spi/vsf_test_spi.h"
#include "test/vsf_test/rng/vsf_test_rng.h"
#include "test/vsf_test/dma/vsf_test_dma.h"

#endif // __VSF_TEST_SUITES_H__
