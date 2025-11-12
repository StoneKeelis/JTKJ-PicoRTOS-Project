/*

Version 0.80

MIT License

Copyright (c)  Iván Sánchez Milara

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

#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @file helper.h
 * @brief USB logging helpers for CDC0 (TinyUSB + FreeRTOS).
 *
 * These helpers write directly to CDC interface 0 using TinyUSB.
 * They are thread-safe (use a mutex internally) and designed to avoid
 * blocking forever. Do not call them from ISRs or TinyUSB callbacks.
 *
 * @note This API does not use pico_stdio_usb. Keep CDC0 free for this writer.
 * @note You must run TinyUSB in a task (e.g., a task that calls tud_task()).
 */


/**
 * @brief Initialize the USB serial logger (CDC0).
 *
 * Creates internal synchronization (mutex) and prepares the TinyUSB
 * writer for CDC0.
 *
 * @pre Call after @c tusb_init() and before the first @c usb_serial_print().
 * @pre FreeRTOS kernel must be available (mutex creation).
 * @warning Does not start TinyUSB; a task must be running @c tud_task().
 *
 * @return @c true on success, @c false if resources could not be created.
 */
bool usb_serial_init(void);

/**
 * @brief Flush pending TX data on CDC0 to the host.
 *
 * Requests TinyUSB to push any buffered data for CDC0. If the device
 * is not mounted or CDC0 is not connected, the function returns quickly.
 * It does not block indefinitely waiting for the host to read.
 *
 * @note Safe to call from task context; avoid calling from ISRs.
 * @note For guaranteed delivery, ensure the host has opened CDC0.
 */
void usb_serial_flush(void);

/**
 * @brief Check whether the host has opened CDC0 (DTR set).
 * 
 * @return @c true if the host opened CDC0; @c false otherwise.
 */
bool usb_serial_connected(void);

/**
 * @brief Thread-safe print of a null-terminated string to CDC0.
 *
 * Writes the string to USB CDC interface 0. The function will not
 * block indefinitely: it uses a mutex and a short TX-space wait inside.
 *
 * @param s Pointer to a null-terminated C string. Must not be @c NULL.
 *
 * @return Number of bytes written (>= 0). Returns 0 if CDC0 is not ready
 *         (device not mounted or port not opened) or if a timeout occurred.
 *         Returns -1 if @p s is @c NULL.
 *
 * @note Do not call from ISRs or TinyUSB callbacks; enqueue to a logger task instead.
 * @note If multiple tasks call this function, output is serialized by the mutex.
 * @note This writes to CDC0; keep CDC1 free for your application data if you use dual CDC.
 *
 * @code
 * // Example
 * if (usb_serial_init()) {
 *   // later, in any task:
 *   usb_serial_print("[DBG] temp=23 lux=450\n");
 * }
 * @endcode
 */
int usb_serial_print(const char *s);


#ifdef __cplusplus
}
#endif