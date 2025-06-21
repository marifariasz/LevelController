# ✨ Sistema de Controle Automático de Nível de Água IoT com Interface Web
<p align="center"> Repositório dedicado ao sistema de controle automático de nível de água utilizando a placa Raspberry Pi Pico W com RP2040, que monitora o nível de água em tempo real, controla bomba d'água automaticamente e oferece interface web para monitoramento e configuração remota.</p>

## :clipboard: Apresentação da tarefa
Para este trabalho foi necessário implementar um sistema IoT de controle automático de nível de água que monitora continuamente o nível através de sensor ADC e controla uma bomba d'água baseado em thresholds configuráveis. O sistema oferece indicação visual através de LEDs convencionais, matriz de LEDs WS2812B, display OLED e alertas sonoros via buzzer. Adicionalmente, possui conectividade WiFi com servidor web integrado para monitoramento remoto e configuração de parâmetros.

## :dart: Objetivos

- Implementar monitoramento contínuo de nível de água via sensor ADC
- Estabelecer controle automático de bomba d'água baseado em thresholds mínimo e máximo
- Fornecer múltiplas formas de indicação visual: LEDs convencionais, matriz LED WS2812B 5x5 e display OLED SSD1306
- Implementar alertas sonoros via buzzer quando nível máximo é atingido
- Estabelecer conectividade WiFi com servidor web para monitoramento remoto
- Permitir configuração remota de thresholds via interface web
- Exibir dados em tempo real no display OLED (nível em % e status da bomba)
- Implementar controle manual através de botão físico para habilitar/desabilitar LEDs
- Garantir funcionamento estável com debounce de botões e proteções de segurança

## :books: Descrição do Projeto
Utilizou-se a placa Raspberry Pi Pico W com o microcontrolador RP2040 para criar um sistema completo de controle automático de nível de água. O sistema monitora o nível através de um sensor conectado ao ADC (pino 28) e controla uma bomba d'água via relé (pino 16) baseado em thresholds configuráveis.
O sistema oferece múltiplas formas de indicação visual:

- LEDs convencionais: LED vermelho (pino 13) para nível baixo e LED verde (pino 11) para nível normal
- Matriz LED WS2812B: Matriz 5x5 (pino 7) que exibe padrões visuais representando 5 níveis diferentes de água
- Display OLED SSD1306: Mostra nível em porcentagem e status da bomba em tempo real via I2C

O controle automático da bomba segue a lógica: liga quando nível < threshold mínimo e desliga quando nível ≥ threshold máximo. Um buzzer (pino 21) emite alertas quando o nível máximo é atingido. Um botão (pino 6) permite habilitar/desabilitar a exibição dos LEDs.
O sistema estabelece conexão WiFi e hospeda um servidor web que oferece:

- API REST: Endpoint /nivel retorna dados em JSON, endpoint /valor_min_max/ permite configuração de thresholds
- Interface Web: Página HTML para monitoramento visual e configuração remota dos parâmetros

## :walking: Integrantes do Projeto
- Bruna Alves
- Eder Renato 
- Matheus Pereira
- Mariana Farias
- 
## :bookmark_tabs: Funcionamento do Projeto
O sistema opera através de várias funções principais organizadas em um loop principal:

- Inicialização: Configuração de I2C, ADC, GPIO, PWM, inicialização da matriz LED WS2812B e conexão WiFi
- Leitura de Sensores: Função read_water_level() que coleta dados do ADC do sensor de nível
- Controle da Bomba: Função control_pump() que implementa a lógica de liga/desliga baseada nos thresholds
- Indicação Visual: Funções control_leds() e control_buzzer_matrix() que controlam os diferentes tipos de LEDs
- Interface de Usuário: Função handle_button() com debounce para controle manual dos LEDs
- Servidor Web: Funções de inicialização WiFi, servidor HTTP e processamento de requisições REST
- Display OLED: Função update_display() que atualiza informações em tempo real
- Matriz LED: Funções para controle da matriz WS2812B com padrões visuais para cada nível

## :eyes: Observações

- O sistema utiliza bibliotecas lwIP para networking e servidor HTTP integrado
- A conectividade WiFi utiliza credenciais configuráveis (WIFI_SSID e WIFI_PASS)
- Implementa debounce de 200ms no botão para evitar leituras múltiplas
- A matriz LED WS2812B utiliza PIO (Programmable I/O) para controle preciso dos timings
- Thresholds padrão: mínimo 250 ADC, máximo 550 ADC (configuráveis via web)
- Sistema de proteção impede funcionamento da bomba acima do nível máximo
- Interface web responsiva permite monitoramento e configuração remota em tempo real

## :arrow_forward: Vídeo no youtube mostrando o funcionamento do programa na placa Raspberry Pi Pico W
<p align="center">
    <a href="[LINK_DO_VIDEO]">Clique aqui para acessar o vídeo</a>
</p>
