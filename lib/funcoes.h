#ifndef FUNCOES_H
#define FUNCOES_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "ssd1306.h"
#include "config.h"

// Declaração das variáveis globais (extern para evitar múltiplas definições)
extern ssd1306_t ssd;
extern uint border_size;

extern PIO pio;
extern uint sm;
extern uint offset;

extern volatile uint32_t ultimo_tempo_A;
extern volatile uint32_t ultimo_tempo_B;
extern volatile uint32_t ultimo_tempo_joy;
extern const uint32_t debounce;

extern bool escolha_jejum;
extern bool escolha_feita;
extern bool voltar_menu;

// Declaração das funções (as implementações estão em `funcoes.c`)
void init_matrix();
void ws2812_put_pixel(uint32_t pixel_grb);
void set_matrix_color(uint32_t color);
void set_matrix_brightness(uint32_t color, uint16_t adc_value);
uint16_t filtrar_adc();
void setup_pwm(uint pin);
void set_led_brightness(uint pin, uint16_t value);
void tone(uint buzzer, uint frequencia, uint duracao);
void glicose_alerta(int glicose);
void glicose_alerta_jejum(int glicose);
void piscar_borda_com_buzzer_e_led(uint buzzer, uint led, uint frequencia, uint duracao, uint ciclos, int glicose);
void menu_inicial();
void botao_callback(uint gpio, uint32_t eventos);
void desenhar_coracao();
void linhas_display();
void leds_pre_diabetes(uint16_t leitura_adc);
void leds_atencao(int glicose_simulada);
void leds_nivel_normal(uint16_t leitura_adc);
void texto_glicose(int glicose_simulada);
void desenha_borda();
void setup_config();

#endif
