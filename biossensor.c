#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"

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

#define PWM_FREQ 50
#define PWM_WRAP 4095

// Variáveis globais
ssd1306_t ssd;
bool led_enabled = true;

void setup_config(){ 
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, DISPLAY_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    adc_init();  
    adc_gpio_init(BIOSSENSOR_OPTICO); 
    adc_select_input(0);
}

void setup_pwm(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin); 
    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_clkdiv(slice, 125.0); 
    pwm_set_enabled(slice, true);
}

// Função para definir o brilho de um LED usando PWM
void set_led_brightness(uint pin, uint16_t value) {
    uint slice = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    pwm_set_chan_level(slice, channel, value);
}

int main() {

    stdio_init_all(); 

    setup_config();

    setup_pwm(LED_BLUE);

    while (1) {
        uint16_t leitura_adc = adc_read(); 

        // Calcula os valores de PWM para controle dos LEDs, dependendo da posição do joystick
        uint16_t pwm_y = led_enabled ? abs(leitura_adc - 2048) : 0;
        
        // Define o brilho do LED azul baseado na posição Y do joystick
        if (leitura_adc > 2190 || leitura_adc < 2140) {
            set_led_brightness(LED_BLUE, leitura_adc);
        } else {
            set_led_brightness(LED_BLUE, 0);
        }
        
        float tensao = (leitura_adc * 3.3) / 4095; 

        // Simulação da glicose no sangue (70-200 mg/dL)
        int glicose_simulada = (int)((tensao / 3.3) * (201 - 70) + 70);
        printf("ADC: %d | Tensão: %.2fV | Glicose Simulada: %d mg/dL\n", leitura_adc, tensao, glicose_simulada);

        sleep_ms(1000);
    }

    return 0;
}