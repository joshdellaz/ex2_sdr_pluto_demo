#include <string.h>
#include <sdr_sband.h>
#include "sdr_driver.h"
#include "fec.h"
#include "osal.h"

#define PIPE_ENTER_MSG_LEN 15
#define PIPE_EXIT_MSG_LEN 16

/* From the EnduroSat manual, these delays assume the UART speed is 19.2KBaud */
static int sdr_uhf_baud_rate_delay[] = {
    [SDR_UHF_1200_BAUD] = 920,
    [SDR_UHF_2400_BAUD] = 460,
    [SDR_UHF_4800_BAUD] = 240,
    [SDR_UHF_9600_BAUD] = 120,
    [SDR_UHF_19200_BAUD] = 60,
    [SDR_UHF_TEST_BAUD] = 20
};

int sdr_uhf_tx(sdr_interface_data_t *ifdata, uint8_t *data, uint16_t len) {
    sdr_uhf_baud_rate_t uhf_baudrate = ifdata->sdr_conf->uhf_conf.uhf_baudrate;

    if (fec_data_to_mpdu(ifdata->mac_data, data, len)) {
        uint8_t *buf;
        int delay_time = sdr_uhf_baud_rate_delay[uhf_baudrate];
        size_t mtu = (size_t)fec_get_next_mpdu(ifdata->mac_data, (void **)&buf);
        while (mtu != 0) {
            (ifdata->tx_func)(ifdata->fd, buf, mtu);
            mtu = fec_get_next_mpdu(ifdata->mac_data, (void **)&buf);
            os_sleep_ms(delay_time);
        }
    }

    return 0;
}

#define SBAND_IDLE 0
#define SBAND_FIRST_FILL 1
#define SBAND_FILL 2
#define SBAND_DRAIN 3

void sdr_sband_tx_start(sdr_interface_data_t *ifdata) {
    sdr_sband_conf_t *sband_conf = &(ifdata->sdr_conf->sband_conf);

    sband_conf->state = SBAND_FIRST_FILL;
    sband_enter_sync_mode();
}

void sdr_sband_tx_stop(sdr_interface_data_t *ifdata) {
    sdr_sband_conf_t *sband_conf = &ifdata->sdr_conf->sband_conf;
    uint16_t fifo_level;
    int delay = 50;

    sband_conf->state = SBAND_IDLE;

    // We should let the FIFO drain before turning off the radio
    if (sband_buffer_count(&fifo_level)) {
        // The manual says the radio transmits at 512 bytes/msec
        delay = (fifo_level + SBAND_DRAIN_RATE/2)/(SBAND_DRAIN_RATE);
    }
    os_sleep_ms(delay);
    sband_enter_conf_mode();
}

int sdr_sband_tx(sdr_interface_data_t *ifdata, uint8_t *data, uint16_t len) {
    sdr_sband_conf_t *sband_conf = &ifdata->sdr_conf->sband_conf;

    if (fec_data_to_mpdu(ifdata->mac_data, data, len)) {
        uint8_t *buf;
        size_t mtu = (size_t)fec_get_next_mpdu(ifdata->mac_data, (void **)&buf);
        while (mtu != 0) {
            if (sband_conf->bytes_until_sync == 0) {
                sband_sync();
                sband_conf->bytes_until_sync = SBAND_SYNC_INTERVAL;
            }

            (ifdata->tx_func)(ifdata->fd, buf, mtu);

            sband_conf->bytes_until_sync -= mtu;
            sband_conf->fifo_count += mtu;

            if (sband_conf->fifo_count >= (SBAND_FIFO_DEPTH - 1024)) {
                if (sband_conf->state == SBAND_FIRST_FILL)
                    sband_enter_data_mode();

                /* Manual says a full drain takes about 41ms */
                os_sleep_ms(30);

                sband_conf->state = SBAND_FILL;
                sband_buffer_count(&(sband_conf->fifo_count));
            }
            mtu = fec_get_next_mpdu(ifdata->mac_data, (void **)&buf);
        }
    }

    return 0;
}

void sdr_rx_isr(void *cb_data, uint8_t *buf, size_t len, void *pxTaskWoken) {
    sdr_interface_data_t *ifdata = (sdr_interface_data_t *)cb_data;

    uint8_t *ptr = buf;
    for (size_t i=0; i<len; i++) {
        ifdata->rx_mpdu[ifdata->rx_mpdu_index] = ptr[i];
        ifdata->rx_mpdu_index++;
        if (ifdata->rx_mpdu_index >= ifdata->mtu) {
            ifdata->rx_mpdu_index = 0;
            // This is an isr, if this fails there's nothing that can be done
            os_queue_enqueue(ifdata->rx_queue, ifdata->rx_mpdu);
            return;
        }
    }

    //Discard transceiver pipe mode status updates
    //Could probably be done differently to use less cpu
    if (ifdata->rx_mpdu_index == (PIPE_ENTER_MSG_LEN)) {
        char pipe_enter_msg[PIPE_ENTER_MSG_LEN] = {'+', 'P', 'I', 'P', 'E', ' ', '7', '8', '7', '8', 'B', '8', 'A', 'E', 0x0D};
        if(strncmp((char *)ifdata->rx_mpdu, pipe_enter_msg, PIPE_ENTER_MSG_LEN) == 0){
            ifdata->rx_mpdu_index = 0;
        }
    }
    if (ifdata->rx_mpdu_index == (PIPE_EXIT_MSG_LEN)) {
        char pipe_exit_msg[PIPE_EXIT_MSG_LEN] = {'+', 'E', 'S', 'T', 'T', 'C', ' ', 'C', 'F', 'B', '5', '2', 'D', '3', '5', 0x0D};
        if (strncmp((char *)(ifdata->rx_mpdu), pipe_exit_msg, PIPE_EXIT_MSG_LEN) == 0) {
            ifdata->rx_mpdu_index = 0;
        }
    }
}

os_task_return_t sdr_rx_task(void *param) {
    sdr_interface_data_t *ifdata = (sdr_interface_data_t *)param;
    sdr_conf_t *sdr_conf = ifdata->sdr_conf;

    uint8_t *mpdu = os_malloc(ifdata->mtu);
    uint8_t *data = 0;

    while (1) {
        if (os_queue_dequeue(ifdata->rx_queue, mpdu) != true) {
            continue;
        }

        int plen = fec_mpdu_to_data(ifdata->mac_data, mpdu, &data, ifdata->mtu);
        if (plen) {
            sdr_conf->rx_callback(sdr_conf->rx_callback_data, data, plen, 0);
            os_free(data);
        }
    }
}
