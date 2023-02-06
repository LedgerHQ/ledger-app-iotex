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

// BOLOS
#include <string.h>
// ZXLIB
#include <zxmacros.h>

#include "view_common.h"

void viewctl_display_page();

const char *dblClickInfo = "DBL-CLICK TO VIEW";

viewctl_s viewctl;

viewctl_delegate_getData viewctl_ehGetData = NULL;
viewctl_delegate_ready viewctl_ehReady = NULL;
viewctl_delegate_exit viewctl_ehExit = NULL;
viewctl_delegate_display_ux viewctl_display_ux = NULL;

//------ External functions

void viewctl_start(int start_page,
                   viewctl_delegate_getData func_getData,
                   viewctl_delegate_ready ehReady,
                   viewctl_delegate_exit ehExit,
                   viewctl_delegate_display_ux func_display_ux) {
    // set handlers
    viewctl_ehGetData = func_getData;
    viewctl_ehReady = ehReady;
    viewctl_ehExit = ehExit;
    viewctl_display_ux = func_display_ux;

    // initialize variables
    viewctl.scrolling_mode = PENDING;
    viewctl.detailsCurrentPage = start_page;
    viewctl.chunksIndex = 0;
    viewctl.chunksCount = 1;

    viewctl_display_page();
    if (viewctl_ehReady != NULL) {
        viewctl_ehReady(0);
    }
}

//------ Event handlers

void submenu_left() {
    viewctl.chunksIndex--;
    viewctl.scrolling_mode = PENDING;
    viewctl_display_page();
}

void submenu_right() {
    viewctl.chunksIndex++;
    viewctl.scrolling_mode = PENDING;
    viewctl_display_page();
}

void menu_left() {
    viewctl.scrolling_mode = PENDING;
    viewctl.chunksIndex = 0;
    viewctl.chunksCount = 1;
    if (viewctl.detailsCurrentPage > 0) {
        viewctl.detailsCurrentPage--;
        viewctl_display_page();
    } else {
        viewctl_ehExit(0);
    }
}

void menu_right() {
    viewctl.scrolling_mode = PENDING;
    viewctl.chunksIndex = 0;
    viewctl.chunksCount = 1;
    if (viewctl.detailsCurrentPage < viewctl.detailsPageCount - 1) {
        viewctl.detailsCurrentPage++;
        viewctl_display_page();
    } else {
        viewctl_ehExit(0);
    }
}

static void viewctl_crop_key() {
    int offset = strlen(viewctl.dataKey) - MAX_SCREEN_LINE_WIDTH;
    if (offset > 0) {
        char *start = viewctl.dataKey;
        for (;;) {
            *start = start[offset];
            if (*start++ == '\0')
                break;
        }
    }
}

void viewctl_dataValue_split() {
#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
    const int dataValueLen = strlen(viewctl.dataValue);

    int offset = 0;
    for (int i = 0; i < MAX_SCREEN_NUM_LINES; i++) {
        viewctl.dataValueChunk[i][0] = 0;   // clean/terminate strings
        if (offset < dataValueLen) {
            snprintf(viewctl.dataValueChunk[i], MAX_SCREEN_LINE_WIDTH, "%s", viewctl.dataValue + offset);
        }
        offset += (MAX_SCREEN_LINE_WIDTH - 1);
    }
#endif
}

void viewctl_display_page() {
    if (viewctl_ehGetData == NULL) {
        return;
    }

    strcpy(viewctl.title, "?");
    strcpy(viewctl.dataKey, "?");
    strcpy(viewctl.dataValue, "?");

    // Read key and value strings
    viewctl_ehGetData(
            viewctl.title, sizeof(viewctl.title),
            viewctl.dataKey, sizeof(viewctl.dataKey),
            viewctl.dataValue, sizeof(viewctl.dataValue),
            viewctl.detailsCurrentPage, viewctl.chunksIndex,
            &viewctl.detailsPageCount, &viewctl.chunksCount);

    // fix possible utf8 issues
    asciify(viewctl.title);
    asciify(viewctl.dataKey);
    asciify(viewctl.dataValue);

    if (viewctl.chunksCount > 0) {
        // If value is very long, we split it into chunks
        // and add chunk index/count information at the end of the key
        if (viewctl.chunksCount > 1) {
            int position = strlen(viewctl.dataKey);
            snprintf(viewctl.dataKey + position,
                     sizeof(viewctl.dataKey) - position,
                     " %d/%d",
                     viewctl.chunksIndex + 1,
                     viewctl.chunksCount);
        }

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
        viewctl_dataValue_split();
#elif defined(TARGET_NANOS)
        switch (viewctl.scrolling_mode) {
        case KEY_SCROLLING_NO_VALUE: {
            viewctl_crop_key();
            viewctl.scrolling_mode = VALUE_SCROLLING;
            break;
        }
        case PENDING: {
            viewctl.scrolling_mode = VALUE_SCROLLING;
            if (strlen(viewctl.dataKey) > MAX_SCREEN_LINE_WIDTH) {
                int value_length = strlen(viewctl.dataValue);
                if (value_length > MAX_SCREEN_LINE_WIDTH) {
                    strcpy(viewctl.dataValue, "DBL-CLICK FOR VALUE");
                    viewctl.scrolling_mode = KEY_SCROLLING_NO_VALUE;
                } else {
                    viewctl.scrolling_mode = KEY_SCROLLING_SHORT_VALUE;
                }
            }
        }
        default:
            break;
    }
#endif
    }

    viewctl_display_ux();
}
