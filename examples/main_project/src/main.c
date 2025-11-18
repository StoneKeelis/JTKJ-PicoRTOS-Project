
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

// Default stack size for the tasks. It can be reduced to 1024 if task is not using lot of memory.
#define DEFAULT_STACK_SIZE 2048
#define MESSAGE_BUFFER_SIZE 2048

// Tilt thresholdit
#define TILT_LEFT_THRESHOLD  -0.3f
#define TILT_RIGHT_THRESHOLD  0.3f

// Tilt enumit
enum tilt_state {
    TILT_LEFT = 0,
    TILT_MIDDLE = 1,
    TILT_RIGHT = 2,
    TILT_UNKNOWN = 3
};

// Tilaesittely
enum state {
    IDLE = 0,
    RECORDING = 1,
    SENDING = 2,
    RECEIVING = 3,
    DISPLAY_UPDATE = 4,
};

// Globaalit tilamuuttujat
volatile enum state system_state = IDLE;

// Globaaalit muuttujat
volatile enum tilt_state current_tilt = TILT_UNKNOWN;
char message_buffer[MESSAGE_BUFFER_SIZE];
volatile uint16_t message_index = 0;
volatile uint32_t last_button_time = 0;
#define DEBOUNCE_MS 200
#define WORD_LENGTH 4

// Buffer vastaanotetuille viesteille
char received_buffer[128];
volatile uint16_t received_index = 0;

/*
Buffereihin käytetty chatgpt:n apua
Promptilla: "Miten voin hyödyntää bufferia koodissa viestin käsittelyyn?"
Koodia on muokattu aloittelijaystävällisemmäksi kasvattamalla indeksiä vasta bufferiin lisäämisen jälkeen
sekä myös lisäämällä globaali muuttuja sanan pituudelle.
*/

// Yksittäinen interrupt handler joka reitittää molemmat napit
static void button_handler(uint gpio, uint32_t events) {
    (void)events;

    // debouncing ettei nappi rekisteröidy useasti
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_button_time < DEBOUNCE_MS) {
        return;
    }
    last_button_time = current_time;

    if (gpio == BUTTON2) {
        // Vasen nappi = lisää symboli
        if (system_state == IDLE) {
            system_state = RECORDING;
        }

        if (system_state == RECORDING && message_index < MESSAGE_BUFFER_SIZE - WORD_LENGTH) {
            switch (current_tilt) {
                case TILT_LEFT:
                    message_buffer[message_index] = '.';
                    message_index++;
                    break;
                case TILT_MIDDLE:
                    message_buffer[message_index] = ' ';
                    message_index++;
                    break;
                case TILT_RIGHT:
                    message_buffer[message_index] = '-';
                    message_index++;
                    break;
                default:
                    break;
            }
            message_buffer[message_index] = '\0';
        }
        return;
    }

    if (gpio == BUTTON1 && system_state == RECORDING) {
        // Oikea nappi = lähetä viesti
        
        if (message_index > 0 && message_index < MESSAGE_BUFFER_SIZE - 3) {
            message_buffer[message_index] = ' ';
            message_index++;
            message_buffer[message_index] = ' ';
            message_index++;
            message_buffer[message_index] = '\n';
            message_index++;
            message_buffer[message_index] = '\0';
            
            system_state = SENDING;
        }

        //system_state = IDLE;
    }
}

