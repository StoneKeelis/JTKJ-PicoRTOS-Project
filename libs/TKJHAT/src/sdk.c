/*

Version 0.8

MIT License

Copyright (c) 2025 Raisul Islam, Iván Sánchez Milara

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

#include <tkjhat/sdk.h>

//#include "tusb.h" //is it needed?
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include <tkjhat/ssd1306.h>
#include <tkjhat/pdm_microphone.h>
#include <stdio.h>
#include <math.h>




/* ===================================
 *  UTILITIES / FIXING HARDWARE BUGS
 * =================================== */

//This trick tries to solve the leakage problem in rp2350 (Errata 9)
#if defined(PICO_RP2350)
    #include "hardware/gpio.h"

    static inline bool gpio_get_safe(uint pin) {
        hw_set_bits(&pads_bank0_hw->io[pin], PADS_BANK0_GPIO0_IE_BITS); // enable input buffer
        __asm volatile("nop; nop; nop; nop;"); // short settle
        bool v = gpio_get(pin);// read
        hw_clear_bits(&pads_bank0_hw->io[pin], PADS_BANK0_GPIO0_IE_BITS); // disable input buffer again
        //printf("Reading 2350");
        return v;
    }

    // Redirect any later uses of gpio_get(...) to our safe version on RP2350
    #define gpio_get(pin) gpio_get_safe((pin))
#endif




/* =========================
 *  GENERAL FUNCTIONS
 * ========================= */
void init_hat_sdk(){
    //Turn off the RGB
    stop_rgb_led();

    //Start the I2C
    init_i2c_default();
}

/* =========================
 *  BUTTONS
 * ========================= */

 void init_sw1() {
    // Initialize the button pin as an input with a pull-up resistor
    gpio_init(SW1_PIN);
    gpio_set_dir(SW1_PIN, GPIO_IN);

}

 void init_sw2() {
    // Initialize the button pin as an input with a pull-up resistor
    gpio_init(SW2_PIN);
    gpio_set_dir(SW2_PIN, GPIO_IN);
}

void init_button1(){
    return init_sw1();
}

void init_button2(){
    return init_sw2();
}

/* =========================
 *  LEDs
 * ========================= */
 void init_red_led() {
    // Initialize the LED pin as an output
    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
}

void init_led(){
    return init_red_led();
}

void toggle_red_led() {
    bool curr = gpio_get(RED_LED_PIN);
    gpio_put(RED_LED_PIN, !curr);
}

void toggle_led() {
    return toggle_red_led();
}

void set_red_led_status(bool status){
    gpio_put(RED_LED_PIN,status);
}

void set_led_status(bool status){
    return set_red_led_status(status);
}

void blink_red_led(int n){
    for (int i=0;i<n;i++){ 
        toggle_red_led(); 
        sleep_ms(120); 
        toggle_red_led(); 
        sleep_ms(120); 
    }
    gpio_put(RED_LED_PIN,false);
}

void blink_led(int n){
    return blink_red_led(n);
}


// RGB related function
 void init_rgb_led() {
    // Initialize the PWM slice for each RGB pin
    gpio_set_function(RGB_LED_R, GPIO_FUNC_PWM);
    gpio_set_function(RGB_LED_G, GPIO_FUNC_PWM);
    gpio_set_function(RGB_LED_B, GPIO_FUNC_PWM);

    // Get the PWM slice numbers for each GPIO pin
    uint slice_num_r = pwm_gpio_to_slice_num(RGB_LED_R);
    uint slice_num_g = pwm_gpio_to_slice_num(RGB_LED_G);
    uint slice_num_b = pwm_gpio_to_slice_num(RGB_LED_B);

    // Set PWM clock to a frequency suitable for LED control
    pwm_set_clkdiv(slice_num_r, 4.0f);
    pwm_set_clkdiv(slice_num_g, 4.0f);
    pwm_set_clkdiv(slice_num_b, 4.0f);

    // Enable PWM outputs
    pwm_set_enabled(slice_num_r, true);
    pwm_set_enabled(slice_num_g, true);
    pwm_set_enabled(slice_num_b, true);
}

//RGB off
void stop_rgb_led(){
     // Stop PWM on those slices (optional)
    pwm_set_enabled(pwm_gpio_to_slice_num(RGB_LED_R), false);
    pwm_set_enabled(pwm_gpio_to_slice_num(RGB_LED_G), false);
    pwm_set_enabled(pwm_gpio_to_slice_num(RGB_LED_B), false);

    // Return pins to GPIO input (Hi-Z)
    gpio_set_function(RGB_LED_R, GPIO_FUNC_SIO);
    gpio_set_dir(RGB_LED_R, GPIO_IN);
    gpio_disable_pulls(RGB_LED_R);

    gpio_set_function(RGB_LED_G, GPIO_FUNC_SIO);
    gpio_set_dir(RGB_LED_G, GPIO_IN);
    gpio_disable_pulls(RGB_LED_G);

    gpio_set_function(RGB_LED_B, GPIO_FUNC_SIO);
    gpio_set_dir(RGB_LED_B, GPIO_IN);
    gpio_disable_pulls(RGB_LED_B);
}

