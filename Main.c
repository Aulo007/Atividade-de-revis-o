#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "lib/buzzer.h"
#include "lib/leds.h"
#include "lib/matrizRGB.h"
#include "lib/ssd1306.h"

// Código para revisar tudo sobre O Embarcatech, e aprender mais, seguir as novas sugestões
// do professor Wilton e Ricardo

static ssd1306_t ssd;
static const uint32_t VRY_PIN = 27;
static const uint32_t VRX_PIN = 26;
static const uint32_t SW_PIN = 22;

static const uint32_t BUTTON_A = 5;
static const uint32_t BUTTON_B = 6;

volatile uint32_t temporizador = 0;
volatile uint32_t last_button_time = 0;
volatile bool modo_debbug_joystick = false;
volatile bool modo_terminal = false;
volatile bool led_rgb_estado = false;
volatile bool matriz_estado = false;

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
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);
    gpio_pull_up(SW_PIN);

    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handle);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handle);
    gpio_set_irq_enabled_with_callback(SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handle);

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

    npInit(7);  // Inicializa a matriz RGB no pino 7
    led_init(); // Inicializa os LEDs RGB

    while (true)
    {

        uint32_t tempo_agora = to_ms_since_boot(get_absolute_time());
        adc_select_input(0);
        adc_value_y = adc_read();

        adc_select_input(1);
        adc_value_x = adc_read();

        if (modo_debbug_joystick)
        {
            ssd1306_fill(&ssd, false); // Limpa a tela
            ssd1306_send_data(&ssd);

            if (tempo_agora - temporizador > 30)
            {
                temporizador = tempo_agora;
                limpar_serial_monitor();
                printf("Valor sendo lido em y %d\nValor sendo lido em x: %d\n", adc_value_y, adc_value_x);
            }
        }
        else if (modo_terminal)
        {
            ssd1306_fill(&ssd, false); // Limpa a tela
            ssd1306_send_data(&ssd);

            if (tempo_agora - temporizador > 3000)
            {
                ssd1306_fill(&ssd, false); // Limpa a tela
                ssd1306_send_data(&ssd);
                temporizador = tempo_agora;
                limpar_serial_monitor();
                printf("Você está no terminal usuário, escolha uma opção: \n");
                printf("Digite 1 para acender/desligar Led RGB\n");
                printf("Digite 2 para Acender/desligar matriz 5 x 5\n");
                printf("Digite 3 para trocar para uma cor aleatória para a Led RGB\n");
                printf("Digite 4 para trocar a cor da matriz 5 x 5 para uma cor aleatória\n");
                printf("Digite 5 para alterar o dutycicle do Led RGB\n");
                printf("Digite 6 para alterar o dutycicle da Matriz  5x5\n");
                printf("Digite 7 para sair do modo terminal\n\n");

                if (stdio_usb_connected())
                {
                    char opcao = getchar(); // Lê a opção do usuário

                    switch (opcao)
                    {
                    case '1':
                        printf("Led RGB aceso/desligado\n");
                        led_rgb_estado = !led_rgb_estado; // Inverte o estado
                        if (led_rgb_estado)
                        {
                            acender_led_rgb(255, 255, 255);
                        }
                        else
                        {
                            turn_off_leds();
                        }
                        break;

                    case '2':
                        printf("Matriz 5x5 acesa/desligada\n");
                        matriz_estado = !matriz_estado; // Inverte o estado
                        if (matriz_estado)
                        {
                            acenderTodaMatrizIntensidade(COLOR_WHITE, 1.0); // Acende a matriz com cor branca
                        }
                        else
                        {
                            npClear(); // Desliga a matriz
                        }
                        break;

                        // case '3':
                        //     printf("Cor aleatória para LED RGB\n");
                        //     r_atual = rand() % 256;
                        //     g_atual = rand() % 256;
                        //     b_atual = rand() % 256;
                        //     if (led_rgb_estado)
                        //     {
                        //         acender_led_rgb(r_atual, g_atual, b_atual);
                        //     }
                        //     printf("Nova cor RGB: (%d, %d, %d)\n", r_atual, g_atual, b_atual);
                        //     break;

                        // case '4':
                        //     printf("Cor aleatória para Matriz 5x5\n");
                        //     matriz_cor = rand() % 3; // 0=vermelho, 1=verde, 2=azul
                        //     if (matriz_estado)
                        //     {
                        //         atualizar_cor_matriz(matriz_cor);
                        //     }
                        //     printf("Nova cor da matriz: %s\n",
                        //            matriz_cor == 0 ? "Vermelho" : matriz_cor == 1 ? "Verde"
                        //                                                           : "Azul");
                        //     break;

                        // case '5':
                        //     printf("Digite o novo duty cycle para o LED RGB (0-100): ");
                        //     int duty_rgb;
                        //     scanf("%d", &duty_rgb);
                        //     if (duty_rgb >= 0 && duty_rgb <= 100)
                        //     {
                        //         dutycycle_rgb = duty_rgb;
                        //         printf("Duty cycle do LED RGB alterado para %d%%\n", dutycycle_rgb);
                        //         if (led_rgb_estado)
                        //         {
                        //             acender_led_rgb(r_atual, g_atual, b_atual); // Atualiza com novo duty cycle
                        //         }
                        //     }
                        //     else
                        //     {
                        //         printf("Valor inválido! O duty cycle deve estar entre 0 e 100\n");
                        //     }
                        //     break;

                        // case '6':
                        //     printf("Digite o novo duty cycle para a Matriz 5x5 (0-100): ");
                        //     int duty_matriz;
                        //     scanf("%d", &duty_matriz);
                        //     if (duty_matriz >= 0 && duty_matriz <= 100)
                        //     {
                        //         dutycycle_matriz = duty_matriz;
                        //         printf("Duty cycle da Matriz 5x5 alterado para %d%%\n", dutycycle_matriz);
                        //         if (matriz_estado)
                        //         {
                        //             atualizar_dutycycle_matriz(dutycycle_matriz);
                        //         }
                        //     }
                        //     else
                        //     {
                        //         printf("Valor inválido! O duty cycle deve estar entre 0 e 100\n");
                        //     }
                        //     break;

                        // case '7':
                        //     printf("Saindo do modo terminal\n");
                        //     modo_terminal = false;
                        //     break;

                        // default:
                        //     printf("Opção inválida!\n");
                        //     break;
                    }
                }
            }
        }
        else if (modo_debbug_joystick == false && modo_terminal == false)
        {
            // Opção padrão exigida na atividade.

            // Remapeando os valores lidos do ADC

            remapear_valores(adc_value_x, adc_value_y, &dados);
            // Desenhando o quadrado na posição remapeada
            ssd1306_fill(&ssd, false); // Limpa a tela
            ssd1306_send_data(&ssd);
            draw_square(&ssd, dados.x_mapeado, dados.y_mapeado);
            ssd1306_send_data(&ssd);
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

    // Verificação de debounce
    if (current_time - last_button_time <= 100) // Debounce de 100ms
        return;

    last_button_time = current_time;

    switch (gpio)
    {
    case BUTTON_A:
        modo_terminal = false;                        // Desabilita o modo terminal
        modo_debbug_joystick = !modo_debbug_joystick; // Alterna para o modo de depuração do debbug
        break;

    case BUTTON_B:
        limpar_serial_monitor(); // Limpa o monitor serial
        printf("Botão B pressionado\n");
        printf("Entrando no modo bootshel\n");
        reset_usb_boot(0, 0);
        break;

    case SW_PIN:
        modo_debbug_joystick = false; // Desabilita o modo de depuração do debbug
        limpar_serial_monitor();      // Limpa o monitor serial
        printf("Você está no terminal usuário, escolha uma opção: \n");
        modo_terminal = !modo_terminal; // Alterna para o modo terminal
        break;
    }
}

void remapear_valores(uint16_t valor_x, uint16_t valor_y, Remapeamento *resultado)
{
    resultado->x_mapeado = (valor_x - 11) * (127 - 8) / (4073 - 11);
    resultado->y_mapeado = (4073 - valor_y) * (63 - 8) / (4073 - 11);
}