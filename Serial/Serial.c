#include "Serial.h"

#if DEVICE_SERIAL

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/*
 * 内部实例注册表
 * Serial_Init() 将每个 Serial 实例的指针存入此表，
 * HAL_UART_RxCpltCallback 通过遍历该表找到触发中断的实例。
 * 最大注册数量由 config.h 中的 SERIAL_NUMS 控制。
 */
static Serial  *s_instances[SERIAL_NUMS];
static uint8_t  s_count = 0;

/* ================================================================
 *  Serial_Init
 * ================================================================ */
void Serial_Init(Serial *serial, UART_HandleTypeDef *huart)
{
    serial->huart   = huart;
    serial->rxByte  = 0;
    serial->rxData  = 0;
    serial->rxFlag  = 0;

    /* 将实例注册到内部表，以便中断回调能找到它 */
    if (s_count < SERIAL_NUMS)
    {
        s_instances[s_count++] = serial;
    }

    /* 启动首次中断接收，每次仅接收 1 字节 */
    HAL_UART_Receive_IT(huart, &serial->rxByte, 1);
}

/* ================================================================
 *  Serial_Printf
 * ================================================================ */
void Serial_Printf(Serial *serial, const char *fmt, ...)
{
    va_list args, args_copy;
    va_start(args, fmt);

    /* 第一次调用：传入 NULL 仅计算所需长度，不写入任何内容 */
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (len > 0)
    {
        /* 按实际长度在栈上分配缓冲区（VLA），避免固定缓冲区浪费或溢出 */
        char buf[len + 1];
        vsnprintf(buf, (size_t)(len + 1), fmt, args);
        HAL_UART_Transmit(serial->huart, (uint8_t *)buf, (uint16_t)len, HAL_MAX_DELAY);
    }

    va_end(args);
}

/* ================================================================
 *  Serial_Received
 * ================================================================ */
bool Serial_Received(Serial *serial)
{
    /* 直接返回标志位，非阻塞，不修改任何状态 */
    return serial->rxFlag != 0U;
}

/* ================================================================
 *  Serial_Read
 * ================================================================ */
unsigned char Serial_Read(Serial *serial)
{
    unsigned char data = serial->rxData;
    /* 清除标志位，使能下一次 Serial_Received() 的检测 */
    serial->rxFlag = 0;
    return data;
}

/* ================================================================
 *  HAL_UART_RxCpltCallback
 *
 *  覆盖 HAL 库的弱定义。每当硬件接收到 1 字节时触发：
 *    1. 在实例表中查找触发中断的 UART 对应的 Serial 实例
 *    2. 将接收到的字节写入 rxData
 *    3. 将 rxFlag 置 1，通知主循环有新数据
 *    4. 重启中断接收，等待下一字节
 *
 *  注意：若项目其他地方也定义了 HAL_UART_RxCpltCallback，
 *  请删除此函数并在自定义回调中手动调用本库的逻辑。
 * ================================================================ */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    for (uint8_t i = 0; i < s_count; i++)
    {
        if (s_instances[i]->huart->Instance == huart->Instance)
        {
            s_instances[i]->rxData = s_instances[i]->rxByte;
            s_instances[i]->rxFlag = 1;
            HAL_UART_Receive_IT(huart, &s_instances[i]->rxByte, 1);
            break;
        }
    }
}

#endif /* DEVICE_SERIAL */