//Channel active to low level (common anode). We need to invert the value
 void rgb_led_write(uint8_t r, uint8_t g, uint8_t b) {

    //Invert the values
    r = 255-r;
    g = 255-g;
    b = 255-b;
    // Get the PWM slice numbers for each GPIO pin
    uint slice_num_r = pwm_gpio_to_slice_num(RGB_LED_R);
    uint slice_num_g = pwm_gpio_to_slice_num(RGB_LED_G);
    uint slice_num_b = pwm_gpio_to_slice_num(RGB_LED_B);

    // Convert the 0-255 value to a 16-bit duty cycle (0-65535)
    uint16_t r_value = r * r; // Mapping 0-255 to 0-65535 (linearly)
    uint16_t g_value = g * g; // Mapping 0-255 to 0-65535 (linearly)
    uint16_t b_value = b * b; // Mapping 0-255 to 0-65535 (linearly)

    // Set the PWM level for each color
    pwm_set_gpio_level(RGB_LED_R, r_value);
    pwm_set_gpio_level(RGB_LED_G, g_value);
    pwm_set_gpio_level(RGB_LED_B, b_value);
}

/* =========================
 *  BUZZER
 * ========================= */

 void init_buzzer() {
    // Initialize the buzzer pin as an output
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

 void buzzer_play_tone(uint32_t frequency, uint32_t duration_ms) {
    // Calculate the period (in microseconds) and number of cycles
    uint32_t period_us = 1000000 / frequency;
    uint32_t num_cycles = duration_ms * frequency / 1000;

    // Generate the tone by toggling the buzzer pin at the specified frequency
    for (uint32_t i = 0; i < num_cycles; i++) {
        gpio_put(BUZZER_PIN, 1);
        busy_wait_us(period_us / 2);
        gpio_put(BUZZER_PIN, 0);
        busy_wait_us(period_us / 2);
    }
}

 void buzzer_turn_off() {
    // Turn off the buzzer by setting the pin to low
    gpio_put(BUZZER_PIN, 0);
}

void deinit_buzzer() {
    // Deinitialize the buzzer pin
    gpio_deinit(BUZZER_PIN);
}

/* =========================
 *  I2C
 * ========================= */
// Initialize I2C peripheral
void init_i2c(uint sda_pin, uint scl_pin) {
    i2c_init(i2c_default, 400*1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

void init_i2c_default(){
    init_i2c(DEFAULT_I2C_SDA_PIN, DEFAULT_I2C_SCL_PIN);
}

// Generic I2C write function
bool i2c_write(uint8_t addr, const uint8_t *src, size_t len, bool nostop) {
    int bytes_written = i2c_write_blocking(i2c_default, addr, src, len, nostop);
    return bytes_written == (int)len;
}

// Generic I2C read function
bool i2c_read(uint8_t addr, uint8_t *dst, size_t len, bool nostop) {
    int bytes_read = i2c_read_blocking(i2c_default, addr, dst, len, nostop);
    return bytes_read == (int)len;
}

/* =========================
 *  MICROPHONE
 * ========================= */
// Uses https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/tree/main
// Uses pio to read pdm data and OpenPDM2PCM library to transform PDM to PCM
// Microphone related functions
// Sample rate: 16Khz
// Buffer size: 256 samples.
 int init_pdm_microphone() {
    const struct pdm_microphone_config config = {
    // GPIO pin for the PDM DAT signal
    .gpio_data = PDM_DATA,

    // GPIO pin for the PDM CLK signal
    .gpio_clk = PDM_CLK,

    // PIO instance to use
    .pio = pio0,

    // PIO State Machine instance to use
    .pio_sm = 0,

    // sample rate in Hz
    .sample_rate = MEMS_SAMPLING_FREQUENCY,

    // number of samples to buffer
    .sample_buffer_size = MEMS_BUFFER_SIZE,
    };

    return pdm_microphone_init(&config);
   
}

 int init_microphone_sampling(){
    return pdm_microphone_start();
}

 void end_microphone_sampling(){
    pdm_microphone_stop();
}

void pdm_microphone_set_callback(pdm_samples_ready_handler_t handler) {
    pdm_microphone_set_samples_ready_handler(handler);
}

int get_microphone_samples(int16_t* buffer, size_t samples) {
    return pdm_microphone_read(buffer,samples);
}


/* =========================
 *  DISPLAY SSD1306
 * ========================= */
// Datasheet can be found at: https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
// Library used can be found at: https://github.com/daschr/pico-ssd1306https://github.com/daschr/pico-ssd1306
 static ssd1306_t disp;

// Display-related functions
 void init_display() {
    // Initialize the SSD1306 display with external VCC
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, SSD1306_I2C_ADDRESS, i2c_default);

    //power it on
    ssd1306_poweron(&disp);

    // Clear the display
    ssd1306_clear(&disp);
}


void write_text_xy(int16_t x0, int16_t y0, const char *text) {
    if (!text) return;

    // Clamp negatives (library expects unsigned)
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;

    const uint8_t scale = 1; //Default font scale is 1

    ssd1306_draw_string(&disp, (uint32_t)x0, (uint32_t)y0, scale, text);
    ssd1306_show(&disp);

    // Delay for 800 milliseconds
    sleep_ms(800);
}

void write_text(const char *text) {

    if (!text)return;

    // Draw the text at the specified position with a font size of 2
    ssd1306_draw_string(&disp, 8, 24, 2, text);

    // Update the display
    ssd1306_show(&disp);

    // Delay for 800 milliseconds
    sleep_ms(800);
}

/**
 * @brief Put a pixel with bounds checking (no immediate display update).
 *
 * Writes a single pixel into the SSD1306 off-screen buffer only if the
 * coordinates are inside the display area. Out-of-bounds are ignored.
 *
 * Coordinate system: origin (0,0) = top-left; X→right, Y→down.
 *
 * Preconditions:
 *  - `disp` must be initialized via ssd1306_init().
 *
 * @param x X coordinate in pixels (0 .. disp.width-1). Negative values are ignored.
 * @param y Y coordinate in pixels (0 .. disp.height-1). Negative values are ignored.
 *
 * @note This does NOT call ssd1306_show(). Batch draws, then call ssd1306_show(&disp).
 */
static inline void putp(int16_t x, int16_t y) {
    if (x >= 0 && y >= 0 &&
        x < (int16_t)disp.width && y < (int16_t)disp.height) {
        ssd1306_draw_pixel(&disp, (uint32_t)x, (uint32_t)y);
    }
}

/**
 * @brief Draw a clipped horizontal span into the off-screen buffer.
 *
 * Draws solid pixels from x1 to x2 inclusive on row y. The span is clipped
 * to display bounds; fully off-screen spans are skipped.
 *
 * Preconditions:
 *  - `disp` must be initialized.
 *
 * @param x1 Left end (can be < 0; will be clipped).
 * @param x2 Right end (can be >= width; will be clipped).
 * @param y  Row index (0 .. disp.height-1). Outside rows are ignored.
 *
 * @note No ssd1306_show() here; meant for filled-shape routines.
 */
static inline void hspan(int16_t x1, int16_t x2, int16_t y) {
    if (y < 0 || y >= (int16_t)disp.height) return;
    if (x1 > x2) { int16_t t = x1; x1 = x2; x2 = t; }
    if (x2 < 0 || x1 >= (int16_t)disp.width) return;
    if (x1 < 0) x1 = 0;
    if (x2 >= (int16_t)disp.width) x2 = (int16_t)disp.width - 1;

    for (int16_t x = x1; x <= x2; ++x)
        ssd1306_draw_pixel(&disp, (uint32_t)x, (uint32_t)y);
}


void draw_circle(int16_t x0, int16_t y0, int16_t r, bool fill) {
    // Draw a circle using the Bresenham algorithm
    if (r < 0) 
        return;
    if (r == 0) { 
        putp(x0, y0); 
        ssd1306_show(&disp); 
        return; 
    }

    // Midpoint circle algorithm
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    if (fill) {
        hspan((int16_t)(x0 - r), (int16_t)(x0 + r), y0);  // center row
    } else {
        putp(x0, (int16_t)(y0 + r));
        putp(x0, (int16_t)(y0 - r));
        putp((int16_t)(x0 + r), y0);
        putp((int16_t)(x0 - r), y0);
    }

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;

        if (fill) {
            hspan((int16_t)(x0 - x), (int16_t)(x0 + x), (int16_t)(y0 + y));
            hspan((int16_t)(x0 - x), (int16_t)(x0 + x), (int16_t)(y0 - y));
            hspan((int16_t)(x0 - y), (int16_t)(x0 + y), (int16_t)(y0 + x));
            hspan((int16_t)(x0 - y), (int16_t)(x0 + y), (int16_t)(y0 - x));
        } else {
            putp((int16_t)(x0 + x), (int16_t)(y0 + y));
            putp((int16_t)(x0 - x), (int16_t)(y0 + y));
            putp((int16_t)(x0 + x), (int16_t)(y0 - y));
            putp((int16_t)(x0 - x), (int16_t)(y0 - y));
            putp((int16_t)(x0 + y), (int16_t)(y0 + x));
            putp((int16_t)(x0 - y), (int16_t)(y0 + x));
            putp((int16_t)(x0 + y), (int16_t)(y0 - x));
            putp((int16_t)(x0 - y), (int16_t)(y0 - x));
        }
    }
    ssd1306_show(&disp);  // remove if batching multiple draws
}

 void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    // Draw a line between the specified points
    ssd1306_draw_line(&disp, x0, y0, x1, y1);

    // Update the display
    ssd1306_show(&disp);
}

 void draw_square(uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool fill) {
    // Draw a square at the specified position with the given width and height
    if (fill)
        ssd1306_draw_square(&disp, x, y, w, h);
    else
        ssd1306_draw_empty_square(&disp, x, y, w, h);

    // Update the display
    ssd1306_show(&disp);
}

