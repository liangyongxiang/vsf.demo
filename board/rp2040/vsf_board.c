/*****************************************************************************
 *   Copyright(C)2009-2022 by VSF Team                                       *
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
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

/*============================ INCLUDES ======================================*/

#define __VSF_HEAP_CLASS_INHERIT__
#include "./vsf_board.h"

// for timer_hw (used to override vsf_test_busy_wait_ms)
#include "hardware/structs/timer.h"
#if VSF_USE_TEST == ENABLED
#   include "component/test/vsf_test/vsf_test.h"
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/
/*============================ GLOBAL VARIABLES ==============================*/

vsf_board_t vsf_board = {
    .usarts = {
        (vsf_usart_t *)&vsf_hw_usart0,  // UART0: GP0=TX, GP1=RX (debug stream)
        (vsf_usart_t *)&vsf_hw_usart1,  // UART1: GP8=TX, GP9=RX (test usart)
    },
    .i2cs = {
        (vsf_i2c_t *)&vsf_hw_i2c1,      // I2C1 on GP18=SCL, GP19=SDA (board I2C0)
        (vsf_i2c_t *)&vsf_hw_i2c0,      // I2C0 on GP20=SDA, GP21=SCL (board I2C1)
    },
    .spis = {
        (vsf_spi_t *)&vsf_hw_spi0,      // SPI0: GP2=SCK, GP3=MOSI, GP4=MISO, GP5=CS
        (vsf_spi_t *)&vsf_hw_spi1,      // SPI1: GP10=SCK, GP11=MOSI, GP12=MISO, GP13=CS
    },
    .gpio   = (vsf_gpio_t  *)&vsf_hw_gpio0,
    .flash  = (vsf_flash_t *)&vsf_hw_flash0,
    .adc    = (vsf_adc_t   *)&vsf_hw_adc0,
    .pwm0   = (vsf_pwm_t   *)&vsf_hw_pwm0,
    .timer0 = (vsf_timer_t *)&vsf_hw_timer0,
    .rtc    = (vsf_rtc_t   *)&vsf_hw_rtc0,
    .wdt    = (vsf_wdt_t   *)&vsf_hw_wdt0,
    .rng    = (vsf_rng_t   *)&vsf_hw_rng0,
    .dma    = (vsf_dma_t   *)&vsf_hw_dma0,
};

/*============================ IMPLEMENTATION ================================*/

VSF_CAL_WEAK(app_config_read)
int app_config_read(const char *cfgname, char *cfgvalue, int valuelen)
{
    return -1;
}

VSF_CAL_WEAK(app_config_write)
int app_config_write(const char *cfgname, char *cfgvalue)
{
    return -1;
}

#if     (!defined(VSF_HAL_USE_DEBUG_STREAM) || VSF_HAL_USE_DEBUG_STREAM == DISABLED)\
    &&  VSF_HAL_USE_USART == ENABLED
#   define __VSF_BOARD_USE_UART_AS_DEBUG_STREAM
#   ifndef VSF_DEBUG_STREAM_CFG_RX_BUF_SIZE
#       define VSF_DEBUG_STREAM_CFG_RX_BUF_SIZE         256
#   endif
static uint8_t __vsf_debug_stream_rx_buff[VSF_DEBUG_STREAM_CFG_RX_BUF_SIZE];
vsf_mem_stream_t VSF_DEBUG_STREAM_RX = {
    .op         = &vsf_mem_stream_op,
    .buffer     = __vsf_debug_stream_rx_buff,
    .size       = sizeof(__vsf_debug_stream_rx_buff),
};

static void __vsf_debug_stream_isrhandler(void *target, vsf_usart_t *uart,
        vsf_usart_irq_mask_t irq_mask)
{
    vsf_stream_t *stream = target;
    uint8_t *buffer;
    uint_fast32_t buflen;

    while (vsf_usart_rxfifo_get_data_count(uart) > 0) {
        buflen = vsf_stream_get_wbuf(stream, &buffer);
        if (!buflen) {
            // buffer full, no consumer — drain to clear interrupt
            uint8_t dummy;
            while (vsf_usart_rxfifo_get_data_count(uart) > 0)
                vsf_usart_rxfifo_read(uart, &dummy, 1);
            break;
        }
        vsf_stream_write(stream, NULL, vsf_usart_rxfifo_read(uart, buffer, buflen));
    }
}

