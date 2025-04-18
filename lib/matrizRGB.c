#include "matrizRGB.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"

#define LED_COUNT 25 // Número de Leds na matriz 5x5

// Buffer de pixels global
npLED_t leds[LED_COUNT];
static PIO np_pio;
static uint sm;


npColor_t colors[] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_WHITE, COLOR_BLACK,
                             COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA, COLOR_PURPLE, COLOR_ORANGE};

// Inicialização da Matrix 5x5, na bitdoglab no pino 7
void npInit(uint8_t pin)
{
    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0)
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de pixels.
    npClear();
}

// Função que seta, basicamente atribui uma cor a cada pino correspondente da matriz da placa

void setMatrizDeLEDSComIntensidade(int matriz[5][5][3], double intensidadeR, double intensidadeG, double intensidadeB)
{
    // Validação das intensidades
    intensidadeR = (intensidadeR < 0.0 || intensidadeR > 1.0) ? 1.0 : intensidadeR;
    intensidadeG = (intensidadeG < 0.0 || intensidadeG > 1.0) ? 1.0 : intensidadeG;
    intensidadeB = (intensidadeB < 0.0 || intensidadeB > 1.0) ? 1.0 : intensidadeB;

    // Loop para configurar os LEDs
    for (uint8_t linha = 0; linha < 5; linha++)
    {
        for (uint8_t coluna = 0; coluna < 5; coluna++)
        {
            // Calcula os valores RGB ajustados pela intensidade
            uint8_t r = (uint8_t)(float)(matriz[linha][coluna][0] * intensidadeR);
            uint8_t g = (uint8_t)(float)(matriz[linha][coluna][1] * intensidadeG);
            uint8_t b = (uint8_t)(float)(matriz[linha][coluna][2] * intensidadeB);

            uint index = getIndex(coluna, linha);

            // Configura o LED diretamente
            leds[index].R = r;
            leds[index].G = g;
            leds[index].B = b;
        }
    }

    npWrite(); // Função para ligar os leds setados.
}

void npWrite()
{
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

// Função para desligar os leds
void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }

    npWrite();
}

// Função para obter a posição correta na matriz 5x5.

int getIndex(int x, int y)
{
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0)
    {
        return 24 - (y * 5 + x); // Linha par (esquerda para direita).
    }
    else
    {
        return 24 - (y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

void acenderTodaMatrizIntensidade(npColor_t cor, float intensidade)
{
    // Limitar intensidade entre 0 e 1
    if (intensidade < 0.0f)
        intensidade = 0.0f;
    if (intensidade > 1.0f)
        intensidade = 1.0f;

    for (int i = 0; i < LED_COUNT; i++)
    {
        leds[i].R = (uint8_t)(cor.r * intensidade);
        leds[i].G = (uint8_t)(cor.g * intensidade);
        leds[i].B = (uint8_t)(cor.b * intensidade);
    }
    npWrite();
}

void animar_desenhos(int PERIODO, int num_desenhos, int caixa_de_desenhos[num_desenhos][5][5][3], double intensidade_r, double intensidade_g, double intensidade_b)
{

    for (int i = 0; i < num_desenhos; i++)
    {
        setMatrizDeLEDSComIntensidade(caixa_de_desenhos[i], intensidade_r, intensidade_g, intensidade_b);
        npWrite();         // Atualiza a matriz de LEDs
        sleep_ms(PERIODO); // Controla o tempo entre cada quadro
    }
}