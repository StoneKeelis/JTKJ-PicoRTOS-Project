/*
 * TinyUSB Configuration File
 * This file configures TinyUSB for CDC-ACM (USB Serial) functionality
 * Educational example - clear and well commented
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

// Define TinyUSB configuration for Raspberry Pi Pico
#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU OPT_MCU_RP2040
#endif

// Do this only if using FreeRTOS
#ifdef CFG_TUSB_OS
  #undef CFG_TUSB_OS
#endif
#define CFG_TUSB_OS  OPT_OS_FREERTOS


// Enable Device stack (vs Host stacki)
#define CFG_TUD_ENABLED (1)

//--------------------------------------------------------------------+
// Board Specific Configuration
//--------------------------------------------------------------------+

// USB Port Hub used. Pico just have one. 
// Legacy RHPORT configuration
// RHPort number used for device
// For RP2040, there's only one USB port, so always use 0
// Device mode and full speed (PICO USB ALWAYS GOES TO FULL SPEED)
#ifndef CFG_TUSB_RHPORT0_MODE
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#endif
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT        (0)
#endif

//Log from level 0 (no log) to level 3 (INFO DEBUG)
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG        0
#endif

//------------- CLASS DRIVERS -------------//

// Enable CDC (Communication Device Class) - we need TWO CDC interfaces
#define CFG_TUD_CDC (2)  // Enable 2 CDC interfaces instead of 1

// CDC buffer sizes
// These determine how much data can be buffered for USB communication
#define CFG_TUD_CDC_RX_BUFSIZE (512)   // Receive buffer size: 512 - 64 Depending size of data
#define CFG_TUD_CDC_TX_BUFSIZE (128)  // Transmit buffer size: 512 - 64 Depending size of data
#define CFG_TUD_CDC_EP_BUFSIZE (64)   // Size of the Endpoint Buffer. In Pico Must be 64 for full speed. 

//Since Pico is Full Speed, endpoint0 size is always 64
#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE  (64)
#endif
 

// We don't need other USB classes for this case
#define CFG_TUD_MSC     0  // Mass Storage Class (USB drive functionality)
#define CFG_TUD_HID     0  // Human Interface Device (keyboard/mouse)
#define CFG_TUD_MIDI    0  // MIDI
#define CFG_TUD_AUDIO   0  // Audio
#define CFG_TUD_VIDEO   0  // Video
#define CFG_TUD_VENDOR  0  // Vendor specific class

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */