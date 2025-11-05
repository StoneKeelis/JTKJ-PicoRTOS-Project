
#include <stdio.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <task.h>

#include "tkjhat/sdk.h"


#define DEFAULT_STACK_SIZE 2048

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


int main() {
    stdio_init_all();
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    init_button1();
    init_button2();
    init_led();
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, btn_fxn);
    gpio_set_irq_enabled(BUTTON2, GPIO_IRQ_EDGE_RISE, true);

    TaskHandle_t hPrintTask;

    BaseType_t result = xTaskCreate(print_task,  // (en) Task function
                "print",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hPrintTask);         // (en) A handle to control the execution of this task

    // Start the scheduler (never returns)
    vTaskStartScheduler();


    return 0;
}

