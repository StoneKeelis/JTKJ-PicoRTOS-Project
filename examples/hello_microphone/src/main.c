#include <stdio.h>
#include <string.h>
#include <hardware/gpio.h>
#include <pico/stdlib.h>
#include <tkjhat/sdk.h>
#include <pico/binary_info.h>
#include <hardware/sync.h>

static inline void _blink(int n){
    for (int i=0;i<n;i++){ 
        toggle_red_led(); 
        sleep_ms(120); 
        toggle_red_led(); 
        sleep_ms(120); 
    }
    gpio_put(RED_LED_PIN,false);
}

    /*============================
    /   MICROPHONE CONFIGURATION
    /=============================*/
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
        sleep_ms(1500); //Wait to see the output.
        init_hat_sdk();
        setvbuf(stdout, NULL, _IONBF, 0);
        printf("Start tests\n");
        
        //Led red off. Only on when transmitting
        init_red_led();
        set_red_led_status(false);

        int is_mic_init = init_pdm_microphone();
        if (is_mic_init < 0){
            printf("PDM microphone initialization failed!\n");
            sleep_ms(1000);
        }
        else 
            printf("Initializing the microphone");
        pdm_microphone_set_callback(on_sound_buffer_ready);
        pdm_microphone_set_filter_max_volume(64); // keep default
        pdm_microphone_set_filter_gain(8);        // safer base gain than 16
        pdm_microphone_set_filter_volume(56);     // was 64 ⇒ lower hiss; raise if still too quiet
        //Each iteration are 5 seconds. 
        while(true){
            //We are going to send 5 seconds. Each sample is two bytes and sampling rate 8Khz. 
            uint32_t target_bytes = MEMS_SAMPLING_FREQUENCY * 2u * 5u;
            uint32_t sent_bytes = 0;
            _blink (5);
            if (is_mic_init >=0) {
                // Wait till usb is ready and after that, turn the mike and inform other end with READY.
                while (!stdio_usb_connected()) 
                    sleep_ms(100);
                if (init_microphone_sampling()<0){
                    printf("Cannot start sampling the microphone\n");
                    sleep_ms(500);
                    continue;
                }
                set_red_led_status(true);
                while (sent_bytes < target_bytes){
                    if (!stdio_usb_connected()) {
                        _blink(1);
                        set_red_led_status(false);
                        break;
                    }
                    if (samples_read == 0){
                        tight_loop_contents(); // yields without sleeping long
                        continue;
                    } 
                    //Collect samples in a temporary buffer.          
                    uint32_t irq = save_and_disable_interrupts();
                    // store and clear the samples read from the callback
                    int sample_count = samples_read;  
                    memcpy(temp_sample_buffer, sample_buffer, (size_t)sample_count * sizeof(sample_buffer[0]));
                    samples_read = 0;// restart the samples read
                    restore_interrupts(irq);

                    // loop through any new collected samples
                    // OPTION 1 using fwrite
                    // First we create a temporary buffer, so i can send data even if I receive another irq. 
                    int sample_sent = fwrite(temp_sample_buffer,sizeof(temp_sample_buffer[0]),sample_count,stdout);
                    sent_bytes += sizeof(temp_sample_buffer[0]) * sample_sent;
                    
                    //stdio_flush();

                    //OPTION 2 using putchar
                    /*for (int i = 0; i < sample_count; i++) {
                        int16_t s = temp_sample_buffer[i];
                        putchar_raw((int8_t)(s & 0xFF));       // LSB
                        ++sent_bytes;
                        putchar_raw((int8_t)(s >> 8));         // MSB
                        ++sent_bytes;
                    }*/
                    //stdio_flush();    

                    //OPTION 3: using printf. Only for showing in graph (e.g. in Arduino Uno plotter)
                    /*for (int i = 0; i < sample_count; i++) {
                        printf("%d\n", temp_sample_buffer[i]);
                        sent_bytes += sizeof(temp_sample_buffer[0]);
                    }
                    stdio_flush();*/
                }
                set_red_led_status(false);
                end_microphone_sampling();
                _blink(3);   
            }
            //Debugging blink.
            _blink(3);
            sleep_ms(5000);
        }   
    return 0;
    }