static void __VSF_DEBUG_STREAM_TX_INIT(void)
{
    // UART0 pinmux for GP0/GP1 is configured in vsf_board_init() before this
    // function runs.
    vsf_usart_t *debug_usart = (vsf_usart_t *)&vsf_hw_usart0;
    vsf_err_t err;

    vsf_stream_connect_tx(&VSF_DEBUG_STREAM_RX.use_as__vsf_stream_t);
    err = vsf_usart_init(debug_usart, &(vsf_usart_cfg_t){
        .mode               = VSF_USART_8_BIT_LENGTH | VSF_USART_1_STOPBIT | VSF_USART_NO_PARITY
                            | VSF_USART_TX_ENABLE | VSF_USART_RX_ENABLE
                            | VSF_USART_RX_FIFO_THRESHOLD_NOT_EMPTY,
        .baudrate           = 115200,
        .isr                = {
            .handler_fn     = __vsf_debug_stream_isrhandler,
            .target_ptr     = &VSF_DEBUG_STREAM_RX,
            // vsf_arch_prio_0 maps to the lowest priority (same as PendSV
            // where the VSF kernel runs), so the RX IRQ would never preempt
            // scheduling. Use vsf_arch_prio_highest so debug RX bytes are
            // serviced while threads run.
            .prio           = vsf_arch_prio_highest,
        },
    });
    if (err != VSF_ERR_NONE) {
        VSF_ASSERT(false);
        return;
    }

    while (fsm_rt_cpl != vsf_usart_enable(debug_usart));
    // Enable both FIFO-threshold and RX-timeout IRQs. The threshold alone
    // would leave the last <threshold bytes of a burst stuck in the FIFO
    // because no IRQ fires; RX_TIMEOUT drains them after a brief silence.
    vsf_usart_irq_enable(debug_usart, VSF_USART_IRQ_MASK_RX | VSF_USART_IRQ_MASK_RX_TIMEOUT);
}

static void __VSF_DEBUG_STREAM_TX_WRITE_BLOCKED(uint8_t *buf, uint_fast32_t size)
{
    vsf_usart_t *debug_usart = (vsf_usart_t *)&vsf_hw_usart0;
    uint_fast16_t cur_size;

    while (size > 0) {
        cur_size = vsf_usart_txfifo_write(debug_usart, buf, size);
        if (cur_size > 0) {
            size -= cur_size;
            buf += cur_size;
        }
    }
}

#undef VSF_HAL_USE_DEBUG_STREAM
#define VSF_HAL_USE_DEBUG_STREAM        ENABLED
#include "hal/driver/common/debug_stream/debug_stream_tx_blocked.inc"

int vsf_stdin_getchar(void)
{
    uint8_t byte;
    while (vsf_stream_read(&VSF_DEBUG_STREAM_RX.use_as__vsf_stream_t, &byte, 1) == 0);
    return byte;
}
#endif

/*============================ HW CONFIG FOR TEST ===============================*/

