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

#include "vsf.h"
#include "vsf_board.h"

/*============================ MACROS ========================================*/

#if VSF_USE_TRACE != ENABLED
#   error this demo depends on VSF_USE_TRACE, please enable it!
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

declare_vsf_thread(__thread_task_t)
define_vsf_thread(__thread_task_t, 1024)

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/

static __thread_task_t __thread_task;

/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

implement_vsf_thread(__thread_task_t)
{
    vk_file_t *root, *child;

    vk_fs_init();
    vk_file_open(NULL, "/", &root);
    vk_fs_mount(root, vsf_board.fsop, vsf_board.fsinfo);

    while (true) {
        vk_file_open(root, NULL, &child);
        if (vsf_eda_get_return_value() != VSF_ERR_NONE) {
            break;
        }

        vsf_trace_info("%s ", child->name);
        vk_file_close(child);
    }
    vsf_trace_info("\n");
    vk_file_close(root);
}

int VSF_USER_ENTRY(void)
{
    vsf_board_init();
    vsf_start_trace();

    init_vsf_thread(__thread_task_t, &__thread_task, vsf_prio_0);
    return 0;
}
