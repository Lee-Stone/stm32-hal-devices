#include "dev_wifi.h"

#if DEVICE_WIFI

#include "errno.h"
#include "ring_buffer.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

static int WIFI_WaitAck(const char *ack, unsigned int timeout);
static int WIFI_Test(void);
static int WIFI_Reset(void);
static int WIFI_SetEcho(int status);
static int WIFI_SetWorkMode(int mode);
static int WIFI_SetReceiveMode(unsigned char mode);
static int WIFI_Transmit(const unsigned char *data, unsigned int length);
static void WIFI_AnalyzeSocketData(unsigned char data);

static volatile unsigned char wifiUart3TxComplete = 0;
static unsigned char wifiUart3RxData = 0;
static unsigned char wifiConnected = 0;
static unsigned char wifiNetworkConnected = 0;
static unsigned int wifiNetworkPort = 0;
static unsigned int wifiNetworkId = 0;
static RingBuffer *wifiResponseBuffer = NULL;
static RingBuffer *wifiNetworkBuffer = NULL;

/**
 * @brief Initialize the WiFi module and start UART3 reception.
 * @param None
 * @retval ESUCCESS on success, or a negative error code.
 */
int WIFI_Init(void)
{
    int ret;

    if(wifiResponseBuffer == NULL)
    {
        wifiResponseBuffer = RingBufferNew(100U);
        if(wifiResponseBuffer == NULL)
        {
            return -ENOMEM;
        }
    }

    if(HAL_UART_Receive_IT(&huart3, &wifiUart3RxData, 1U) != HAL_OK)
    {
        return -EIO;
    }

    ret = WIFI_Test();
    if(ret != ESUCCESS) return ret;

    ret = WIFI_Reset();
    if(ret != ESUCCESS) return ret;

    ret = WIFI_SetEcho(0);
    if(ret != ESUCCESS) return ret;

    ret = WIFI_SetWorkMode(AP_STA);
    if(ret != ESUCCESS) return ret;

    return WIFI_SetReceiveMode(1U);
}

/**
 * @brief Connect the WiFi module to a wireless network.
 * @param name WiFi network name.
 * @param password WiFi network password.
 * @retval ESUCCESS on success, or a negative error code.
 */
int WIFI_Connect(const char *name, const char *password)
{
    char command[100];
    unsigned int length;
    int ret;

    if(name == NULL || password == NULL || wifiResponseBuffer == NULL)
    {
        return -EINVAL;
    }

    snprintf(command, sizeof(command), "AT+WJAP=%s,%s\r\n", name, password);
    length = (unsigned int)strlen(command);
    wifiResponseBuffer->Clear(wifiResponseBuffer);

    ret = WIFI_Transmit((const unsigned char *)command, length);
    if(ret != ESUCCESS) return ret;

    ret = WIFI_WaitAck("WIFI_GOT_IP\r\n", 5000U);
    if(ret == ESUCCESS) wifiConnected = 1U;
    return ret;
}

/**
 * @brief Disconnect the WiFi module from the wireless network.
 * @param None
 * @retval ESUCCESS on success, or a negative error code.
 */
int WIFI_Disconnect(void)
{
    const char command[] = "AT+WDISCONNECT\r\n";
    int ret;

    if(wifiResponseBuffer == NULL) return -ENODEV;
    wifiResponseBuffer->Clear(wifiResponseBuffer);

    ret = WIFI_Transmit((const unsigned char *)command, sizeof(command) - 1U);
    if(ret != ESUCCESS) return ret;

    ret = WIFI_WaitAck("OK\r\n", 100U);
    if(ret == ESUCCESS) wifiConnected = 0U;
    return ret;
}

/**
 * @brief Connect one TCP or UDP socket.
 * @param type Socket type.
 * @param ip Remote server IP address.
 * @param port Remote server port.
 * @retval ESUCCESS on success, or a negative error code.
 */
int WIFI_NetConnect(unsigned char type, const char *ip, unsigned int port)
{
    char command[128];
    unsigned int length;
    int ret;

    if(!wifiConnected || ip == NULL || wifiResponseBuffer == NULL)
    {
        return -EINVAL;
    }
    if(wifiNetworkConnected) return -EALREADY;

    wifiNetworkId = 1U;
    snprintf(command, sizeof(command), "AT+SOCKET=%u,%s,%u,0,%u\r\n",
             type, ip, port, wifiNetworkId);
    length = (unsigned int)strlen(command);
    wifiResponseBuffer->Clear(wifiResponseBuffer);

    ret = WIFI_Transmit((const unsigned char *)command, length);
    if(ret != ESUCCESS) return ret;

    ret = WIFI_WaitAck("OK\r\n", 500U);
    if(ret != ESUCCESS) return ret;

    wifiNetworkBuffer = RingBufferNew(128U);
    if(wifiNetworkBuffer == NULL) return -ENOMEM;

    wifiNetworkPort = port;
    wifiNetworkConnected = 1U;
    return ESUCCESS;
}

