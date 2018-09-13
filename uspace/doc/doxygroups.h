/* Definitions of modules and its relations for generating Doxygen documentation */

/**
 * @defgroup uspace uspace
 * @brief HelenOS userspace
 */

/**
 * @defgroup apps HelenOS applications
 * @ingroup uspace
 */

/**
 * @defgroup drvs HelenOS drivers
 * @brief HelenOS device drivers using the DDF framework
 * @ingroup uspace
 */

/**
 * @defgroup libs HelenOS libraries
 * @ingroup uspace
 */

/**
 * @defgroup srvs HelenOS services
 * @ingroup uspace
 */

/*
 * SPECIAL COMPOSITIONS
 */

/**
 * @defgroup usb USB
 * @ingroup uspace
 * @brief USB support for HelenOS.
 */

/**
 *     @defgroup libusb Base USB library
 *     @ingroup usb
 *     @brief Common definitions for any driver or application
 *     dealing with USB.
 */

/**
 *     @defgroup libusbdev USB library for device drivers
 *     @ingroup usb
 *     @brief Library for writing drivers of endpoint devices (functions).
 */

/**
 *     @defgroup libusbhost USB library for host controller drivers
 *     @ingroup usb
 *     @brief Library for writing host controller drivers.
 */

/**
 *     @defgroup libusbhid USB library for HID devices
 *     @ingroup usb
 *     @brief Library for writing USB HID drivers.
 */

/**
 *     @defgroup usbvirt USB virtualization
 *     @ingroup usb
 *     @brief Support for virtual USB devices.
 */

/**
 *         @defgroup libusbvirt USB virtualization library
 *         @ingroup usbvirt
 *         @brief Library for creating virtual USB devices.
 */

/**
 *         @defgroup drvusbvhc Virtual USB host controller
 *         @ingroup usbvirt
 *         @brief Driver simulating work of USB host controller.
 */

/**
 *         @defgroup usbvirthub Virtual USB hub
 *         @ingroup usbvirt
 *         @brief Extra virtual USB hub for virtual host controller.
 *         @details
 *         Some of the sources are shared with virtual host controller,
 *         see @ref drvusbvhc for the rest of the files.
 */

/**
 *         @defgroup usbvirtkbd Virtual USB keybaord
 *         @ingroup usbvirt
 *         @brief Virtual USB keyboard for virtual host controller.
 */

/**
 *     @defgroup usbinfo USB info application
 *     @ingroup usb
 *     @brief Application for querying USB devices.
 *     @details
 *     The intended usage of this application is to query new USB devices
 *     for their descriptors etc. to simplify driver writing.
 */

/**
 *     @defgroup lsusb HelenOS version of lsusb command
 *     @ingroup usb
 *     @brief Application for listing USB host controllers.
 *     @details
 *     List all found host controllers.
 */

/**
 *     @defgroup drvusbmid USB multi interface device driver
 *     @ingroup usb
 *     @brief USB multi interface device driver
 *     @details
 *     This driver serves as a mini hub (or bus) driver for devices
 *     that have the class defined at interface level (those devices
 *     usually have several interfaces).
 *
 *     The term multi interface device driver (MID) was borrowed
 *     Solaris operating system.
 */

/**
 *     @defgroup drvusbhub USB hub driver
 *     @ingroup usb
 *     @brief USB hub driver.
 */

/**
 *     @defgroup drvusbhid USB HID driver
 *     @ingroup usb
 *     @brief USB driver for HID devices.
 */

/**
 *     @defgroup drvusbmast USB mass storage driver
 *     @ingroup usb
 *     @brief USB driver for mass storage devices (bulk-only protocol).
 *     This driver is a only a stub and is currently used only for
 *     testing that bulk transfers work.
 */

/**
 *     @addtogroup drvusbuhci
 *     @ingroup usb
 */

/**
 *     @addtogroup drvusbohci
 *     @ingroup usb
 */

/**
 *     @addtogroup drvusbehci
 *     @ingroup usb
 */

/**
 *     @addtogroup drvusbxhci
 *     @ingroup usb
 */

/**
 *     @defgroup drvusbfallback USB fallback driver
 *     @ingroup usb
 *     @brief Fallback driver for any USB device.
 *     @details
 *     The purpose of this driver is to simplify querying of unknown
 *     devices from within HelenOS (without a driver, no node at all
 *     may appear under /loc/devices).
 */
