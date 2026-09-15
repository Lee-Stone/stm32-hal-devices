#include "ES8388.h"

#if DEVICE_ES8388

#include <stddef.h>

/* ==================== 寄存器与固定配置 ==================== */

#define ES8388_REG_CONTROL1          0x00U
#define ES8388_REG_CONTROL2          0x01U
#define ES8388_REG_CHIPPOWER         0x02U
#define ES8388_REG_ADCPOWER          0x03U
#define ES8388_REG_DACPOWER          0x04U
#define ES8388_REG_MASTERMODE        0x08U
#define ES8388_REG_ADCCONTROL1       0x09U
#define ES8388_REG_ADCCONTROL2       0x0AU
#define ES8388_REG_ADCCONTROL3       0x0BU
#define ES8388_REG_ADCCONTROL4       0x0CU
#define ES8388_REG_ADCCONTROL5       0x0DU
#define ES8388_REG_ADCCONTROL8       0x10U
#define ES8388_REG_ADCCONTROL9       0x11U
#define ES8388_REG_DACCONTROL1       0x17U
#define ES8388_REG_DACCONTROL2       0x18U
#define ES8388_REG_DACCONTROL3       0x19U
#define ES8388_REG_DACCONTROL4       0x1AU
#define ES8388_REG_DACCONTROL5       0x1BU
#define ES8388_REG_DACCONTROL16      0x26U
#define ES8388_REG_DACCONTROL17      0x27U
#define ES8388_REG_DACCONTROL20      0x2AU
#define ES8388_REG_DACCONTROL21      0x2BU
#define ES8388_REG_DACCONTROL23      0x2DU
#define ES8388_REG_DACCONTROL24      0x2EU
#define ES8388_REG_DACCONTROL25      0x2FU
#define ES8388_REG_DACCONTROL26      0x30U
#define ES8388_REG_DACCONTROL27      0x31U

#define ES8388_DAC_MUTED             0x24U
#define ES8388_DAC_UNMUTED           0x20U
#define ES8388_DAC_POWER_OFF         0xC0U
#define ES8388_DAC_POWER_HEADPHONE   0x30U
#define ES8388_DAC_POWER_SPEAKER     0x0CU
#define ES8388_ADC_POWER_OFF         0xFCU
#define ES8388_AUDIO_CHUNK_WORDS      512U

/* ==================== 内部状态 ==================== */

static uint8_t ES8388_Initialized;
static uint8_t ES8388_Muted = 1U;
static uint8_t ES8388_HeadphoneVolume = ES8388_HEADPHONE_VOLUME_DEFAULT;
static uint8_t ES8388_SpeakerVolume = ES8388_SPEAKER_VOLUME_DEFAULT;
static ES8388_Input_t ES8388_CurrentInput = ES8388_INPUT_NONE;
static ES8388_Output_t ES8388_CurrentOutput = ES8388_OUTPUT_NONE;
static uint16_t ES8388_Silence[ES8388_AUDIO_CHUNK_WORDS];

/* ==================== 寄存器访问 ==================== */

/**
  * @brief  通过 I2C 读取一个 ES8388 寄存器
  * @param  reg 寄存器地址
  * @param  value 寄存器值输出地址
  * @retval ES8388_OK 读取成功，其他值读取失败
  */
ES8388_Status_t ES8388_ReadRegister(uint8_t reg, uint8_t *value)
{
    if (value == NULL)
    {
        return ES8388_ERROR_ARGUMENT;
    }
    return (HAL_I2C_Mem_Read(&ES8388_I2C_HANDLE,
                             ES8388_I2C_ADDRESS_HAL, reg,
                             I2C_MEMADD_SIZE_8BIT, value, 1U,
                             ES8388_I2C_TIMEOUT_MS) == HAL_OK) ?
           ES8388_OK : ES8388_ERROR_I2C;
}

/**
  * @brief  通过 I2C 写入一个 ES8388 寄存器
  * @param  reg 寄存器地址
  * @param  value 寄存器值
  * @retval ES8388_OK 写入成功，其他值写入失败
  */
