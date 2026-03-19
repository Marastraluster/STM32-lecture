#include "main.h"
#include "key.h"

#define KEY_COUNT 6u
#define KEY_DEBOUNCE_MS 20u

volatile uint8_t keyNum = KEY_NONE;

static volatile uint8_t s_key_event = KEY_NONE;
static uint8_t s_last_sample[KEY_COUNT] = {1u, 1u, 1u, 1u, 1u, 1u};
static uint8_t s_stable_state[KEY_COUNT] = {1u, 1u, 1u, 1u, 1u, 1u};
static uint8_t s_stable_cnt[KEY_COUNT] = {0u, 0u, 0u, 0u, 0u, 0u};

static GPIO_TypeDef *const s_key_ports[KEY_COUNT] = {
	K1_GPIO_Port, K2_GPIO_Port, K3_GPIO_Port, K4_GPIO_Port, K5_GPIO_Port, K6_GPIO_Port
};

static const uint16_t s_key_pins[KEY_COUNT] = {
	K1_Pin, K2_Pin, K3_Pin, K4_Pin, K5_Pin, K6_Pin
};

/**
 * @brief 按键扫描任务（由 TIM2 1ms 中断调用）。
 * @param None
 * @retval None
 */
void Key_TimerScan1ms(void)
{
	uint8_t i;

	for (i = 0u; i < KEY_COUNT; ++i) {
		uint8_t sample = (HAL_GPIO_ReadPin(s_key_ports[i], s_key_pins[i]) == GPIO_PIN_SET) ? 1u : 0u;

		if (sample == s_last_sample[i]) {
			if (s_stable_cnt[i] < KEY_DEBOUNCE_MS) {
				s_stable_cnt[i]++;
			}
		} else {
			s_stable_cnt[i] = 0u;
			s_last_sample[i] = sample;
		}

		if (s_stable_cnt[i] == KEY_DEBOUNCE_MS && sample != s_stable_state[i]) {
			s_stable_state[i] = sample;
			if (sample == 0u && s_key_event == KEY_NONE) {
				s_key_event = (uint8_t)(i + 1u);
			}
		}
	}
}

/**
 * @brief 非阻塞读取一次按键按下事件。
 * @param None
 * @retval KEY_NONE 表示无新事件，其它值对应 KEY_1 到 KEY_6。
 */
uint8_t Key_Scan(void)
{
	uint8_t event = s_key_event;

	if (event != KEY_NONE) {
		s_key_event = KEY_NONE;
	}

	keyNum = event;
	return event;

}