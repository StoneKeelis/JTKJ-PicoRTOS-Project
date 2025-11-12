/**
 * @file imu_data_collector/main.c
 * @brief IMU Data Collection Tool for Threshold Calibration
 * 
 * PURPOSE:
 * This program helps you collect real accelerometer and gyroscope data
 * while holding the device in different positions (tilt left, middle, right).
 * Use this data to determine accurate thresholds for position detection.
 * 
 * USAGE:
 * 1. Hold the Raspberry Pi Pico in your RIGHT hand
 * 2. Position your thumb on the LEFT BUTTON (SW1/BUTTON1)
 * 3. Tilt the device to desired position (left/middle/right)
 * 4. Press BUTTON1 to capture and print current sensor data
 * 5. Copy the printed data to analyze and determine thresholds
 * 
 * OUTPUT FORMAT:
 * timestamp, ax, ay, az, gx, gy, gz, temp
 * 
 * Where:
 * - timestamp: milliseconds since boot
 * - ax, ay, az: acceleration on X, Y, Z axes (in g units, 1g ≈ 9.81 m/s²)
 * - gx, gy, gz: gyroscope rotation on X, Y, Z axes (in degrees/second)
 * - temp: temperature in Celsius
 * 
 * TIPS:
 * - Collect at least 10-20 samples for each position
 * - Keep the device still when pressing the button
 * - Copy all data to a CSV file for analysis (Excel, Python, etc.)
 * - Look for patterns in the X-axis (ax) values for tilt detection
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <tkjhat/sdk.h>

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Debouncing
volatile uint32_t last_button_time = 0;
#define DEBOUNCE_MS 300

// Sample counter
volatile uint32_t sample_count = 0;

// ============================================================================
// BUTTON INTERRUPT HANDLER
// ============================================================================

/**
 * @brief Button interrupt handler - directly reads and prints IMU data
 * 
 * This interrupt is triggered when BUTTON1 is pressed.
 * It reads the current IMU sensor data and prints it to serial.
 */
static void button_handler(uint gpio, uint32_t events) {
    (void)gpio;
    (void)events;
    
    float ax, ay, az, gx, gy, gz, t;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Debouncing - ignore button presses that are too close together
    if (current_time - last_button_time < DEBOUNCE_MS) {
        return;
    }
    last_button_time = current_time;
    
    // Read current sensor data directly in the interrupt
    if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
        
        uint32_t timestamp = to_ms_since_boot(get_absolute_time());
        sample_count++;
        
        // Print data in CSV format
        printf("%lu, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.2f\n",
               timestamp, ax, ay, az, gx, gy, gz, t);
        
        // Also print human-readable format for quick reference
        printf("# Sample %lu: ax=%.3f | Position hint: ", sample_count, ax);
        if (ax < -0.3f) {
            printf("TILTED LEFT\n");
        } else if (ax > 0.3f) {
            printf("TILTED RIGHT\n");
        } else {
            printf("MIDDLE\n");
        }
        printf("\n");
        fflush(stdout);  // Ensure immediate output
        
    } else {
        printf("✗ ERROR: Failed to read IMU data\n");
    }
}

// ============================================================================
// INITIALIZATION FUNCTION
// ============================================================================

/**
 * @brief Initialize IMU sensor
 * 
 * This function sets up the IMU sensor once at startup.
 * After initialization, the sensor is ready to be read by interrupts.
 */
void init_imu_sensor(void) {
    if (init_ICM42670() == 0) {
        printf("✓ ICM-42670P initialized successfully!\n");
        if (ICM42670_start_with_default_values() != 0){
            printf("✗ ERROR: Could not start accelerometer or gyroscope\n");
        }
    } else {
        printf("✗ ERROR: Failed to initialize ICM-42670P\n");
    }
    
    printf("✓ IMU sensor ready\n");
    printf("✓ Ready to collect data\n\n");
    
    // Print CSV header
    printf("# CSV FORMAT:\n");
    printf("# timestamp_ms, ax, ay, az, gx, gy, gz, temp_c\n");
    printf("# ================================================\n\n");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    stdio_init_all();
    
    // Wait for serial connection (important for data collection!)
    printf("Waiting for serial connection...\n");
    while (!stdio_usb_connected()){
        sleep_ms(100);
    }
    
    init_hat_sdk();
    sleep_ms(300);
    
    // Print header
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║         IMU DATA COLLECTOR                            ║\n");
    printf("║         For Threshold Calibration                     ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("📋 INSTRUCTIONS:\n");
    printf("  1. Hold Pico in RIGHT hand, thumb on BUTTON1 (left button)\n");
    printf("  2. Tilt device to desired position\n");
    printf("  3. Press BUTTON1 to capture data\n");
    printf("  4. Collect 10-20 samples for each position:\n");
    printf("     - Tilted LEFT\n");
    printf("     - MIDDLE (neutral)\n");
    printf("     - Tilted RIGHT\n");
    printf("  5. Copy all output data for analysis\n");
    printf("\n");
    printf("💡 TIP: Focus on the 'ax' (X-axis acceleration) values\n");
    printf("   to determine your tilt thresholds!\n");
    printf("\n");
    printf("════════════════════════════════════════════════════════\n\n");
    
    // Initialize IMU sensor
    init_imu_sensor();
    
    // Initialize button
    init_button1();
    
    // Set up button interrupt with callback
    // The interrupt handler will directly read IMU data when button is pressed
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_FALL, true, button_handler);
    
    printf("✓ Button interrupt configured\n");
    printf("✓ System ready - press BUTTON1 to collect data\n\n");
    
    // Main loop - just keep the program running
    // All data collection happens via interrupt
    while (1) {
        tight_loop_contents();  // Low-power idle loop
    }
    
    return 0;
}