/**
 * @brief Disconnect the current socket.
 * @param port Remote server port.
 * @retval ESUCCESS on success, or a negative error code.
 */
int WIFI_NetDisconnect(unsigned int port)
{
    char command[64];
    unsigned int length;
    int ret;

    if(!wifiNetworkConnected || port != wifiNetworkPort)
    {
        return -ENODEV;
    }

    snprintf(command, sizeof(command), "AT+SOCKETDEL=%u\r\n", wifiNetworkId);
    length = (unsigned int)strlen(command);
    wifiResponseBuffer->Clear(wifiResponseBuffer);

    ret = WIFI_Transmit((const unsigned char *)command, length);
    if(ret != ESUCCESS) return ret;

    ret = WIFI_WaitAck("OK\r\n", 500U);
    if(ret != ESUCCESS) return ret;

    wifiNetworkBuffer->Free(wifiNetworkBuffer);
    wifiNetworkBuffer = NULL;
    wifiNetworkConnected = 0U;
    wifiNetworkPort = 0U;
    wifiNetworkId = 0U;
    return ESUCCESS;
}

/**
 * @brief Send data through the current socket.
 * @param port Remote server port.
 * @param data Data buffer.
 * @param length Data length in bytes.
 * @retval Number of bytes sent, or a negative error code.
 */
int WIFI_Write(unsigned int port, const unsigned char *data, unsigned int length)
{
    char command[48];
    unsigned int commandLength;
    int ret;

    if(!wifiNetworkConnected || port != wifiNetworkPort || data == NULL || length == 0U)
    {
        return -EINVAL;
    }

    snprintf(command, sizeof(command), "AT+SOCKETSEND=%u,%u\r\n",
             wifiNetworkId, length);
    commandLength = (unsigned int)strlen(command);
    wifiResponseBuffer->Clear(wifiResponseBuffer);

    ret = WIFI_Transmit((const unsigned char *)command, commandLength);
    if(ret != ESUCCESS) return ret;

    ret = WIFI_WaitAck(">", 100U);
    if(ret != ESUCCESS) return ret;

    ret = WIFI_Transmit(data, length);
    if(ret != ESUCCESS) return ret;

    ret = WIFI_WaitAck("OK\r\n", 100U);
    if(ret != ESUCCESS) return ret;
    return (int)length;
}

/**
 * @brief Read data from the current socket receive buffer.
 * @param port Remote server port.
 * @param data Destination buffer.
 * @param length Destination buffer length.
 * @retval Number of bytes read, or a negative error code.
 */
int WIFI_Read(unsigned int port, unsigned char *data, unsigned int length)
{
    if(!wifiNetworkConnected || port != wifiNetworkPort || data == NULL || length == 0U)
    {
        return -EINVAL;
    }
    return wifiNetworkBuffer->Read(wifiNetworkBuffer, data, length);
}

