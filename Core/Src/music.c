#include "music.h"

static TIM_HandleTypeDef *g_music_tim = NULL;
static uint32_t g_music_channel = TIM_CHANNEL_1;
static const Music_Tone *g_melody = NULL;
static uint16_t g_melody_len = 0u;
static uint16_t g_melody_idx = 0u;
static uint8_t g_melody_duty = 50u;
static uint8_t g_melody_busy = 0u;
static uint8_t g_note_gap_phase = 0u;
static uint32_t g_next_tick = 0u;

#define MUSIC_NOTE_GAP_MS 10u

/**
 * @brief 比较当前时间是否到达目标 Tick（支持溢出）。
 * @param now 当前 Tick。
 * @param target 目标 Tick。
 * @retval 1 已到达或超过。
 * @retval 0 尚未到达。
 */
static uint8_t Music_TimeReached(uint32_t now, uint32_t target)
{
	return ((int32_t)(now - target) >= 0) ? 1u : 0u;
}

/**
 * @brief 将当前音符下发到 PWM 并更新到期时间。
 * @param now 当前 Tick。
 * @retval HAL_OK 设置成功。
 * @retval HAL_ERROR 音符参数非法或频率配置失败。
 */
static HAL_StatusTypeDef Music_ApplyCurrentTone(uint32_t now)
{
	if (g_melody == NULL || g_melody_idx >= g_melody_len) {
		return HAL_ERROR;
	}

	if (Music_SetFrequency(g_melody[g_melody_idx].freq_hz, g_melody_duty) != HAL_OK) {
		return HAL_ERROR;
	}

	g_note_gap_phase = 0u;
	g_next_tick = now + (uint32_t)g_melody[g_melody_idx].duration_ms;
	return HAL_OK;
}

/**
 * @brief 按 APB 分频规则获取定时器实际输入时钟。
 * @param htim 定时器句柄指针。
 * @retval 定时器时钟频率（Hz）。
 */
