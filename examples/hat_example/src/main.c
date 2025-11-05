
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <queue.h>
#include <task.h>

#include <tkjhat/sdk.h>
#include <pico/binary_info.h>

#if CFG_TUSB_OS != OPT_OS_FREERTOS
# error "This should be using FREERTOS but the CFG_TUSB_OS is not OPT_OS_FREERTOS"
#endif

/*#if   CFG_TUSB_OS == OPT_OS_FREERTOS
# pragma message("TinyUSB: FreeRTOS")
#elif CFG_TUSB_OS == OPT_OS_PICO
# pragma message("TinyUSB: OS PICO")
#elif CFG_TUSB_OS == OPT_OS_NONE
# pragma message("TinyUSB: sin RTOS")
#endif*/

// Flag to indicate button press
volatile bool sw1_pressed = false;
volatile bool sw2_pressed = false;

void led_simple_task(void *pwParameters);
void sw1_task(void *pvParameters);
void sw2_task(void *pvParameters);
void led_task(void *pvParameters);
void rgb_task(void *pvParameters);
void buzzer_task(void *pvParameters);
void light_sensor_task(void *pvParameters);
void ths_task(void *pvParameters);
void imu_task(void *pvParameters);


// void display_task(void *pvParameters);
// void mic_task(void *pvParameters);


// // Callback function when PDM samples are ready
// void on_mic_data_ready(){
//     // Read the samples into the sample_buffer
//     samples_read =pdm_microphone_read(sample_buffer, 256);

// }




void sw2_task(void *pvParameters) {
    (void)pvParameters;

    while (1) {
        sw2_pressed = gpio_get(SW2_PIN);
        vTaskDelay(10);
    }
}

void rgb_task(void *pvParameters) {
    (void)pvParameters;

    while (1) {
        rgb_led_write(20, 30, 255);
        vTaskDelay(500);
        rgb_led_write(255, 30, 10);
        vTaskDelay(500);
        rgb_led_write(50, 255, 10);
        vTaskDelay(500);
    }
}

void buzzer_task(void *pvParameters) {
    (void)pvParameters;

    while (1) {
        if (sw2_pressed) {
            buzzer_play_tone(440, 500);
        }
        vTaskDelay(10);
    }
}

void light_sensor_task(void *pvParameters) {
    (void)pvParameters;

    while (1) {
        uint16_t light = veml6030_read_light();
        printf("Light level: %d\n", light);
        vTaskDelay(1000);
    }
}

void ths_task(void *pvParameters) {
    (void)pvParameters;

    while (1) {
        float temp = hdc2021_read_temperature();
        float humid = hdc2021_read_humidity();
        printf("Temperature: %.2f°C, Humidity: %.2f%%\n", temp, humid);
        vTaskDelay(1000);
    }
}


void imu_task(void *pvParameters) {
    (void)pvParameters;
    
    float ax, ay, az, gx, gy, gz, t;

    while (1)
    {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            float temp_c = (float)t / 128.0f;
            printf("Accel: X=%d, Y=%d, Z=%d | Gyro: X=%d, Y=%d, Z=%d | Temp: %.2f°C\n", ax, ay, az, gx, gy, gz, temp_c);

        } else {
            printf("Failed to read imu data\n");
        }
        vTaskDelay(pdMS_TO_TICKS(60));
    }

}

// void display_task(void *pvParameters) {

//     (void)pvParameters;

//     while (1) {
//         clear_display();
//         // write_text("Hello");
//         // draw_circle(1, 1, 20);
//         // draw_line(0, 0, 127, 0);
//         draw_square(30,10,20,20);
//         draw_square(70,10,20,20);
//         vTaskDelay(1000);
//         clear_display();
//         draw_line(30, 20, 50, 20);
//         draw_line(70, 20, 90, 20);
//         vTaskDelay(300);

//     }
// }

// void mic_task(void *pvParameters) {
//     (void)pvParameters;

//     while (1) {
//        if (samples_read !=0){

//         // store and clear the samples read from the callback
//         int sample_count = samples_read;
//         samples_read = 0;

