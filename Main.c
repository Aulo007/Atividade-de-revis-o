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

typedef struct
{
    uint16_t x_mapeado;
    uint16_t y_mapeado;
} Remapeamento;

void remapear_valores(uint16_t valor_x, uint16_t valor_y, Remapeamento *resultado);

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

    // Debugação, aqui eu sei que o meu caractere A que ocupa 8 pixeis de espaços
    // Porém ele ocupa isso da direita para esquerda e de cima para baixo, ou seja,na primeira posição x: 0 e y: 0
    // Ele vai aparecer normalmente, mas se eu colocar na última posição x: 128 e y: 64, ele não vai aparecer, devido ao
    // fato de ocupar 8 pixeis a mais para direita e 8 pixeis a mais para baixo, logo ele vai sair do display
    // Então basta diminuir os 8 pixeis das posições limites do display, ou seja, 128 - 8 e 64 - 8 e ele aparecerá.
    // Isso vai ser importante para fazer o quadrado no centro, porque como ele é 8 x 8, para eu centralizar ele bonitinho
    // no caso a posição central é 63 x 31, logo preciso fazer 63 - 4 e 31 - 4, para ele ficar centralizado.
    // E também preciso encontrar todas as bordas limites, por exemplo, para x máximo, 128 - 8 e para y máximo 64 - 8
    // logo para o canto inferior esquerdo terei x: 0 e y: 64 - 8, e para o canto superior direito terei x: 128 - 8 e y: 0
    // E para o canto inferior direito terei x: 128 - 8 e y: 64 - 8, e para o superior esquerdo x: 0 e y:0 como já sabemos
    // Isso é importante porque assim posso fazer os limites dos meus valores adc_y e adc_x para bater com o máximo da minha borda
    // Logo, para adc_y que com a função com interrupção com o botão A, consigo debugar os valores dele, tenho o seguinte:
    // para o Joystick parado, o valor de y é em torno de 1993 até 1995 e o de x: 2084 até 2085
    // o Valor máximo de y é 4073 e o mínimo é 11 - 12
    // Para o x, o valor máximo é 4073 e o mínimo é 11 - 12 também
    // Logo, definindo-se uma função que converta 4073y para 63 - 8
    // Logo para remapear os valores, vou criar uma função remap que receba os valores adc_y e adc_x e volte os valores remapeados
    // Também utilizarei Struct e ponteiro para seguir a sugestão do professor Ricardo.
    ssd1306_draw_string(&ssd, "A", 123 - 8, 63 - 8);

    // Usando isso:

    Remapeamento dados;

    remapear_valores(1993, 2084, &dados);

    // Desenhar A na posição central com base nos valores centrais dos joysticks
    ssd1306_draw_string(&ssd, "A", dados.x_mapeado, dados.y_mapeado);
    ssd1306_send_data(&ssd);

    while (true)
    {

        uint32_t tempo_agora = to_ms_since_boot(get_absolute_time());
        adc_select_input(0);
        adc_value_y = adc_read();

        adc_select_input(1);
        adc_value_x = adc_read();

        // Remapeando os valores lidos do ADC

        remapear_valores(adc_value_x, adc_value_y, &dados);
        // Desenhando o quadrado na posição remapeada
        ssd1306_fill(&ssd, false); // Limpa a tela
        ssd1306_send_data(&ssd);
        draw_square(&ssd, dados.x_mapeado, dados.y_mapeado);
        ssd1306_send_data(&ssd);
        

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

void remapear_valores(uint16_t valor_x, uint16_t valor_y, Remapeamento *resultado)
{
    resultado->x_mapeado = (valor_x - 11) * (127 - 8) / (4073 - 11);
    resultado->y_mapeado = (4073 - valor_y) * (63 - 8) / (4073 - 11);
}