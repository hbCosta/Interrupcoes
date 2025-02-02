/*
 * Por: Humberto Bandeira Costa
 * 
 * Este código é uma adaptação do código original do professor wilton
 * para a utilização da matriz de LEDs WS2812 do BitDogLab.
 * 
 * 
 * 
 * 
 * Original em:
 * https://github.com/raspberrypi/pico-examples/tree/master/pio/ws2812
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define tempo 400

// Variável global para armazenar a cor (Entre 0 e 255 para intensidade)
uint8_t led_r = 0; // Intensidade do vermelho
uint8_t led_g = 0; // Intensidade do verde
uint8_t led_b = 20; // Intensidade do azul

// Configurações dos pinos
const uint ledC_pin = 13; // Red  => GPIO13
const uint ledA_pin = 12; // Blue => GPIO12
const uint ledB_pin = 11; // Green=> GPIO11
const uint button_0 = 5; 
const uint button_1 = 6;

// Variáveis globais
static volatile uint a = 1;
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)


#define tempoRGB 200 // Tempo de piscar em milissegundos

// Buffer para armazenar quais LEDs estão ligados matriz 5x5
bool led_buffer0[NUM_PIXELS] = {
    1, 1, 1, 1, 1, 
    1, 0, 0, 0, 1, 
    1, 0, 0, 0, 1, 
    1, 0, 0, 0, 1, 
    1, 1, 1, 1, 1
};

bool led_buffer1[NUM_PIXELS] = {
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0
};

bool led_buffer2[NUM_PIXELS] = {
    1, 1, 1, 1, 1, 
    1, 0, 0, 0, 0, 
    1, 1, 1, 1, 1, 
    0, 0, 0, 0, 1, 
    1, 1, 1, 1, 1
};

bool led_buffer3[NUM_PIXELS] = {
    1, 1, 1, 1, 1, 
    0, 0, 0, 0, 1, 
    1, 1, 1, 1, 1, 
    0, 0, 0, 0, 1, 
    1, 1, 1, 1, 1
};

bool led_buffer4[NUM_PIXELS]={
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0, 
    0, 1, 1, 1, 1, 
    0, 1, 1, 0, 0, 
    0, 0, 1, 0, 0
};

bool led_buffer5[NUM_PIXELS]={
    1, 1, 1, 1, 1, 
    0, 0, 0, 0, 1, 
    1, 1, 1, 1, 1, 
    1, 0, 0, 0, 0, 
    1, 1, 1, 1, 1
};

bool led_buffer6[NUM_PIXELS]={
    1, 1, 1, 1, 1, 
    1, 0, 0, 0, 1, 
    1, 1, 1, 1, 1, 
    1, 0, 0, 0, 0, 
    1, 1, 1, 1, 1
};

bool led_buffer7[NUM_PIXELS] = {
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0, 
    0, 1, 1, 1, 0, 
    0, 0, 1, 0, 0, 
    0, 0, 1, 1, 1
};

bool led_buffer8[NUM_PIXELS] = {
    1, 1, 1, 1, 1, 
    1, 0, 0, 0, 1, 
    1, 1, 1, 1, 1, 
    1, 0, 0, 0, 1, 
    1, 1, 1, 1, 1
};

bool led_buffer9[NUM_PIXELS] = {
    1, 0, 0, 0, 0, 
    0, 0, 0, 0, 1, 
    1, 1, 1, 1, 1, 
    1, 0, 0, 0, 1, 
    1, 1, 1, 1, 1
};

bool* led_buffers[] = {led_buffer0, led_buffer1, led_buffer2, led_buffer3, led_buffer4, led_buffer5, led_buffer6, led_buffer7, led_buffer8, led_buffer9};
int current_buffer_index = 0;

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b);
}


void set_one_led(uint8_t r, uint8_t g, uint8_t b)
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Obtém o buffer atual
    bool* current_buffer = led_buffers[current_buffer_index];

    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (current_buffer[i])
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0);  // Desliga os LEDs com zero no buffer
        }
    }
}

// Função para piscar um LED
void piscar_led(uint led_pin, uint tempo_ms) {
    gpio_put(led_pin, true);  // Liga o LED
    sleep_ms(tempo_ms);       // Espera o tempo definido
    gpio_put(led_pin, false); // Desliga o LED
    sleep_ms(tempo_ms);       // Espera o tempo definido
}


// Função de interrupção com debouncing
void gpio_irq_handler(uint gpio, uint32_t events)
{
    // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 200000) // 200 ms de debouncing
    {
        last_time = current_time; // Atualiza o tempo do último evento

        if(gpio == button_0){
            current_buffer_index = (current_buffer_index + 1) %10;
        }else if(gpio == button_1){
            current_buffer_index = (current_buffer_index - 1 + 10) % 10;
        }

        set_one_led(led_r, led_g, led_b);

    }
}

int main()
{
    // Inicializações
    stdio_init_all();

    // Inicializa LEDs
    gpio_init(ledA_pin); 
    gpio_init(ledB_pin); 
    gpio_init(ledC_pin); 

    // Inicializa botões
    gpio_init(button_0);
    gpio_init(button_1);
    // Configura o pino como entrada
    gpio_set_dir(button_0, GPIO_IN);
    gpio_set_dir(button_1, GPIO_IN); 
    // Habilita o pull-up interno
    gpio_pull_up(button_0);   
    gpio_pull_up(button_1);       


    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);



    // Configura como saída os LEDs
    gpio_set_dir(ledA_pin, GPIO_OUT);
    gpio_set_dir(ledB_pin, GPIO_OUT);
    gpio_set_dir(ledC_pin, GPIO_OUT);
    
    set_one_led(led_r, led_g, led_b);

    // Configuração da interrupção com callback
    gpio_set_irq_enabled_with_callback(button_0, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_1, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);


    while (1)
    {

        // Piscar o led vermelho
        piscar_led(ledC_pin, tempoRGB);


    }

    return 0;
}
