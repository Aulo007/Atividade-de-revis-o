#include "buzzer.h"
#include <stdlib.h>
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "buzzer.h"

// Slices PWM usados pelos buzzers
static uint slice_buzzer1;
static uint slice_buzzer2;

typedef struct
{
    uint16_t frequency;
    uint16_t duration_ms;
} note_t;

// Protótipo da função usada antes da definição
void turn_off_buzzer(uint8_t buzzer);

void buzzer_init(void)
{
    // Configurar os pinos como saída PWM
    gpio_set_function(BUZZER_PIN_1, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER_PIN_2, GPIO_FUNC_PWM);

    // Obter slices correspondentes aos pinos
    slice_buzzer1 = pwm_gpio_to_slice_num(BUZZER_PIN_1);
    slice_buzzer2 = pwm_gpio_to_slice_num(BUZZER_PIN_2);

    // Configurar PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f); // Divisor de clock (frequência alta)
    pwm_config_set_wrap(&config, 4095);   // Resolução de 12 bits (0-4095)

    pwm_init(slice_buzzer1, &config, true);
    pwm_init(slice_buzzer2, &config, true);

    // Iniciar buzzers desligados
    turn_off_buzzer(1);
    turn_off_buzzer(2);
}

void turn_off_buzzer(uint8_t buzzer)
{
    if (buzzer == 1)
    {
        pwm_set_gpio_level(BUZZER_PIN_1, 0);
    }
    else if (buzzer == 2)
    {
        pwm_set_gpio_level(BUZZER_PIN_2, 0);
    }
}

void potencia_buzzer(uint8_t buzzer, float dutycicle)
{
    // Limitar o duty cycle entre 0 e 100%
    if (dutycicle < 0.0f)
        dutycicle = 0.0f;
    if (dutycicle > 100.0f)
        dutycicle = 100.0f;

    // Converter de porcentagem (0-100) para o valor do contador PWM (0-4095)
    uint16_t valor_pwm = (uint16_t)((dutycicle / 100.0f) * 4095);

    // Aplicar o duty cycle ao buzzer selecionado
    if (buzzer == 1)
    {
        pwm_set_gpio_level(BUZZER_PIN_1, valor_pwm);
    }
    else if (buzzer == 2)
    {
        pwm_set_gpio_level(BUZZER_PIN_2, valor_pwm);
    }
}

void play_note(uint8_t buzzer, uint16_t frequency, uint16_t duration_ms)
{
    if (frequency == 0)
    {
        potencia_buzzer(buzzer, 0); // Silêncio
        sleep_ms(duration_ms);
        return;
    }

    uint slice = (buzzer == 1) ? slice_buzzer1 : slice_buzzer2;
    uint pin = (buzzer == 1) ? BUZZER_PIN_1 : BUZZER_PIN_2;

    uint32_t clock = clock_get_hz(clk_sys);
    uint32_t top = clock / (frequency * 2);
    if (top == 0)
        top = 1;

    pwm_set_wrap(slice, top);
    pwm_set_gpio_level(pin, top / 2); // 50% duty
    sleep_ms(duration_ms);
    pwm_set_gpio_level(pin, 0); // Desliga som após nota
    sleep_ms(30);               // Pequena pausa entre notas
}

void play_mario_kart_theme(uint8_t buzzer)
{

    const note_t melody[] = {
        {659, 150}, {659, 150}, {0, 100}, {659, 150}, {0, 100}, {523, 150}, {659, 150}, {0, 150}, {784, 150}, {0, 300}, {392, 150}, {0, 150},

        {523, 150},
        {0, 150},
        {392, 150},
        {0, 150},
        {330, 150},
        {0, 150},
        {440, 150},
        {0, 150},
        {494, 150},
        {0, 150},
        {466, 150},
        {0, 150},
        {440, 150},
        {0, 150},
        {392, 150},
        {659, 150},
        {784, 150},
        {0, 150},
        {880, 150},
        {0, 300}};

    const size_t melody_length = sizeof(melody) / sizeof(note_t);

    for (size_t i = 0; i < melody_length; ++i)
    {
        play_note(buzzer, melody[i].frequency, melody[i].duration_ms);
    }

    turn_off_buzzer(buzzer);
}
