#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "lib/ssd1306.h"

// Código para revisar tudo sobre O Embarcatech, e aprender mais, seguir as novas sugestões
// do professor Wilton e Ricardo

static ssd1306_t ssd;
static const uint32_t VRY_PIN = 27;
static const uint32_t VRX_PIN = 26;

static const uint32_t BUTTON_A = 5;
static const uint32_t BUTTON_B = 6;

static volatile uint32_t temporizador = 0;
static volatile uint32_t last_button_time = 0;
static volatile bool modo_debbug_joystick = false;

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Variáveis para leitura do ADC
uint16_t adc_value_x;
uint16_t adc_value_y;

void limpar_serial_monitor();
void gpio_irq_handle(uint gpio, uint32_t events);

int main()
{
    stdio_init_all(); // Inicializa a comunicação serial

    // Inicialização do I2C a 400 kHz, os pinos, endereço, joguei na lib ssd1306.h para diminuir o código da main
    i2c_init(I2C_PORT, 400 * 1000);

    // Configura os pinos para a função I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização e configuração do SSD1306
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);

    // Configurando os pinos dos botões como entrada com pull-up
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);


    
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handle);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handle);

    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    ssd1306_draw_string(&ssd, "A", 63, 31); // Desenha a string no display
    ssd1306_draw_string(&ssd, "A", 0, 0); // Desenha a string no display
    ssd1306_draw_string(&ssd, "A", 127, 63); // Desenha a string no display
    ssd1306_send_data(&ssd);


    while (true)
    {

        uint32_t tempo_agora = to_ms_since_boot(get_absolute_time());
        adc_select_input(0);
        adc_value_y = adc_read();

        adc_select_input(1);
        adc_value_x = adc_read();

        // O display tem 128 x 64, logo o centro dele é 64 x 32, a posição 0 dele é 0 x 0

        draw_square(&ssd, adc_value_x / 4, adc_value_y / 4); // Desenha o quadrado na posição do joystick
        ssd1306_send_data(&ssd); // Envia os dados para o display

        if (modo_debbug_joystick)
        {
            if (tempo_agora - temporizador > 30)
            {
                temporizador = tempo_agora;
                limpar_serial_monitor();
                printf("Valor sendo lido em y %d\nValor sendo lido em x: %d\n", adc_value_y, adc_value_x);
            }
        }
    }
}

void limpar_serial_monitor()
{
    printf("\033[2J\033[H");
    system("cls");
}

void gpio_irq_handle(uint gpio, uint32_t events)
{
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (current_time - last_button_time > 100) // Debounce de 100ms
    {
        last_button_time = current_time;

        if (gpio == BUTTON_A)
        {
            modo_debbug_joystick = !modo_debbug_joystick; // Alterna para o modo de depuração do debbug
        }
        else if (gpio == BUTTON_B)
        {
            printf("Botão B pressionado\n");
        }
    }
}