#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "extra/Desenho.h"

// ==============================
// Bibliotecas personalizadas
// ==============================

#include "lib/buzzer.h"
#include "lib/leds.h"
#include "lib/matrizRGB.h"
#include "lib/ssd1306.h"

// ==============================
// Definições dos pinos
// ==============================

static const uint VRY_PIN = 27;
static const uint VRX_PIN = 26;
static const uint SW_PIN = 22;
static const uint BUTTON_A = 5;
static const uint BUTTON_B = 6;

// ==============================
// Estado do sistema
// ==============================

typedef enum
{
    MODO_PADRAO = 0,
    MODO_DEBBUG = 1,
    MODO_TERMINAL = 2,
} Estado;

volatile Estado estado_atual = MODO_PADRAO;
volatile bool mudanca_estado = true;

// ==============================
// Constantes globais, essenciais para o código
// ==============================

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define I2C_ADDR 0x3C

volatile uint32_t last_button_time = 0;
static ssd1306_t ssd;
volatile uint16_t adc_x_anterior = 0;
volatile uint16_t adc_y_anterior = 0;
volatile uint16_t adc_x_valor = 0;
volatile uint16_t adc_y_valor = 0;
volatile bool led_rgb_estado = false;
volatile bool matriz_estado = false;

// ==============================
// Funções auxiliares
// ==============================

typedef struct
{
    uint16_t x_mapeado;
    uint16_t y_mapeado;
} Remapeamento;

void init_i2c()
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void init_display()
{
    ssd1306_init(&ssd, 128, 64, false, I2C_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
}

void init_joystick_adc()
{
    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);
}

void init_buttons()
{
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);
    gpio_pull_up(SW_PIN);
}

// ==============================
// Declaração de funções do código
// ==============================

void remapear_valores(uint16_t valor_x, uint16_t valor_y, Remapeamento *resultado);
void limpar_serial_monitor();
void gpio_irq_handle(uint gpio, uint32_t events);
void mostrarMenu();

int main(void)
{
    stdio_init_all();

    init_i2c();
    init_display();
    init_joystick_adc();
    init_buttons();

    npInit(7);
    led_init();
    buzzer_init();

    Remapeamento dados;
    uint32_t tempo_anterior = 0;

    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handle);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handle);
    gpio_set_irq_enabled_with_callback(SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handle);

    while (true)
    {
        // ==============================
        // Realiza leitura do joystick
        // ==============================

        adc_select_input(0);
        uint16_t adc_y = adc_read();
        adc_select_input(1);
        uint16_t adc_x = adc_read();

        /*
    Debugação:
    sd1306_draw_string(&ssd, "A", 123 - 8, 63 - 8)
    O caractere 'A' ocupa um espaço de 8 pixels, sendo desenhado da direita para a esquerda e de cima para baixo.
    Na posição (x: 0, y: 0), ele aparece normalmente, mas se colocado na posição (x: 127, y: 63), não será exibido,
    pois ultrapassa os limites do display em 8 pixels para a direita e para baixo.

    Para garantir sua visibilidade, basta ajustar a posição dentro dos limites do display: (127 - 8) e (63 - 8).

    Essa lógica também é essencial para centralizar um quadrado de 8x8 pixels. A posição central do display é (63, 31),
    então para alinhá-lo corretamente, basta calcular: (63 - 4, 31 - 4).

    Também é necessário determinar todas as bordas:
    - Canto inferior esquerdo: (x: 0, y: 63 - 8)
    - Canto superior direito: (x: 127 - 8, y: 0)
    - Canto inferior direito: (x: 127 - 8, y: 63 - 8)
    - Canto superior esquerdo: (x: 0, y: 0)

    Dessa forma, os valores de adc_y e adc_x podem ser ajustados para corresponder aos limites do display.

    Valores do joystick com botão A:
    - Joystick parado: y varia entre 1993 e 1995, x entre 2084 e 2085.
    - Valor máximo de y: 4073, mínimo: 11-12.
    - Valor máximo de x: 4073, mínimo: 11-12.

    Para remapear os valores, será criada a função remap, que receberá adc_y e adc_x e retornará os valores ajustados.
    Também será utilizada `struct` e ponteiro, conforme sugestão do professor Ricardo.
*/

        ssd1306_draw_string(&ssd, "A", 123 - 8, 63 - 8);

        // Só vou realmente atualizar os valores se houver uma diferença significativa entre os valores atuais e os anteriores.
        // Isso evita o display ficar piscando sem parar e etc.
        if (abs(adc_y - adc_y_anterior) > 50 || abs(adc_x - adc_x_anterior) > 50)
        {
            adc_y_valor = adc_y;
            adc_x_valor = adc_x;
            adc_y_anterior = adc_y;
            adc_x_anterior = adc_x;
        }

        // ==============================
        // Máquina de estados
        // ==============================

        uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());

        switch (estado_atual)
        {
        case MODO_PADRAO:
        {
            if (mudanca_estado)
            {
                ssd1306_fill(&ssd, false);
                ssd1306_send_data(&ssd);
                limpar_serial_monitor();
                mudanca_estado = false;
            }

            remapear_valores(adc_x_valor, adc_y_valor, &dados);
            ssd1306_fill(&ssd, false);
            draw_square(&ssd, dados.x_mapeado, dados.y_mapeado);
            ssd1306_send_data(&ssd);
            break;
        }

        case MODO_DEBBUG:

        {
            if (mudanca_estado)
            {
                limpar_serial_monitor();
                ssd1306_fill(&ssd, false);
                ssd1306_send_data(&ssd);
                mudanca_estado = false;
            }

            if ((tempo_atual - tempo_anterior) > 200)
            {
                tempo_anterior = tempo_atual;
                limpar_serial_monitor();
                printf("X: %-4d Y: %-4d\n", adc_x_valor, adc_y_valor);
            }
            break;
        }

        case MODO_TERMINAL:

        {
            if (mudanca_estado)
            {
                mostrarMenu();
                mudanca_estado = false;
            }

            if (stdio_usb_connected())
            {
                int opcao = getchar_timeout_us(0); // Não bloqueante
                if (opcao != PICO_ERROR_TIMEOUT)
                {
                    switch (opcao)
                    {
                    case '1':
                        led_rgb_estado = !led_rgb_estado;
                        led_rgb_estado ? acender_led_rgb(255, 255, 255) : turn_off_leds();
                        mostrarMenu();
                        break;

                    case '2':
                        matriz_estado = !matriz_estado;
                        matriz_estado ? acenderTodaMatrizIntensidade(COLOR_WHITE, 1.0) : npClear();
                        mostrarMenu();
                        break;

                    case '3':
                        play_mario_kart_theme(1);
                        mostrarMenu();
                        break;

                    case '4':
                        if (led_rgb_estado)
                        {
                            acender_led_rgb_cor_aleatoria();
                            mostrarMenu();
                        }

                        break;

                    case '5':
                        if (led_rgb_estado)
                        {
                            força_leds(rand() % 100);
                            mostrarMenu();
                        }

                        break;

                    case '6':
                        matriz_estado = true;
                        animar_desenhos(350, 10, caixa_de_desenhos, (1), (1), (1));
                        mostrarMenu();
                        break;

                    case '7':
                        estado_atual = MODO_PADRAO;
                        mudanca_estado = true;
                        break;

                    default:
                        // Ignora outros caracteres
                        break;
                    }
                }
            }
            break;
        }
        }
    }
}

