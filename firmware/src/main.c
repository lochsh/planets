#include <stdint.h>
#include <stdlib.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/f4/timer.h>

#define DUTY_CYCLE_0        34
#define DUTY_CYCLE_1        67
#define RESET_PERIOD        7500
#define BIT_PERIOD          105
#define NUM_LEDS            300
#define BITS_PER_CHANNEL    8
#define NUM_CHANNELS        3
#define BITS_PER_LED        (BITS_PER_CHANNEL * NUM_CHANNELS)

typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} led_t;


static led_t pattern[NUM_LEDS];
static uint8_t bit_sequence[BITS_PER_LED * NUM_LEDS];
static uint8_t* current_bit;


static void bi_pride_gradient(void) {

    for (size_t led_idx = 0; led_idx < NUM_LEDS; led_idx++) {
        pattern[led_idx].green = 10;
        pattern[led_idx].red = NUM_LEDS - (led_idx * 5);
        pattern[led_idx].blue = led_idx * 5;
    }
}


static void pattern_to_bit_sequence(void) {

    for (size_t led_idx = 0; led_idx < NUM_LEDS; led_idx++) {
        const led_t current_led = pattern[led_idx];
        const size_t offset = led_idx * BITS_PER_LED;

        for (size_t i = 0; i < BITS_PER_CHANNEL; i++) {
            const size_t shift = BITS_PER_CHANNEL - i - 1;

            bit_sequence[i + offset] =
                (current_led.green >> shift) & 1;

            bit_sequence[i + BITS_PER_CHANNEL + offset] =
                (current_led.red >> shift) & 1;

            bit_sequence[i + (2 * BITS_PER_CHANNEL) + offset] =
                (current_led.blue >> shift) & 1;
        }
    }
}


static void clock_setup(void) {
    rcc_clock_setup_hse_3v3(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_TIM3);
}


static void gpio_setup(void) {
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0);
    gpio_set_af(GPIOB, GPIO_AF2, GPIO0);
}


static void timer_setup(void) {
    timer_reset(TIM3);
    timer_set_mode(
            TIM3,
            TIM_CR1_CKD_CK_INT,
            TIM_CR1_CMS_EDGE,
            TIM_CR1_DIR_UP);

    timer_set_oc_mode(TIM3, TIM_OC3, TIM_OCM_PWM1);
    timer_set_oc_value(TIM3, TIM_OC3, DUTY_CYCLE_1);
    timer_enable_oc_output(TIM3, TIM_OC3);

    timer_enable_irq(TIM3, TIM_DIER_CC3IE);
    nvic_enable_irq(NVIC_TIM3_IRQ);

    timer_set_period(TIM3, BIT_PERIOD);
    timer_set_prescaler(TIM3, 0);
    timer_enable_counter(TIM3);
}


void tim3_isr(void) {

    if (!timer_get_flag(TIM3, TIM_SR_CC3IF)) {
        return;
    }

    timer_clear_flag(TIM3, TIM_SR_CC3IF);

    if ((current_bit - bit_sequence) >= (BITS_PER_LED * NUM_LEDS)) {
        timer_set_period(TIM3, RESET_PERIOD);
        timer_set_oc_value(TIM3, TIM_OC3, 0);
        current_bit = &bit_sequence[0];
    } else {
        timer_set_period(TIM3, BIT_PERIOD);
        timer_set_oc_value(
                TIM3,
                TIM_OC3,
                *current_bit ? DUTY_CYCLE_1: DUTY_CYCLE_0);
        current_bit++;
    }

}


int main(void) {
    clock_setup();
    gpio_setup();

    bi_pride_gradient();
    pattern_to_bit_sequence();
    current_bit = &bit_sequence[0];

    timer_setup();

    return 0;
}
