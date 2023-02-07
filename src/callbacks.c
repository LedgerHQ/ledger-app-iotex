// SDK
#include <os.h>
#include <ux.h>
// zxlib
#include <apdu_codes.h>
#include <zxmacros.h>
// IoTeX
#include "lib/tx_display.h"
#include "callbacks.h"
#include "crypto.h"
#include "settings.h"
#include "view.h"
#include "lib/transaction.h"
#include "lib/biginteger.h"

static const char SIGN_MAGIC[] = "\x16IoTeX Signed Message:\n";

//region View Transaction Handlers
int16_t tx_getData(char *title,
                   int16_t max_title_length,
                   char *key,
                   int16_t max_key_length,
                   char *value,
                   int16_t max_value_length,
                   int16_t page_index,
                   int16_t chunk_index,
                   int16_t *page_count_out,
                   int16_t *chunk_count_out) {
    /* Nonos max length 9 chars, Nonox can be different */
    static const char action_name[][11] = {
        "INVALID",
        "Transfer",
        "Executio",
        "Create",
        "Unstake",
        "Withdraw",
        "Add Depo",
        "Restake",
        "ChgCandi",
        "TxOwners",
        "CandiReg",
        "CdUpdate",
        "DpReward",
        "ClReward",
    };

    *page_count_out = tx_display_num_pages();
    *chunk_count_out = 1;

    if (*page_count_out > 0) {
        int action_idx = tx_ctx.actiontype < ACTION_MAX_INVALID ? tx_ctx.actiontype : 0;
        snprintf(title,
                 max_title_length,
                 "Act:%s %02d/%02d",
                 action_name[action_idx],
                 page_index + 1,
                 *page_count_out);

        INIT_QUERY(key, max_key_length, value, max_value_length, chunk_index)
        decode_pb(transaction_get_buffer(), transaction_get_buffer_length(), NULL, page_index);
    }

    return *chunk_count_out;
}

void tx_reject();
void tx_accept_sign() {
    // Generate keys
    cx_ecfp_public_key_t publicKey;
    cx_ecfp_private_key_t privateKey;
    uint8_t privateKeyData[32];

    if (tx_ctx.has_contract_data && !N_settings.contractDataAllowed) {
        tx_reject();
        return;
    }

    unsigned int length = 0;
    int result = 0;
    switch (current_sigtype) {
        case SECP256K1:
            os_perso_derive_node_bip32(CX_CURVE_256K1,
                                       bip32_path,
                                       bip32_depth,
                                       privateKeyData,
                                       NULL);
            keys_secp256k1(&publicKey, &privateKey, privateKeyData);
            explicit_bzero(privateKeyData, sizeof(privateKeyData));
            result = sign_secp256k1(transaction_get_buffer(),
                                    transaction_get_buffer_length(),
                                    G_io_apdu_buffer,
                                    IO_APDU_BUFFER_SIZE,
                                    &length,
                                    &privateKey);
            break;
        default:
            THROW(APDU_CODE_INS_NOT_SUPPORTED);
            break;
    }

    if (result == 1) {
        set_code(G_io_apdu_buffer, length, APDU_CODE_OK);
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, length + 2);
        view_idle(0);
    } else {
        set_code(G_io_apdu_buffer, length, APDU_CODE_SIGN_VERIFY_ERROR);
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, length + 2);
        view_idle(0);
    }
}

void tx_reject() {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    view_idle(0);
}

//endregion

//region View Address Handlers

int16_t addr_getData(char *title,
                     int16_t max_title_length,
                     char *key,
                     int16_t max_key_length,
                     char *value,
                     int16_t max_value_length,
                     int16_t page_index,
                     int16_t chunk_index,
                     int16_t *page_count_out,
                     int16_t *chunk_count_out) {
    UNUSED(max_value_length); // max_value_length is always large enough to store address
    UNUSED(chunk_index);

    if (page_count_out)
        *page_count_out = 1;
    if (chunk_count_out)
        *chunk_count_out = 1;

    snprintf(title, max_title_length, "Account %d", bip32_path[2] & 0x7FFFFFF);
    snprintf(key, max_key_length, "index %d", page_index);
    bip32_path[bip32_depth - 1] = page_index;

    // get address from the current bip32_path
    get_bech32_addr(value);
    return 0;
}

