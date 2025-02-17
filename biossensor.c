#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define DISPLAY_ADDR 0x3C 

#define BIOSSENSOR_OPTICO 26 
#define JOYSTICK_PB 22
       
#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12

#define PWM_FREQ 50
#define PWM_WRAP 4095

void setup_config(){ 

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    adc_init();  
    adc_gpio_init(26); 
    adc_select_input(0);
}

int main() {

    stdio_init_all(); 
    
    setup_config();

    while (1) {
        uint16_t leitura_adc = adc_read(); 
        float tensao = (leitura_adc * 3.3) / 4095; 
        // Teste
        printf("Leitura ADC: %d | Tensão: %.2fV\n", leitura_adc, tensao);
        sleep_ms(1000);
    }

    return 0;
}