void clear_display() {
    // Clear the display
    ssd1306_clear(&disp);
    // Update the display
    ssd1306_show(&disp);
}

void stop_display() {
    ssd1306_poweroff(&disp);
}


/* =========================
 *  LIGHT SENSOR VEML6030
 * =========================  */
// Useful info at: https://learn.sparkfun.com/tutorials/qwiic-ambient-light-sensor-veml6030-hookup-guide/all#arduino-library
// Programming application: https://www.vishay.com/docs/84367/designingveml6030.pdf
// Datasheet: https://www.vishay.com/docs/84366/veml6030.pdf
void init_veml6030() {
    // Configure sensor settings (100ms integration time, gain 1/8, power on)
    //Bit 12:11 = 10 (gain1/8)
    //Bit 9:6 = 0000 (Integration time 100ms)
    //Bit 5:4 Persisentec protect number setting (00 -> 1) 
    //Bit 1 =0 INT disable
    //Bit 0 = 0 Power on
    // 0b0001 0000 0000 0000 -> =0x1000

    uint8_t config[3] = {
        VEML6030_CONFIG_REG,  // Configuration register
        0x00,                 // High byte: Gain 1/8 (00), reserved bits
        0x10                  // Low byte: 100ms integration time (010 in bits 6-8), power on (bit 0 = 0)
    };
    
    // Write configuration to sensor
    i2c_write_blocking(i2c_default, VEML6030_I2C_ADDR, config, sizeof(config), false);
    sleep_ms(10);
}

