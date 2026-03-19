#include "main.h"
#include "oled.h"

#include "oled_data.h"
#include <string.h>

#define OLED_DMA_TIMEOUT_MS 100u
#define OLED_I2C_ADDR_3C (0x3Cu << 1)
#define OLED_I2C_ADDR_3D (0x3Du << 1)

extern I2C_HandleTypeDef hi2c1;

static uint8_t oled_framebuffer[OLED_WIDTH * OLED_PAGE_COUNT];
static uint8_t oled_cursor_col;
static uint8_t oled_cursor_page;
static uint8_t oled_tx_data[OLED_WIDTH * OLED_PAGE_COUNT + 1u];
static uint16_t oled_i2c_address = OLED_I2C_ADDR_3C;

/**
 * @brief 等待 I2C 外设进入就绪状态。
 * @param timeout_ms 超时时间（毫秒）。
 * @retval HAL_OK I2C 已就绪。
 * @retval HAL_TIMEOUT 等待超时。
 */
static HAL_StatusTypeDef OLED_WaitI2CReady(uint32_t timeout_ms)
{
	uint32_t start = HAL_GetTick();

	while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
		if ((HAL_GetTick() - start) >= timeout_ms) {
			return HAL_TIMEOUT;
		}
	}

	return HAL_OK;
}

/**
 * @brief 启动一次 I2C DMA 发送并等待结束。
 * @param data 发送缓冲区指针。
 * @param size 发送字节数。
 * @retval HAL_OK 发送完成。
 * @retval HAL_ERROR/HAL_TIMEOUT 发送失败或超时。
 */
static HAL_StatusTypeDef OLED_TransmitDMA(uint8_t *data, uint16_t size)
{
	HAL_StatusTypeDef status;

	if (hi2c1.hdmatx == NULL) {
		return HAL_ERROR;
	}

	status = OLED_WaitI2CReady(OLED_DMA_TIMEOUT_MS);
	if (status != HAL_OK) {
		return status;
	}

	status = HAL_I2C_Master_Transmit_DMA(&hi2c1, oled_i2c_address, data, size);
	if (status != HAL_OK) {
		return status;
	}

	return OLED_WaitI2CReady(OLED_DMA_TIMEOUT_MS);
}

/**
 * @brief 在常见地址中探测 SSD1306 I2C 地址。
 * @param None
 * @retval HAL_OK 探测成功并保存地址。
 * @retval HAL_ERROR 未检测到设备响应。
 */
static HAL_StatusTypeDef OLED_DetectAddress(void)
{
	if (HAL_I2C_IsDeviceReady(&hi2c1, OLED_I2C_ADDR_3C, 2, 20) == HAL_OK) {
		oled_i2c_address = OLED_I2C_ADDR_3C;
		return HAL_OK;
	}

	if (HAL_I2C_IsDeviceReady(&hi2c1, OLED_I2C_ADDR_3D, 2, 20) == HAL_OK) {
		oled_i2c_address = OLED_I2C_ADDR_3D;
		return HAL_OK;
	}

	return HAL_ERROR;
}

/**
 * @brief 发送一条 SSD1306 命令。
 * @param cmd 命令字节。
 * @retval HAL_OK 发送成功。
 * @retval HAL_ERROR/HAL_TIMEOUT I2C 发送失败。
 */
static HAL_StatusTypeDef OLED_SendCommand(uint8_t cmd)
{
	uint8_t packet[2] = {0x00, cmd};
	return OLED_TransmitDMA(packet, sizeof(packet));
}

/**
 * @brief 设置 SSD1306 页模式地址。
 * @param page 目标页号。
 * @retval HAL_OK 命令序列发送成功。
 * @retval HAL_ERROR/HAL_TIMEOUT I2C 发送失败。
 */
static HAL_StatusTypeDef OLED_SetPageAddress(uint8_t page)
{
	HAL_StatusTypeDef status;

	status = OLED_SendCommand((uint8_t)(0xB0u | page));
	if (status != HAL_OK) {
		return status;
	}

	status = OLED_SendCommand(0x00);
	if (status != HAL_OK) {
		return status;
	}

	return OLED_SendCommand(0x10);
}

