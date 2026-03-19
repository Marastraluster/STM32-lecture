#ifndef __LED_H
#define __LED_H

#include "main.h"

typedef enum {
	LED_DIR_FORWARD = 0,
	LED_DIR_REVERSE = 1
} LED_Direction;

/**
 * @brief 初始化流水灯模块并设置默认参数。
 * @param None
 * @retval None
 */
void LED_Init(void);

/**
 * @brief 流水灯任务函数（非阻塞），需在主循环周期调用。
 * @param None
 * @retval None
 */
void LED_Task(void);

/**
 * @brief 切换流水灯运行/暂停状态。
 * @param None
 * @retval None
 */
void LED_ToggleRun(void);

/**
 * @brief 速度档位循环切换：10ms -> 100ms -> 1000ms。
 * @param None
 * @retval None
 */
void LED_CycleSpeedPreset(void);

/**
 * @brief 切换流水灯方向（正向/反向）。
 * @param None
 * @retval None
 */
void LED_ToggleDirection(void);

/**
 * @brief 获取当前流水灯步进间隔（毫秒）。
 * @param None
 * @retval 当前速度，单位 ms。
 */
uint16_t LED_GetStepMs(void);

/**
 * @brief 获取当前流水灯方向。
 * @param None
 * @retval LED_DIR_FORWARD 或 LED_DIR_REVERSE。
 */
LED_Direction LED_GetDirection(void);

/**
 * @brief 查询流水灯是否处于运行状态。
 * @param None
 * @retval 1 表示运行中，0 表示暂停。
 */
uint8_t LED_IsRunning(void);


#endif /* __LED_H */