// Read light level from VEML6030
// Ligt in LUX
// Note: sampling time should be > IT -> in this case it has been 100ms by defintion. 
uint32_t veml6030_read_light() {
    uint8_t txBuffer[1] = { VEML6030_ALS_REG }; // Register address to read
    uint8_t rxBuffer[2];

    // Exercise 2: In order to get the luminance we need to read the value of the VEML6030_ALS_REG (see VEML6030 datasheet)
    //            Use functions i2c_write_blocking and i2_read_blocking to collect luminance data.
    //            These functions are found in the Pico SDK:
    //            https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#group_hardware_i2c
    //            The i2c that you must use is i2c_default.  
    //            from the sensor. Make necessary bitwise operation to store the results in a register of 16 bits.
    //            Multiply the value by the adequate value considering Integration Time of 100 ms and Gain of 1/8
    //            using data of page 5 of VEML6030 design application document: https://www.vishay.com/docs/84367/designingveml6030.pdf
    //            Finally, store the value in the variable luxVal_uncorrected.
    //
    // Tehtävä 2: Saadaksemme luminanssin meidän tulee lukea VEML6030_ALS_REG -rekisterin arvo (katso VEML6030-datalehti).
    //            Käytä funktioita i2c_write_blocking ja i2_read_blocking luminanssidatan keräämiseen.
    //            Nämä funktiot löytyvät Pico SDK:sta:
    //            https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#group_hardware_i2c
    //            Käytettävä i2c-väylä on i2c_default.
    //            Tee tarvittavat bittikohtaiset operaatiot tallettaksesi tuloksen 16-bittiseen rekisteriin.
    //            Kerro arvo sopivalla kertoimella huomioiden 100 ms integraatioaika ja vahvistus 1/8
    //            käyttäen VEML6030-sovellussuunnitteluasiakirjan sivun 5 tietoja:https://www.vishay.com/docs/84367/designingveml6030.pdf
    //            Lopuksi tallenna arvo muuttujaan luxVal_uncorrected.
  
    uint32_t luxVal_uncorrected = 0; 

    if(i2c_write_blocking(i2c_default, VEML6030_I2C_ADDR, txBuffer, 1, true) == 1) {
        if(i2c_read_blocking(i2c_default, VEML6030_I2C_ADDR, rxBuffer, 2, false) == 2) {
            luxVal_uncorrected = ((uint16_t)rxBuffer[1] << 8) | rxBuffer[0]; // Combine MSB and LSB
        }
    }
    
    if (luxVal_uncorrected > 1000) {
        // Polynomial is pulled from pg 10 of the datasheet.
        double luxVal = (0.00000000000060135 * pow((double)luxVal_uncorrected, 4)) -
                        (0.0000000093924 * pow((double)luxVal_uncorrected, 3)) +
                        (0.000081488 * pow((double)luxVal_uncorrected, 2)) +
                        (1.0023 * (double)luxVal_uncorrected);
        return (uint32_t)luxVal;
    }
    return luxVal_uncorrected;
}


