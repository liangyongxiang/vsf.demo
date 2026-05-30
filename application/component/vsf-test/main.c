/*****************************************************************************
 *   Copyright(C)2009-2026 by VSF Team                                       *
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

#include "vsf.h"
#include "vsf_board.h"
#include "vsf_test_suite_registry.h"

/*============================ ENTRY =========================================*/

int VSF_USER_ENTRY(void)
{
#if VSF_USE_KERNEL == DISABLED
    vsf_arch_init();
    vsf_hal_init_early();
    vsf_service_init();
    vsf_hal_init();
#endif
    vsf_board_init();
    vsf_start_trace();

    vsf_test_t __vsf_test = {
        .suites      = __vsf_test_data.suites,
        .suite_count = dimof(__vsf_test_data.suites),
    };
    vsf_test_hw_setup(&__vsf_test);
    vsf_test_run(&__vsf_test);

    return 0;
}

/* EOF */

