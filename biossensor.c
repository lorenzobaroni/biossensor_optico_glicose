#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
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
#define BOTAO_A 5
#define BOTAO_B 6

#define MATRIX_PIN 7
#define NUM_LEDS 25

#define PWM_FREQ 50
#define PWM_WRAP 4095

#define NUM_AMOSTRAS 5  // Número de amostras para a média

#define BRILHO 0.1  // Ajuste entre 0.0 (apagado) e 1.0 (brilho total)


// Variáveis globais
ssd1306_t ssd;
bool led_enabled = true;
bool border_style = true;
int border_size = 2;

PIO pio = pio0;
uint sm;
uint offset;

volatile bool escolha_feita = false;
volatile bool escolha_jejum = false;
volatile bool voltar_menu = false;

volatile uint32_t ultimo_tempo_A = 0;
volatile uint32_t ultimo_tempo_B = 0;
volatile uint32_t ultimo_tempo_joy = 0;
const uint32_t debounce = 200; 

// Inicializa a Matriz WS2812
void init_matrix() {
    offset = pio_add_program(pio, &ws2812_program);
    sm = pio_claim_unused_sm(pio, true);
    ws2812_program_init(pio, sm, offset, MATRIX_PIN, 800000, false);
}

void ws2812_put_pixel(uint32_t pixel_grb) {
    // Aguarda o FIFO estar disponível
    while (pio_sm_is_tx_fifo_full(pio, sm));
    // Envia os bits do pixel no formato GRB
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

// Função para reduzir brilho sem distorcer a cor
uint32_t ajustar_brilho(uint32_t cor, float fator) {
    uint8_t r = (cor >> 16) & 0xFF;  // Extrai o componente Vermelho (R)
    uint8_t g = (cor >> 8) & 0xFF;   // Extrai o componente Verde (G)
    uint8_t b = cor & 0xFF;          // Extrai o componente Azul (B)

    // Aplica o fator de brilho corretamente para cada canal
    r = (uint8_t)(r * fator);
    g = (uint8_t)(g * fator);
    b = (uint8_t)(b * fator);

    // Reconstroi a cor reduzida
    return (r << 16) | (g << 8) | b;
}

// Cores ajustadas dinamicamente
uint32_t RED    = 0x00FF00;  // Alerta
uint32_t GREEN  = 0xFF0000;  // Normal
uint32_t YELLOW = 0xFFFF00;  // Pré-diabetes
uint32_t BLUE   = 0x0000FF;  // Baixa Glicose em Jejum

// Função para converter RGB em formato GRB (usado pelo WS2812)
uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

// Define a cor da matriz
void set_matrix_color(uint32_t color) {
    for (int i = 0; i < NUM_LEDS; i++) {
        ws2812_put_pixel(color);
    }
}

// Aplicando uma média para suavizar a oscilação do ADC
uint16_t filtrar_adc() {
    uint32_t soma = 0;
    for (int i = 0; i < NUM_AMOSTRAS; i++) {
        soma += adc_read();
        sleep_ms(10);  // Pequeno delay para estabilizar a leitura
    }
    return soma / NUM_AMOSTRAS;
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

void glicose_alerta(int glicose){
    // 🔹 Redesenha a mensagem de alerta corretamente
    if (glicose < 80) {
        ssd1306_draw_string(&ssd, "ALERTA:", 10, 40);
        ssd1306_draw_string(&ssd, "Baixa Glicose!", 10, 50);
    } else if (glicose >= 80 && glicose <= 140) {
        ssd1306_draw_string(&ssd, "Nivel Normal", 10, 40); 
    } else if (glicose >= 140 && glicose <= 199) {
        ssd1306_draw_string(&ssd, "ATENCAO:", 10, 40);
        ssd1306_draw_string(&ssd, "Pre-Diabetes!", 10, 50);
    } else if (glicose >= 200) {
        ssd1306_draw_string(&ssd, "ALERTA:", 10, 40);
        ssd1306_draw_string(&ssd, "Diabetes!", 10, 50);
    }
}

void glicose_alerta_jejum(int glicose){
    // 🔹 Redesenha a mensagem de alerta corretamente
    if (glicose < 70) {
        ssd1306_draw_string(&ssd, "ALERTA:", 10, 40);
        ssd1306_draw_string(&ssd, "Baixa Glicose!", 10, 50);
    } else if (glicose >= 70 && glicose <= 99) {
        ssd1306_draw_string(&ssd, "Nivel Normal", 10, 40);
    } else if (glicose >= 100 && glicose <= 125) {
        ssd1306_draw_string(&ssd, "ATENCAO:", 10, 40);
        ssd1306_draw_string(&ssd, "Pre-Diabetes!", 10, 50);
    } else if (glicose >= 126) {
        ssd1306_draw_string(&ssd, "ALERTA:", 10, 40);
        ssd1306_draw_string(&ssd, "Diabetes!", 10, 50);
    }
}

void piscar_borda_com_buzzer_e_led(uint buzzer, uint led, uint frequencia, uint duracao, uint ciclos, int glicose) {
    for (uint i = 0; i < ciclos; i++) {
        // Liga o LED vermelho e a matriz de LEDs
        set_led_brightness(led, PWM_WRAP);
        set_matrix_color(RED);  // Define a cor da matriz para vermelho durante o alerta

        // Toca o buzzer
        tone(buzzer, frequencia, duracao);

        // Limpa apenas a borda anterior para evitar sobreposição de gráficos
        for (int j = 0; j < border_size; j++) {
            ssd1306_rect(&ssd, j, j, WIDTH - (2 * j), HEIGHT - (2 * j), false, false);
        }

        // Alterna o tamanho da borda para criar o efeito de piscar
        border_size = (border_size == 2) ? 4 : 2;

        // Desenha a nova borda
        for (int j = 0; j < border_size; j++) {
            ssd1306_rect(&ssd, j, j, WIDTH - (2 * j), HEIGHT - (2 * j), true, false);
        }

        // Exibir alerta de glicose conforme o modo selecionado
        if (escolha_jejum) {
            glicose_alerta_jejum(glicose);
        } else {
            glicose_alerta(glicose);
        }

        // Atualiza o display OLED
        ssd1306_send_data(&ssd);

        // Aguarda para manter o estado ligado de todos os componentes
        sleep_ms(duracao);

        // Desliga o LED vermelho, buzzer e a matriz de LEDs ao mesmo tempo
        set_led_brightness(led, 0);
        gpio_put(buzzer, 0);
        set_matrix_color(0x000000);  // Apaga a matriz de LEDs

        // Pequeno atraso para sincronizar os ciclos corretamente
        sleep_ms(100);
    }
}

bool menu_inicial() {
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Jejum: 8h", 25, 10);
    ssd1306_draw_string(&ssd, "Sim (A)", 35, 30);
    ssd1306_draw_string(&ssd, "Nao (B)", 35, 40);
    ssd1306_rect(&ssd, 0, 0, WIDTH - 1, HEIGHT - 1, true, false);
    ssd1306_send_data(&ssd);
}

// Função de interrupção para os botões
void botao_callback(uint gpio, uint32_t eventos) {
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());

    if (gpio == BOTAO_A && (tempo_atual - ultimo_tempo_A > debounce)) {
        ultimo_tempo_A = tempo_atual;
        escolha_jejum = true;
        escolha_feita = true;
    }
    if (gpio == BOTAO_B && (tempo_atual - ultimo_tempo_B > debounce)) {
        ultimo_tempo_B = tempo_atual;
        escolha_jejum = false;
        escolha_feita = true;
    }
    if (gpio == JOYSTICK_PB && (tempo_atual - ultimo_tempo_joy > debounce)) {
        ultimo_tempo_joy = tempo_atual;
        voltar_menu = true;  // Sinaliza que deve voltar ao menu
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

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    gpio_init(JOYSTICK_PB);
    gpio_set_dir(JOYSTICK_PB, GPIO_IN);
    gpio_pull_up(JOYSTICK_PB);

    init_matrix();

    // Ajusta brilho das cores
    RED    = ajustar_brilho(RED, BRILHO);
    GREEN  = ajustar_brilho(GREEN, BRILHO);
    YELLOW = ajustar_brilho(YELLOW, BRILHO);
    BLUE   = ajustar_brilho(BLUE, BRILHO);

    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, botao_callback);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, botao_callback);
    gpio_set_irq_enabled_with_callback(JOYSTICK_PB, GPIO_IRQ_EDGE_FALL, true, botao_callback);
}