static uint32_t Music_GetTimerClock(TIM_HandleTypeDef *htim)
{
	uint32_t pclk;
	uint32_t apb_prescaler;

	if (htim->Instance == TIM1 || htim->Instance == TIM8 ||
		htim->Instance == TIM9 || htim->Instance == TIM10 || htim->Instance == TIM11) {
		pclk = HAL_RCC_GetPCLK2Freq();
		apb_prescaler = ((RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos);
	} else {
		pclk = HAL_RCC_GetPCLK1Freq();
		apb_prescaler = ((RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos);
	}

	if (apb_prescaler >= 4u) {
		return pclk * 2u;
	}

	return pclk;
}

/**
 * @brief 初始化无源蜂鸣器使用的 PWM 通道。
 * @param htim 蜂鸣器对应的定时器句柄。
 * @param channel PWM 通道标识。
 * @retval HAL_OK 初始化成功。
 * @retval HAL_ERROR 参数无效或 HAL 启动失败。
 */
HAL_StatusTypeDef Music_Init(TIM_HandleTypeDef *htim, uint32_t channel)
{
	if (htim == NULL) {
		return HAL_ERROR;
	}

	g_music_tim = htim;
	g_music_channel = channel;

	if (HAL_TIM_PWM_Start(g_music_tim, g_music_channel) != HAL_OK) {
		return HAL_ERROR;
	}

	__HAL_TIM_SET_COMPARE(g_music_tim, g_music_channel, 0u);
	return HAL_OK;
}

/**
 * @brief 重新配置蜂鸣器输出频率和占空比。
 * @param freq_hz 目标频率（Hz），0 表示静音。
 * @param duty_percent 占空比百分比。
 * @retval HAL_OK 配置成功。
 * @retval HAL_ERROR 模块未初始化或参数超出定时器范围。
 */
HAL_StatusTypeDef Music_SetFrequency(uint16_t freq_hz, uint8_t duty_percent)
{
	uint32_t tim_clk;
	uint32_t psc_div;
	uint32_t period_cnt;
	uint32_t arr;
	uint32_t compare;

	if (g_music_tim == NULL) {
		return HAL_ERROR;
	}

	if (freq_hz == 0u) {
		__HAL_TIM_SET_COMPARE(g_music_tim, g_music_channel, 0u);
		return HAL_OK;
	}

	if (duty_percent > 100u) {
		duty_percent = 50u;
	}

	tim_clk = Music_GetTimerClock(g_music_tim);
	psc_div = g_music_tim->Instance->PSC + 1u;
	if (psc_div == 0u) {
		return HAL_ERROR;
	}

	period_cnt = tim_clk / psc_div / (uint32_t)freq_hz;
	if (period_cnt < 2u) {
		return HAL_ERROR;
	}

	arr = period_cnt - 1u;
	if (arr > 0xFFFFu) {
		return HAL_ERROR;
	}

	compare = (period_cnt * duty_percent) / 100u;
	if (compare > arr) {
		compare = arr;
	}

	__HAL_TIM_SET_AUTORELOAD(g_music_tim, arr);
	__HAL_TIM_SET_COMPARE(g_music_tim, g_music_channel, compare);
	__HAL_TIM_SET_COUNTER(g_music_tim, 0u);
	g_music_tim->Instance->EGR = TIM_EGR_UG;

	return HAL_OK;
}

/**
 * @brief 以阻塞方式播放单个音符。
 * @param freq_hz 音符频率（Hz）。
 * @param duration_ms 播放时长（毫秒）。
 * @param duty_percent 占空比百分比。
 * @retval HAL_OK 播放完成。
 * @retval HAL_ERROR 频率设置失败。
 */
HAL_StatusTypeDef Music_PlayTone(uint16_t freq_hz, uint16_t duration_ms, uint8_t duty_percent)
{
	uint32_t start;

	if (Music_SetFrequency(freq_hz, duty_percent) != HAL_OK) {
		return HAL_ERROR;
	}

	start = HAL_GetTick();
	while (!Music_TimeReached(HAL_GetTick(), start + (uint32_t)duration_ms)) {
	}

	Music_Stop();
	return HAL_OK;
}

/**
 * @brief 以阻塞方式播放旋律数组。
 * @param melody 旋律数组指针。
 * @param length 旋律中的音符数量。
 * @param duty_percent 占空比百分比。
 * @retval HAL_OK 播放完成。
 * @retval HAL_ERROR 输入无效或播放失败。
 */
HAL_StatusTypeDef Music_PlayMelody(const Music_Tone *melody, uint16_t length, uint8_t duty_percent)
{
	if (Music_StartMelody(melody, length, duty_percent) != HAL_OK) {
		return HAL_ERROR;
	}

	while (Music_IsBusy() != 0u) {
		Music_Task();
	}

	return HAL_OK;
}

/**
 * @brief 以非阻塞方式启动旋律播放。
 * @param melody 旋律数组指针。
 * @param length 旋律中的音符数量。
 * @param duty_percent 占空比百分比。
 * @retval HAL_OK 启动成功。
 * @retval HAL_ERROR 输入无效或频率设置失败。
 */
HAL_StatusTypeDef Music_StartMelody(const Music_Tone *melody, uint16_t length, uint8_t duty_percent)
{
	if (melody == NULL || length == 0u || g_music_tim == NULL) {
		return HAL_ERROR;
	}

	if (duty_percent > 100u) {
		duty_percent = 50u;
	}

	g_melody = melody;
	g_melody_len = length;
	g_melody_idx = 0u;
	g_melody_duty = duty_percent;
	g_melody_busy = 1u;

	if (Music_ApplyCurrentTone(HAL_GetTick()) != HAL_OK) {
		g_melody_busy = 0u;
		g_melody = NULL;
		g_melody_len = 0u;
		return HAL_ERROR;
	}

	return HAL_OK;
}

/**
 * @brief 音乐任务处理函数，需周期调用以推进旋律。
 * @param None
 * @retval None
 */
void Music_Task(void)
{
	uint32_t now;

	if (g_melody_busy == 0u) {
		return;
	}

	now = HAL_GetTick();
	if (Music_TimeReached(now, g_next_tick) == 0u) {
		return;
	}

	if (g_note_gap_phase == 0u) {
		Music_Stop();
		g_note_gap_phase = 1u;
		g_next_tick = now + MUSIC_NOTE_GAP_MS;
		return;
	}

	++g_melody_idx;
	if (g_melody_idx >= g_melody_len) {
		g_melody_busy = 0u;
		g_melody = NULL;
		g_melody_len = 0u;
		Music_Stop();
		return;
	}

	if (Music_ApplyCurrentTone(now) != HAL_OK) {
		g_melody_busy = 0u;
		g_melody = NULL;
		g_melody_len = 0u;
		Music_Stop();
	}
}

/**
 * @brief 查询旋律播放状态。
 * @param None
 * @retval 1 表示播放中，0 表示空闲。
 */
uint8_t Music_IsBusy(void)
{
	return g_melody_busy;
}

/**
 * @brief 将比较值置零以停止蜂鸣器输出。
 * @param None
 * @retval None
 */
void Music_Stop(void)
{
	if (g_music_tim != NULL) {
		__HAL_TIM_SET_COMPARE(g_music_tim, g_music_channel, 0u);
	}

	g_melody_busy = 0u;
	g_melody = NULL;
	g_melody_len = 0u;
	g_melody_idx = 0u;
	g_note_gap_phase = 0u;
}
