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

#include <zxmacros.h>

#include "view.h"
#include "crypto.h"
#include "settings.h"

#include <string.h>
#include <stdio.h>
#include <os_io_seproxyhal.h>
#include <ux.h>

#include <nbgl_use_case.h>

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

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


static void display_home_page();

static void on_quit(void) {
    os_sched_exit(-1);
}

enum {
    CONTRACT_DATA_TOKEN = FIRST_USER_TOKEN,
    SHOW_ACCOUNT_TOKEN,
};


/*
 * Settings menu
 */
static const char *const infoTypes[] = {"Version", "IoTeX"};
static const char *const infoContents[] = {APPVERSION, "(c) 2023 IoTeX Foundation"};

#define SETTINGS_CHARSET_OPTIONS_NUMBER 5
#define SETTINGS_MISC_OPTIONS_NUMBER    1
#define SETTINGS_INFO_NUMBER            2
#define SETTINGS_PAGE_NUMBER            3

static nbgl_layoutSwitch_t switches[1];
static const char * const bars_text[1] = { "Display account address" };
static const uint8_t bars_token[1] = { SHOW_ACCOUNT_TOKEN };

static bool display_settings_navigation(uint8_t page, nbgl_pageContent_t *content) {
    if (page == 0) {
        content->type = BARS_LIST;
        content->barsList.barTexts = &bars_text[0];
        content->barsList.tokens = &bars_token[0];
        content->barsList.nbBars = 1;
        content->barsList.tuneId = TUNE_TAP_CASUAL;
    } else if (page == 1) {
        switches[0] = (nbgl_layoutSwitch_t){.initState = N_settings.contractDataAllowed,
                                            .text = "Allow contract data",
                                            .subText = NULL,
                                            .token = CONTRACT_DATA_TOKEN,
                                            .tuneId = TUNE_TAP_CASUAL};
        content->type = SWITCHES_LIST;
        content->switchesList.nbSwitches = SETTINGS_MISC_OPTIONS_NUMBER;
        content->switchesList.switches = &switches[0];
    } else if (page == 2) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = SETTINGS_INFO_NUMBER;
        content->infosList.infoTypes = (const char **) infoTypes;
        content->infosList.infoContents = (const char **) infoContents;
    } else {
        return false;
    }
    return true;
}

static void charset_settings_callback(const int token,
                                      const uint8_t index __attribute__((unused))) {
    if (token == CONTRACT_DATA_TOKEN) {
        uint8_t value = !N_settings.contractDataAllowed;
        PRINTF("Writting new value for contractDataAllowed settings: '%d'\n", value);
        nvm_write((void*) &N_settings.contractDataAllowed, (void*) &value, sizeof(uint8_t));
    }
}

static void display_settings_page() {
    nbgl_useCaseSettings("IoTeX infos",
                         0,
                         SETTINGS_PAGE_NUMBER,
                         false,
                         display_home_page,
                         display_settings_navigation,
                         charset_settings_callback);
}

static void display_home_page() {
    nbgl_useCaseHomeExt("IoTeX",
                        &C_iotex_ledger64_white,
                        "",
                        true,
                        NULL,
                        NULL,
                        display_settings_page,
                        on_quit);
}

/********************
 * Public functions *
 ********************/

void view_init(void) {
    nbgl_objInit();
    display_home_page();
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
