#ifndef __KEY_H
#define __KEY_H

#include "main.h"

#define KEY_NONE 0u
#define KEY_1    1u
#define KEY_2    2u
#define KEY_3    3u
#define KEY_4    4u
#define KEY_5    5u
#define KEY_6    6u

extern volatile uint8_t keyNum;

/**
 * @brief 按键扫描任务（由 TIM2 1ms 中断调用）。
 * @param None
 * @retval None
 */
void Key_TimerScan1ms(void);

/**
 * @brief 非阻塞读取一次按键按下事件。
 * @param None
 * @retval KEY_NONE 表示无新事件，其它值对应 KEY_1 到 KEY_6。
 */
uint8_t Key_Scan(void);


#endif /* __KEY_H */