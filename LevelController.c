#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

// Definições dos pinos
#define WATER_LEVEL_ADC_PIN 28      // ADC
#define PUMP_RELAY_PIN 16           // Relé da bomba
#define RED_LED_PIN 13              
#define GREEN_LED_PIN 11            
#define PUSH_BUTTON_PIN 6     
#define OLED_I2C_SDA_PIN 15         // SDA do OLED  
#define OLED_I2C_SCL_PIN 14         // SCL do OLED  
#define I2C_OLED_PORT i2c1  

// Definições dos thresholds (valores ADC de 0-4095)
#define WATER_LEVEL_MIN_THRESHOLD 1000   // Nível mínimo
#define WATER_LEVEL_MAX_THRESHOLD 3000   // Nível máximo

// Constantes do sistema
#define DEBOUNCE_TIME_MS 200         // Tempo de debounce do botão
#define SAMPLE_INTERVAL_MS 100      // Intervalo entre leituras do ADC

// Variáveis globais do sistema
bool leds_enabled = true;              // Estado dos LEDs
bool pump_active = false;              // Estado da bomba
bool button_last_state = false;        // Último estado do botão
absolute_time_t last_button_time;      
uint16_t water_level_adc = 0;          // Variável nível
ssd1306_t oled;

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
    float water_level_float = (float)water_level / 4095.0 * 100.0; // Convertendo ADC para porcentagem
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

// Função principal
int main() {
    // Inicializar hardware
    init_hardware();
    
    printf("Sistema iniciado com sucesso!\n");
    printf("Thresholds configurados - Min: %d, Max: %d\n", 
           WATER_LEVEL_MIN_THRESHOLD, WATER_LEVEL_MAX_THRESHOLD);
    
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
        
        // Exibir status
        print_system_status(water_level_adc);

        // Atualiza o display OLED
        update_display(water_level_adc, pump_active);

        // Aguardar antes da próxima leitura
        sleep_ms(SAMPLE_INTERVAL_MS);
    }
    
    return 0;
}