ES8388_Status_t ES8388_WriteRegister(uint8_t reg, uint8_t value)
{
    return (HAL_I2C_Mem_Write(&ES8388_I2C_HANDLE,
                              ES8388_I2C_ADDRESS_HAL, reg,
                              I2C_MEMADD_SIZE_8BIT, &value, 1U,
                              ES8388_I2C_TIMEOUT_MS) == HAL_OK) ?
           ES8388_OK : ES8388_ERROR_I2C;
}

/* ==================== 内部函数 ==================== */

/**
  * @brief  按顺序写入一组寄存器值
  * @param  values 每项依次为寄存器地址和值
  * @param  count 寄存器项数量
  * @retval ES8388_OK 全部写入成功，其他值写入失败
  */
static ES8388_Status_t es8388_write_values(const uint8_t values[][2],
                                            uint32_t count)
{
    uint32_t index;

    for (index = 0U; index < count; index++)
    {
        if (ES8388_WriteRegister(values[index][0],
                                 values[index][1]) != ES8388_OK)
        {
            return ES8388_ERROR_I2C;
        }
    }
    return ES8388_OK;
}

/**
  * @brief  检查输入枚举是否有效
  * @param  input 输入枚举
  * @retval 1 参数有效，0 参数无效
  */
static uint8_t es8388_is_input_valid(ES8388_Input_t input)
{
    return ((input == ES8388_INPUT_NONE) ||
            (input == ES8388_INPUT_BOARD_MIC) ||
            (input == ES8388_INPUT_HEADSET_MIC)) ? 1U : 0U;
}

/**
  * @brief  检查输出枚举是否有效
  * @param  output 输出枚举
  * @retval 1 参数有效，0 参数无效
  */
static uint8_t es8388_is_output_valid(ES8388_Output_t output)
{
    return ((output == ES8388_OUTPUT_NONE) ||
            (output == ES8388_OUTPUT_HEADPHONE) ||
            (output == ES8388_OUTPUT_SPEAKER)) ? 1U : 0U;
}

/**
  * @brief  在错误时立即关闭 HT6872 并清除软件路由状态
  * @param  无
  * @retval 无
  */
static void es8388_fail_safe(void)
{
    ES8388_AMP_DISABLE();
    ES8388_Muted = 1U;
    ES8388_CurrentInput = ES8388_INPUT_NONE;
    ES8388_CurrentOutput = ES8388_OUTPUT_NONE;
}

/**
  * @brief  配置已验证的 16 位 Philips I2S 公共数字通路
  * @param  无
  * @retval ES8388_OK 配置成功，其他值配置失败
  */
static ES8388_Status_t es8388_configure_common_path(void)
{
    static const uint8_t values[][2] =
    {
        {ES8388_REG_CONTROL2,     0x50U},
        {ES8388_REG_CHIPPOWER,    0x00U},
        {ES8388_REG_MASTERMODE,   0x00U},
        {ES8388_REG_ADCPOWER,     ES8388_ADC_POWER_OFF},
        {ES8388_REG_DACPOWER,     ES8388_DAC_POWER_OFF},
        {ES8388_REG_CONTROL1,     0x06U},
        {ES8388_REG_DACCONTROL3,  0x04U},
        {ES8388_REG_DACCONTROL1,  0x18U},
        {ES8388_REG_DACCONTROL2,  0x02U},
        {ES8388_REG_DACCONTROL4,  0x00U},
        {ES8388_REG_DACCONTROL5,  0x00U},
        {ES8388_REG_DACCONTROL16, 0x00U},
        {ES8388_REG_DACCONTROL17, 0x90U},
        {ES8388_REG_DACCONTROL20, 0x90U},
        {ES8388_REG_DACCONTROL21, 0x80U},
        {ES8388_REG_DACCONTROL23, 0x00U},
        {ES8388_REG_ADCCONTROL1,  0x77U},
        {ES8388_REG_DACCONTROL24, ES8388_HEADPHONE_VOLUME_DEFAULT},
        {ES8388_REG_DACCONTROL25, ES8388_HEADPHONE_VOLUME_DEFAULT},
        {ES8388_REG_DACCONTROL3,  ES8388_DAC_MUTED}
    };

    return es8388_write_values(values,
        (uint32_t)(sizeof(values) / sizeof(values[0])));
}

/**
  * @brief  关闭 ADC、DAC 和所有模拟输出，保持控制接口可用
  * @param  无
  * @retval ES8388_OK 关闭成功，其他值关闭失败
  */
