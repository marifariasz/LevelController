#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "web_site.h"
#include "hardware/pwm.h"      // Modulação por largura de pulso (PWM) para buzzer
#include "hardware/clocks.h"   // Controle de clocks do sistema
#include "ws2818b.pio.h"       // Programa PIO para controlar LEDs WS2812B
#include "hardware/pio.h"      // Interface para Programmable I/O (PIO)

// Definições dos pinos
#define WATER_LEVEL_ADC_PIN 28     // ADC
#define PUMP_RELAY_PIN 16           // Relé da bomba
#define RED_LED_PIN 13              
#define GREEN_LED_PIN 11            
#define PUSH_BUTTON_PIN 6     
#define OLED_I2C_SDA_PIN 15         // SDA do OLED  
#define OLED_I2C_SCL_PIN 14         // SCL do OLED  
#define I2C_OLED_PORT i2c1 
#define BUZZER 21
#define LED_PIN 7              // Pino para matriz de LEDs WS2812B
uint sm;                           // Máquina de estado do PIO para LEDs
#define LED_COUNT 25               // Número total de LEDs na matriz 5x5

// Constantes do sistema
#define DEBOUNCE_TIME_MS 200         // Tempo de debounce do botão
#define SAMPLE_INTERVAL_MS 100      // Intervalo entre leituras do ADC

// Definições dos thresholds (valores ADC)
uint16_t WATER_LEVEL_MIN_THRESHOLD = 250; // Nível mínimo
uint16_t WATER_LEVEL_MAX_THRESHOLD = 550; // Nível máximo

// Variáveis globais do sistema
bool leds_enabled = true;              // Estado dos LEDs
bool pump_active = false;              // Estado da bomba
bool button_last_state = false;        // Último estado do botão
absolute_time_t last_button_time;      
uint16_t water_level_adc = 0;          // Variável nível
ssd1306_t oled;


// Funções para a matriz de leds

// Estrutura para representar um pixel RGB na matriz de LEDs
struct pixel_t {
    uint8_t G, R, B;           // Componentes de cor: verde, vermelho, azul
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;       // Tipo para LEDs NeoPixel (WS2812B)

// Variáveis globais para controle da matriz de LEDs
npLED_t leds[LED_COUNT];       // Array que armazena o estado de cada LED
PIO np_pio;                    // Instância do PIO para controlar a matriz
void npDisplayDigit(int digit);
// Matrizes que definem os padrões de exibição na matriz de LEDs (5x5 pixels)
const uint8_t digits[5][5][5][3] = {
    // Situação 1: nível mínimo
    {
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}}
    },
    // Situação 2
    {
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}}
    },
    // Situação 3
    {
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}}
    },
    // Situação 4
    {
        {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}}
    },
    // Situação 5: nível máximo
    {
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}},
        {{0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}, {0, 0, 100}}
    }
};

// Define as cores de um LED na matriz
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    leds[index].R = r; // Componente vermelho
    leds[index].G = g; // Componente verde
    leds[index].B = b; // Componente azul
}

// Limpa a matriz de LEDs, exibindo o padrão de dígito 4 (padrão para limpar)
void npClear() {
    npDisplayDigit(5);
}

// Inicializa a matriz de LEDs WS2812B usando o PIO
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program); // Carrega programa PIO
    np_pio = pio0; // Usa PIO0
    sm = pio_claim_unused_sm(np_pio, true); // Reserva uma máquina de estado
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f); // Inicializa PIO
    npClear(); // Limpa a matriz ao inicializar
}

// Escreve os dados dos LEDs na matriz
void npWrite() {
    for (uint i = 0; i < LED_COUNT; i++) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G); // Envia componente verde
        pio_sm_put_blocking(np_pio, sm, leds[i].R); // Envia componente vermelho
        pio_sm_put_blocking(np_pio, sm, leds[i].B); // Envia componente azul
    }
    sleep_us(100); // Pequeno atraso para estabilizar a comunicação
}

