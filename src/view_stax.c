/*******************************************************************************
*   (c) 2016 Ledger
*   (c) 2018, 2019 ZondaX GmbH
*   (c) 2019 IoTeX Foundation
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#if defined(HAVE_NBGL)

#include "view.h"

#include <ux.h>
#include <os_io_seproxyhal.h>

#include <zxmacros.h>

#include "crypto.h"
#include "settings.h"

#include <string.h>
#include <stdio.h>

viewctl_delegate_getData ehGetData = NULL;
viewctl_delegate_accept ehAccept = NULL;
viewctl_delegate_reject ehReject = NULL;

#define MAX_VAL(a, b) ( (a)>(b) ? (a) : (b) )
#define MIN_VAL(a, b) ( (a)<(b) ? (a) : (b) )

#define VIEW_ADDR_MODE_ACCOUNT 0
#define VIEW_ADDR_MODE_INDEX 1
#define VIEW_ADDR_MODE_SHOW 2
#define VIEW_ADDR_MODE_CONFIRM 3

#define UIID_ICONACCEPT 0x50
#define UIID_ICONREJECT 0x51
#define UIID_ICONLEFT1  0x52
#define UIID_ICONRIGHT1 0x53
#define UIID_ICONLEFT2  0x54
#define UIID_ICONRIGHT2 0x55

#define UIID_MARKER1    0x60
#define UIID_MARKER2    0x61

struct {
    // modes
    // 0 - select account
    // 1 - select index
    struct {
        unsigned int mode: 4;
        unsigned int marker_blink: 1;
    } status;
    uint32_t account;
    uint32_t index;
} view_addr_choose_data;

#include <ux.h>
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

/********************
 * Public functions *
 ********************/

void view_init(void) {
    nbgl_objInit();
}

void view_idle(unsigned int _ __attribute__((unused))) {
    display_home_page();
}

void view_tx_show(unsigned int start_page) {
    UNUSED(start_page);
    if (ehGetData == NULL) { return; }
}

void view_smsg_show(unsigned int start_page) {
    UNUSED(start_page);
    if (!ehGetData)  {
        return;
    }
}

void view_addr_confirm(unsigned int _) {
    UNUSED(_);
    view_addr_choose_data.status.mode = VIEW_ADDR_MODE_CONFIRM;
    view_addr_choose_data.account = BIP32_ACCOUNT;
    view_addr_choose_data.index = BIP32_INDEX;
}

void view_set_handlers(viewctl_delegate_getData func_getData,
                       viewctl_delegate_accept func_accept,
                       viewctl_delegate_reject func_reject) {
    ehGetData = func_getData;
    ehAccept = func_accept;
    ehReject = func_reject;
}

#endif // HAVE_NBGL
