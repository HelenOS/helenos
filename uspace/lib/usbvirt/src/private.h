#include <usbvirt/device.h>

int process_control_transfer(usbvirt_device_t *,
    usbvirt_control_request_handler_t *,
    usb_device_request_setup_packet_t *,
    uint8_t *, size_t *);

extern usbvirt_control_request_handler_t library_handlers[];