// IMU task - seuraa kaltevuutta
static void imu_task(void *pvParameters) {
    (void)pvParameters;
    
    float ax, ay, az, gx, gy, gz, t;
    
    // Setting up the sensor. 
    if (init_ICM42670() == 0) {
        printf("ICM-42670P initialized successfully!\n");
        if (ICM42670_start_with_default_values() != 0){
            printf("ERROR: Could not start accelerometer or gyroscope\n");
            vTaskDelete(NULL);
        }
    } else {
        printf("ERROR: Failed to initialize ICM-42670P.\n");
        vTaskDelete(NULL);
    }
    
    printf("IMU task running...\n");
    
    for (;;) {
        if (system_state == RECORDING) {
            if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
                // määritetään kaltevuus
                if (ax < TILT_LEFT_THRESHOLD) {
                    current_tilt = TILT_LEFT;
                } else if (ax > TILT_RIGHT_THRESHOLD) {
                    current_tilt = TILT_RIGHT;
                } else {
                    current_tilt = TILT_MIDDLE;
                }
            } else {
                printf("ERROR: Failed to read sensor\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void receiver_task(void *pvParameters) {
    while(1) {
        int ch = getchar_timeout_us(0);

        if (ch != PICO_ERROR_TIMEOUT) {
            if (system_state == IDLE || system_state == RECEIVING) {
                system_state = RECEIVING;

                // Lisää merkki bufferiin
                if (received_index < 127 && ch != '\n') {
                    received_buffer[received_index] = (char)ch;
                    received_index++;
                    received_buffer[received_index] = '\0';
                }
                
                // Kun viesti on valmis, näytä se
                if (ch == '\n') {
                    system_state = DISPLAY_UPDATE;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void display_task(void *pvParameters) {
    while (1) {
        if (system_state == DISPLAY_UPDATE) {
            clear_display();
            write_text(received_buffer);
            
            // Soita morse-koodi summerilla
            int len = strlen(received_buffer);
            for (int i = 0; i < len; i++) {
                switch (received_buffer[i]) {
                    case '.':
                        buzzer_play_tone(1000, 100);
                        sleep_ms(100);
                        break;
                    case '-':
                        buzzer_play_tone(1000, 300);
                        sleep_ms(100);
                        break;
                    case ' ':
                        sleep_ms(700);
                        break;
                }
            }
            
            // Anna aikaa lukea näyttö
            vTaskDelay(pdMS_TO_TICKS(1500));
            
            // Tyhjennä receive buffer
            received_index = 0;
            received_buffer[0] = '\0';

            system_state = IDLE;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void sender_task(void *pvParameters) {
    while (1) {
        if (system_state == SENDING) {
            printf("%s", message_buffer);
            
            message_index = 0;
            message_buffer[0] = '\0';

            clear_display();
            write_text("Msg sent!");

            system_state = IDLE;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

int main() {
    stdio_init_all();
    
    // odotetaan että USB on yhdistetty
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    
    printf("Starting...\n");
    
    init_hat_sdk();
    sleep_ms(300);
    
    init_display();
    clear_display();
    write_text("LCD is OK");

    // alusta buzzer
    init_buzzer();

    // alusta bufferit
    message_buffer[0] = '\0';
    message_index = 0;
    received_buffer[0] = '\0';
    received_index = 0;
    
    // alusta napit
    init_button1();
    init_button2();
    
    // Rekisteröi interrupt molemille napeille
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, button_handler);
    gpio_set_irq_enabled(BUTTON2, GPIO_IRQ_EDGE_FALL, true);
    
    // IMU taski
    TaskHandle_t hIMUTask = NULL;

    BaseType_t result = xTaskCreate(
        imu_task, // taski funktio
        "IMU", // taski nimi
        DEFAULT_STACK_SIZE, // stackin koko
        NULL, // taski argumentit
        3, // prioriteetti
        &hIMUTask // handle
    );

    // Receiver taski
    TaskHandle_t hReceiverTask = NULL;

    BaseType_t result_r = xTaskCreate(
        receiver_task, // taski funktio
        "RECEIVER", // taski nimi
        DEFAULT_STACK_SIZE, // stackin koko
        NULL, // taski argumentit
        2, // prioriteetti
        &hReceiverTask // handle
    );

    // Display taski
    TaskHandle_t hDisplayTask = NULL;

    BaseType_t result_d = xTaskCreate(
        display_task, // taski funktio
        "DISPLAY", // taski nimi
        DEFAULT_STACK_SIZE, // stackin koko
        NULL, // taski argumentit
        2, // prioriteetti
        &hDisplayTask // handle
    );

    // Sender taski
    TaskHandle_t hSenderTask = NULL;

    BaseType_t result_s = xTaskCreate(
        sender_task, // taski funktio
        "SENDER", // taski nimi
        DEFAULT_STACK_SIZE, // stackin koko
        NULL, // taski argumentit
        2, // prioriteetti
        &hSenderTask // handle
    );

    /*
    // (en) We create a task
    BaseType_t result1 = xTaskCreate(
        myTask1Fxn,  // (en) Task function
        "MY_TASK1",  // (en) Name of the task
        STACKSIZE,  // (en) Size of the stack for this task
        (void *) 1, // (en) Arguments of the task
        2,  // (en) Priority of this task
        &myTaskHandle1 // (en) A handle to control the execution of this task
    );
    */
    
    if(result != pdPASS || result_r != pdPASS || result_d != pdPASS) {
        printf("Task creation failed\n");
        return 0;
    }
 
    // Start the scheduler (never returns)
    vTaskStartScheduler();

    // Never reach this line.
    return 0;
}
