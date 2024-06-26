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

/*
 * vsf_board distbus slave for hal
 *
 * Dependency:
 * Board:
 *   VSF_USB_DC0
 *
 * Source:
 *   application/app/distbus/*
 *
 * Include Directories:
 *   application/app/distbus
 *
 * Extra:
 *   dependency for the specific transport
 */

/*============================ INCLUDES ======================================*/

#include "vsf.h"
#include "vsf_board.h"

#include "hal/vsf_distbus_hal.h"
#include "transport/vsf_distbus_transport.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

typedef struct __user_distbus_msg_t {
    implement(vsf_distbus_msg_t)
    uint8_t buffer[VSF_HAL_DISTBUS_CFG_MTU + offset_of(vsf_distbus_msg_t, header)];
} __user_distbus_msg_t;

dcl_vsf_pool(__user_distbus_msg_pool)
def_vsf_pool(__user_distbus_msg_pool, __user_distbus_msg_t)

typedef struct __user_distbus_t {
    vsf_distbus_t                           distbus;
    vsf_pool(__user_distbus_msg_pool)       msg_pool;
    vsf_distbus_transport_t                 transport;

    vsf_callback_timer_t                    timer;
    vsf_distbus_hal_t                       hal;

#define __APP_DISTBUS_HAL_BIND(__N, __TYPE, __PREFIX)                           \
    .__TYPE[__N].target = (VSF_MCONNECT(vsf_, __TYPE, _t) *)&VSF_MCONNECT(__PREFIX, _, __TYPE, __N),
#define APP_DISTBUS_HAL_BIND_MULTI(__INSTANCE, __TYPE, __DEV_NUM, __PREFIX)     \
    .hal.__TYPE.dev_num = __DEV_NUM,                                            \
    .hal.__TYPE.dev = (__INSTANCE).__TYPE,                                      \
    VSF_MREPEAT(__DEV_NUM, __APP_DISTBUS_HAL_BIND, __TYPE, __PREFIX)

    // for gpio, map all
    // for other port like i2c/spi, map device in vsf_board
#if VSF_HAL_USE_GPIO == ENABLED
    vsf_distbus_hal_gpio_t                  gpio[VSF_HW_GPIO_COUNT];
#   define APP_DISTBUS_HAL_BIND_GPIO_MULTI(__INSTANCE, __DEV_NUM, __PREFIX)     \
        APP_DISTBUS_HAL_BIND_MULTI(__INSTANCE, gpio, __DEV_NUM, __PREFIX)
#endif
#if VSF_HAL_USE_USART == ENABLED
    vsf_distbus_hal_usart_t                 usart[VSF_HW_USART_COUNT];
#   define APP_DISTBUS_HAL_BIND_USART_MULTI(__INSTANCE, __DEV_NUM, __PREFIX)    \
        APP_DISTBUS_HAL_BIND_MULTI(__INSTANCE, usart, __DEV_NUM, __PREFIX)
#endif
} __user_distbus_t;

#if APP_DISTBUS_CFG_DEBUG == ENABLED
typedef struct __debug_distbus_t {
    vsf_distbus_t                           distbus;
    vsf_distbus_transport_t                 transport;
    vsf_hal_distbus_t                       hal;
} __debug_distbus_t;
#endif

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

static void __user_distbus_on_error(vsf_distbus_t *distbus);
static void * __user_distbus_alloc_msg(uint_fast32_t size);
static void __user_distbus_free_msg(void *msg);
static void __user_distbus_on_connected(vsf_distbus_t *distbus);

/*============================ LOCAL VARIABLES ===============================*/

#if VSF_DISTBUS_TRANSPORT_USE_STREAM == ENABLED
describe_mem_stream(vsf_distbus_transport_stream_rx, 1024)
describe_mem_stream(vsf_distbus_transport_stream_tx, 1024)
#endif

static __user_distbus_t __user_distbus = {
    .distbus                    = {
        .on_connected           = __user_distbus_on_connected,
        .op                     = {
            .mem                = {
                .alloc_msg      = __user_distbus_alloc_msg,
                .free_msg       = __user_distbus_free_msg,
            },
            .bus                = {
                .transport      = &__user_distbus.transport,
                .init           = vsf_distbus_transport_init,
                .send           = vsf_distbus_transport_send,
                .recv           = vsf_distbus_transport_recv,
            },
            .on_error           = __user_distbus_on_error,
        },
    },
#if VSF_DISTBUS_TRANSPORT_USE_STREAM == ENABLED
    .transport                  = {
        .stream_rx              = &vsf_distbus_transport_stream_rx.use_as__vsf_stream_t,
        .stream_tx              = &vsf_distbus_transport_stream_tx.use_as__vsf_stream_t,
    },
#endif

#if VSF_HAL_USE_GPIO == ENABLED
    APP_DISTBUS_HAL_BIND_GPIO_MULTI(__user_distbus, VSF_HW_GPIO_COUNT, vsf_hw)
#endif
#if VSF_HAL_USE_USART == ENABLED
    // TODO: get vsf_hw device by index
//    APP_DISTBUS_HAL_BIND_USART_MULTI(__user_distbus, VSF_HW_USART_COUNT, vsf_hw)
    .hal.usart.dev_num          = VSF_HW_USART_COUNT,
    .hal.usart.dev              = __user_distbus.usart,
    .usart[0].target            = (vsf_usart_t *)&vsf_hw_usart1,
    .usart[1].target            = (vsf_usart_t *)&vsf_hw_usart2,
#endif
};