static ES8388_Status_t es8388_enter_idle(void)
{
    static const uint8_t values[][2] =
    {
        {ES8388_REG_DACCONTROL3, ES8388_DAC_MUTED},
        {ES8388_REG_DACPOWER,    ES8388_DAC_POWER_OFF},
        {ES8388_REG_ADCPOWER,    ES8388_ADC_POWER_OFF},
        {ES8388_REG_CHIPPOWER,   0x00U},
        {ES8388_REG_CONTROL1,    0x06U}
    };
    ES8388_Status_t status;

    ES8388_AMP_DISABLE();
    status = es8388_write_values(values,
        (uint32_t)(sizeof(values) / sizeof(values[0])));
    if (status == ES8388_OK)
    {
        ES8388_CurrentInput = ES8388_INPUT_NONE;
        ES8388_CurrentOutput = ES8388_OUTPUT_NONE;
        ES8388_Muted = 1U;
    }
    return status;
}

/* ==================== API 函数实现 ==================== */

/**
  * @brief  初始化 ES8388，初始化完成后输入和输出均保持关闭
  * @param  无
  * @retval ES8388_OK 初始化成功，其他值初始化失败
  */
ES8388_Status_t ES8388_Init(void)
{
    ES8388_Status_t status;

    ES8388_AMP_DISABLE();
    ES8388_Initialized = 0U;
    ES8388_Muted = 1U;
    ES8388_HeadphoneVolume = ES8388_HEADPHONE_VOLUME_DEFAULT;
    ES8388_SpeakerVolume = ES8388_SPEAKER_VOLUME_DEFAULT;
    ES8388_CurrentInput = ES8388_INPUT_NONE;
    ES8388_CurrentOutput = ES8388_OUTPUT_NONE;

    if (ES8388_WriteRegister(ES8388_REG_CONTROL1, 0x80U) != ES8388_OK)
    {
        return ES8388_ERROR_I2C;
    }
    HAL_Delay(20U);
    if (ES8388_WriteRegister(ES8388_REG_CONTROL1, 0x00U) != ES8388_OK)
    {
        return ES8388_ERROR_I2C;
    }
    HAL_Delay(20U);

    status = es8388_configure_common_path();
    if (status != ES8388_OK)
    {
        es8388_fail_safe();
        return status;
    }
    ES8388_Initialized = 1U;
    status = es8388_enter_idle();
    if (status != ES8388_OK)
    {
        ES8388_Initialized = 0U;
        es8388_fail_safe();
    }
    return status;
}

/**
  * @brief  静音、关闭音频通路并复位 ES8388
  * @param  无
  * @retval ES8388_OK 复位成功，其他值复位失败
  */
ES8388_Status_t ES8388_DeInit(void)
{
    static const uint8_t values[][2] =
    {
        {ES8388_REG_DACCONTROL3, ES8388_DAC_MUTED},
        {ES8388_REG_DACPOWER,    ES8388_DAC_POWER_OFF},
        {ES8388_REG_ADCPOWER,    ES8388_ADC_POWER_OFF},
        {ES8388_REG_CONTROL1,    0x80U}
    };
    ES8388_Status_t status;

    if (ES8388_Initialized == 0U)
    {
        return ES8388_ERROR_NOT_INITIALIZED;
    }
    ES8388_AMP_DISABLE();
    status = es8388_write_values(values,
        (uint32_t)(sizeof(values) / sizeof(values[0])));
    if (status == ES8388_OK)
    {
        HAL_Delay(20U);
        ES8388_Initialized = 0U;
    }
    es8388_fail_safe();
    return status;
}

/**
  * @brief  选择录音输入，并在打开 ADC 前关闭 DAC 和 HT6872
  * @param  input 板载麦克风、耳机麦克风或关闭输入
  * @retval ES8388_OK 切换成功，其他值切换失败
  */
