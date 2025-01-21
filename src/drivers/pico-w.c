#if MG_ENABLE_TCPIP && MG_ARCH == MG_ARCH_PICOSDK && \
    defined(MG_ENABLE_DRIVER_PICO_W) && MG_ENABLE_DRIVER_PICO_W

#include "drivers/pico-w.h"
#include "net_builtin.h"
#include "pico/stdlib.h"

static struct mg_tcpip_if *s_ifp;

static bool mg_tcpip_driver_pico_w_init(struct mg_tcpip_if *ifp) {
  struct mg_tcpip_driver_pico_w_data *d =
      (struct mg_tcpip_driver_pico_w_data *) ifp->driver_data;
  s_ifp = ifp;
  if (cyw43_arch_init() != 0)
    return false;  // initialize async_context and WiFi chip
  if (d->apmode && d->apssid != NULL){
    MG_DEBUG(("Starting AP '%s' (%u)", d->apssid, d->apchannel));
    cyw43_wifi_ap_set_channel(&cyw43_state, d->apchannel);
    cyw43_arch_enable_ap_mode(d->apssid, d->appass, CYW43_AUTH_WPA2_AES_PSK);
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, ifp->mac);
  } else {
    cyw43_arch_enable_sta_mode();
    cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_AP, ifp->mac);
    if (d->ssid != NULL && d->pass != NULL) {
      MG_DEBUG(("Connecting to '%s'", d->ssid));
      return (cyw43_arch_wifi_connect_bssid_async(d->ssid, NULL, d->pass, CYW43_AUTH_WPA2_AES_PSK) == 0);
    }
  }
  return true;
}

static size_t mg_tcpip_driver_pico_w_tx(const void *buf, size_t len,
                                        struct mg_tcpip_if *ifp) {
  (void) ifp;
  return cyw43_send_ethernet(&cyw43_state, CYW43_ITF_STA, len, buf, false)
             ? 0
             : len;
}

static bool mg_tcpip_driver_pico_w_poll(struct mg_tcpip_if *ifp, bool s1) {
  (void) ifp;
  cyw43_arch_poll(); // not necessary, except when IRQs are disabled (OTA)
  return s1 ? (cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) ==
               CYW43_LINK_JOIN)
            : false;
}

struct mg_tcpip_driver mg_tcpip_driver_pico_w = {
    mg_tcpip_driver_pico_w_init,
    mg_tcpip_driver_pico_w_tx,
    NULL,
    mg_tcpip_driver_pico_w_poll,
};

// Called once per outstanding frame by async_context
void cyw43_cb_process_ethernet(void *cb_data, int itf, size_t len,
                               const uint8_t *buf) {
  if (itf != CYW43_ITF_STA) return;
  mg_tcpip_qwrite((void *) buf, len, s_ifp);
  (void) cb_data;
}

// Called by async_context
void cyw43_cb_tcpip_set_link_up(cyw43_t *self, int itf) {
}
void cyw43_cb_tcpip_set_link_down(cyw43_t *self, int itf) {
}

// there's life beyond lwIP
void pbuf_copy_partial(void) {
  (void) 0;
}

#endif