#if APP_DISTBUS_CFG_DEBUG == ENABLED
static __debug_distbus_t __debug_distbus = {
    .distbus                    = {
        .op                     = {
            .mem                = {
                .alloc_msg      = __user_distbus_alloc_msg,
                .free_msg       = __user_distbus_free_msg,
            },
            .bus                = {
                .transport      = &__debug_distbus.transport,
                .init           = vsf_distbus_transport_init,
                .send           = vsf_distbus_transport_send,
                .recv           = vsf_distbus_transport_recv,
            },
            .on_error           = __user_distbus_on_error,
        },
    },
    .transport                  = {
        .stream_rx              = &vsf_distbus_transport_stream_tx.use_as__vsf_stream_t,
        .stream_tx              = &vsf_distbus_transport_stream_rx.use_as__vsf_stream_t,
    },
};
#endif

/*============================ IMPLEMENTATION ================================*/

imp_vsf_pool(__user_distbus_msg_pool, __user_distbus_msg_t)

static void __user_distbus_on_error(vsf_distbus_t *distbus)
{
    VSF_ASSERT(false);
}

static void * __user_distbus_alloc_msg(uint_fast32_t size)
{
    VSF_ASSERT(size <= sizeof(((__user_distbus_msg_t *)NULL))->buffer);
    return VSF_POOL_ALLOC(__user_distbus_msg_pool, &__user_distbus.msg_pool);
}

static void __user_distbus_free_msg(void *msg)
{
    VSF_POOL_FREE(__user_distbus_msg_pool, &__user_distbus.msg_pool, msg);
}

static void __user_distbus_on_connected(vsf_distbus_t *distbus)
{
    __user_distbus_t *user_distbus = container_of(distbus, __user_distbus_t, distbus);
    vsf_distbus_hal_start(&user_distbus->hal);
    vsf_callback_timer_add_ms(&user_distbus->timer, 1000);
}

static void __user_distbus_connection_check_on_timer(vsf_callback_timer_t *timer)
{
    __user_distbus_t *user_distbus = container_of(timer, __user_distbus_t, timer);
    if (!user_distbus->hal.remote_connected) {
        __user_distbus_on_connected(&user_distbus->distbus);
    } else {
#define APP_DISTBUS_HAL_POLL(__TYPE)                                            \
        for (uint8_t i = 0; i < dimof(__user_distbus.__TYPE); i++) {            \
            VSF_MCONNECT(vsf_distbus_hal_, __TYPE, _poll)(&__user_distbus.__TYPE[i]);\
        }

#if 0
#define __VSF_DISTBUS_HAL_ENUM  APP_DISTBUS_HAL_POLL
#include "hal/vsf_distbus_hal_enum.inc"
#else
        APP_DISTBUS_HAL_POLL(gpio)
        APP_DISTBUS_HAL_POLL(usart)
#endif

        vsf_callback_timer_add_ms(&user_distbus->timer, 1);
    }
}

#if APP_DISTBUS_CFG_DEBUG == ENABLED
void vsf_hal_distbus_on_new(vsf_hal_distbus_t *hal_distbus, vsf_hal_distbus_type_t type, uint8_t num, void *devs)
{
    static const char *__types_str[] = {
#define APP_DISTBUS_HAL_TYPE_STR(__TYPE)                                        \
        [VSF_MCONNECT(VSF_HAL_DISTBUS_, __TYPE)] = VSF_STR(__TYPE),

#define __VSF_DISTBUS_HAL_ENUM  APP_DISTBUS_HAL_TYPE_STR
#include "hal/vsf_distbus_hal_enum.inc"
    };
    VSF_ASSERT(type < dimof(__types_str));
    VSF_ASSERT(__types_str[type] != NULL);
    vsf_trace_info("[hal_distbus] new %s %d 0x%p" VSF_TRACE_CFG_LINEEND, __types_str[type], num, devs);

    switch (type) {
    case VSF_HAL_DISTBUS_GPIO:
        while (true) {
            vsf_gpio_set(__debug_distbus.hal.gpio.dev, 1 << 15);
            vsf_gpio_clear(__debug_distbus.hal.gpio.dev, 1 << 15);
        }
        break;
    }
}
#endif

int VSF_USER_ENTRY(void)
{
    vsf_board_init();
#if VSF_USE_TRACE == ENABLED
    vsf_start_trace();
#endif

#if VSF_DISTBUS_TRANSPORT_USE_STREAM == ENABLED
    VSF_STREAM_INIT(&vsf_distbus_transport_stream_rx);
    VSF_STREAM_INIT(&vsf_distbus_transport_stream_tx);
#endif
    VSF_POOL_INIT(__user_distbus_msg_pool, &__user_distbus.msg_pool, APP_DISTBUS_CFG_POOL_NUM);

    vsf_callback_timer_init(&__user_distbus.timer);
    __user_distbus.timer.on_timer = __user_distbus_connection_check_on_timer;
    vsf_distbus_init(&__user_distbus.distbus);
    vsf_distbus_hal_register(&__user_distbus.distbus, &__user_distbus.hal);
    vsf_distbus_start(&__user_distbus.distbus);

#if APP_DISTBUS_CFG_DEBUG == ENABLED
    vsf_distbus_init(&__debug_distbus.distbus);
    vsf_hal_distbus_register(&__debug_distbus.distbus, &__debug_distbus.hal);
    vsf_distbus_start(&__debug_distbus.distbus);
#endif
    return 0;
}