//         // loop through any new collected samples
//         for (int i = 0; i < sample_count; i++) {
//             printf("%b\n", sample_buffer[i]);
//         }
//        }
//     }
// }

//





/**
 * @brief FreeRTOS task for testing the red LED.
 *
 * This task toggles the state of the red LED once at startup,
 * then enters an infinite loop where it simply delays by 1000 ticks
 * on each iteration. The task yields to the scheduler, allowing other
 * tasks to run, but does not continue toggling the LED after the first change.
 *
 * @param[in] pvParameters
 *        Pointer to task parameters. Not used in this implementation,
 *        should be passed as NULL when creating the task.
 *
 * @note This implementation is useful to verify task creation and scheduling,
 *       but it does not produce a blinking LED.
 * @see toggle_red_led()
 * @see vTaskDelay()
 */
void led_simple_task(void *pvParameters) {
    toggle_red_led();
    while(1){
        vTaskDelay(1000);
    };
}

/**
 * @brief FreeRTOS task for monitoring switch SW1 state.
 *
 * This task continuously reads the digital input connected to SW1
 * and stores its state in the global variable @ref sw1_pressed.
 *
 * @param[in] pvParameters
 *        Pointer to task parameters. Not used in this implementation,
 *        should be passed as NULL when creating the task.
 * @see gpio_get()
 * @see vTaskDelay()
 */
void sw1_task(void *pvParameters) {
    (void)pvParameters;

    while (1) {
        
        sw1_pressed = gpio_get(SW1_PIN);
        vTaskDelay(10);
    }
}

/**
 * @brief FreeRTOS task for controlling the red LED.
 *
 * This task continuously sets the red LED output level based on the
 * global variable @ref sw1_pressed, which reflects the current state
 * of switch SW1. If the switch is pressed, the LED is turned on;
 * otherwise, it is turned off.
 * @param[in] pvParameters
 *        Pointer to task parameters. Not used in this implementation,
 *        should be passed as NULL when creating the task.
 *
 * @warning This task depends on @ref sw1_task to update @ref sw1_pressed.
 *          Ensure that @ref sw1_task is running concurrently; otherwise,
 *          the LED state will not change.
 *
 * @see gpio_put()
 * @see vTaskDelay()
 */
void led_task(void *pvParameters) {
    (void)pvParameters;

    while (1) {
        gpio_put(RED_LED_PIN, sw1_pressed);
        /*if (!sw1_pressed){
            printf("Button pressed");
        }*/
        vTaskDelay(100);
    }
}
    /*============================
        MICROPHONE CONFIGURATION
     =============================*/
    //Internal sample buffer for sound samples
    int16_t sample_buffer[MEMS_BUFFER_SIZE];
    int16_t temp_sample_buffer[MEMS_BUFFER_SIZE];//use to have two different buffers.
    volatile int samples_read = 0;    

    void on_sound_buffer_ready(){
        // callback from library when all the samples in the library
        // internal sample buffer are ready for reading 
        samples_read = get_microphone_samples(sample_buffer, MEMS_BUFFER_SIZE);
    }

