
#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <task.h>

#include "tkjhat/sdk.h"


#define DEFAULT_STACK_SIZE 2048

#define INPUT_BUFFER_SIZE 256

static const char *hellotext [] = {
    ".... ",   // H
    ". ",     // E
    ".-.. ",  // L
    ".-.. ",  // L
    "---  ",  // O  (two spaces: end of word)
    ".-- ",   // W
    "--- ",   // O
    ".-. ",   // R
    ".-.. ",  // L
    "-..  ",  // D  (two spaces end of text)
    "\n", 
    NULL
};

static const char *hellotext_debug [] = {
    ".... ",   // H
    ". ",     // E
    ".-.. ",  // L
    ".-.. ",  // L
    "---  ",  // O  (two spaces: end of word)
    "__Some debug goes here__",
    ".-- ",   // W
    "--- ",   // O
    ".-. ",   // R
    ".-.. ",  // L
    "-..  ",  // D  (two spaces end of text)
    "\n", 
    NULL
};


//Alternative using just a string
//static const char hellotext[] = ".... . .-.. .-.. ---  .-- --- .-. .-.. -..  \n"

static volatile uint8_t button_pressed, debug_pressed;
static void btn_fxn(uint gpio, uint32_t eventMask) {
    if (gpio  == BUTTON1)
        button_pressed = true;
    else if (gpio == BUTTON2)
        debug_pressed = true;
    toggle_led();
}


static void print_task(void *arg){
    (void)arg;
    
    while(1){

        if (button_pressed) {
             for (int i = 0; hellotext[i] != NULL; i++) {
                printf("%s", hellotext[i]);
            }
            button_pressed = false;
        }
        if (debug_pressed){
            for (int i = 0; hellotext_debug[i] != NULL; i++) {
                printf("%s", hellotext_debug[i]);
            }
            debug_pressed = false;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void receive_task(void *arg){
    (void)arg;
    char line[INPUT_BUFFER_SIZE];
    size_t index = 0;
    
    while (1){
        //OPTION 1
        // Using getchar_timeout_us https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#group_pico_stdio_1ga5d24f1a711eba3e0084b6310f6478c1a
        // take one char per time and store it in line array, until reeceived the \n
        // The application should instead play a sound, or blink a LED. 
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT){// I have received a character
            if (c == '\r') continue; // ignore CR, wait for LF if (ch == '\n') { line[len] = '\0';
            if (c == '\n'){
                // terminate and process the collected line
                line[index] = '\0'; 
                printf("__[RX]:\"%s\"__\n", line); //Print as debug in the output
                index = 0;
                vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
            }
            else if(index < INPUT_BUFFER_SIZE - 1){
                line[index++] = (char)c;
            }
            else { //Overflow: print and restart the buffer with the new character. 
                line[INPUT_BUFFER_SIZE - 1] = '\0';
                printf("__[RX]:\"%s\"__\n", line);
                index = 0; 
                line[index++] = (char)c; 
            }
        }
        else {
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
        }
        //OPTION 2. Use the whole buffer. 
        /*absolute_time_t next = delayed_by_us(get_absolute_time,500);//Wait 500 us
        int read = stdio_get_until(line,INPUT_BUFFER_SIZE,next);
        if (read == PICO_ERROR_TIMEOUT){
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
        }
        else {
            line[read] = '\0'; //Last character is 0
            printf("__[RX] \"%s\"\n__", line);
            vTaskDelay(pdMS_TO_TICKS(50));
        }*/
    }


}


int main() {
    stdio_init_all();
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    init_button1();
    init_button2();
    init_led();
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, btn_fxn);
    gpio_set_irq_enabled(BUTTON2, GPIO_IRQ_EDGE_RISE, true);

    TaskHandle_t hPrintTask, hReceiveTask;

    BaseType_t result = xTaskCreate(print_task,  // (en) Task function
                "print",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hPrintTask);         // (en) A handle to control the execution of this task
    result = xTaskCreate(receive_task,  // (en) Task function
                "receive",             // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hReceiveTask);         // (en) A handle to control the execution of this task

    // Start the scheduler (never returns)
    vTaskStartScheduler();


    return 0;
}

