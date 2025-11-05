
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

// Exercise 4. Include the libraries necessaries to use the usb-serial-debug, and tinyusb
// Tehtävä 4 . Lisää usb-serial-debugin ja tinyusbin käyttämiseen tarvittavat kirjastot.



#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1


// Tehtävä 3: Tilakoneen esittely Add missing states.
// Exercise 3: Definition of the state machine. Add missing states.
enum state { WAITING=1};
enum state programState = WAITING;

// Tehtävä 3: Valoisuuden globaali muuttuja
// Exercise 3: Global variable for ambient light
uint32_t ambientLight;

static void btn_fxn(uint gpio, uint32_t eventMask) {
    uint8_t pinValue = gpio_get(LED1);
    pinValue = !pinValue;
    gpio_put(LED1, pinValue);
    toggle_led();
    // Tehtävä 1: Vaihda LEDin tila.
    //            Tarkista SDK, ja jos et löydä vastaavaa funktiota, sinun täytyy toteuttaa se itse.
    // Exercise 1: Toggle the LED. 
    //             Check the SDK and if you do not find a function you would need to implement it yourself. 
}

static void sensor_task(void *arg){
    (void)arg;
    // Tehtävä 2: Alusta valoisuusanturi. Etsi SDK-dokumentaatiosta sopiva funktio.
    // Exercise 2: Init the light sensor. Find in the SDK documentation the adequate function.
   
    for(;;){
        
        // Tehtävä 2: Muokkaa tästä eteenpäin sovelluskoodilla. Kommentoi seuraava rivi.
        //             
        // Exercise 2: Modify with application code here. Comment following line.
        //             Read sensor data and print it out as string; 
        //tight_loop_contents(); 


        // Tehtävä 2: Lue valoisuusanturin arvo ja tulosta se debug-ikkunaan.
        //            Etsi SDK-dokumentaatiosta sopiva funktio.
        // Exercise 2: Read the value of the light sensor and print it out to debug window.
        //             Find in the SDK documentation the adequate function.
        uint32_t sensorValue = veml6030_read_light();
        printf("Luminance: %lu lux\n", sensorValue);


        // Tehtävä 3:  Muokkaa aiemmin Tehtävässä 2 tehtyä koodia ylempänä.
        //             Jos olet oikeassa tilassa, tallenna anturin arvo tulostamisen sijaan
        //             globaaliin muuttujaan.
        //             Sen jälkeen muuta tilaa.
        // Exercise 3: Modify previous code done for Exercise 2, in previous lines. 
        //             If you are in adequate state, instead of printing save the sensor value 
        //             into the global variable.
        //             After that, modify state





        
        // Exercise 2. Just for sanity check. Please, comment this out
        // Tehtävä 2: Just for sanity check. Please, comment this out
        printf("sensorTask\n");

        // Do not remove this
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void print_task(void *arg){
    (void)arg;
    
    while(1){
        
        // Tehtävä 3: Kun tila on oikea, tulosta sensoridata merkkijonossa debug-ikkunaan
        //            Muista tilamuutos
        //            Älä unohda kommentoida seuraavaa koodiriviä.
        // Exercise 3: Print out sensor data as string to debug window if the state is correct
        //             Remember to modify state
        //             Do not forget to comment next line of code.
        tight_loop_contents();
        


        
        // Exercise 4. Use the usb_serial_print() instead of printf or similar in the previous line.
        //             Check the rest of the code that you do not have printf (substitute them by usb_serial_print())
        //             Use the TinyUSB library to send data through the other serial port (CDC 1).
        //             You can use the functions at https://github.com/hathach/tinyusb/blob/master/src/class/cdc/cdc_device.h
        //             You can find an example at hello_dual_cdc
        //             The data written using this should be provided using csv
        //             timestamp, luminance
        // Tehtävä 4. Käytä usb_serial_print()-funktiota printf:n tai vastaavien sijaan edellisellä rivillä.
        //            Tarkista myös muu koodi ja varmista, ettei siinä ole printf-kutsuja
        //            (korvaa ne usb_serial_print()-funktiolla).
        //            Käytä TinyUSB-kirjastoa datan lähettämiseen toisen sarjaportin (CDC 1) kautta.
        //            Voit käyttää funktioita: https://github.com/hathach/tinyusb/blob/master/src/class/cdc/cdc_device.h
        //            Esimerkki löytyy hello_dual_cdc-projektista.
        //            Tällä menetelmällä kirjoitettu data tulee antaa CSV-muodossa:
        //            timestamp, luminance




        // Exercise 3. Just for sanity check. Please, comment this out
        // Tehtävä 3: Just for sanity check. Please, comment this out
        printf("printTask\n");
        
        // Do not remove this
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


// Exercise 4: Uncomment the following line to activate the TinyUSB library.  
// Tehtävä 4:  Poista seuraavan rivin kommentointi aktivoidaksesi TinyUSB-kirjaston. 

/*
static void usbTask(void *arg) {
    (void)arg;
    while (1) {
        tud_task();              // With FreeRTOS wait for events
                                 // Do not add vTaskDelay. 
    }
}*/

int main() {

    // Exercise 4: Comment the statement stdio_init_all(); 
    //             Instead, add AT THE END OF MAIN (before vTaskStartScheduler();) adequate statements to enable the TinyUSB library and the usb-serial-debug.
    //             You can see hello_dual_cdc for help
    //             In CMakeLists.txt add the cfg-dual-usbcdc
    //             In CMakeLists.txt deactivate pico_enable_stdio_usb
    // Tehtävä 4:  Kommentoi lause stdio_init_all();
    //             Sen sijaan lisää MAIN LOPPUUN (ennen vTaskStartScheduler();) tarvittavat komennot aktivoidaksesi TinyUSB-kirjaston ja usb-serial-debugin.
    //             Voit katsoa apua esimerkistä hello_dual_cdc.
    //             Lisää CMakeLists.txt-tiedostoon cfg-dual-usbcdc
    //             Poista CMakeLists.txt-tiedostosta käytöstä pico_enable_stdio_usb

    stdio_init_all();

    // Uncomment this lines if you want to wait till the serial monitor is connected
    /*while (!stdio_usb_connected()){
        sleep_ms(10);
    }*/ 
    
    init_hat_sdk();
    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    gpio_init(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);
    
    gpio_init(LED1);
    gpio_set_dir(LED1, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, btn_fxn);
    // Exercise 1: Initialize the button and the led and define an register the corresponding interrupton.
    //             Interruption handler is defined up as btn_fxn
    // Tehtävä 1:  Alusta painike ja LEd ja rekisteröi vastaava keskeytys.
    //             Keskeytyskäsittelijä on määritelty yläpuolella nimellä btn_fxn



    
    
    TaskHandle_t hSensorTask, hPrintTask, hUSB = NULL;

    // Exercise 4: Uncomment this xTaskCreate to create the task that enables dual USB communication.
    // Tehtävä 4: Poista tämän xTaskCreate-rivin kommentointi luodaksesi tehtävän,
    // joka mahdollistaa kaksikanavaisen USB-viestinnän.

    /*
    xTaskCreate(usbTask, "usb", 2048, NULL, 3, &hUSB);
    #if (configNUMBER_OF_CORES > 1)
        vTaskCoreAffinitySet(hUSB, 1u << 0);
    #endif
    */


    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(sensor_task, // (en) Task function
                "sensor",                        // (en) Name of the task 
                DEFAULT_STACK_SIZE,              // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                            // (en) Arguments of the task 
                2,                               // (en) Priority of this task
                &hSensorTask);                   // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Sensor task creation failed\n");
        return 0;
    }
    result = xTaskCreate(print_task,  // (en) Task function
                "print",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hPrintTask);         // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Print Task creation failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    // Never reach this line.
    return 0;
}