//This method might be utility method in the future.
static uint16_t _veml6030_read_register(uint8_t reg) {
    uint8_t data[2] = {0,0};

    // Select ALS output register
    i2c_write_blocking(i2c_default, VEML6030_I2C_ADDR, &reg, 1, true);
    // Read two bytes (MSB first)
    i2c_read_blocking(i2c_default, VEML6030_I2C_ADDR, data, sizeof(data), false);
    //data [0] contains the LSB and data[1] the MSB
    return ((uint16_t)data[0]) |((uint16_t) data[1]<<8);
}

void veml6030_stop(){
    uint8_t config[3] = {
        VEML6030_CONFIG_REG,  // Configuration register
        0x00,                 // High byte: Gain 1/8 (00), reserved bits
        0x11                  // Low byte: 100ms integration time (010 in bits 6-8), power on (bit 0 = 0). Power off (last bit to 1)
    };
    
    // Write configuration to sensor
    i2c_write_blocking(i2c_default, VEML6030_I2C_ADDR, config, sizeof(config), false);
    sleep_ms(10);
}




/* ===============================================
 *  TEMPERATURE AND HUMIDITY SENSOR HDC 2021
 * ===============================================  */
// Temperature & Humidity sensor related function
// https://www.ti.com/lit/ds/symlink/hdc2021.pdf?ts=1757522824481&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FHDC2021
// https://www.ti.com/lit/ug/snau250/snau250.pdf?ts=1757438909914

 static int8_t read_hdc2021_register(uint8_t reg) {
    uint8_t data;
    i2c_write(HDC2021_I2C_ADDRESS, &reg, 1, true);
    i2c_read(HDC2021_I2C_ADDRESS, &data, 1, false);
    return data;
}

 static void write_register(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    i2c_write(HDC2021_I2C_ADDRESS, data, sizeof(data), false);
}

 static void hdc2021_reset() {
    uint8_t configContents = read_hdc2021_register(HDC2021_CONFIG);
    write_register(HDC2021_CONFIG, configContents | 0x80);
    sleep_ms(50);
}

static void hdc2021_setMeasurementMode() {
    uint8_t configContents = read_hdc2021_register(HDC2021_MEASUREMENT_CONFIG);
    write_register(HDC2021_MEASUREMENT_CONFIG, configContents & 0xF9);
}

static void hdc2021_setRate() {
    uint8_t configContents = read_hdc2021_register(HDC2021_CONFIG);
    configContents = (configContents & 0x8F) | 0x50; // Set 1 measurement/second
    write_register(HDC2021_CONFIG, configContents);
}

static void hdc2021_setTempRes() {
    uint8_t configContents = read_hdc2021_register(HDC2021_MEASUREMENT_CONFIG);
    write_register(HDC2021_MEASUREMENT_CONFIG, configContents & 0x3F); // 14-bit
}

static void hdc2021_setHumidityRes() {
    uint8_t configContents = read_hdc2021_register(HDC2021_MEASUREMENT_CONFIG);
    write_register(HDC2021_MEASUREMENT_CONFIG, configContents & 0xCF); // 14-bit
}

 static void hdc2021_triggerMeasurement() {
    uint8_t configContents = read_hdc2021_register(HDC2021_MEASUREMENT_CONFIG);
    write_register(HDC2021_MEASUREMENT_CONFIG, configContents | 0x01);
}

void hdc2021_set_low_temp_threshold(float temp) {
    temp = (temp < -40.0f) ? -40.0f : (temp > 125.0f) ? 125.0f : temp;
    uint8_t temp_thresh = (uint8_t)((temp + 40.0f) * 256.0f / 165.0f);
    write_register(HDC2021_TEMP_THR_L, temp_thresh);
}

