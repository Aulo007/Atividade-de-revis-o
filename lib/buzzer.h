#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// Definição dos pinos dos buzzers
#define BUZZER_PIN_1 10
#define BUZZER_PIN_2 21

// Constantes para configuração de frequência e PWM
#define BUZZER_1_FREQ 1000     // Frequência do primeiro buzzer (1000 Hz)
#define BUZZER_2_FREQ 1500     // Frequência do segundo buzzer (1500 Hz)
#define PWM_WRAP 4095          // Resolução de 12 bits para PWM
#define MIN_THRESHOLD_NORMAL 11 // Valor mínimo para joystick
#define MAX_THRESHOLD 4098      // Valor máximo do ADC
#define MAX_PWM_LEVEL 255      // Nível máximo de PWM

// Pino do microfone para identificação
#define MICROFONE_PIN 28

// Protótipos das funções
void buzzer_init(void);
void buzzer_control(int estado_equilibrio, int desvio, int estado_atual, int pino_origem);

#endif