ES8388_Status_t ES8388_SetInput(ES8388_Input_t input)
{
    static const uint8_t input_values[][2] =
    {
        {ES8388_REG_DACCONTROL3, ES8388_DAC_MUTED},
        {ES8388_REG_DACPOWER,    ES8388_DAC_POWER_OFF},
        {ES8388_REG_CHIPPOWER,   0x55U},
        {ES8388_REG_CONTROL1,    0x05U},
        {ES8388_REG_ADCCONTROL4, 0x0CU},
        {ES8388_REG_ADCCONTROL5, 0x02U},
        {ES8388_REG_ADCCONTROL8, 0x00U},
        {ES8388_REG_ADCCONTROL9, 0x00U},
        {ES8388_REG_ADCPOWER,    0x00U}
    };
    ES8388_Status_t status;
    uint8_t input_select;

    if (ES8388_Initialized == 0U)
    {
        return ES8388_ERROR_NOT_INITIALIZED;
    }
    if (es8388_is_input_valid(input) == 0U)
    {
        return ES8388_ERROR_ARGUMENT;
    }
    if (input == ES8388_INPUT_NONE)
    {
        return es8388_enter_idle();
    }

    ES8388_AMP_DISABLE();
    ES8388_CurrentOutput = ES8388_OUTPUT_NONE;
    ES8388_Muted = 1U;
    status = es8388_write_values(input_values,
        (uint32_t)(sizeof(input_values) / sizeof(input_values[0])));
    if (status != ES8388_OK)
    {
        es8388_fail_safe();
        return status;
    }

    input_select = (input == ES8388_INPUT_HEADSET_MIC) ? 0x50U : 0xF0U;
    if ((ES8388_WriteRegister(ES8388_REG_ADCCONTROL2,
                              input_select) != ES8388_OK) ||
        (ES8388_WriteRegister(ES8388_REG_ADCCONTROL3,
                              0x02U) != ES8388_OK))
    {
        es8388_fail_safe();
        return ES8388_ERROR_I2C;
    }
    ES8388_CurrentInput = input;
    return ES8388_OK;
}

/**
  * @brief  选择耳机或扬声器输出，并在打开 DAC 前关闭 ADC
  * @param  output 耳机、扬声器或关闭输出
  * @retval ES8388_OK 切换成功，其他值切换失败
  */
ES8388_Status_t ES8388_SetOutput(ES8388_Output_t output)
{
    static const uint8_t output_values[][2] =
    {
        {ES8388_REG_DACCONTROL3,  ES8388_DAC_MUTED},
        {ES8388_REG_DACPOWER,     ES8388_DAC_POWER_OFF},
        {ES8388_REG_ADCPOWER,     ES8388_ADC_POWER_OFF},
        {ES8388_REG_CHIPPOWER,    0xAAU},
        {ES8388_REG_CONTROL1,     0x06U},
        {ES8388_REG_DACCONTROL1,  0x18U},
        {ES8388_REG_DACCONTROL2,  0x02U},
        {ES8388_REG_DACCONTROL4,  0x00U},
        {ES8388_REG_DACCONTROL5,  0x00U},
        {ES8388_REG_DACCONTROL16, 0x00U},
        {ES8388_REG_DACCONTROL17, 0x90U},
        {ES8388_REG_DACCONTROL20, 0x90U},
        {ES8388_REG_DACCONTROL21, 0x80U},
        {ES8388_REG_DACCONTROL23, 0x00U}
    };
    ES8388_Status_t status;

    if (ES8388_Initialized == 0U)
    {
        return ES8388_ERROR_NOT_INITIALIZED;
    }
    if (es8388_is_output_valid(output) == 0U)
    {
        return ES8388_ERROR_ARGUMENT;
    }
    if (output == ES8388_OUTPUT_NONE)
    {
        return es8388_enter_idle();
    }

    ES8388_AMP_DISABLE();
    ES8388_CurrentInput = ES8388_INPUT_NONE;
    ES8388_CurrentOutput = ES8388_OUTPUT_NONE;
    ES8388_Muted = 1U;
    status = es8388_write_values(output_values,
        (uint32_t)(sizeof(output_values) / sizeof(output_values[0])));
    if (status != ES8388_OK)
    {
        es8388_fail_safe();
        return status;
    }

    if (output == ES8388_OUTPUT_HEADPHONE)
    {
        if ((ES8388_WriteRegister(ES8388_REG_DACCONTROL26, 0x00U) != ES8388_OK) ||
            (ES8388_WriteRegister(ES8388_REG_DACCONTROL27, 0x00U) != ES8388_OK) ||
            (ES8388_WriteRegister(ES8388_REG_DACCONTROL24,
                                  ES8388_HeadphoneVolume) != ES8388_OK) ||
            (ES8388_WriteRegister(ES8388_REG_DACCONTROL25,
                                  ES8388_HeadphoneVolume) != ES8388_OK) ||
            (ES8388_WriteRegister(ES8388_REG_DACPOWER,
                                  ES8388_DAC_POWER_HEADPHONE) != ES8388_OK))
        {
            es8388_fail_safe();
            return ES8388_ERROR_I2C;
        }
    }
    else
    {
        if ((ES8388_WriteRegister(ES8388_REG_DACCONTROL24, 0x00U) != ES8388_OK) ||
            (ES8388_WriteRegister(ES8388_REG_DACCONTROL25, 0x00U) != ES8388_OK) ||
            (ES8388_WriteRegister(ES8388_REG_DACCONTROL26,
                                  ES8388_SpeakerVolume) != ES8388_OK) ||
            (ES8388_WriteRegister(ES8388_REG_DACCONTROL27,
                                  ES8388_SpeakerVolume) != ES8388_OK) ||
            (ES8388_WriteRegister(ES8388_REG_DACPOWER,
                                  ES8388_DAC_POWER_SPEAKER) != ES8388_OK))
        {
            es8388_fail_safe();
            return ES8388_ERROR_I2C;
        }
    }

    ES8388_CurrentOutput = output;
    return ES8388_SetMute(0U);
}

