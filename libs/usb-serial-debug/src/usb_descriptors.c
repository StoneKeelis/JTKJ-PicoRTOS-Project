/*

Version 0.80

MIT License

Copyright (c) 2025 Iván Sánchez Milara

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
 * USB Descriptors for Dual CDC-ACM Device
 * Creates TWO separate CDC interfaces:
 * - CDC0: For printf/debug output
 * - CDC1: For communication messages  
 * 
 */

#include "tusb.h"
#include "pico/unique_id.h"
#include "bsp/board_api.h"


// set some example Vendor and Product ID
// the board will use to identify at the host
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define CDC_EXAMPLE_VID     0xCafe                  // If problem use 0x2E8A (Raspberry pi)
// use _PID_MAP to generate unique PID for each interface
#define CDC_EXAMPLE_PID     (0x4000 | _PID_MAP(CDC, 0))  //If using Raspberry Pi VID 0x000A
// set USB 2.0
#define CDC_BCD     0x0200  


//--------------------------------------------------------------------
// DEVICE DESCRIPTOR
//--------------------------------------------------------------------
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),             //size of this struct
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = CDC_BCD,                                 //USB 2.0
    .bDeviceClass       = TUSB_CLASS_MISC,             // Miscellaneous device class
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,        // Common subclass  
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,           // Interface Association Descriptor

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,      //64 pico full speed

    .idVendor = CDC_EXAMPLE_VID,
    .idProduct = CDC_EXAMPLE_PID,
    .bcdDevice = 0x0100,                               // Device release number

    .iManufacturer = 0x01,                             // Index of manufacturer string
    .iProduct = 0x02,                                  // Index of product string
    .iSerialNumber = 0x03,                             // Index of serial number string

    .bNumConfigurations = 0x01 // 1 configuration
};

// called when host requests to get device descriptor
uint8_t const * tud_descriptor_device_cb(void);

enum {
    ITF_NUM_CDC_0 = 0,
    ITF_NUM_CDC_0_DATA,
    ITF_NUM_CDC_1,
    ITF_NUM_CDC_1_DATA,
    ITF_NUM_TOTAL
};

//--------------------------------------------------------------------
// CONFIGURATION DESCRIPTOR  
// This creates a composite device with TWO CDC interfaces
//--------------------------------------------------------------------

// Calculate total length: config + 2 CDC interfaces
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_CDC_DESC_LEN)

// Endpoint numbers for first CDC interface (CDC0 - Debug/Printf)
#define EPNUM_CDC0_NOTIF 0x81    // CDC0 notification endpoint
#define EPNUM_CDC0_OUT   0x02    // CDC0 data out endpoint  
#define EPNUM_CDC0_IN    0x82    // CDC0 data in endpoint

// Endpoint numbers for second CDC interface (CDC1 - Button messages)
#define EPNUM_CDC1_NOTIF 0x83    // CDC1 notification endpoint
#define EPNUM_CDC1_OUT   0x04    // CDC1 data out endpoint
#define EPNUM_CDC1_IN    0x84    // CDC1 data in endpoint

// configure descriptor (for 2 CDC interfaces)
uint8_t const desc_configuration[] = {
    // config descriptor | how much power in mA, count of interfaces, ...
    TUD_CONFIG_DESCRIPTOR(1,             // Configuration number
                          ITF_NUM_TOTAL, // Number of interfaces (2 CDC = 4 interfaces total)
                          0,             // String index
                          CONFIG_TOTAL_LEN, 
                          TUSB_DESC_CONFIG_ATT_SELF_POWERED,          //  TUSB_DESC_CONFIG_ATT_SELF_POWERED | TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP attributes: bit7=1, bit6=self-powered, bit5=remote-wakeup
                          100),          //100mA      

    // CDC 0: Communication Interface - TODO: get 64 from tusb_config.h
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0,                   //Communication Interface number
                                   4,                   // String index
                                   EPNUM_CDC0_NOTIF,   // interrupt IN endpoint address
                                   8,                   // notification EP size
                                   EPNUM_CDC0_OUT,     //bulk OUT endpoint address (host->device)
                                   EPNUM_CDC0_IN,      // bulk IN  endpoint address (device->host)
                                   CFG_TUD_CDC_EP_BUFSIZE),                 // bulk max packet size (FS max = 64)

    // CDC 1: Communication Interface - TODO: get 64 from tusb_config.h
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_1, 
                                   5, 
                                   EPNUM_CDC1_NOTIF, 
                                   8, 
                                   EPNUM_CDC1_OUT, 
                                   EPNUM_CDC1_IN, 
                                   CFG_TUD_CDC_EP_BUFSIZE),

};

// called when host requests to get configuration descriptor
uint8_t const * tud_descriptor_configuration_cb(uint8_t index);

// more device descriptor this time the qualifier. 
// TODO: Check that this is not needed in the Pico. It might contain errors. 
/*tusb_desc_device_qualifier_t const desc_device_qualifier = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = CDC_BCD, 

    .bDeviceClass = TUSB_CLASS_CDC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved = 0x00
};*/

// called when host requests to get device qualifier descriptor
 uint8_t const* tud_descriptor_device_qualifier_cb(void);


//--------------------------------------------------------------------
// STRING DESCRIPTORS
//--------------------------------------------------------------------

enum {
    STRID_LANGID = 0,   // 0: supported language ID
    STRID_MANUFACTURER, // 1: Manufacturer
    STRID_PRODUCT,      // 2: Product
    STRID_SERIAL,       // 3: Serials
    STRID_CDC_0,        // 4: CDC Interface 0
    STRID_CDC_1,        // 5: CDC Interface 1
};


char const* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 },   // 0: Language code (English US)
    "University of Oulu",            // 1: Manufacturer
    "TKJHAT",                        // 2: Product  
    "123456",                        // 3: Serial number (overwritten with unique ID)
    "Stdout CDC",                    // 4: CDC0 Interface (Debug/Printf)
    "Communication CDC",             // 5: CDC1 Interface (Messages)
    //"Reset"                          // 6: Reset interface (not added)
};

// buffer to hold the string descriptor during the request | plus 1 for the null terminator
static uint16_t _desc_str[32 + 1];

// called when host request to get string descriptor
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid);



//--------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------
uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    // avoid unused parameter warning and keep function signature consistent
    (void)index;

    return desc_configuration;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    size_t char_count;

    // Determine which string descriptor to return
    switch (index) {
        case STRID_LANGID:
            memcpy(&_desc_str[1], string_desc_arr[STRID_LANGID], 2);
            char_count = 1;
            break;
        case STRID_SERIAL:
            char_count = board_usb_get_serial(_desc_str + 1, 32);
            break;
        default: 
            // Regular string descriptor
            if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))) {
                return NULL;
            }
            // Copy string descriptor into _desc_str
            const char *str = string_desc_arr[index];

            char_count = strlen(str);
            size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type
            // Cap at max char
            if (char_count > max_count) {
                char_count = max_count;
            }

            // Convert ASCII string into UTF-16
            for (size_t i = 0; i < char_count; i++) {
                _desc_str[1 + i] = str[i];
            }
            break;
            
        }
    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8 ) | (2 * char_count + 2));
    return _desc_str;
}

//NOT NEEDED FOR PICO: FULL SPEEED.
uint8_t const* tud_descriptor_device_qualifier_cb(void) {
  return NULL; // Not needed in  high-speed capable
}

uint8_t const* tud_descriptor_other_speed_configuration_cb(uint8_t index) {
  (void) index;
  return NULL; // Not applicable
}