#if VSF_USE_TEST == ENABLED
void vsf_test_hw_config(vsf_peripheral_type_t peripheral_type, const void *fixture, bool init)
{
    switch (peripheral_type) {
    case VSF_PERIPHERAL_TYPE_USART:
        if (init) {
            vsf_hw_gpio_ports_config_pin(
                (vsf_gpio_port_cfg_pin_t []){ {VSF_PA8, VSF_GPIO_AF, GPIO_FUNC_UART},
                                              {VSF_PA9, VSF_GPIO_AF, GPIO_FUNC_UART} }, 2);
            /* RP2040 IO_BANK0 pinmux takes effect asynchronously. Without a
             * settle delay the first UART1 RX after boot can latch a
             * spurious start bit from the GPIO→AF transition on GP9. */
            vsf_test_busy_wait_ms(1);
        }
        break;

    case VSF_PERIPHERAL_TYPE_I2C:
        if (init) {
            if (fixture == VSF_BOARD_I2C0_INSTANCE)
                vsf_hw_gpio_ports_config_pin(
                    (vsf_gpio_port_cfg_pin_t []){ {VSF_PA18, VSF_GPIO_AF, GPIO_FUNC_I2C},
                                                  {VSF_PA19, VSF_GPIO_AF, GPIO_FUNC_I2C} }, 2);
            else if (fixture == VSF_BOARD_I2C1_INSTANCE)
                vsf_hw_gpio_ports_config_pin(
                    (vsf_gpio_port_cfg_pin_t []){ {VSF_PA20, VSF_GPIO_AF, GPIO_FUNC_I2C},
                                                  {VSF_PA21, VSF_GPIO_AF, GPIO_FUNC_I2C} }, 2);
        }
        break;

    case VSF_PERIPHERAL_TYPE_I2C_SLAVE:
        if (init) {
            vsf_hw_gpio_ports_config_pin(
                (vsf_gpio_port_cfg_pin_t []){ {VSF_PA18, VSF_GPIO_AF, GPIO_FUNC_I2C},
                                              {VSF_PA19, VSF_GPIO_AF, GPIO_FUNC_I2C},
                                              {VSF_PA20, VSF_GPIO_AF, GPIO_FUNC_I2C},
                                              {VSF_PA21, VSF_GPIO_AF, GPIO_FUNC_I2C} }, 4);
        }
        break;

    case VSF_PERIPHERAL_TYPE_SPI:
        if (init) {
            /* Flash ROM ops leave GP2-5 in QSPI mode. Reset and re-assert SPI. */
            resets_hw->reset |= RESETS_RESET_SPI0_BITS;
            resets_hw->reset &= ~RESETS_RESET_SPI0_BITS;
            while (!(resets_hw->reset_done & RESETS_RESET_SPI0_BITS));
            vsf_hw_gpio_ports_config_pin(
                (vsf_gpio_port_cfg_pin_t []){ {VSF_PA2, VSF_GPIO_AF, GPIO_FUNC_SPI},
                                              {VSF_PA3, VSF_GPIO_AF, GPIO_FUNC_SPI},
                                              {VSF_PA4, VSF_GPIO_AF, GPIO_FUNC_SPI},
                                              {VSF_PA5, VSF_GPIO_AF, GPIO_FUNC_SPI} }, 4);
        }
        break;

    case VSF_PERIPHERAL_TYPE_DMA:
        if (!init) {
            vsf_dma_init(vsf_board.dma, &(vsf_dma_cfg_t){0});
        }
        break;

    default:
        break;
    }
}

const vsf_test_inst_t vsf_board_test_instances[] = {
    { .peripheral_type = VSF_PERIPHERAL_TYPE_I2C,       .context = VSF_BOARD_I2C0_INSTANCE  },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_I2C,       .context = VSF_BOARD_I2C1_INSTANCE  },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_I2C_SLAVE, .context = (const void *[]){ VSF_BOARD_I2C0_INSTANCE, VSF_BOARD_I2C1_INSTANCE } },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_USART,     .context = VSF_BOARD_USART_INSTANCE },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_SPI,       .context = VSF_BOARD_SPI_INSTANCE   },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_ADC,       .context = VSF_BOARD_ADC_INSTANCE   },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_PWM,       .context = VSF_BOARD_PWM_INSTANCE   },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_DMA,       .context = VSF_BOARD_DMA_INSTANCE   },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_TIMER,     .context = VSF_BOARD_TIMER_INSTANCE },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_RTC,       .context = VSF_BOARD_RTC_INSTANCE   },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_FLASH,     .context = VSF_BOARD_FLASH_INSTANCE },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_WDT,       .context = VSF_BOARD_WDT_INSTANCE   },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_RNG,       .context = VSF_BOARD_RNG_INSTANCE   },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_GPIO,      .context = VSF_BOARD_GPIO_INSTANCE  },
    { .peripheral_type = VSF_PERIPHERAL_TYPE_GPIO_PINMUX, .context = (const void *[]){ VSF_BOARD_GPIO_INSTANCE, VSF_BOARD_PINMUX_USART_INSTANCE } },
};
uint8_t vsf_board_test_instance_count = dimof(vsf_board_test_instances);

void vsf_test_hw_setup(vsf_test_t *test)
{
    test->wdt.entries      = NULL;
    test->wdt.count        = 0;
    test->reboot.entries   = (vsf_test_reboot_t *[]){ vsf_arch_reset };
    test->reboot.count     = 1;
    test->instances        = vsf_board_test_instances;
    test->instance_count   = dimof(vsf_board_test_instances);
}
#endif

VSF_CAL_WEAK(vsf_board_get_uart_funcsel)
uint8_t vsf_board_get_uart_funcsel(void)
{
    return GPIO_FUNC_UART;
}

void vsf_board_init(void)
{
    static const vsf_gpio_port_cfg_pin_t __pinmux_cfgs[] = {
        // UART0: GP0=TX, GP1=RX (debug stream) — kept here, boot-time need
        {VSF_PA0,   VSF_GPIO_AF,  GPIO_FUNC_UART},
        {VSF_PA1,   VSF_GPIO_AF,  GPIO_FUNC_UART},
        // SPI0: GP2=SCK, GP3=MOSI, GP4=MISO, GP5=CS
        // SPI test scenarios not yet enabled; keep in boot table for now.
        {VSF_PA2,   VSF_GPIO_AF,  GPIO_FUNC_SPI},
        {VSF_PA3,   VSF_GPIO_AF,  GPIO_FUNC_SPI},
        {VSF_PA4,   VSF_GPIO_AF,  GPIO_FUNC_SPI},
        {VSF_PA5,   VSF_GPIO_AF,  GPIO_FUNC_SPI},
    };
    vsf_hw_gpio_ports_config_pin((vsf_gpio_port_cfg_pin_t *)__pinmux_cfgs, dimof(__pinmux_cfgs));

#ifdef __VSF_BOARD_USE_UART_AS_DEBUG_STREAM
    VSF_STREAM_INIT(&VSF_DEBUG_STREAM_TX);
#endif

#if VSF_HAL_USE_DMA == ENABLED
    vsf_dma_init(vsf_board.dma, &(vsf_dma_cfg_t){0});
#endif
}

// newlib exit()/abort() require _exit on bare-metal targets.
// VSF Linux subsystem provides its own _exit; avoid duplicate definition.
#if !defined(VSF_USE_LINUX) || (VSF_USE_LINUX != ENABLED)
void _exit(int status)
{
    (void)status;
    while (1);
}

int _getpid(void)   { return 1; }
int _kill(int pid, int sig)
{
    (void)pid; (void)sig;
    return -1;
}
#endif

// When the kernel is disabled, vsf_sched_lock/unlock don't exist.
// queue_stream.c (and other service code) still references them via
// vsf_protect_scheduler() / vsf_unprotect_scheduler() macros.
#if VSF_USE_KERNEL == DISABLED
vsf_protect_t vsf_sched_lock(void)  { return 0; }
void vsf_sched_unlock(vsf_protect_t state)  { (void)state; }

// vsf_kernel_bsp.c (which defines this weak function) is not compiled when
// the kernel is disabled, but vsf_service_init() still calls it.
static uint8_t __vsf_heap_buffer[1024];
vsf_mem_t vsf_service_req___heap_memory_buffer___from_usr(void)
{
    return (vsf_mem_t){.ptr = __vsf_heap_buffer, .size = sizeof(__vsf_heap_buffer)};
}
#endif

// Override vsf_test_busy_wait_ms with RP2040 hardware timer (1MHz) for accurate
// wall-clock timing. The default CPU-cycle loop is too short on RP2040 @125MHz,
// causing IRQ-driven scenario tests (e.g. timer alarms 500ms in the future) to
// give up before the IRQ fires.
void vsf_test_busy_wait_ms(uint32_t ms)
{
    uint32_t start = timer_hw->timerawl;
    uint32_t delay_us = ms * 1000;
    while ((timer_hw->timerawl - start) < delay_us);
}