void hdc2021_set_high_temp_threshold(float temp) {
    temp = (temp < -40.0f) ? -40.0f : (temp > 125.0f) ? 125.0f : temp;
    uint8_t temp_thresh = (uint8_t)((temp + 40.0f) * 256.0f / 165.0f);
    write_register(HDC2021_TEMP_THR_H, temp_thresh);
}

void hdc2021_set_high_humidity_threshold(float humid) {
    humid = (humid < 0.0f) ? 0.0f : (humid > 100.0f) ? 100.0f : humid;
    uint8_t humid_thresh = (uint8_t)(humid * 2.56f);
    write_register(HDC2021_HUMID_THR_H, humid_thresh);
}

void hdc2021_set_low_humidity_threshold(float humid) {
    humid = (humid < 0.0f) ? 0.0f : (humid > 100.0f) ? 100.0f : humid;
    uint8_t humid_thresh = (uint8_t)(humid * 2.56f);
    write_register(HDC2021_HUMID_THR_L, humid_thresh);
}
// By default it sets following modes: 
// Measurement methods: Temp + Measurement
// Sampling rate: 1 Hz
// Temperature resolution: 14 bits
// Humidity resolution: 14 bits
// It triggers continous measurements. 
 void init_hdc2021_() {
    hdc2021_reset();
    hdc2021_set_high_temp_threshold(50);
    hdc2021_set_low_temp_threshold(-30);
    hdc2021_set_high_humidity_threshold(100);
    hdc2021_set_low_humidity_threshold(0);
    hdc2021_setMeasurementMode();
    hdc2021_setRate();
    hdc2021_setTempRes();
    hdc2021_setHumidityRes();
    hdc2021_triggerMeasurement();
}

// Note that sampling rate is 1Hz
float hdc2021_read_temperature() {
    uint8_t reg = HDC2021_TEMP_LOW;
    uint8_t data[2];
    
    i2c_write(HDC2021_I2C_ADDRESS, &reg, 1, true);
    i2c_read(HDC2021_I2C_ADDRESS, data, 2, false);
    uint16_t raw = ((uint16_t) data[1] << 8) | data[0];
    return (raw * 165.0f / 65536.0f) - 40.0f;
}

//Note that sampling rate is 1 HX
float hdc2021_read_humidity() {
    uint8_t reg = HDC2021_HUMIDITY_LOW;
    uint8_t data[2];
    
    i2c_write(HDC2021_I2C_ADDRESS, &reg, 1, true);
    i2c_read(HDC2021_I2C_ADDRESS, data, 2, false);
    
    uint16_t raw = ((uint16_t) data[1] << 8) | data[0];
    return (raw * 100.0f / 65536.0f);
}

void stop_hdc2021() {
    uint8_t cfg = read_hdc2021_register(HDC2021_CONFIG);  // 0x0E
    cfg &= 0x8F;  // clear AMM[2:0] (bits 6:4) -> 000 = AMM disabled
    write_register(HDC2021_CONFIG, cfg);
    // Make sure we don't accidentally retrigger
    uint8_t meas = read_hdc2021_register(HDC2021_MEASUREMENT_CONFIG); // 0x0F
    meas &= (uint8_t)~0x01; // clear MEAS_TRIG (bit 0)
    write_register(HDC2021_MEASUREMENT_CONFIG, meas);
    //turn heater & DRDY pin off to minimize current
    cfg &= ~(uint8_t)(1<<3); // HEAT_EN=0
    cfg &= ~(uint8_t)(1<<2); // DRDY/INT_EN=0 (pin Hi-Z)

}

/* =========================
 *  ICM-42670 (IMU)
 * ========================= */

// IMU related function
// https://invensense.tdk.com/wp-content/uploads/2021/07/DS-000451-ICM-42670-P-v1.0.pdf

float aRes, gRes;      // scale resolutions per LSB for the sensors

static int icm_i2c_write_byte(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    int result = i2c_write_blocking(i2c_default, ICM42670_I2C_ADDRESS, buf, 2, false);
    return result == 2 ? 0 : -1;
}

// helper to read a byte from a register
static int icm_i2c_read_byte(uint8_t reg, uint8_t *value) {
    int result = i2c_write_blocking(i2c_default, ICM42670_I2C_ADDRESS, &reg, 1, true);
    if (result != 1) return -1;
    result = i2c_read_blocking(i2c_default, ICM42670_I2C_ADDRESS, value, 1, false);
    return result == 1 ? 0 : -1;
}

