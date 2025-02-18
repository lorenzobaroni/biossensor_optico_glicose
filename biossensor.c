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
#define BUZZER 21

#define PWM_FREQ 50
#define PWM_WRAP 4095

#define NUM_AMOSTRAS 5  // Número de amostras para a média

// Variáveis globais
ssd1306_t ssd;
bool led_enabled = true;

// Aplicando uma média para suavizar a oscilação do ADC
uint16_t filtrar_adc() {
    uint32_t soma = 0;
    for (int i = 0; i < NUM_AMOSTRAS; i++) {
        soma += adc_read();
        sleep_ms(10);  // Pequeno delay para estabilizar a leitura
    }
    return soma / NUM_AMOSTRAS;
}

// Função para gerar tons no buzzer
void tone(uint buzzer, uint frequencia, uint duracao) {
    uint32_t periodo = 1000000 / frequencia; 
    uint32_t meio_periodo = periodo / 2;    
    uint32_t ciclos = frequencia * duracao / 1000;

    for (uint32_t i = 0; i < ciclos; i++) {
        gpio_put(buzzer, 1); 
        sleep_us(meio_periodo);
        gpio_put(buzzer, 0); 
        sleep_us(meio_periodo);
    }
}

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

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    gpio_put(BUZZER, 0);
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
    setup_pwm(LED_RED);
    setup_pwm(LED_GREEN);

    while (true) {
        uint16_t leitura_adc = filtrar_adc(); 

        // Calcula os valores de PWM para controle dos LEDs, dependendo da posição do joystick
        uint16_t pwm_y = led_enabled ? abs(leitura_adc - 2048) : 0;
        
        // Define o brilho do LED azul baseado na posição Y do joystick
        if ((leitura_adc > 2180 && leitura_adc < 4078) || (leitura_adc > 16 && leitura_adc < 2140)) {
            set_led_brightness(LED_BLUE, leitura_adc);
            set_led_brightness(LED_RED, 0);
            set_led_brightness(LED_GREEN, 0);
        } 
        else if (leitura_adc >= 4078 || leitura_adc <= 16) {
            set_led_brightness(LED_RED, PWM_WRAP);  // LED vermelho com brilho máximo
            tone(BUZZER, 200, 500);
            set_led_brightness(LED_BLUE, 0);  // Desliga o LED azul
            set_led_brightness(LED_GREEN, 0);
        } 
        else {
            set_led_brightness(LED_BLUE, 0);
            set_led_brightness(LED_RED, 0);
            set_led_brightness(LED_GREEN, 1000);
        }
        
        float tensao = (leitura_adc * 3.3) / 4095; 

        // Simulação da glicose no sangue (70-200 mg/dL)
        int glicose_simulada = (int)((tensao * 131.0 / 3.3) + 70);
        printf("ADC: %d | Tensão: %.2fV | Glicose Simulada: %d mg/dL\n", leitura_adc, tensao, glicose_simulada);

        // Limpa a tela antes de exibir um novo valor
        ssd1306_rect(&ssd, 10, 10, 115, 50, false, true); // Limpa apenas a área do texto

        // Buffer para armazenar o texto
        char buffer[32];
        ssd1306_draw_string(&ssd, "Glicose: ", 10, 10);
        sprintf(buffer, "%d mg/dL", glicose_simulada);

        if (glicose_simulada < 80) {
            ssd1306_draw_string(&ssd, "ALERTA:", 10, 40);
            ssd1306_draw_string(&ssd, "Baixa Glicose!", 10, 50);
        } else if (glicose_simulada > 140) {
            ssd1306_draw_string(&ssd, "ALERTA:", 10, 40);
            ssd1306_draw_string(&ssd, "Alta Glicose!", 10, 50);
        }        

        ssd1306_rect(&ssd, 0, 0, WIDTH - 1, HEIGHT - 1, true, false);

        // Exibe a glicose no display
        ssd1306_draw_string(&ssd, buffer, 10, 20);
        ssd1306_send_data(&ssd);

        sleep_ms(50);
    }

    return 0;
}