// Calcula o índice de um LED na matriz com base nas coordenadas (x, y)
int getIndex(int x, int y) {
    if (y % 2 == 0) {
        return 24 - (y * 5 + x); // Linhas pares: ordem direta
    } else {
        return 24 - (y * 5 + (4 - x)); // Linhas ímpares: ordem invertida
    }
}

// Exibe um dígito ou padrão na matriz de LEDs
void npDisplayDigit(int digit) {
    for (int coluna = 0; coluna < 5; coluna++) {
        for (int linha = 0; linha < 5; linha++) {
            int posicao = getIndex(linha, coluna); // Calcula índice do LED
            npSetLED(posicao, digits[digit][coluna][linha][0], // Componente R
                              digits[digit][coluna][linha][1], // Componente G
                              digits[digit][coluna][linha][2]); // Componente B
        }
    }
    npWrite(); // Atualiza a matriz com os novos dados
}

// Função para o Buzzer
// Toca um som no buzzer com frequência e duração especificadas
void play_buzzer(uint pin, uint frequency, uint duration_ms) {
    gpio_set_function(pin, GPIO_FUNC_PWM); // Configura o pino como PWM
    uint slice_num = pwm_gpio_to_slice_num(pin); // Obtém o slice PWM
    pwm_config config = pwm_get_default_config(); // Carrega configuração padrão
    // Ajusta o divisor de clock para atingir a frequência desejada
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (frequency * 4096));
    pwm_init(slice_num, &config, true); // Inicializa o PWM
    pwm_set_gpio_level(pin, 2048); // Define duty cycle (~50%)
    sleep_ms(duration_ms); // Aguarda a duração do som
    pwm_set_gpio_level(pin, 0); // Desliga o buzzer
    pwm_set_enabled(slice_num, false); // Desativa o PWM
}

