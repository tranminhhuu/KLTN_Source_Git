#include <stdbool.h>

#include "nordic_common.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include "nrf_delay.h"
#include "nrf_sdm.h"
#include "app_timer.h"

#include "buzzer.h"
#include "bike_tracker_201402.h"

static uint32_t                        m_pwm_next_value;
static uint32_t                        m_pwm_io_ch;
static uint32_t                        m_pwm_running;
static uint32_t                        m_pwm_max_value;
static app_timer_id_t                  m_buzzer_timeout_timer_id;
static bool                            m_buzzer_timed_out = false;

// Sine wave
const uint8_t sin_table[] = {0,0,1,2,4,6,9,12,16,20,24,29,35,40,46,53,59,66,74,81,88,96,104,112,120,128,136,144,152,160,168,175,182,190,197,203,210,216,221,227,
		232,236,240,244,247,250,252,254,255,255,255,255,255,254,252,250,247,244,240,236,232,227,221,216,210,203,197,190,182,175,168,160,152,144,136,128,120,112,104,
		96,88,81,74,66,59,53,46,40,35,29,24,20,16,12,9,6,4,2,1,0};

static void buzzer_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    m_buzzer_timed_out = true;
}

static void nrf_pwm_set_value(uint32_t pwm_value)
{
    m_pwm_next_value = pwm_value;
    PWM_TIMER->EVENTS_COMPARE[3] = 0;
    PWM_TIMER->SHORTS = TIMER_SHORTS_COMPARE3_CLEAR_Msk | TIMER_SHORTS_COMPARE3_STOP_Msk;

    if ((PWM_TIMER->INTENSET & TIMER_INTENSET_COMPARE3_Msk) == 0)
    {
        PWM_TIMER->TASKS_STOP = 1;
        PWM_TIMER->INTENSET = TIMER_INTENSET_COMPARE3_Msk;
    }

    PWM_TIMER->TASKS_START = 1;
}

void PWM_IRQHandler(void)
{
    PWM_TIMER->EVENTS_COMPARE[3] = 0;
    PWM_TIMER->INTENCLR = 0xFFFFFFFF;

	if (m_pwm_next_value == 0)
	{
		nrf_gpiote_unconfig(PWM_GPIOTE_CHANNEL_0);
		nrf_gpio_pin_write(m_pwm_io_ch, 0);
		m_pwm_running = 0;
	}
	else if (m_pwm_next_value >= m_pwm_max_value)
	{
		nrf_gpiote_unconfig(PWM_GPIOTE_CHANNEL_0);
		nrf_gpio_pin_write(m_pwm_io_ch, 1);
		m_pwm_running = 0;
	}
	else
	{
		PWM_TIMER->CC[0] = m_pwm_next_value * 2;

		if (!m_pwm_running)
		{
			nrf_gpiote_task_config(PWM_GPIOTE_CHANNEL_0, m_pwm_io_ch, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
			m_pwm_running = 1;
		}
	}

    PWM_TIMER->SHORTS = TIMER_SHORTS_COMPARE3_CLEAR_Msk;
    PWM_TIMER->TASKS_START = 1;
}

void buzzer_init(void)
{
	uint32_t err_code;

	nrf_gpio_cfg_output(PIN_BUZZER_ON);

	m_pwm_io_ch = (uint32_t)PIN_BUZZER_ON;
	m_pwm_running = 0;

	// Create timer once, start and stop the same timer multiple times.
	err_code = app_timer_create(&m_buzzer_timeout_timer_id, APP_TIMER_MODE_SINGLE_SHOT, buzzer_timeout_handler);
	APP_ERROR_CHECK(err_code);

	PWM_TIMER->PRESCALER = 0;
	PWM_TIMER->TASKS_CLEAR = 1;
	PWM_TIMER->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
	PWM_TIMER->MODE = TIMER_MODE_MODE_Timer;
	PWM_TIMER->SHORTS = TIMER_SHORTS_COMPARE3_CLEAR_Msk;
	PWM_TIMER->EVENTS_COMPARE[0] = PWM_TIMER->EVENTS_COMPARE[1] = PWM_TIMER->EVENTS_COMPARE[2] = PWM_TIMER->EVENTS_COMPARE[3] = 0;

	sd_ppi_channel_assign(PWM_PPI_CHANNEL_0, &PWM_TIMER->EVENTS_COMPARE[0], &NRF_GPIOTE->TASKS_OUT[PWM_GPIOTE_CHANNEL_0]);
	sd_ppi_channel_enable_set(1 << PWM_PPI_CHANNEL_0);

	// Strangely, omitting these two lines makes for a different, lower tone. No idea why.
	sd_ppi_channel_assign(PWM_PPI_CHANNEL_1, &PWM_TIMER->EVENTS_COMPARE[3], &NRF_GPIOTE->TASKS_OUT[PWM_GPIOTE_CHANNEL_0]);
	sd_ppi_channel_enable_set(1 << PWM_PPI_CHANNEL_1);

	sd_nvic_SetPriority(PWM_IRQn, PWM_IRQ_PRIORITY | 0x01);
	sd_nvic_EnableIRQ(PWM_IRQn);
}

static void buzzer_on(uint32_t frequency, uint32_t duration)
{
	uint32_t err_code;
	uint32_t counter = 0;

	// Only valid if the prescaler is 0:
	m_pwm_max_value = (16 * 1000 * 1000 / (2 * frequency));
	PWM_TIMER->CC[3] = m_pwm_max_value * 2;

	PWM_TIMER->TASKS_START = 1;

    // Start timeout timer again.
    m_buzzer_timed_out = false;

    err_code = app_timer_start(m_buzzer_timeout_timer_id, APP_TIMER_TICKS(duration, 0), NULL);
    APP_ERROR_CHECK(err_code);

    while (!m_buzzer_timed_out)
    {
        nrf_pwm_set_value(sin_table[counter]);
        counter = (counter + 1) % 100;

        // Add a delay to control the speed of the sine wave. Messing with this alters the sound a
        // bit but not in an interesting way.
        nrf_delay_us(8000);
    }

    PWM_TIMER->TASKS_STOP = 1;

    err_code = app_timer_stop(m_buzzer_timeout_timer_id);
    APP_ERROR_CHECK(err_code);
}

void buzzer_ble_connect(void)
{
    buzzer_on(440, 60);
    buzzer_on(880, 180);
    nrf_delay_ms(1000);
}

void buzzer_ble_disconnect(void)
{
    buzzer_on(880, 60);
    buzzer_on(440, 180);
    nrf_delay_ms(1000);
}