int main() {
    stdio_init_all(); 
    setup_config();

    setup_pwm(LED_BLUE);
    setup_pwm(LED_RED);
    setup_pwm(LED_GREEN);

    while (true) {
        menu_inicial();
        escolha_feita = false;
        voltar_menu = false;

        while (!escolha_feita && !voltar_menu) {
            sleep_ms(50);  // Espera a escolha do usuário ou o retorno ao menu
        }

        if (voltar_menu) {
            continue; // Volta diretamente para o menu
        }

        while (!voltar_menu) {
            uint16_t leitura_adc = filtrar_adc(); 
            float tensao = (leitura_adc * 3.3) / 4095;
            int glicose_simulada;
            
            if (escolha_jejum) {
                // 🔹 Modo Jejum (começa de 90 mg/dL)
                glicose_simulada = (int)((tensao * (99.0 - 90.0) / 3.3) + 90);
            } else {
                // 🔹 Modo Normal (padrão)
                glicose_simulada = (int)((tensao * (201.0 - 70.0) / 3.3) + 70);
            }

            printf("ADC: %d | Tensão: %.2fV | Glicose Simulada: %d mg/dL\n", leitura_adc, tensao, glicose_simulada);

            if(escolha_jejum){
                glicose_simulada = (int)((tensao * (130.0 - 60.0) / 3.3) + 60);
                // Calcula os valores de PWM para controle dos LEDs, dependendo da posição do joystick
                uint16_t pwm_y = led_enabled ? abs(leitura_adc - 2048) : 0;
                
                // Define o brilho do LED azul baseado na posição Y do joystick
                if ((leitura_adc > 2180 && leitura_adc < 3870)) {
                    set_led_brightness(LED_BLUE, 0);
                    set_led_brightness(LED_RED, leitura_adc);
                    set_led_brightness(LED_GREEN, leitura_adc);
                    set_matrix_color(YELLOW);
                }
                else if (leitura_adc >= 3870 || leitura_adc <= 575) {
                    set_led_brightness(LED_BLUE, 0);
                    set_led_brightness(LED_GREEN, 0);
                    set_matrix_color(RED);
                
                    // Sincroniza a borda, buzzer e LED vermelho piscando 2 vezes
                    piscar_borda_com_buzzer_e_led(BUZZER, LED_RED, 500, 250, 2, glicose_simulada);
                }
                            
                else {
                    set_led_brightness(LED_BLUE, 0);
                    set_led_brightness(LED_RED, 0);
                    set_led_brightness(LED_GREEN, leitura_adc);
                    set_matrix_color(GREEN);
                }

                // Limpa a tela antes de exibir um novo valor
                ssd1306_rect(&ssd, 10, 10, 115, 50, false, true); // Limpa apenas a área do texto

                // Buffer para armazenar o texto
                char buffer[32];
                ssd1306_draw_string(&ssd, "Glicose: ", 10, 10);
                sprintf(buffer, "%d mg/dL", glicose_simulada);
                ssd1306_draw_string(&ssd, buffer, 10, 20);

                // Chama o alerta adequado
                glicose_alerta_jejum(glicose_simulada);

                // Desenha a borda da tela, com tamanho alternável
                for (int i = 0; i < border_size; i++) {
                    ssd1306_rect(&ssd, i, i, WIDTH - (2 * i), HEIGHT - (2 * i), true, false);
                }
                
                ssd1306_send_data(&ssd);

                sleep_ms(50);
            } else {
                // Se escolheu "Não (B)", esse espaço fica reservado para o novo programa
                glicose_simulada = (int)((tensao * (201.0 - 70.0) / 3.3) + 70);
                // Calcula os valores de PWM para controle dos LEDs, dependendo da posição do joystick
                uint16_t pwm_y = led_enabled ? abs(leitura_adc - 2048) : 0;
                
                // Define o brilho do LED azul baseado na posição Y do joystick
                if ((leitura_adc > 2180 && leitura_adc < 4078)) {
                    set_led_brightness(LED_BLUE, 0);
                    set_led_brightness(LED_RED, leitura_adc);
                    set_led_brightness(LED_GREEN, leitura_adc);
                    set_matrix_color(YELLOW);
                } 
                else if (leitura_adc >= 4078 || leitura_adc <= 16) {
                    set_led_brightness(LED_BLUE, 0);
                    set_led_brightness(LED_GREEN, 0);
                    set_matrix_color(RED);
                                
                    // Sincroniza a borda, buzzer e LED vermelho piscando 2 vezes
                    piscar_borda_com_buzzer_e_led(BUZZER, LED_RED, 500, 250, 2, glicose_simulada);
                }
                                            
                else {
                    set_led_brightness(LED_BLUE, 0);
                    set_led_brightness(LED_RED, 0);
                    set_led_brightness(LED_GREEN, leitura_adc);
                    set_matrix_color(GREEN);
                }
                
                // Limpa a tela antes de exibir um novo valor
                ssd1306_rect(&ssd, 10, 10, 115, 50, false, true); // Limpa apenas a área do texto
                // Buffer para armazenar o texto
                char buffer[32];
                ssd1306_draw_string(&ssd, "Glicose: ", 10, 10);
                sprintf(buffer, "%d mg/dL", glicose_simulada);
                ssd1306_draw_string(&ssd, buffer, 10, 20);

                glicose_alerta(glicose_simulada);
                for (int i = 0; i < border_size; i++) {
                    ssd1306_rect(&ssd, i, i, WIDTH - (2 * i), HEIGHT - (2 * i), true, false);
                }
                ssd1306_send_data(&ssd);
            }
        }
    }
    return 0;
}