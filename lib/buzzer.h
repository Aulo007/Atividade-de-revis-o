#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// Definição dos pinos dos buzzers
#define BUZZER_PIN_1 10
#define BUZZER_PIN_2 21

// Protótipos das funções
void buzzer_init(void);
void turn_off_buzzer(uint8_t buzzer);
void potencia_buzzer(uint8_t buzzer, float dutycicle);
void play_note(uint8_t buzzer, uint16_t frequency, uint16_t duration_ms);
void play_mario_kart_theme(uint8_t buzzer);

#endif