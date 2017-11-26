#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/f4/timer.h>

#define DUTY_CYCLE_1 67
#define DUTY_CYCLE_0 34
#define BIT_PERIOD   105


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
}


int main(void) {
    clock_setup();
    gpio_setup();
    timer_setup();
    return 0;
}
