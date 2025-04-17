#include "buzzer.h"
#include <stdlib.h>

// Variáveis globais para armazenar os números dos slices PWM
static uint slice_num_1;
static uint slice_num_2;

void buzzer_init(void)
{
    // Configuração do primeiro buzzer (1000 Hz)
    gpio_set_function(BUZZER_PIN_1, GPIO_FUNC_PWM);
    slice_num_1 = pwm_gpio_to_slice_num(BUZZER_PIN_1);
    pwm_config config_1 = pwm_get_default_config();

    // Calcula e configura o divisor de clock apropriado para 1000 Hz
    float divider_1 = clock_get_hz(clk_sys) / (BUZZER_1_FREQ * PWM_WRAP);
    pwm_config_set_clkdiv(&config_1, divider_1);
    pwm_config_set_wrap(&config_1, PWM_WRAP);
    pwm_init(slice_num_1, &config_1, true);
    pwm_set_gpio_level(BUZZER_PIN_1, 0);

    // Configuração do segundo buzzer (1500 Hz)
    gpio_set_function(BUZZER_PIN_2, GPIO_FUNC_PWM);
    slice_num_2 = pwm_gpio_to_slice_num(BUZZER_PIN_2);
    pwm_config config_2 = pwm_get_default_config();

    // Calcula e configura o divisor de clock apropriado para 1500 Hz
    float divider_2 = clock_get_hz(clk_sys) / (BUZZER_2_FREQ * PWM_WRAP);
    pwm_config_set_clkdiv(&config_2, divider_2);
    pwm_config_set_wrap(&config_2, PWM_WRAP);
    pwm_init(slice_num_2, &config_2, true);
    pwm_set_gpio_level(BUZZER_PIN_2, 0);
}

// Função auxiliar para mapear valores de um intervalo para outro
static inline int map_range(int value, int in_min, int in_max, int out_min, int out_max)
{
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void buzzer_control(int estado_equilibrio, int desvio, int estado_atual, int pino_origem)
{
    int valor_convertido = 0;

    // Tratamento específico para o microfone
    if (pino_origem == MICROFONE_PIN)
    {
        // Para o microfone, consideramos apenas valores acima do equilíbrio
        if (estado_atual <= 2080)
        {
            // Desliga os buzzers se estiver abaixo ou igual ao primeiro limiar
            pwm_set_gpio_level(BUZZER_PIN_1, 0);
            pwm_set_gpio_level(BUZZER_PIN_2, 0);
            return;
        }
        else if (estado_atual <= 2200)
        {
            // Ativa o primeiro buzzer se estiver entre 2080 e 2200
            pwm_set_gpio_level(BUZZER_PIN_1, MAX_PWM_LEVEL / 2);
            pwm_set_gpio_level(BUZZER_PIN_2, 0);
        }
        else
        {
            // Ativa ambos os buzzers se estiver acima de 2200
            pwm_set_gpio_level(BUZZER_PIN_1, MAX_PWM_LEVEL);
            pwm_set_gpio_level(BUZZER_PIN_2, MAX_PWM_LEVEL);
        }

    }

    // Tratamento para joystick (sensores normais)
    else
    {
        // Calcula o desvio absoluto em relação ao estado de equilíbrio
        int desvio_absoluto = abs(estado_atual - estado_equilibrio);

        // Verifica se está dentro da zona morta
        if (desvio_absoluto <= desvio)
        {
            // Desliga ambos os buzzers
            pwm_set_gpio_level(BUZZER_PIN_1, 0);
            pwm_set_gpio_level(BUZZER_PIN_2, 0);
            return;
        }

        // Mapeia o desvio para o intervalo PWM
        valor_convertido = map_range(desvio_absoluto,
                                     MIN_THRESHOLD_NORMAL,
                                     MAX_THRESHOLD,
                                     0,
                                     MAX_PWM_LEVEL);
    }

    // Limita o valor convertido ao intervalo válido
    if (valor_convertido > MAX_PWM_LEVEL)
    {
        valor_convertido = MAX_PWM_LEVEL;
    }
    else if (valor_convertido < 0)
    {
        valor_convertido = 0;
    }

    // Calcula os valores PWM escalados para cada buzzer
    // Escala baseada na frequência para manter volume percebido similar
    uint16_t pwm_1 = (uint16_t)((float)valor_convertido * PWM_WRAP / MAX_PWM_LEVEL);
    uint16_t pwm_2 = (uint16_t)((float)valor_convertido * PWM_WRAP / MAX_PWM_LEVEL * 0.816); // ~sqrt(1000/1500)

    // Define os níveis PWM com compensação de frequência
    pwm_set_gpio_level(BUZZER_PIN_1, pwm_1);
    pwm_set_gpio_level(BUZZER_PIN_2, pwm_2);
}