/**
 * @brief 配置 SSD1306 整屏刷新窗口（列/页范围）。
 * @param None
 * @retval HAL_OK 窗口设置成功。
 * @retval HAL_ERROR/HAL_TIMEOUT I2C 发送失败。
 */
static HAL_StatusTypeDef OLED_SetFullFrameWindow(void)
{
	HAL_StatusTypeDef status;

	status = OLED_SendCommand(0x21);
	if (status != HAL_OK) {
		return status;
	}

	status = OLED_SendCommand(0x00);
	if (status != HAL_OK) {
		return status;
	}

	status = OLED_SendCommand((uint8_t)(OLED_WIDTH - 1u));
	if (status != HAL_OK) {
		return status;
	}

	status = OLED_SendCommand(0x22);
	if (status != HAL_OK) {
		return status;
	}

	status = OLED_SendCommand(0x00);
	if (status != HAL_OK) {
		return status;
	}

	return OLED_SendCommand((uint8_t)(OLED_PAGE_COUNT - 1u));
}

/**
 * @brief 兼容旧接口的初始化包装函数。
 * @param None
 * @retval None
 */
void oled_init(void)
{
	(void)OLED_Init();
}

/**
 * @brief 初始化 OLED 控制器并清空显示内容。
 * @param None
 * @retval HAL_OK 初始化成功。
 * @retval HAL_ERROR 地址探测或命令发送失败。
 */
HAL_StatusTypeDef OLED_Init(void)
{
	const uint8_t init_cmds[] = {
		0xAE, 0x20, 0x00, 0xB0, 0xC8, 0x00, 0x10,
		0x40, 0x81, 0x7F, 0xA1, 0xA6, 0xA8, 0x3F,
		0xA4, 0xD3, 0x00, 0xD5, 0x80, 0xD9, 0xF1,
		0xDA, 0x12, 0xDB, 0x40, 0x8D, 0x14, 0xAF
	};
	uint32_t i;

	OLED_Clear();
	oled_cursor_col = 0;
	oled_cursor_page = 0;

	if (OLED_DetectAddress() != HAL_OK) {
		return HAL_ERROR;
	}

	for (i = 0; i < (sizeof(init_cmds) / sizeof(init_cmds[0])); ++i) {
		if (OLED_SendCommand(init_cmds[i]) != HAL_OK) {
			return HAL_ERROR;
		}
	}

	return OLED_UpdateScreenDMA();
}

/**
 * @brief 清空显存内容。
 * @param None
 * @retval None
 */
void OLED_Clear(void)
{
	memset(oled_framebuffer, 0x00, sizeof(oled_framebuffer));
}

/**
 * @brief 使用同一值填充整块显存。
 * @param on 为 0 时全灭，非 0 时全亮。
 * @retval None
 */
void OLED_Fill(uint8_t on)
{
	memset(oled_framebuffer, on ? 0xFF : 0x00, sizeof(oled_framebuffer));
}

/**
 * @brief 设置文本绘制光标位置。
 * @param col 起始列坐标。
 * @param page 页坐标。
 * @retval None
 */
void OLED_SetCursor(uint8_t col, uint8_t page)
{
	if (col < OLED_WIDTH) {
		oled_cursor_col = col;
	}

	if (page < OLED_PAGE_COUNT) {
		oled_cursor_page = page;
	}
}

/**
 * @brief 在显存中绘制单个像素。
 * @param x 像素 X 坐标。
 * @param y 像素 Y 坐标。
 * @param on 为 0 清除像素，非 0 点亮像素。
 * @retval None
 */
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t on)
{
	uint16_t index;
	uint8_t bit_mask;

	if (x >= OLED_WIDTH || y >= OLED_HEIGHT) {
		return;
	}

	index = (uint16_t)x + ((uint16_t)(y / 8u) * OLED_WIDTH);
	bit_mask = (uint8_t)(1u << (y % 8u));

	if (on) {
		oled_framebuffer[index] |= bit_mask;
	} else {
		oled_framebuffer[index] &= (uint8_t)~bit_mask;
	}
}

