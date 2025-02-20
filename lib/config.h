/*
O arquivo config.h armazena todas as configurações globais do sistema, como pinos, endereços I2C, constantes e parâmetros utilizados em todo o código.
Este arquivo tambem adiciona o modo de Depuração(DEBUG).
Habilita/desabilita mensagens de debug para facilitar o desenvolvimento e manutenções.
*/


#ifndef CONFIG_H
#define CONFIG_H

// Habilita ou desabilita o modo de depuração
#define DEBUG 1  // Defina como 0 para desativar prints

#if DEBUG
    #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
#endif

// Configuração dos pinos do biossensor e botões
#define BIOSSENSOR_OPTICO 26 
#define JOYSTICK_PB 22
       
#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12
#define BUZZER 21
#define BOTAO_A 5
#define BOTAO_B 6

// Configuração do I2C para o display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define DISPLAY_ADDR 0x3C 

// Configuração da matriz de LEDs WS2812
#define MATRIX_PIN 7
#define NUM_LEDS 25

// Configuração do PWM
#define PWM_FREQ 50
#define PWM_WRAP 4095

// Número de amostras para suavizar a leitura do ADC
#define NUM_AMOSTRAS 5

#endif
