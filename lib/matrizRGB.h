#ifndef MATRIZRGB_H
#define MATRIZRGB_H
#include <stdint.h>
#define LED_COUNT 25

// Definição do pixel/LED
typedef struct
{
    uint8_t G, R, B;
} npLED_t;

#define COLOR_RED (npColor_t){255, 0, 0}
#define COLOR_GREEN (npColor_t){0, 255, 0}
#define COLOR_BLUE (npColor_t){0, 0, 255}
#define COLOR_WHITE (npColor_t){255, 255, 255}
#define COLOR_BLACK (npColor_t){0, 0, 0}
#define COLOR_YELLOW (npColor_t){255, 255, 0}
#define COLOR_CYAN (npColor_t){0, 255, 255}
#define COLOR_MAGENTA (npColor_t){255, 0, 255}
#define COLOR_PURPLE (npColor_t){128, 0, 128}
#define COLOR_ORANGE (npColor_t){255, 165, 0}

// Tipo para representar uma cor RGB
typedef struct
{
    uint8_t r, g, b;
} npColor_t;

extern npColor_t colors[];

// Declaração de funções

void acenderTodaMatrizIntensidade(npColor_t cor, float intensidade);
void animar_desenhos(int PERIODO, int num_desenhos, int caixa_de_desenhos[num_desenhos][5][5][3], double intensidade_r, double intensidade_g, double intensidade_b);
void npInit(uint8_t pin);
void npClear();
void npWrite();
void setMatrizDeLEDSComIntensidade(int matriz[5][5][3], double intensidadeR, double intensidadeG, double intensidadeB);
int getIndex(int x, int y);
extern npLED_t leds[LED_COUNT]; // Torna a variável visível externamente
#endif                          // MATRIZRGB_H