// Inicializando
void init_hardware() {
    // Inicializar stdio
    stdio_init_all();
    
    // Configurar I2C
    i2c_init(I2C_OLED_PORT, 400000);
    gpio_set_function(OLED_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_I2C_SDA_PIN);
    gpio_pull_up(OLED_I2C_SCL_PIN);

    // Configurar ADC
    adc_init();
    adc_gpio_init(WATER_LEVEL_ADC_PIN);
    adc_select_input(2); // ADC1 (GPIO 28)
    
    // Configurar GPIO
    gpio_init(PUMP_RELAY_PIN);
    gpio_set_dir(PUMP_RELAY_PIN, GPIO_OUT);
    gpio_put(PUMP_RELAY_PIN, 0); // Bomba inicialmente desligada
    
    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_put(RED_LED_PIN, 0);
    
    gpio_init(GREEN_LED_PIN);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_put(GREEN_LED_PIN, 0);
    
    gpio_init(PUSH_BUTTON_PIN);
    gpio_set_dir(PUSH_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(PUSH_BUTTON_PIN);

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    
    // Inicializar tempo
    last_button_time = get_absolute_time();
    
    // Inicializar OLED
    ssd1306_init(&oled, 128, 64, false, 0x3C, I2C_OLED_PORT);
    ssd1306_config(&oled);
    ssd1306_fill(&oled, false);
    ssd1306_send_data(&oled);
}

// Função para ler o ADC
uint16_t read_water_level() {
    return adc_read();
}

// Função para controlar os LEDs
void control_leds(uint16_t water_level) {
    if (!leds_enabled) {
        // Se os LEDs estão desabilitados, apagar ambos
        gpio_put(RED_LED_PIN, 0);
        gpio_put(GREEN_LED_PIN, 0);
        return;
    }
    
    if (water_level < WATER_LEVEL_MIN_THRESHOLD) {
        // Nível abaixo do mínimo - LED vermelho ligado, verde apagado
        gpio_put(RED_LED_PIN, 1);
        gpio_put(GREEN_LED_PIN, 0);
    } else if (water_level >= WATER_LEVEL_MIN_THRESHOLD && water_level < WATER_LEVEL_MAX_THRESHOLD) {
        // Nível entre mínimo e máximo - LED verde ligado, vermelho apagado
        gpio_put(RED_LED_PIN, 0);
        gpio_put(GREEN_LED_PIN, 1);
    } else {
        // Nível no máximo ou acima - ambos LEDs apagados
        gpio_put(RED_LED_PIN, 0);
        gpio_put(GREEN_LED_PIN, 0);
    }
}


// Função para controlar matriz de leds e o buzzer


void control_buzzer_matrix(uint16_t water_level) {
    if (!leds_enabled) {
        // Se os LEDs estão desabilitados, apagar ambos
        npClear();
    }
    if (water_level < WATER_LEVEL_MIN_THRESHOLD) {
        // Nível abaixo do mínimo - LED vermelho ligado, verde apagado
        npDisplayDigit(0);
    } else if (water_level >= WATER_LEVEL_MIN_THRESHOLD && water_level < 300) {
        // Nível entre mínimo e máximo - LED verde ligado, vermelho apagado
        npDisplayDigit(1);

    }else if (water_level >= WATER_LEVEL_MIN_THRESHOLD && water_level < 450) {
        // Nível entre mínimo e máximo - LED verde ligado, vermelho apagado
        npDisplayDigit(2);

    }else if (water_level >= WATER_LEVEL_MIN_THRESHOLD && water_level <500) {
        // Nível entre mínimo e máximo - LED verde ligado, vermelho apagado
        npDisplayDigit(3);
    } 
    else {
        // Nível no máximo ou acima - ambos LEDs apagados
        npDisplayDigit(4);
        play_buzzer(BUZZER, 3000, 100);

    }
}


// Função para controlar a bomba
void control_pump(uint16_t water_level) {
    if (water_level < WATER_LEVEL_MIN_THRESHOLD && !pump_active) {
        // Ativar bomba quando nível estiver abaixo do mínimo
        pump_active = true;
        gpio_put(PUMP_RELAY_PIN, 1);
        printf("Bomba LIGADA - Nível baixo detectado (ADC: %d)\n", water_level);
    } else if (water_level >= WATER_LEVEL_MAX_THRESHOLD && pump_active) {
        // Desativar bomba quando nível atingir o máximo
        pump_active = false;
        gpio_put(PUMP_RELAY_PIN, 0);
        printf("Bomba DESLIGADA - Nível máximo atingido (ADC: %d)\n", water_level);
    }
    
    // Proteção adicional: desligar bomba se passar do máximo
    /*if (water_level > WATER_LEVEL_MAX_THRESHOLD && pump_active) {
        pump_active = false;
        gpio_put(PUMP_RELAY_PIN, 0);
        printf("Bomba DESLIGADA - Proteção: nível acima do máximo (ADC: %d)\n", water_level);
    }
    */
}

// Função para tratar o botão com debounce
void handle_button() {
    bool current_button_state = !gpio_get(PUSH_BUTTON_PIN); // Inverte porcausa do pull-up
    absolute_time_t current_time = get_absolute_time();
    
    if (absolute_time_diff_us(last_button_time, current_time) > (DEBOUNCE_TIME_MS * 1000)) {
        // Detectar borda de subida (botão pressionado)
        if (current_button_state && !button_last_state) {
            leds_enabled = !leds_enabled;
            last_button_time = current_time;
            
            printf("Botão pressionado - LEDs %s\n", 
                   leds_enabled ? "HABILITADOS" : "DESABILITADOS");
        }
        
        button_last_state = current_button_state;
    }
}

// Função para exibir status do sistema
void print_system_status(uint16_t water_level) {
    static uint32_t last_print_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Imprimir status a cada 2 segundos
    if (current_time - last_print_time >= 2000) {
        printf("\n=== STATUS DO SISTEMA ===\n");
        printf("Nível de água (ADC): %d\n", water_level);
        printf("Thresholds - Min: %d, Max: %d\n", WATER_LEVEL_MIN_THRESHOLD, WATER_LEVEL_MAX_THRESHOLD);
        printf("Bomba: %s\n", pump_active ? "LIGADA" : "DESLIGADA");
        printf("LEDs: %s\n", leds_enabled ? "HABILITADOS" : "DESABILITADOS");
        
        if (leds_enabled) {
            if (water_level < WATER_LEVEL_MIN_THRESHOLD) {
                printf("Status: NÍVEL BAIXO (LED Vermelho)\n");
            } else if (water_level < WATER_LEVEL_MAX_THRESHOLD) {
                printf("Status: NÍVEL OK (LED Verde)\n");
            } else {
                printf("Status: NÍVEL MÁXIMO (LEDs Apagados)\n");
            }
        }
        printf("========================\n\n");
        
        last_print_time = current_time;
    }
}

void update_display(uint16_t water_level, bool pump_state) {
    char buffer[32];
    char estado[32];
    float water_level_float = ((float)water_level - WATER_LEVEL_MIN_THRESHOLD) / (WATER_LEVEL_MAX_THRESHOLD - WATER_LEVEL_MIN_THRESHOLD) * 100.0; // Convertendo ADC para porcentagem
    ssd1306_fill(&oled, false);
    
    snprintf(buffer, sizeof(buffer), "Nivel de agua");
    ssd1306_draw_string(&oled, buffer, 1, 8);
    
    snprintf(buffer, sizeof(buffer), "%.2f%%", water_level_float);
    ssd1306_draw_string(&oled, buffer, 1, 24);
    
    snprintf(buffer, sizeof(buffer), "Estado da Bomba");
    ssd1306_draw_string(&oled, buffer, 1, 40);
    if (pump_state){
        snprintf(buffer, sizeof(buffer), "Ligada");
    } else{
        snprintf(buffer, sizeof(buffer), "Desligada");
    }
    ssd1306_draw_string(&oled, buffer, 1, 54);
    ssd1306_send_data(&oled);
}

void update() //função para atualizar o web site
{
    update_web_site(water_level_adc, pump_active);
    if (nivelConfig.min != WATER_LEVEL_MIN_THRESHOLD || nivelConfig.max != WATER_LEVEL_MAX_THRESHOLD)
    {
        WATER_LEVEL_MIN_THRESHOLD = nivelConfig.min;
        WATER_LEVEL_MAX_THRESHOLD = nivelConfig.max;
        printf("Minimo: %d\n",WATER_LEVEL_MIN_THRESHOLD);
        printf("Maximo: %d\n",WATER_LEVEL_MAX_THRESHOLD);
    }
}

// Função principal
int main() {
    // Inicializar hardware
    init_hardware();
    npInit(LED_PIN);
    
    printf("Sistema iniciado com sucesso!\n");
    printf("Thresholds configurados - Min: %d, Max: %d\n", 
           WATER_LEVEL_MIN_THRESHOLD, WATER_LEVEL_MAX_THRESHOLD);
    
    //iniciar web
    init_web_site();
    
    // Loop principal
    while (true) {
        // Ler nível de água
        water_level_adc = read_water_level();
        
        // Tratar botão
        handle_button();
        
        // Controlar bomba
        control_pump(water_level_adc);
        
        // Controlar LEDs
        control_leds(water_level_adc);

        // Controlar matriz de leds e o buzzer
        control_buzzer_matrix(water_level_adc);
        
        // Exibir status
        print_system_status(water_level_adc);

        // Atualiza o display OLED
        update_display(water_level_adc, pump_active);

        //atualiza dados do web site
        update();

        // Aguardar antes da próxima leitura
        sleep_ms(SAMPLE_INTERVAL_MS);
    }
    
    return 0;
}