static int icm_i2c_read_bytes(uint8_t reg, uint8_t *buffer, uint8_t len) {
    int result = i2c_write_blocking(i2c_default, ICM42670_I2C_ADDRESS, &reg, 1, true);
    if (result != 1) return -1;
    result = i2c_read_blocking(i2c_default, ICM42670_I2C_ADDRESS, buffer, len, false);
    return result == len ? 0 : -2;
}

static int icm_soft_reset(void) {
    int rc = icm_i2c_write_byte(ICM42670_REG_SIGNAL_PATH_RESET, ICM42670_RESET_CONFIG_BITS);
    sleep_us(400);   // small wait: datasheet calls for ~200 µs before other writes
    //TODO: For making more robust. Wait till the MCKL_READY is on (clock is running again)
    return rc;
}

//TRY TO SOLVE PROBLEM OF FLOATING AD0 pin, JUST IN CASE THE ADDRESS IS CHANGING. 
static int ICM42670_autodetect_address(void) {
    const uint8_t cand[2] = { ICM42670_I2C_ADDRESS, ICM42670_I2C_ADDRESS_ALT };
    for (int i = 0; i < 2; ++i) {
        // Try a few times to avoid picking up a one-off glitch
        int hits = 0;
        for (int t = 0; t < 4; ++t) {
            uint8_t who = 0, reg = ICM42670_REG_WHO_AM_I;
            if (i2c_write_blocking(i2c_default, cand[i], &reg, 1, true) != 1) continue;
            if (i2c_read_blocking(i2c_default, cand[i], &who, 1, false) != 1) continue;
            if (who == ICM42670_WHO_AM_I_RESPONSE) ++hits;
        }
        if (hits >= 3) { return cand[i]; } // majority wins
    }
    return -1;
}

//TODO do initial calibration. 
static void calibrateAccel(float *dest1){

}

static void calibrateGyro(float *dest2){


}

int init_ICM42670() {
    blink_led(5);
    
    //Soft reset
    icm_soft_reset();

    //DETECT ADDRESS FOR AD0 floating pin: 
    int address = ICM42670_autodetect_address();
    if (address == -1){
        printf("Address could not be found");
    }
    else 
        printf ("Address: 0x%02X\n",address);
    // Step 1: Check WHO_AM_I
    uint8_t who = 0;
    if (icm_i2c_read_byte(ICM42670_REG_WHO_AM_I, &who) != 0) {
        return -2;
    };
    if (who != ICM42670_WHO_AM_I_RESPONSE) {
        return -3;
    };   

    // Step 2: Configure INT1 pin - push-pull, active-low, pulsed
    if(icm_i2c_write_byte(ICM42670_INT_CONFIG, ICM42670_INT1_CONFIG_VALUE) != 0){ 
        return -4;
    }
    // tiny guard delay after init writes
    sleep_us(200);
    
    // Step 3: Success
    return 0;
}

int ICM42670_startAccel(uint16_t odr_hz, uint16_t fsr_g) {
    uint8_t fsr_bits = 0;
    uint8_t odr_bits = 0;

    // Map FSR to register bits, aRes (See Datasheet Table 2)
    switch (fsr_g) {
        case 2:  
            fsr_bits = ICM42670_ACCEL_FSR_2G;
            aRes = 16384; 
            break;
        case 4:  
            fsr_bits = ICM42670_ACCEL_FSR_4G;
            aRes = 8192;
            break;
        case 8:  
            fsr_bits = ICM42670_ACCEL_FSR_8G; 
            aRes =4096;
            break;
        case 16: 
            fsr_bits = ICM42670_ACCEL_FSR_16G;
            aRes = 2048;
            break;
        default: return -1; // invalid FSR
    }

    // Map ODR to register bits
    switch (odr_hz) {
        case 25:   odr_bits = ICM42670_ACCEL_ODR_25HZ; break;
        case 50:   odr_bits = ICM42670_ACCEL_ODR_50HZ; break;
        case 100:  odr_bits = ICM42670_ACCEL_ODR_100HZ; break;
        case 200:  odr_bits = ICM42670_ACCEL_ODR_200HZ; break;
        case 400:  odr_bits = ICM42670_ACCEL_ODR_400HZ; break;
        case 800:  odr_bits = ICM42670_ACCEL_ODR_800HZ; break;
        case 1600: odr_bits = ICM42670_ACCEL_ODR_1600HZ; break;
        default:   return -2; // invalid ODR
    }

    // Combine into ACCEL_CONFIG0: [7:5] = fsr, [3:0] = odr
    uint8_t accel_config0_val = (fsr_bits << 5) | (odr_bits & 0x0F);
    int rc = icm_i2c_write_byte(ICM42670_ACCEL_CONFIG0_REG, accel_config0_val);
    sleep_us(200); 
    if (rc != 0) return -3;
    return 0; // success
}

