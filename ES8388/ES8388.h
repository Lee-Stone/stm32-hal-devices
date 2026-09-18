#ifndef __ES8388_H
#define __ES8388_H

#include "config.h"

#if DEVICE_ES8388

#include <stdint.h>

/* ==================== 芯片与音频参数宏定义 ==================== */

#define ES8388_I2C_ADDRESS_7BIT          0x10U
#define ES8388_I2C_ADDRESS_HAL           (ES8388_I2C_ADDRESS_7BIT << 1U)
#define ES8388_I2C_TIMEOUT_MS             100U
#define ES8388_AUDIO_BITS                  16U
#define ES8388_AUDIO_CHANNELS               2U
#define ES8388_HEADPHONE_VOLUME_DEFAULT  0x16U
#define ES8388_SPEAKER_VOLUME_DEFAULT    0x1EU
#define ES8388_OUTPUT_VOLUME_MAX         0x1EU

/* ==================== 类型定义 ==================== */

typedef enum
{
    ES8388_OK = 0,
    ES8388_ERROR_ARGUMENT,
    ES8388_ERROR_I2C,
    ES8388_ERROR_I2S,
    ES8388_ERROR_NOT_INITIALIZED,
    ES8388_ERROR_SOURCE
} ES8388_Status_t;

typedef enum
{
    ES8388_INPUT_NONE = 0,
    ES8388_INPUT_BOARD_MIC,
    ES8388_INPUT_HEADSET_MIC
} ES8388_Input_t;

typedef enum
{
    ES8388_OUTPUT_NONE = 0,
    ES8388_OUTPUT_HEADPHONE,
    ES8388_OUTPUT_SPEAKER
} ES8388_Output_t;

/**
  * @brief  音频流读取回调
  * @param  context 用户数据，可指向数组位置、文件对象或 MP3 解码器
  * @param  samples PCM 输出缓冲区
  * @param  max_words 本次最多写入的 16 位 PCM 字数
  * @retval 大于 0 为实际 PCM 字数，0 为流结束，小于 0 为读取或解码失败
  */
typedef int32_t (*ES8388_ReadCallback_t)(void *context, int16_t *samples,
                                         uint32_t max_words);

/* ==================== API 函数声明 ==================== */

ES8388_Status_t ES8388_Init(void);                                           // 初始化编解码器，默认不打开输入或输出
ES8388_Status_t ES8388_DeInit(void);                                         // 静音并复位编解码器
ES8388_Status_t ES8388_ReadRegister(uint8_t reg, uint8_t *value);            // 读取一个寄存器
ES8388_Status_t ES8388_WriteRegister(uint8_t reg, uint8_t value);            // 写入一个寄存器
ES8388_Status_t ES8388_SetInput(ES8388_Input_t input);                       // 选择一路录音输入，同时关闭所有输出
ES8388_Status_t ES8388_SetOutput(ES8388_Output_t output);                    // 选择一路播放输出，同时关闭所有输入
ES8388_Status_t ES8388_SetInputGain(uint8_t gain_db);                        // 设置 ADC 增益，0~24 dB，步进 3 dB
ES8388_Status_t ES8388_SetOutputVolume(ES8388_Output_t output,
                                       uint8_t volume);                      // 设置指定输出音量，范围 0x00~0x1E
ES8388_Status_t ES8388_SetMute(uint8_t muted);                               // 设置当前播放输出的静音状态
ES8388_Status_t ES8388_Transmit(const int16_t *samples, uint32_t word_count,
                                uint32_t timeout_ms);                         // 阻塞发送双声道 PCM
ES8388_Status_t ES8388_Receive(int16_t *samples, uint32_t word_count,
                               uint32_t timeout_ms);                          // 阻塞接收双声道 PCM
ES8388_Status_t ES8388_PlayBuffer(const int16_t *samples,
                                  uint32_t word_count,
                                  ES8388_Output_t output,
                                  uint32_t timeout_ms);                       // 播放数组或动态内存中的 PCM
ES8388_Status_t ES8388_PlayStream(ES8388_ReadCallback_t read_callback,
                                  void *context, int16_t *work_buffer,
                                  uint32_t work_words,
                                  ES8388_Output_t output,
                                  uint32_t timeout_ms);                       // 通过回调播放文件或解码后的 PCM 流
ES8388_Status_t ES8388_RecordBuffer(int16_t *samples, uint32_t word_count,
                                    ES8388_Input_t input,
                                    uint32_t timeout_ms);                     // 录制一路输入到用户缓冲区

#endif /* DEVICE_ES8388 */

#endif /* __ES8388_H */