void mostrarMenu()
{
    limpar_serial_monitor();
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    printf("Terminal Ativo - Escolha uma opção:\n");
    printf("1 - Ligar/Desligar LED RGB\n");
    printf("2 - Ligar/Desligar matriz RGB 5x5\n");
    printf("3 - Tocar tema do Mário\n");
    printf("4 - Trocar cor do Led RGB para uma cor aleatória\n");
    printf("5 - Alterar força do Led de forma aleatória\n");
    printf("6 - Mostrar números de 0 a 9 na matriz RGB 5x5\n");
    printf("7 - Sair do terminal\n");
}

float ler_float_nao_bloqueante()
{
    char buffer[16] = {0}; // Buffer para armazenar a entrada
    int index = 0;
    int caractere;

    while ((caractere = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT && index < sizeof(buffer) - 1)
    {
        if (caractere == '\n')
            break; // Finaliza ao detectar Enter
        buffer[index++] = (char)caractere;
    }

    return (index > 0) ? atof(buffer) : -1; // Converte para float ou retorna erro (-1)
}

void remapear_valores(uint16_t valor_x, uint16_t valor_y, Remapeamento *resultado)
{
    resultado->x_mapeado = (valor_x - 11) * (127 - 8) / (4073 - 11);
    resultado->y_mapeado = (4073 - valor_y) * (63 - 8) / (4073 - 11);
}

void limpar_serial_monitor()
{
    printf("\033[2J\033[H");
}

void gpio_irq_handle(uint gpio, uint32_t events)
{
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    printf("Ruído\n"); // Para debbugar o debouce. Verificar funcionamento.

    if (current_time - last_button_time > 200)
    { // Debounce 100ms
        last_button_time = current_time;

        // Ignora todos os botões se estiver no modo terminal
        if (estado_atual == MODO_TERMINAL)
            return;

        switch (gpio)
        {
        case BUTTON_A:
            if (estado_atual == MODO_DEBBUG)
            {
                estado_atual = MODO_PADRAO;
                mudanca_estado = true;
                break;
            }
            estado_atual = MODO_DEBBUG;
            mudanca_estado = true;
            printf("Botão A apertado\n"); // Para debbugar o debouce. Verificar funcionamento.
            break;

        case BUTTON_B:
            reset_usb_boot(0, 0);
            break;

        case SW_PIN:
            estado_atual = MODO_TERMINAL;
            mudanca_estado = true;
            limpar_serial_monitor();
            break;
        }
    }
}