int ICM42670_startGyro(uint16_t odr_hz, uint16_t fsr_dps) {
    uint8_t fsr_bits = 0;
    uint8_t odr_bits = 0;
 
    // Map FSR, gRes (See Datasheet Table 1)
    switch (fsr_dps) {
        case 250:  
            fsr_bits = 0x03;
            gRes = 131; 
            break;
        case 500:  
            fsr_bits = 0x02;
            gRes = 65.5;
            break;
        case 1000: 
            fsr_bits = 0x01;
            gRes = 32.8;
            break;
        case 2000: 
            fsr_bits = 0x00;
            gRes = 16.4;
            break;
        default:   return -1;
    }

    // Map ODR
    switch (odr_hz) {
        case 25:   odr_bits = 0x0B; break;
        case 50:   odr_bits = 0x0A; break;
        case 100:  odr_bits = 0x09; break;
        case 200:  odr_bits = 0x08; break;
        case 400:  odr_bits = 0x07; break;
        case 800:  odr_bits = 0x06; break;
        case 1600: odr_bits = 0x05; break;
        default:   return -2;
    }

    // Write GYRO_CONFIG0
    uint8_t gyro_config0_val = (fsr_bits << 5) | (odr_bits & 0x0F);
    if (icm_i2c_write_byte(ICM42670_GYRO_CONFIG0_REG, gyro_config0_val) != 0) return -3;
    sleep_us(200); 
    return 0;
}

//put in low noise both acc and gyr
int ICM42670_enable_accel_gyro_ln_mode() {
    int rc = icm_i2c_write_byte(ICM42670_PWR_MGMT0_REG , 0x0F); // bits 3:2 = gyro LN, bits 1:0 = accel LN
    sleep_us(200);
    return rc;
}

//Remove gyro and low power mode for accelearoter
int ICM42670_enable_ultra_low_power_mode(void) {
    // Accel = LP (10), Gyro = OFF (00)
    // PWR_MGMT0 = 0b00000010 = 0x02
    int rc = icm_i2c_write_byte(ICM42670_PWR_MGMT0_REG , 0x02);
    sleep_us(200);
    return rc;
}

//Both gyro and acceleremoter in low power. Usually the gyro has lot of errors. 
int ICM42670_enable_accel_gyro_lp_mode(void) {
    // Gyro = 10 (LP), Accel = 10 (LP)
    // 0b00001010 = 0x0A
    int rc = icm_i2c_write_byte(ICM42670_PWR_MGMT0_REG, 0x0A);
    sleep_us(200);
    return rc;
}

int ICM42670_start_with_default_values(void) {
    int rc;

    // Start accelerometer with defaults (e.g., 100 Hz, ±4 g)
    rc = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT,
                             ICM42670_ACCEL_FSR_DEFAULT);
    if (rc != 0) return rc;

    // Start gyroscope with defaults (e.g., 100 Hz, ±250 dps)
    rc = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT,
                            ICM42670_GYRO_FSR_DEFAULT);
    if (rc != 0) return rc;

    // Put both sensors into Low-Noise mode
    rc = ICM42670_enable_accel_gyro_ln_mode();
    if (rc != 0) return rc;

    return 0;
}


int ICM42670_read_sensor_data(float *ax, float *ay, float *az,
    float *gx, float *gy, float *gz,float *t) {
        
        uint8_t raw[14]; // 14 bytes total from TEMP to GYRO Z

        int rc = icm_i2c_read_bytes(ICM42670_SENSOR_DATA_START_REG, raw, sizeof(raw));
        if (rc != 0) return rc;

        // Convert to signed 16-bit integers (big-endian)
        int16_t t_raw = (int16_t)((raw[0] << 8) | raw[1]);
        int16_t ax_raw = ((int16_t)raw[2] << 8) | raw[3];
        int16_t ay_raw = ((int16_t)raw[4] << 8) | raw[5];
        int16_t az_raw = ((int16_t)raw[6] << 8) | raw[7];
        int16_t gx_raw = (int16_t)((raw[8] << 8) | raw[9]);
        int16_t gy_raw = (int16_t)((raw[10] << 8) | raw[11]);
        int16_t gz_raw = (int16_t)((raw[12] << 8) | raw[13]);

        *t = ((float)t_raw / 128.0f)+ 25.0;
        *ax =  (float)ax_raw / aRes; 
        *ay =  (float)ay_raw / aRes; 
        *az =  (float)az_raw / aRes;
        *gx =  (float)gx_raw / gRes; 
        *gy =  (float)gy_raw / gRes; 
        *gz =  (float)gz_raw / gRes;
        return 0; // success
}