void addr_accept() {
#if defined(TARGET_NANOS)
    print_key("Returning");
    print_value("Address...");
    view_status();
    UX_WAIT();
#endif
    // Send pubkey
    uint8_t *pk = G_io_apdu_buffer;
    get_pk_compressed(pk);
    int pos = PK_COMPRESSED_LEN;

    // Convert pubkey to bech32 address
    char *bech32_out = (char *) (G_io_apdu_buffer + pos);
    get_bech32_addr(bech32_out);
    pos += strlen(bech32_out);

    set_code(G_io_apdu_buffer, pos, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, pos + 2);
    view_idle(0);
}


//endregion

void addr_reject() {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    view_idle(0);
}


// region sign personal message
int16_t smsg_getData(char *title,
                     int16_t max_title_length,
                     char *key,
                     int16_t max_key_length,
                     char *value,
                     int16_t max_value_length,
                     int16_t page_index,
                     int16_t chunk_index,
                     int16_t *page_count_out,
                     int16_t *chunk_count_out) {
    UNUSED(chunk_index);

    uint32_t length;
    const uint32_t max_display_length = 128;
    const uint32_t last_page_length = (transaction_get_buffer_length() * 2) % max_display_length;

    int16_t page_count =
        (transaction_get_buffer_length() * 2) / max_display_length + (last_page_length != 0);
    if (page_count_out) {
        *page_count_out = page_count;
    }

    if (chunk_count_out) {
        *chunk_count_out = 1;
    }

    if (page_index + 1 != page_count) {
        length = max_display_length;
    }
    else {
        length = last_page_length ? last_page_length : max_display_length;
    }

    length = length ? length : max_display_length;

    if (transaction_get_buffer_length()) {
        snprintf(title, max_title_length, "Raw Message %02d/%02d", page_index + 1, page_count);
        snprintf(key, max_key_length, "Length: %d", transaction_get_buffer_length());
        snprintf(value,
                 max_value_length,
                 "%.*H",
                 length / 2,
                 transaction_get_buffer() + page_index * max_display_length / 2);
    }
    else {
        snprintf(title, max_title_length, "Raw Message %02d/%02d", 0, page_count);
        snprintf(key, max_key_length, "Length: %d", transaction_get_buffer_length());
        snprintf(value, max_value_length, "(null empty)");
    }

    return 0;
}

void smsg_accept() {
    int result;
    uint32_t length;
    uint8_t sign_msg[280] = {0};
    uint8_t private_key_data[32];
    const uint32_t sign_magic_length = strlen(SIGN_MAGIC);

    cx_ecfp_public_key_t public_key;
    cx_ecfp_private_key_t private_key;

    /* Copy sign magic to sign message */
    memcpy(sign_msg, SIGN_MAGIC, sign_magic_length);

    /* Append byte length and byte to sign msg */
    length = bigint_u642str(transaction_get_buffer_length(),
                            (char *)(sign_msg + sign_magic_length),
                            sizeof(sign_msg) - sign_magic_length);
    memcpy(sign_msg + sign_magic_length + length,
           transaction_get_buffer(),
           transaction_get_buffer_length());


    os_perso_derive_node_bip32(CX_CURVE_256K1,
                               bip32_path, bip32_depth,
                               private_key_data, NULL);

    keys_secp256k1(&public_key, &private_key, private_key_data);
    explicit_bzero(private_key_data, sizeof(private_key_data));

    result = sign_secp256k1(sign_msg,
                            sign_magic_length + length + transaction_get_buffer_length(),
                            G_io_apdu_buffer,
                            IO_APDU_BUFFER_SIZE,
                            &length,
                            &private_key);

    set_code(G_io_apdu_buffer, length, result ? APDU_CODE_OK : APDU_CODE_SIGN_VERIFY_ERROR);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, length + 2);
    view_idle(0);
}

void smsg_reject() {
    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    view_idle(0);
}

// endregion
