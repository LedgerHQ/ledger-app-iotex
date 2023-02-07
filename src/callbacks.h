#pragma once

#include <stdint.h>

// sign transaction
int16_t tx_getData(char *title,
                   int16_t max_title_length,
                   char *key,
                   int16_t max_key_length,
                   char *value,
                   int16_t max_value_length,
                   int16_t page_index,
                   int16_t chunk_index,
                   int16_t *page_count_out,
                   int16_t *chunk_count_out);
void tx_accept_sign();
void tx_reject();

// get address
void addr_accept();
void addr_reject();

// sign message
int16_t smsg_getData(char *title,
                     int16_t max_title_length,
                     char *key,
                     int16_t max_key_length,
                     char *value,
                     int16_t max_value_length,
                     int16_t page_index,
                     int16_t chunk_index,
                     int16_t *page_count_out,
                     int16_t *chunk_count_out);
void smsg_accept();
void smsg_reject();