/**
  * @brief  设置左右 ADC 的模拟增益
  * @param  gain_db 增益，范围 0~24 dB，步进 3 dB
  * @retval ES8388_OK 设置成功，其他值设置失败
  */
ES8388_Status_t ES8388_SetInputGain(uint8_t gain_db)
{
    uint8_t gain_step;

    if (ES8388_Initialized == 0U)
    {
        return ES8388_ERROR_NOT_INITIALIZED;
    }
    if ((gain_db > 24U) || ((gain_db % 3U) != 0U))
    {
        return ES8388_ERROR_ARGUMENT;
    }
    gain_step = gain_db / 3U;
    return ES8388_WriteRegister(ES8388_REG_ADCCONTROL1,
                                (uint8_t)((gain_step << 4U) | gain_step));
}

/**
  * @brief  设置指定模拟输出的左右声道音量
  * @param  output 耳机或扬声器输出
  * @param  volume 寄存器音量，范围 0x00~0x1E，数值越大音量越高
  * @retval ES8388_OK 设置成功，其他值设置失败
  */
ES8388_Status_t ES8388_SetOutputVolume(ES8388_Output_t output,
                                       uint8_t volume)
{
    uint8_t left_register;
    uint8_t right_register;

    if (ES8388_Initialized == 0U)
    {
        return ES8388_ERROR_NOT_INITIALIZED;
    }
    if (((output != ES8388_OUTPUT_HEADPHONE) &&
         (output != ES8388_OUTPUT_SPEAKER)) ||
        (volume > ES8388_OUTPUT_VOLUME_MAX))
    {
        return ES8388_ERROR_ARGUMENT;
    }

    if (output == ES8388_OUTPUT_HEADPHONE)
    {
        ES8388_HeadphoneVolume = volume;
        left_register = ES8388_REG_DACCONTROL24;
        right_register = ES8388_REG_DACCONTROL25;
    }
    else
    {
        ES8388_SpeakerVolume = volume;
        left_register = ES8388_REG_DACCONTROL26;
        right_register = ES8388_REG_DACCONTROL27;
    }
    if ((ES8388_WriteRegister(left_register, volume) != ES8388_OK) ||
        (ES8388_WriteRegister(right_register, volume) != ES8388_OK))
    {
        return ES8388_ERROR_I2C;
    }
    return ES8388_OK;
}

/**
  * @brief  设置当前播放输出的静音状态
  * @param  muted 非零值静音，0 取消静音
  * @retval ES8388_OK 设置成功，其他值设置失败
  */