int main() {
    stdio_init_all();
    // Uncomment this lines if you want to wait till the serial monitor is connected
    while (!stdio_usb_connected()){
        sleep_ms(10);
    } 
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    printf("Start tests\n");
    
    // Initialize LED
    init_red_led();
    printf("Initializing red led\r\n");
    
    //Testing RED LED
    // printf("Testing red led should be on\r\n");
   


    // Test SW1 
    //init_sw1();
    //printf("Initializing switch 1\r\n");

    //Test SW2
    //init_sw2();
    //printf("Initializing switch 2\r\n");

    //Test RGB
    //init_rgb_led();
    //printf("Initializing RGB LED\r\n");


   // Initialize Buzzer
   //init_buzzer();
   //printf("Initializing the buzzer\n");

   
    // Initialize I2C
    //init_i2c_default();
    //printf("Initializing the i2c\n");

    //Initialize Light Sesnsor VEML6030
    //veml6030_init();
    //printf("Initializing the light sensor\n");

    //Initialize the Temp and Humidity Sensor
    //hdc2021_init();
    //printf("Initializing the temp/humidity sensor\n");

    //Initialize the display
    //init_display();
    //printf("Initializating display\n");

    //Initialize the microphone
    //Microhpone test in test_microphone.c

    //Initialize IMU
    if (init_ICM42670() == 0) {
        printf("ICM-42670P initialized successfully!\n");
        int _gyro = ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT);
        printf ("Gyro return:  %d\n", _gyro);
        int _accel = ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT);
        printf ("Accel return:  %d\n", _accel);

        /*
        if (ICM42670_startAccel(ICM42670_ACCEL_ODR_DEFAULT, ICM42670_ACCEL_FSR_DEFAULT) != 0){
            printf("Wrong values to init the accelerometer in ICM-42670P.\n");
        }
        if (ICM42670_startGyro(ICM42670_GYRO_ODR_DEFAULT, ICM42670_GYRO_FSR_DEFAULT) != 0){
            printf("Wrong values to init the gyroscope in ICM-42670P.\n");
        };
        ICM42670_enable_accel_gyro_ln_mode();
        */
       int _enablegyro = ICM42670_enable_accel_gyro_ln_mode();
       printf ("Enable gyro: %d\n",_enablegyro);
    } else {
        printf("Failed to initialize ICM-42670P.\n");
    }
    float ax, ay, az, gx, gy, gz, t;

    while(true){
        set_red_led_status(true);
        //printf("SW1 state: %d\n", gpio_get(SW1_PIN));
        //printf("SW2 state: %d\n", gpio_get(SW2_PIN));
        //buzzer_play_tone(440, 500);
        /*rgb_led_write(255,0,0);
        sleep_ms(1000);
        rgb_led_write(0,255,0);
        sleep_ms(1000);
        rgb_led_write(0,0,255);*/
        //uint16_t reg = _veml6030_read_register(VEML6030_CONFIG_REG);
        //printf("Register: 0x%04X\n",(unsigned int)reg);
        //uint32_t light = veml6030_read_light();
        //printf("Light level: %u\n", light);
        //float temp = hdc2021_read_temperature();
        //float humid = hdc2021_read_humidity();
        //printf("Temperature: %.2f°C, Humidity: %.2f%%\n", temp, humid);
        // clear_display();
        // write_text("Hello");
        // draw_circle(25, 25, 20, true);
        //draw_line(0, 0, 127, 0);
        //draw_square(30,10,20,20, true);
        // draw_square(70,10,20,20, false);
        // write_text_xy(20,40, "Bye");
        

        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            
            printf("Accel: X=%f, Y=%f, Z=%f | Gyro: X=%f, Y=%f, Z=%f| Temp: %2.2f°C\n", ax, ay, az, gx, gy, gz, t);

        } else {
            printf("Failed to read imu data\n");
        }
        sleep_ms(1000);
        

        

    }// end infite loop
        

 


    
    

   

    // Create tasks
    
    // xTaskCreate(sw2_task, "SW2Task", 256, NULL, 1, NULL);
    // xTaskCreate(rgb_task, "RGBTask", 256, NULL, 1, NULL);
    // xTaskCreate(buzzer_task, "BuzzerTask", 256, NULL, 4, NULL);
    // xTaskCreate(light_sensor_task, "LightSensorTask", 256, NULL, 3, NULL);
    // xTaskCreate(ths_task, "THSTask", 256, NULL, 1, NULL);
    // xTaskCreate(imu_task, "IMUTask", 256, NULL, 1, NULL);
    // xTaskCreate(led_simple_task, "LEDSimpleTask", 64, NULL, 1, NULL);
    // xTaskCreate(sw1_task, "SW1Task", 64, NULL, 1, NULL);
    // xTaskCreate(led_task, "LEDTask", 64, NULL, 2, NULL);
 

    //  xTaskCreate(display_task, "DisplayTask", 256, NULL, 2, NULL);
    // // xTaskCreate(mic_task, "MicTask", 256, NULL, 1, NULL);
    
    // Start the FreeRTOS scheduler
    //vTaskStartScheduler();

    return 0;
}

