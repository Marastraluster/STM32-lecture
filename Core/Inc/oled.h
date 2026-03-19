#ifndef __OLED_H
#define __OLED_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define OLED_WIDTH 128u
#define OLED_HEIGHT 64u
#define OLED_PAGE_COUNT (OLED_HEIGHT / 8u)

/**
 * @brief 通过 I2C DMA 初始化 SSD1306 屏幕。
 * @param None
 * @retval HAL_OK 初始化成功。
 * @retval HAL_ERROR I2C 地址探测失败或命令发送失败。
 */
HAL_StatusTypeDef OLED_Init(void);

/**
 * @brief 清空显存，关闭所有像素。
 * @param None
 * @retval None
 */
void OLED_Clear(void);

/**
 * @brief 按给定值填充整块显存。
 * @param on 为 0 时清屏，非 0 时全部点亮。
 * @retval None
 */
void OLED_Fill(uint8_t on);

/**
 * @brief 设置文本光标位置。
 * @param col 列坐标（像素单位）。
 * @param page 页坐标（范围 0 到 OLED_PAGE_COUNT-1）。
 * @retval None
 */
void OLED_SetCursor(uint8_t col, uint8_t page);

/**
 * @brief 在显存中绘制或清除一个像素点。
 * @param x X 坐标（范围 0 到 OLED_WIDTH-1）。
 * @param y Y 坐标（范围 0 到 OLED_HEIGHT-1）。
 * @param on 为 0 清除像素，非 0 点亮像素。
 * @retval None
 */
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t on);

/**
 * @brief 在当前光标处写入一个 ASCII 字符。
 * @param c 待写入字符。
 * @retval None
 */
void OLED_WriteChar(char c);

/**
 * @brief 在当前光标处写入以 '\0' 结尾的字符串。
 * @param text 字符串指针。
 * @retval None
 */
void OLED_WriteString(const char *text);

/**
 * @brief 通过 I2C DMA 刷新整屏显存到 OLED。
 * @param None
 * @retval HAL_OK 传输完成。
 * @retval HAL_ERROR/HAL_TIMEOUT I2C 传输失败或超时。
 */
HAL_StatusTypeDef OLED_UpdateScreenDMA(void);

/**
 * @brief 通过连续整屏刷新测试显示帧率。
 * @param frames 测试帧数。
 * @param fps_x100 输出帧率，放大 100 倍。
 * @param frame_ms_x100 输出单帧耗时（毫秒），放大 100 倍。
 * @retval HAL_OK 测试完成。
 * @retval HAL_ERROR 输入参数非法或传输失败。
 */
HAL_StatusTypeDef OLED_BenchmarkFPS(uint16_t frames, uint32_t *fps_x100, uint32_t *frame_ms_x100);

#endif /* __OLED_H  */