/**
 * @brief UART receive complete callback for the WiFi module.
 * @param huart UART handle that triggered the callback.
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART3)
    {
        HAL_UART_Receive_IT(&huart3, &wifiUart3RxData, 1U);
        if(wifiResponseBuffer != NULL)
        {
            wifiResponseBuffer->Write(wifiResponseBuffer, &wifiUart3RxData, 1U);
        }
        WIFI_AnalyzeSocketData(wifiUart3RxData);
    }
}

/**
 * @brief UART transmit complete callback for the WiFi module.
 * @param huart UART handle that triggered the callback.
 * @retval None
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART3)
    {
        wifiUart3TxComplete = 1U;
    }
}

static int WIFI_Transmit(const unsigned char *data, unsigned int length)
{
    if(data == NULL || length == 0U || length > UINT16_MAX)
    {
        return -EINVAL;
    }

    while(HAL_UART_GetState(&huart3) == HAL_UART_STATE_BUSY_TX) {}

    wifiUart3TxComplete = 0U;
    if(HAL_UART_Transmit_IT(&huart3, (unsigned char *)data,
                            (uint16_t)length) != HAL_OK)
    {
        return -EIO;
    }

    while(wifiUart3TxComplete == 0U) {}
    return ESUCCESS;
}

static int WIFI_WaitAck(const char *ack, unsigned int timeout)
{
    char response[128] = {0};
    unsigned int index = 0U;

    while(timeout > 0U)
    {
        if(wifiResponseBuffer->Read(wifiResponseBuffer,
                                    (unsigned char *)&response[index], 1U) == 1)
        {
            response[index + 1U] = '\0';
            if(strstr(response, ack) != NULL) return ESUCCESS;
            if(strstr(response, "ERR") != NULL || strstr(response, "Unknown cmd") != NULL)
            {
                return -EIO;
            }
            index = (index + 1U) % (sizeof(response) - 1U);
        }
        timeout--;
        HAL_Delay(1U);
    }
    return -ETIMEDOUT;
}

static int WIFI_Test(void)
{
    const char command[] = "AT\r\n";
    wifiResponseBuffer->Clear(wifiResponseBuffer);
    if(WIFI_Transmit((const unsigned char *)command, sizeof(command) - 1U) != ESUCCESS)
    {
        return -EIO;
    }
    return WIFI_WaitAck("OK\r\n", 100U);
}

static int WIFI_Reset(void)
{
    const char command[] = "AT+RST\r\n";
    wifiResponseBuffer->Clear(wifiResponseBuffer);
    if(WIFI_Transmit((const unsigned char *)command, sizeof(command) - 1U) != ESUCCESS)
    {
        return -EIO;
    }
    return WIFI_WaitAck("ready\r\n", 2000U);
}

static int WIFI_SetEcho(int status)
{
    char command[12];
    snprintf(command, sizeof(command), "ATE%d\r\n", status);
    wifiResponseBuffer->Clear(wifiResponseBuffer);
    if(WIFI_Transmit((const unsigned char *)command, (unsigned int)strlen(command)) != ESUCCESS)
    {
        return -EIO;
    }
    return WIFI_WaitAck("OK\r\n", 100U);
}

static int WIFI_SetWorkMode(int mode)
{
    char command[32];
    snprintf(command, sizeof(command), "AT+WMODE=%d,0\r\n", mode);
    wifiResponseBuffer->Clear(wifiResponseBuffer);
    if(WIFI_Transmit((const unsigned char *)command, (unsigned int)strlen(command)) != ESUCCESS)
    {
        return -EIO;
    }
    return WIFI_WaitAck("OK\r\n", 100U);
}

static int WIFI_SetReceiveMode(unsigned char mode)
{
    char command[32];
    snprintf(command, sizeof(command), "AT+SOCKETRECVCFG=%u\r\n", mode);
    wifiResponseBuffer->Clear(wifiResponseBuffer);
    if(WIFI_Transmit((const unsigned char *)command, (unsigned int)strlen(command)) != ESUCCESS)
    {
        return -EIO;
    }
    return WIFI_WaitAck("OK\r\n", 100U);
}

static void WIFI_AnalyzeSocketData(unsigned char data)
{
    static unsigned char buffer[256];
    static unsigned int index = 0U;
    static unsigned char step = 0U;
    static unsigned int connectionId = 0U;
    static unsigned int receiveLength = 0U;

    switch(step)
    {
        case 0U:
            if(data != '+') return;
            index = 0U;
            buffer[index++] = data;
            buffer[index] = '\0';
            step = 1U;
            break;
        case 1U:
            buffer[index++] = data;
            buffer[index] = '\0';
            if(strstr((char *)buffer, "+EVENT:SocketDown,") != NULL)
            {
                index = 0U;
                step = 2U;
            }
            else if(index >= 18U)
            {
                index = 0U;
                step = 0U;
            }
            break;
        case 2U:
            if(data == ',') step = 3U;
            else connectionId = connectionId * 10U + data - '0';
            break;
        case 3U:
            if(data == ',') step = 4U;
            else receiveLength = receiveLength * 10U + data - '0';
            break;
        case 4U:
            if(connectionId == wifiNetworkId && wifiNetworkBuffer != NULL)
            {
                wifiNetworkBuffer->Write(wifiNetworkBuffer, &data, 1U);
            }
            index++;
            if(index >= receiveLength)
            {
                index = 0U;
                connectionId = 0U;
                receiveLength = 0U;
                step = 0U;
            }
            break;
        default:
            index = 0U;
            connectionId = 0U;
            receiveLength = 0U;
            step = 0U;
            break;
    }
}

#endif /* DEVICE_WIFI */
