#include "sdr_driver.h"
#include "fec.h"
#include "osal.h"

void sdr_rx_dsr(void *cb_data, void *buf, size_t len, void *pxTaskWoken) {
    sdr_uhf_conf_t *sdr_conf = (sdr_uhf_conf_t *) cb_data;
    sdr_interface_data_t *ifdata = sdr_conf->if_data;

    uint8_t *ptr = buf;
    for (size_t i=0; i<len; i++) {
        ifdata->rx_mpdu[ifdata->rx_mpdu_index] = ptr[i];
        ifdata->rx_mpdu_index++;
        if (ifdata->rx_mpdu_index >= sdr_conf->mtu) {
            ifdata->rx_mpdu_index = 0;
            // This is an isr, if this fails there's nothing that can be done
            os_queue_enqueue(ifdata->rx_queue, ifdata->rx_mpdu);
        }
    }
}

os_task_return_t sdr_rx_task(void *param) {
    sdr_uhf_conf_t *sdr_conf = (sdr_uhf_conf_t *)param;
    sdr_interface_data_t *ifdata = sdr_conf->if_data;
    uint8_t *mpdu = os_malloc(sdr_conf->mtu);
    uint8_t *packet = 0;

    while (1) {
        if (os_queue_dequeue(ifdata->rx_queue, mpdu) != true) {
            continue;
        }
        
        int plen = fec_mpdu_to_data(ifdata->mac_data, mpdu, &packet, sdr_conf->mtu);
        if (plen) {
            sdr_conf->rx_callback(sdr_conf, packet, plen);
            os_free(packet);
        }
    }
}
