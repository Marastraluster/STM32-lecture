#ifndef __MUSIC_H
#define __MUSIC_H

#include "main.h"
#include "music_data.h"
#include <stdint.h>

typedef struct {
	/** @brief 音符频率（Hz），静音可用 NOTE_REST。 */
	uint16_t freq_hz;
	/** @brief 音符持续时间（毫秒）。 */
	uint16_t duration_ms;
} Music_Tone;

/**
 * @brief 初始化无源蜂鸣器 PWM 模块。
 * @param htim 用于输出 PWM 的定时器句柄。
 * @param channel PWM 通道（例如 TIM_CHANNEL_1）。
 * @retval HAL_OK 初始化成功。
 * @retval HAL_ERROR 参数无效或 PWM 启动失败。
 */
HAL_StatusTypeDef Music_Init(TIM_HandleTypeDef *htim, uint32_t channel);

/**
 * @brief 设置蜂鸣器输出频率和占空比。
 * @param freq_hz 目标频率（Hz），为 0 表示静音。
 * @param duty_percent PWM 占空比（0-100）。
 * @retval HAL_OK 配置成功。
 * @retval HAL_ERROR 模块未初始化或频率超出定时器可配置范围。
 */
HAL_StatusTypeDef Music_SetFrequency(uint16_t freq_hz, uint8_t duty_percent);

/**
 * @brief 以阻塞方式播放一个音符。
 * @param freq_hz 音符频率（Hz），0 表示休止。
 * @param duration_ms 音符时长（毫秒）。
 * @param duty_percent PWM 占空比（0-100）。
 * @retval HAL_OK 播放完成。
 * @retval HAL_ERROR 频率配置失败。
 */
HAL_StatusTypeDef Music_PlayTone(uint16_t freq_hz, uint16_t duration_ms, uint8_t duty_percent);

/**
 * @brief 以阻塞方式播放一段旋律。
 * @param melody 旋律数组指针。
 * @param length 旋律数组元素个数。
 * @param duty_percent PWM 占空比（0-100）。
 * @retval HAL_OK 旋律播放完成。
 * @retval HAL_ERROR 输入无效或播放过程中出错。
 */
HAL_StatusTypeDef Music_PlayMelody(const Music_Tone *melody, uint16_t length, uint8_t duty_percent);

/**
 * @brief 以非阻塞方式启动旋律播放。
 * @param melody 旋律数组指针。
 * @param length 旋律数组元素个数。
 * @param duty_percent PWM 占空比（0-100）。
 * @retval HAL_OK 启动成功。
 * @retval HAL_ERROR 输入无效或模块未初始化。
 */
HAL_StatusTypeDef Music_StartMelody(const Music_Tone *melody, uint16_t length, uint8_t duty_percent);

/**
 * @brief 音乐任务函数，需在主循环中周期调用。
 * @param None
 * @retval None
 */
void Music_Task(void);

/**
 * @brief 查询当前是否正在播放旋律。
 * @param None
 * @retval 1 表示播放中，0 表示空闲。
 */
uint8_t Music_IsBusy(void);

/**
 * @brief 立即停止蜂鸣器输出。
 * @param None
 * @retval None
 */
void Music_Stop(void);

#endif /* __MUSIC_H */
