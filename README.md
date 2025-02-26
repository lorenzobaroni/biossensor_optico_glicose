# 🚀 Monitoramento de Glicose com Biossensor e Alerta Visual/Sonoro

## 📌 Sobre o Projeto
Este projeto consiste em um **sistema embarcado para monitoramento de glicose**, utilizando a Raspberry Pi Pico W e um biossensor óptico para a leitura de glicose, aqui está sendo simulado com o Joystick da BitDogLab. O sistema apresenta os resultados em um **display OLED SSD1306**, além de emitir **alertas visuais e sonoros** por meio de LEDs RGB e um buzzer. O sistema também conta com uma **matriz de LEDs WS2812** para indicação de níveis de glicose e um **joystick** para controle do biossensor simulado.

## 🎥 Demonstração
O vídeo com a execução da simulação pode ser acessado em:
[🔗 Link para o vídeo](https://www.youtube.com/watch?v=zqj2er6IKrg&ab_channel=LorenzoBaroni)

## 🎯 Objetivos
- Criar um sistema de monitoramento embarcado de glicose.
- Exibir os valores simulados em um **display OLED**.
- Emitir alertas **visuais (LEDs e Matriz WS2812)** e **sonoros (Buzzer)** em situações críticas.
- Implementar um **menu interativo** para seleção de "Jejum" ou "Modo Normal".
- Controlar os LEDs e alertas **baseados nos níveis de glicose lidos**.

## 🛠 Tecnologias Utilizadas
- **Raspberry Pi Pico W**
- **Linguagem C**
- **Display OLED SSD1306** (Protocolo I2C)
- **Matriz de LEDs WS2812** (Controle via PIO)
- **Buzzer** (Alertas Sonoros)
- **Joystick/Biossensor** (Simulação de leitura de glicose)
- **GPIOs para controle de LEDs RGB e botões**

## 🏗 Estrutura do Projeto

```
📂 biossensor_optico_glicose
├── 📂 lib
│   ├── funcoes.h       # Cabeçalho das funções
│   ├── funcoes.c       # Implementação das funções do sistema
│   ├── ssd1306.h       # Biblioteca para o display OLED
│   ├── ssd1306.c       # Implementação do display OLED
│   ├── config.h        # Configuração dos pinos e periféricos
│   ├── ws2812.pio.h    # Biblioteca para controle da matriz WS2812
│   ├── font.h          # Fonte personalizada para o display OLED
├── biossensor.c        # Arquivo principal do projeto
├── README.md           # Documentação do projeto
├── CMakeLists.txt      # Script de compilação para Raspberry Pi Pico
└── diagram.json        # Integração com Wokwi
```

## 📜 Funcionalidades
- **Leitura Simulada de Glicose**: A partir do **joystick**, simulamos a variação da glicose com base na entrada analógica.
- **Exibição no Display OLED**: Mostra a glicose em mg/dL e indicações de risco.
- **Alertas Visuais e Sonoros**:
  - **LEDs RGB** indicam o nível de glicose (Verde = Normal, Amarelo = Pré-Diabetes, Vermelho = Perigo).
  - **Matriz WS2812** reflete o estado atual da glicose.
  - **Display OLED** Borda altera de tamanho em níveis críticos.
  - **Buzzer** emite sinais sonoros em níveis críticos.
- **Modo de Seleção (Jejum/Normal)**: O usuário pode escolher um dos modos ao iniciar o sistema.
- **Depuração via Serial**: Impressão de valores no terminal para testes e ajustes.

## 🔌 Hardware e Conexões

| Componente | Pino | Descrição |
|------------|------|------------|
| Biossensor/Joystick | GPIO 26 | Entrada Analógica (ADC) |
| Botão A | GPIO 5 | Seleção de "Jejum" |
| Botão B | GPIO 6 | Seleção de "Modo Normal" |
| LED Vermelho | GPIO 13 | Indica nível crítico |
| LED Verde | GPIO 11 | Indica nível normal |
| LED Verde + LED Vermelho | GPIO 11, 13 | Indica nível pré-diabetes |
| Buzzer | GPIO 21 | Emite som em alerta |
| Display OLED (SDA) | GPIO 14 | Comunicação I2C |
| Display OLED (SCL) | GPIO 15 | Comunicação I2C |
| Matriz WS2812 | GPIO 7 | Iluminação Indicativa |

## 🖥️ Execução do Projeto
### 🔧 Instalação e Configuração
### 1️⃣ Configurar o ambiente
Antes de compilar e rodar o código, certifique-se de ter:
- **SDK do Raspberry Pi Pico** corretamente instalado.
- **Compilador GCC ARM** para processadores Cortex M0+.
- **CMake e Ninja** para build do projeto.

### 2️⃣ Clonar o repositório
```sh
git clone https://github.com/lorenzobaroni/biossensor_optico_glicose
```

### 3️⃣ Compilar o projeto

### 4️⃣ Transferir o firmware para a Pico W
- Conecte a **Pico W** ao PC em **modo BOOTSEL**.
- Copie o arquivo `biossensor.uf2` gerado para a unidade da Pico.

### 📡 Monitoramento via Serial
Para visualizar os dados enviados pela Raspberry Pi Pico W abra um Monitor Serial e acompanhe as informações.

## 📝 Licença
Este programa foi desenvolvido como um exemplo educacional e pode ser usado livremente para fins de estudo e aprendizado.

## 📌 Autor
LORENZO GIUSEPPE OLIVEIRA BARONI