ES8388_Status_t ES8388_SetMute(uint8_t muted)
{
    if (ES8388_Initialized == 0U)
    {
        return ES8388_ERROR_NOT_INITIALIZED;
    }
    if ((muted == 0U) && (ES8388_CurrentOutput == ES8388_OUTPUT_NONE))
    {
        return ES8388_ERROR_ARGUMENT;
    }
    if (muted != 0U)
    {
        ES8388_AMP_DISABLE();
        ES8388_Muted = 1U;
        return ES8388_WriteRegister(ES8388_REG_DACCONTROL3,
                                    ES8388_DAC_MUTED);
    }

    ES8388_AMP_DISABLE();
    if (ES8388_CurrentOutput == ES8388_OUTPUT_SPEAKER)
    {
        HAL_Delay(10U);
        ES8388_AMP_ENABLE();
        HAL_Delay(10U);
    }
    if (ES8388_WriteRegister(ES8388_REG_DACCONTROL3,
                             ES8388_DAC_UNMUTED) != ES8388_OK)
    {
        ES8388_AMP_DISABLE();
        ES8388_Muted = 1U;
        return ES8388_ERROR_I2C;
    }
    ES8388_Muted = 0U;
    return ES8388_OK;
}

/**
  * @brief  通过 I2S 阻塞发送双声道 16 位 PCM 字
  * @param  samples PCM 数据缓冲区，排列为左、右、左、右
  * @param  word_count 16 位 PCM 字数，必须为偶数
  * @param  timeout_ms 每个分块的 HAL 超时时间
  * @retval ES8388_OK 发送成功，其他值发送失败
  */
ES8388_Status_t ES8388_Transmit(const int16_t *samples, uint32_t word_count,
                                uint32_t timeout_ms)
{
    if (ES8388_Initialized == 0U)
    {
        return ES8388_ERROR_NOT_INITIALIZED;
    }
    if ((samples == NULL) || (word_count == 0U) ||
        ((word_count & 1U) != 0U) ||
        (ES8388_CurrentOutput == ES8388_OUTPUT_NONE) ||
        (ES8388_Muted != 0U))
    {
        return ES8388_ERROR_ARGUMENT;
    }

    while (word_count != 0U)
    {
        uint16_t chunk = (word_count > 0xFFFEU) ?
                         0xFFFEU : (uint16_t)word_count;

        if (HAL_I2S_Transmit(&ES8388_I2S_HANDLE, (uint16_t *)samples,
                             chunk, timeout_ms) != HAL_OK)
        {
            ES8388_AMP_DISABLE();
            (void)ES8388_WriteRegister(ES8388_REG_DACCONTROL3,
                                       ES8388_DAC_MUTED);
            ES8388_Muted = 1U;
            return ES8388_ERROR_I2S;
        }
        samples += chunk;
        word_count -= chunk;
    }
    return ES8388_OK;
}

/**
  * @brief  通过 I2S2 全双工接口阻塞接收双声道 16 位 PCM 字
  * @param  samples PCM 接收缓冲区
  * @param  word_count 16 位 PCM 字数，必须为偶数
  * @param  timeout_ms 每个分块的 HAL 超时时间
  * @retval ES8388_OK 接收成功，其他值接收失败
  */
ES8388_Status_t ES8388_Receive(int16_t *samples, uint32_t word_count,
                               uint32_t timeout_ms)
{
    if (ES8388_Initialized == 0U)
    {
        return ES8388_ERROR_NOT_INITIALIZED;
    }
    if ((samples == NULL) || (word_count == 0U) ||
        ((word_count & 1U) != 0U) ||
        (ES8388_CurrentInput == ES8388_INPUT_NONE))
    {
        return ES8388_ERROR_ARGUMENT;
    }

    while (word_count != 0U)
    {
        uint16_t chunk = (word_count > ES8388_AUDIO_CHUNK_WORDS) ?
                         ES8388_AUDIO_CHUNK_WORDS :
                         (uint16_t)word_count;

        if (HAL_I2SEx_TransmitReceive(&ES8388_I2S_HANDLE, ES8388_Silence,
                                      (uint16_t *)samples, chunk,
                                      timeout_ms) != HAL_OK)
        {
            return ES8388_ERROR_I2S;
        }
        samples += chunk;
        word_count -= chunk;
    }
    return ES8388_OK;
}

