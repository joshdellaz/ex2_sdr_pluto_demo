#ifndef SDR_DRIVER_H_
#define SDR_DRIVER_H_

#include <stddef.h>
#include <stdint.h>
#include "osal.h"
#include "sdr_sband.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SDR_UHF_1200_BAUD = 0,
    SDR_UHF_2400_BAUD,
    SDR_UHF_4800_BAUD,
    SDR_UHF_9600_BAUD,
    SDR_UHF_19200_BAUD,
    SDR_UHF_TEST_BAUD,
    SDR_UHF_END_BAUD,
} sdr_uhf_baud_rate_t;

#define SDR_UHF_MAX_MTU 128
#define SDR_SBAND_MAX_MTU 128

typedef enum {
    SDR_ERR_NONE,
    SDR_ERR_NOMEM,
    SDR_ERR_TIMEOUT,
    SDR_ERR_DRIVER,
} sdr_error_t;

/**
   sdr_tx_t - Send MPDU frame (implemented by driver).

   @param[in] fd interface file descriptor
   @param[in] data data to send
   @param[in] len length of \a data.
   @return 0 on success, otherwise an error code.
*/
typedef int (*sdr_tx_t)(int fd, const void * data, size_t data_length);

/**
   sdr_rx_callback - user provided callback to receive a data packet

   @param[in] udata - handle to connection state
   @param[in] data received data
   @param[in] len length of \a data.
*/
typedef void (*sdr_rx_callback_t) (void *udata, uint8_t *data, size_t len, void* unused);

struct sdr_interface_data;

typedef struct sdr_uhf_conf {
    sdr_uhf_baud_rate_t uhf_baudrate;
    int uart_baudrate;
    char *device_file;
} sdr_uhf_conf_t;

typedef struct sdr_conf {
    bool use_fec;
    sdr_rx_callback_t rx_callback;
    void *rx_callback_data;
    struct sdr_uhf_conf uhf_conf;
    struct sdr_sband_conf sband_conf;
} sdr_conf_t;

typedef struct sdr_interface_data {
    uint16_t mtu;
    uintptr_t fd;
    /** Low Level Transmit Function */
    sdr_tx_t tx_func;
    /** Low level Receive function */
    os_queue_handle_t rx_queue;
    void *mac_data;
    sdr_conf_t *sdr_conf;
    /** Low level buffer state */
    uint16_t rx_mpdu_index;
    uint8_t *rx_mpdu;
    OS_TickType last_rx;
} sdr_interface_data_t;

#define SDR_IF_UHF_NAME "UHF"
#define SDR_IF_SBAND_NAME "S-BAND"
#define SDR_IF_LOOPBACK_NAME "LOOPBACK"

sdr_interface_data_t* sdr_interface_init(const sdr_conf_t *conf, const char *ifname);

int sdr_uart_driver_init(sdr_interface_data_t *ifdata);
int sdr_gnuradio_driver_init(sdr_interface_data_t *ifdata);

int sdr_uhf_tx(sdr_interface_data_t *ifdata, uint8_t *data, uint16_t len);

void sdr_rx_isr(void *cb_data, uint8_t *buf, size_t len, void *pxTaskWoken);

os_task_return_t sdr_rx_task(void *param);

#ifdef __cplusplus
}
#endif
#endif // MPDU_DRIVER_H
