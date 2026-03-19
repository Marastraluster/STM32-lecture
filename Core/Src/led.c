#include "led.h"

static const uint16_t s_speed_presets_ms[3] = {10u, 100u, 1000u};
static const uint16_t s_led_pins[8] = {
	L1_Pin, L2_Pin, L3_Pin, L4_Pin, L5_Pin, L6_Pin, L7_Pin, L8_Pin
};

static uint8_t s_led_index = 0u;
static uint8_t s_running = 1u;
static uint8_t s_speed_idx = 1u;
static LED_Direction s_direction = LED_DIR_FORWARD;
static uint32_t s_last_tick = 0u;

static void LED_OutputCurrent(void)
{
	uint8_t i;

	for (i = 0u; i < 8u; ++i) {
		HAL_GPIO_WritePin(GPIOE, s_led_pins[i], GPIO_PIN_RESET);
	}

	HAL_GPIO_WritePin(GPIOE, s_led_pins[s_led_index], GPIO_PIN_SET);
}

/**
 * @brief 初始化流水灯模块并设置默认参数。
 * @param None
 * @retval None
 */
void LED_Init(void)
{
	s_led_index = 0u;
	s_running = 1u;
	s_speed_idx = 1u;
	s_direction = LED_DIR_FORWARD;
	s_last_tick = HAL_GetTick();
	LED_OutputCurrent();
}

/**
 * @brief 流水灯任务函数（非阻塞），需在主循环周期调用。
 * @param None
 * @retval None
 */
void LED_Task(void)
{
	uint32_t now;
	uint16_t step_ms;

	if (s_running == 0u) {
		return;
	}

	now = HAL_GetTick();
	step_ms = s_speed_presets_ms[s_speed_idx];
    if ((now - s_last_tick) < step_ms) {
		return;
	}

	s_last_tick = now;

	if (s_direction == LED_DIR_FORWARD) {
		s_led_index = (uint8_t)((s_led_index + 1u) % 8u);
	} else {
		s_led_index = (uint8_t)((s_led_index + 7u) % 8u);
	}

	LED_OutputCurrent();
}

/**
 * @brief 切换流水灯运行/暂停状态。
 * @param None
 * @retval None
 */
void LED_ToggleRun(void)
{
	s_running = (uint8_t)!s_running;
	s_last_tick = HAL_GetTick();
}

/**
 * @brief 速度档位循环切换：10ms -> 100ms -> 1000ms。
 * @param None
 * @retval None
 */
void LED_CycleSpeedPreset(void)
{
	s_speed_idx = (uint8_t)((s_speed_idx + 1u) % 3u);
	s_last_tick = HAL_GetTick();
}

/**
 * @brief 切换流水灯方向（正向/反向）。
 * @param None
 * @retval None
 */
void LED_ToggleDirection(void)
{
	if (s_direction == LED_DIR_FORWARD) {
		s_direction = LED_DIR_REVERSE;
	} else {
		s_direction = LED_DIR_FORWARD;
	}
}

/**
 * @brief 获取当前流水灯步进间隔（毫秒）。
 * @param None
 * @retval 当前速度，单位 ms。
 */
uint16_t LED_GetStepMs(void)
{
	return s_speed_presets_ms[s_speed_idx];
}

/**
 * @brief 获取当前流水灯方向。
 * @param None
 * @retval LED_DIR_FORWARD 或 LED_DIR_REVERSE。
 */
LED_Direction LED_GetDirection(void)
{
	return s_direction;
}

/**
 * @brief 查询流水灯是否处于运行状态。
 * @param None
 * @retval 1 表示运行中，0 表示暂停。
 */
uint8_t LED_IsRunning(void)
{
	return s_running;
}
