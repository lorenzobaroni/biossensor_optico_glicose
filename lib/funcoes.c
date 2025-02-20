/*
O Arquivo funcoes.c possui todas as funções principais do programa relacionadas com a manipulação do Display OLED, 
da Matriz de Leds, Interrupções dos botõas A e B, PWM e ADC.
A separação dessas funções aqui deixa o codigo modularizado, organizado e legível.
*/


#include "funcoes.h"

// Definição das variáveis globais --------------------------------------------------------------------------------------------------------------------------------
ssd1306_t ssd;
PIO pio = pio0;
uint offset = 0;
uint sm = 0;
uint border_size = 2;
volatile uint32_t ultimo_tempo_joy = 0;
volatile uint32_t ultimo_tempo_A = 0;
volatile uint32_t ultimo_tempo_B = 0;
const uint32_t debounce = 200;
bool escolha_jejum = false;
bool escolha_feita = false;
bool voltar_menu = false;


// Função de configurações basicas -------------------------------------------------------------------------------------------------------------------------------

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

    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, botao_callback);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, botao_callback);
    gpio_set_irq_enabled_with_callback(JOYSTICK_PB, GPIO_IRQ_EDGE_FALL, true, botao_callback);
}

//Função para exibir o menu inicial --------------------------------------------------------------------------------------------------------------------------------

void menu_inicial() {
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Jejum: 8h", 25, 10);
    ssd1306_draw_string(&ssd, "Sim (A)", 35, 30);
    ssd1306_draw_string(&ssd, "Nao (B)", 35, 40);
    ssd1306_rect(&ssd, 0, 0, WIDTH - 1, HEIGHT - 1, true, false);
    ssd1306_send_data(&ssd);
}

// A seguir são as funções diretamente relacionadas com a Matriz de Leds ws2812 ------------------------------------------------------------------------------------

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

// Define a cor da matriz
void set_matrix_color(uint32_t color) {
    for (int i = 0; i < NUM_LEDS; i++) {
        ws2812_put_pixel(color);
    }
}

void set_matrix_brightness(uint32_t color, uint16_t adc_value) {
    // Normaliza a leitura do ADC para um valor entre 0 e 255 (PWM simulado)
    uint8_t intensidade = (adc_value * 20) / 4095; // Intensidade dos leds

    // Extrai os componentes RGB da cor base
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    // Ajusta a intensidade proporcionalmente
    r = (r * intensidade) / 255;
    g = (g * intensidade) / 255;
    b = (b * intensidade) / 255;

    // Constrói a nova cor PWM ajustada
    uint32_t cor_pwm = (r << 16) | (g << 8) | b;

    // Aplica a nova cor PWM à matriz de LEDs
    set_matrix_color(cor_pwm);
}

// Cores ajustadas dinamicamente para a matriz de LED's WS2812 (g r b)
uint32_t RED    = 0x00FF00;  // Alerta
uint32_t GREEN  = 0xFF0000;  // Normal
uint32_t YELLOW = 0xFFFF00;  // Pré-diabetes
uint32_t BLUE   = 0x0000FF;  // Baixa Glicose em Jejum

//----------------------------------------------------------------------------------------------- Fim das Funções diretamente relacionadas com a Matriz de Leds ws2812

// A seguir são as funções diretamente relacionadas com o Display Oled ssd1306 ---------------------------------------------------------------------------------------

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

void desenhar_coracao() {
    int x = WIDTH - 25;  // Posição X no canto superior direito
    int y = 12;           // Posição Y na parte superior

    // Matriz do coração (10x9 pixels) - 1 = pixel ligado, 0 = pixel apagado
    const uint8_t coracao[11][12] = {
        { 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0 },  
        { 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1 },  
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  
        { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },  
        { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },  
        { 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0 },  
        { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 }   
    };

    // Desenha o coração no display
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < 12; j++) {
            if (coracao[i][j]) {
                ssd1306_pixel(&ssd, x + j, y + i, true);
            }
        }
    }
}

void linhas_display(){
    // Desenha uma linha horizontal no meio do display (altura = 32)
    ssd1306_draw_line(&ssd, 0, 32, WIDTH - 1, 32, true);
    ssd1306_draw_line(&ssd, 0, 33, WIDTH - 1, 33, true);
    ssd1306_draw_line(&ssd, 90, 0, 90, HEIGHT / 2, true);
    ssd1306_draw_line(&ssd, 91, 0, 91, HEIGHT / 2, true);
}

void texto_glicose(int glicose_simulada){
    char buffer[32];
    ssd1306_draw_string(&ssd, "Glicose: ", 10, 10);
    sprintf(buffer, "%d mg/dL", glicose_simulada);
    ssd1306_draw_string(&ssd, buffer, 10, 20);
}

void desenha_borda(){
    for (int i = 0; i < border_size; i++) {
        ssd1306_rect(&ssd, i, i, WIDTH - (2 * i), HEIGHT - (2 * i), true, false);
    }
}

//----------------------------------------------------------------------------------------------- Fim das Funções diretamente relacionadas com o Display Oled ssd1306

// A seguir são as funções diretamente relacionadas com PWM E ADC ---------------------------------------------------------------------------------------------------

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

void leds_pre_diabetes(uint16_t leitura_adc){
    set_led_brightness(LED_BLUE, 0);
    set_led_brightness(LED_RED, leitura_adc);
    set_led_brightness(LED_GREEN, leitura_adc);
    set_matrix_brightness(YELLOW, leitura_adc);
}

void leds_atencao(int glicose_simulada){
    set_led_brightness(LED_BLUE, 0);
    set_led_brightness(LED_GREEN, 0);                               
    // Sincroniza a borda, buzzer e LED vermelho piscando 2 vezes
    piscar_borda_com_buzzer_e_led(BUZZER, LED_RED, 500, 250, 2, glicose_simulada);
}

void leds_nivel_normal(uint16_t leitura_adc){
    set_led_brightness(LED_BLUE, 0);
    set_led_brightness(LED_RED, 0);
    set_led_brightness(LED_GREEN, leitura_adc);
    set_matrix_brightness(GREEN, leitura_adc);
}

//------------------------------------------------------------------------------------------------------------- Fim das Funções diretamente relacionadas com PWM e ADC


// Função para gerar tons no buzzer-----------------------------------------------------------------------------------------------------------------------------------

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

// Função para piscar ao mesmo tempo o Led Rgb, a Matriz de leds, o buzzer e a borda do display ----------------------------------------------------------------------

void piscar_borda_com_buzzer_e_led(uint buzzer, uint led, uint frequencia, uint duracao, uint ciclos, int glicose) {
    for (uint i = 0; i < ciclos; i++) {
        // Liga o LED vermelho e a matriz de LEDs
        set_led_brightness(led, PWM_WRAP);
        set_matrix_brightness(RED, 1000);  // Define a cor da matriz para vermelho durante o alerta

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
        set_matrix_brightness(0, 0); // Apaga a matriz de LEDs

        // Pequeno atraso para sincronizar os ciclos corretamente
        sleep_ms(100);
    }
}

// Função de interrupção para os botões A e B ---------------------------------------------------------------------------------------------------------------------

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