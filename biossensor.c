#include "lib/funcoes.h"

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
                // 🔹 Modo Jejum (começa de 97 mg/dL)
                glicose_simulada = (int)((tensao * (130.0 - 60.0) / 3.3) + 60);
            } else {
                // 🔹 Modo Normal ((começa de 139 mg/dL))
                glicose_simulada = (int)((tensao * (201.0 - 70.0) / 3.3) + 70);
            }

            printf("ADC: %d | Tensão: %.2fV | Glicose Simulada: %d mg/dL\n", leitura_adc, tensao, glicose_simulada);

            // Se escolheu "Sim (A)" 
            if(escolha_jejum){               
                // Define o brilho do LED azul baseado na posição Y do joystick
                if ((leitura_adc > 2180 && leitura_adc < 3870)) {
                    leds_pre_diabetes(leitura_adc);

                } else if (leitura_adc >= 3870 || leitura_adc <= 575) {
                    leds_atencao(glicose_simulada);

                } else {     
                    leds_nivel_normal(leitura_adc);
                }

                // Limpa a tela antes de exibir um novo valor
                ssd1306_rect(&ssd, 10, 10, 115, 50, false, true); // Limpa apenas a área do texto

                // Buffer para armazenar o texto
                texto_glicose(glicose_simulada);

                desenhar_coracao();

                linhas_display();

                glicose_alerta_jejum(glicose_simulada);

                desenha_borda();
                
                ssd1306_send_data(&ssd);

                sleep_ms(50);
            } 
            // Se escolheu "Não (B)" 
            else {                               
                // Define o brilho do LED azul baseado na posição Y do joystick
                if ((leitura_adc > 2180 && leitura_adc < 4078)) {
                    leds_pre_diabetes(leitura_adc);

                } else if (leitura_adc >= 4078 || leitura_adc <= 16) {
                    leds_atencao(glicose_simulada);

                } else {
                    leds_nivel_normal(leitura_adc); 
                }
                
                // Limpa a tela antes de exibir um novo valor
                ssd1306_rect(&ssd, 10, 10, 115, 50, false, true); // Limpa apenas a área do texto

                // Buffer para armazenar o texto
                texto_glicose(glicose_simulada);

                desenhar_coracao();

                linhas_display();

                glicose_alerta(glicose_simulada);

                desenha_borda();

                ssd1306_send_data(&ssd);

                sleep_ms(50);
            }
        }
    }
    return 0;
}