/**
  * @brief  播放数组或动态内存中的 PCM，结束后关闭输出
  * @param  samples PCM 数据缓冲区
  * @param  word_count 16 位 PCM 字数
  * @param  output 耳机或扬声器输出
  * @param  timeout_ms 每个 I2S 分块的 HAL 超时时间
  * @retval ES8388_OK 播放成功，其他值播放失败
  */
ES8388_Status_t ES8388_PlayBuffer(const int16_t *samples,
                                  uint32_t word_count,
                                  ES8388_Output_t output,
                                  uint32_t timeout_ms)
{
    ES8388_Status_t status;
    ES8388_Status_t stop_status;

    if ((output != ES8388_OUTPUT_HEADPHONE) &&
        (output != ES8388_OUTPUT_SPEAKER))
    {
        return ES8388_ERROR_ARGUMENT;
    }
    status = ES8388_SetOutput(output);
    if (status != ES8388_OK)
    {
        return status;
    }
    status = ES8388_Transmit(samples, word_count, timeout_ms);
    stop_status = ES8388_SetOutput(ES8388_OUTPUT_NONE);
    return (status != ES8388_OK) ? status : stop_status;
}

/**
  * @brief  通过用户回调连续取得 PCM 并播放，结束后关闭输出
  * @param  read_callback PCM 读取或解码回调
  * @param  context 回调使用的用户数据
  * @param  work_buffer 用户提供的临时 PCM 缓冲区
  * @param  work_words 临时缓冲区可容纳的 16 位字数，必须为偶数
  * @param  output 耳机或扬声器输出
  * @param  timeout_ms 每个 I2S 分块的 HAL 超时时间
  * @retval ES8388_OK 播放完成，其他值播放失败
  */
ES8388_Status_t ES8388_PlayStream(ES8388_ReadCallback_t read_callback,
                                  void *context, int16_t *work_buffer,
                                  uint32_t work_words,
                                  ES8388_Output_t output,
                                  uint32_t timeout_ms)
{
    ES8388_Status_t status;
    ES8388_Status_t stop_status;

    if ((read_callback == NULL) || (work_buffer == NULL) ||
        (work_words == 0U) || ((work_words & 1U) != 0U) ||
        ((output != ES8388_OUTPUT_HEADPHONE) &&
         (output != ES8388_OUTPUT_SPEAKER)))
    {
        return ES8388_ERROR_ARGUMENT;
    }

    status = ES8388_SetOutput(output);
    while (status == ES8388_OK)
    {
        int32_t words_read = read_callback(context, work_buffer, work_words);

        if (words_read == 0)
        {
            break;
        }
        if ((words_read < 0) || ((uint32_t)words_read > work_words) ||
            ((((uint32_t)words_read) & 1U) != 0U))
        {
            status = ES8388_ERROR_SOURCE;
            break;
        }
        status = ES8388_Transmit(work_buffer, (uint32_t)words_read,
                                 timeout_ms);
    }

    stop_status = ES8388_SetOutput(ES8388_OUTPUT_NONE);
    return (status != ES8388_OK) ? status : stop_status;
}

/**
  * @brief  从一路输入录制 PCM 到用户缓冲区，结束后关闭输入
  * @param  samples PCM 接收缓冲区
  * @param  word_count 16 位 PCM 字数
  * @param  input 板载麦克风或耳机麦克风
  * @param  timeout_ms 每个 I2S 分块的 HAL 超时时间
  * @retval ES8388_OK 录制成功，其他值录制失败
  */
ES8388_Status_t ES8388_RecordBuffer(int16_t *samples, uint32_t word_count,
                                    ES8388_Input_t input,
                                    uint32_t timeout_ms)
{
    ES8388_Status_t status;
    ES8388_Status_t stop_status;

    if ((input != ES8388_INPUT_BOARD_MIC) &&
        (input != ES8388_INPUT_HEADSET_MIC))
    {
        return ES8388_ERROR_ARGUMENT;
    }
    status = ES8388_SetInput(input);
    if (status != ES8388_OK)
    {
        return status;
    }
    status = ES8388_Receive(samples, word_count, timeout_ms);
    stop_status = ES8388_SetInput(ES8388_INPUT_NONE);
    return (status != ES8388_OK) ? status : stop_status;
}

#endif /* DEVICE_ES8388 */
