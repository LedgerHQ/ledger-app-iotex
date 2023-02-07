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

// BOLOS
#include <string.h>
#include <stdio.h>
#include <os_io_seproxyhal.h>
#include <ux.h>
#include <nbgl_use_case.h>
// ZXLIB
#include <zxmacros.h>

#include "view.h"
#include "crypto.h"
#include "settings.h"
#include "view_common.h"

view_addr_choose_data_t view_addr_choose_data;

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

viewctl_delegate_getData ehGetData = NULL;
viewctl_delegate_accept ehAccept = NULL;
viewctl_delegate_reject ehReject = NULL;

static void display_home_page();
static void display_address_page();

static void on_quit(void) {
    os_sched_exit(-1);
}

enum {
    CONTRACT_DATA_TOKEN = FIRST_USER_TOKEN,
    SHOW_ACCOUNT_TOKEN,
};

/*
 * Calls the callbacks registered in `ehAccept` or `ehReject` if they exist, depending on
 * the `success` argument.
 */
static void trigger_teardown_callback(const bool success) {
    if (success && ehAccept != NULL) {
        ehAccept();
    } else if (ehReject != NULL) {
        ehReject();
    }
}


/*
 * Settings menu
 */
#define SETTINGS_MISC_OPTIONS_NUMBER    1
#define SETTINGS_INFO_NUMBER            2
#define SETTINGS_PAGE_NUMBER            3

static nbgl_layoutSwitch_t switches[1];
static const char * const bars_text[1] = { "Display account address" };
static const uint8_t bars_token[1] = { SHOW_ACCOUNT_TOKEN };
static const char *const info_types[] = {"Version", "IoTeX"};
static const char *const info_contents[] = {APPVERSION, "(c) 2023 IoTeX Foundation"};


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
        content->infosList.infoTypes = (const char **) info_types;
        content->infosList.infoContents = (const char **) info_contents;
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
    if (token == SHOW_ACCOUNT_TOKEN) {
        PRINTF("Showing the account address\n");
        display_address_page();
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

// setting submenu: showing the address
static const char * address_info_types[1] = {0};

// Copy&rework of view_nano.c:view_addr_choose_update. Refactorize?
static void view_addr_choose_update() {
    print_title("Account %u", view_addr_choose_data.account);
    viewctl.dataKey[0] = 0;
    print_value("...");
    if (view_addr_choose_data.status.mode == VIEW_ADDR_MODE_SHOW) {
        print_value("....?....");
    }
    if (view_addr_choose_data.status.mode == VIEW_ADDR_MODE_SHOW ||
        view_addr_choose_data.status.mode == VIEW_ADDR_MODE_CONFIRM) {
        print_value("This is a very long string that needs to be scrolled otherwise it does not fit");
        bip32_depth = 5;
        bip32_path[0] = BIP32_0_DEFAULT;
        bip32_path[1] = BIP32_1_DEFAULT;
        bip32_path[2] = 0x80000000 | view_addr_choose_data.account;
        bip32_path[3] = BIP32_3_DEFAULT;
        bip32_path[4] = view_addr_choose_data.index;
        get_bech32_addr(viewctl.dataValue);
    }
}

static bool display_address_navigation(uint8_t page, nbgl_pageContent_t *content) {
    if (page == 0) {
        // Copy&rework of view_nano.c:view_addr_choose_show. Refactorize?
        view_addr_choose_data.status.mode = VIEW_ADDR_MODE_SHOW;
        view_addr_choose_data.account = 0;
        view_addr_choose_data.index = 0;
        strcpy(bech32_hrp, "io");

        view_addr_choose_update();
        PRINTF("Account address written in viewctl.dataValue: '%s'\n", viewctl.dataValue);
        address_info_types[0] = &viewctl.dataValue[0];
        content->type = INFOS_LIST;
        content->infosList.nbInfos = 1;
        content->infosList.infoTypes = (const char **) address_info_types;
        return true;
    }
    return false;
}

static void display_address_page() {
    nbgl_useCaseSettings("Account address",
                         0,
                         1,
                         false,
                         display_settings_page,
                         display_address_navigation,
                         charset_settings_callback);
}

/*
 * Home page
 */
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

/*
 * Confirmation for sending address through APDU
 */
void display_share_address_choice_page(void) {
    PRINTF("Current dataValue: '%s'\n", viewctl.dataValue);
    nbgl_useCaseChoice(NULL,
                       "Allow to share\nthe address?",
                       viewctl.dataValue,
                       "Yes, share","No, don't share",
                       trigger_teardown_callback);
}

/********************
 * Public functions *
 ********************/

void view_init() {
    nbgl_objInit();
    display_home_page();
}

void view_idle(unsigned int i __attribute__((unused))) {
    display_home_page();
}

void view_tx_show(unsigned int start_page __attribute__((unused))) {
    if (ehGetData == NULL) { return; }
}

void view_smsg_show(unsigned int start_page __attribute__((unused))) {
    if (!ehGetData)  {
        return;
    }
}

void view_addr_confirm(unsigned int i __attribute__((unused))) {
    view_addr_choose_data.status.mode = VIEW_ADDR_MODE_CONFIRM;
    view_addr_choose_data.account = BIP32_ACCOUNT;
    view_addr_choose_data.index = BIP32_INDEX;
    display_share_address_choice_page();
}

void view_set_handlers(viewctl_delegate_getData func_getData,
                       viewctl_delegate_accept func_accept,
                       viewctl_delegate_reject func_reject) {
    ehGetData = func_getData;
    ehAccept = func_accept;
    ehReject = func_reject;
}

#endif // HAVE_NBGL