/**
 * @brief 使用 6x8 字模绘制单个字符。
 * @param c 待绘制字符。
 * @retval None
 */
void OLED_WriteChar(char c)
{
	const uint8_t *glyph;
	uint8_t i;
	uint16_t base;

	if (c == '\n') {
		oled_cursor_col = 0;
		oled_cursor_page = (uint8_t)((oled_cursor_page + 1u) % OLED_PAGE_COUNT);
		return;
	}

	if ((oled_cursor_col + OLED_FONT_WIDTH) > OLED_WIDTH) {
		oled_cursor_col = 0;
		oled_cursor_page = (uint8_t)((oled_cursor_page + 1u) % OLED_PAGE_COUNT);
	}

	glyph = OLED_GetGlyph(c);
	base = (uint16_t)oled_cursor_page * OLED_WIDTH + oled_cursor_col;

	for (i = 0; i < OLED_FONT_WIDTH; ++i) {
		oled_framebuffer[base + i] = glyph[i];
	}

	oled_cursor_col = (uint8_t)(oled_cursor_col + OLED_FONT_WIDTH);
}

/**
 * @brief 从当前光标开始绘制字符串到显存。
 * @param text 指向 '\0' 结尾字符串的指针。
 * @retval None
 */
void OLED_WriteString(const char *text)
{
	if (text == NULL) {
		return;
	}

	while (*text != '\0') {
		OLED_WriteChar(*text);
		++text;
	}
}

/**
 * @brief 通过一次 DMA 大块传输刷新整屏显存。
 * @param None
 * @retval HAL_OK 刷新完成。
 * @retval HAL_ERROR/HAL_TIMEOUT 传输失败。
 */
HAL_StatusTypeDef OLED_UpdateScreenDMA(void)
{
	HAL_StatusTypeDef status;

	status = OLED_SetFullFrameWindow();
	if (status != HAL_OK) {
		return status;
	}

	oled_tx_data[0] = 0x40;
	memcpy(&oled_tx_data[1], oled_framebuffer, sizeof(oled_framebuffer));

	status = OLED_TransmitDMA(oled_tx_data, (uint16_t)sizeof(oled_tx_data));
	if (status != HAL_OK) {
		return status;
	}

	return HAL_OK;
}

/**
 * @brief 通过重复整屏刷新测试显示速度。
 * @param frames 测试帧数。
 * @param fps_x100 输出帧率，放大 100 倍。
 * @param frame_ms_x100 输出单帧周期（毫秒），放大 100 倍。
 * @retval HAL_OK 测试完成。
 * @retval HAL_ERROR 输入无效或传输失败。
 */
HAL_StatusTypeDef OLED_BenchmarkFPS(uint16_t frames, uint32_t *fps_x100, uint32_t *frame_ms_x100)
{
	uint16_t i;
	uint32_t start_tick;
	uint32_t elapsed_ms;
	HAL_StatusTypeDef status;

	if (frames == 0u || fps_x100 == NULL || frame_ms_x100 == NULL) {
		return HAL_ERROR;
	}

	start_tick = HAL_GetTick();

	for (i = 0u; i < frames; ++i) {
		OLED_Fill((uint8_t)(i & 0x01u));
		status = OLED_UpdateScreenDMA();
		if (status != HAL_OK) {
			return status;
		}
	}

	elapsed_ms = HAL_GetTick() - start_tick;
	if (elapsed_ms == 0u) {
		elapsed_ms = 1u;
	}

	*fps_x100 = (uint32_t)(((uint64_t)frames * 100000ull) / (uint64_t)elapsed_ms);
	*frame_ms_x100 = (uint32_t)(((uint64_t)elapsed_ms * 100ull) / (uint64_t)frames);

	return HAL_OK;
}