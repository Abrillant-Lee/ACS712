/**
 * @file    acs712.c
 * @brief   ACS712电流传感器驱动
 * @author  Abrillant Lee
 * @date    2023.11.6
 * @version v1.0
 */
#include <stdio.h>
#include <unistd.h>
#include "stdbool.h"

#include "ohos_init.h"
#include "cmsis_os2.h"
#include <string.h>
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "iot_adc.h"
#include "hi_adc.h"
#include "iot_uart.h"
#include "hi_uart.h"
#include "iot_watchdog.h"
#include "iot_errno.h"

/*
 * name  :  ACS712_Init
 * brief :  ACS712初始化
 * param :  void
 * return:  void
 */
void ACS712_Init(void)
{
    // 根据手册可知，GPIo_07:ADC3
    IoTGpioInit(IOT_IO_NAME_GPIO_7);
    /* 设置GPIO07的管脚复用关系为GPIO */
    IoSetFunc(IOT_IO_NAME_GPIO_7, IOT_IO_FUNC_GPIO_13_GPIO);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_7, IOT_GPIO_DIR_IN);
}

/*
 * name  :  Get_ACS712_Voltage
 * brief :  获取ACS712电压
 * param :  无
 * return:  电流值
 */
float Get_ACS712_Voltage(void)
{
    unsigned short data = 0; // 定义一个无符号短整型变量data并初始化为0
    bool flag = 0;           // 定义一个布尔型变量flag并初始化为0
    // 调用AdcRead函数读取ADC采样值，并将结果存储在data中
    // idx为ADC通道号，IOT_ADC_EQU_MODEL_8为ADC采样精度，IOT_ADC_CUR_BAIS_DEFAULT为ADC偏置电压，0xff为超时时间
    flag = AdcRead(IOT_ADC_CHANNEL_0, &data, IOT_ADC_EQU_MODEL_8, IOT_ADC_CUR_BAIS_DEFAULT, 0xff);
    // 判断ADC采样是否成功，若失败则输出错误信息
    if (flag != IOT_SUCCESS)
    {
        printf("hi_adc_read failed\n");
    }
    // 计算电压值并将结果存储在整型变量voltage中
    // data为ADC采样值，1.8为ADC参考电压，4为电路放大倍数，4096.0为ADC采样精度
    float voltage = (float)data * 1.8 * 8 / 4096.0;
    // 返回计算得到的电压值
    return voltage;
}
/*
 * name  :  Get_ACS712_Current
 * brief :  获取ACS712电流值
 * param :  无
 * return:  电流值
 */
float Get_ACS712_Current()
{
    // 调用Get_ACS712_Voltage函数获取电压值，并计算电流值
    float current = (Get_ACS712_Voltage() - 2.5) / 0.185;
    // 返回计算得到的电流值
    return current;
}

/*
 * name  :  UART_Config
 * brief :  uart0配置
 * param :  无
 * return:  无
 * 注意： 小熊派的printf（）函数已经可以使用，无需再配置uart0
 */
void UART_Config(void)
{
    // 初始化GPIO
    IoTGpioInit(IOT_IO_NAME_GPIO_3);
    IoTGpioInit(IOT_IO_NAME_GPIO_4);
    // 设置GPIO3的管脚复用关系为UART0_TX
    IoSetFunc(IOT_IO_NAME_GPIO_3, IOT_IO_FUNC_GPIO_3_UART0_TXD);
    // 设置GPIO4的管脚复用关系为UART0_RX
    IoSetFunc(IOT_IO_NAME_GPIO_4, IOT_IO_FUNC_GPIO_4_UART0_RXD);
    // 初始化UART配置
    IotUartAttribute uart_attr = {
        .baudRate = 115200,
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };
    uint32_t flag = IoTUartInit(HI_UART_IDX_0, &uart_attr);
    if (flag != IOT_SUCCESS)
    {
        printf("Init Uart0 Falied Error No : %d\n", flag);
        return;
    }
}

/*
 * name  :  adcTask
 * brief :  ACS712任务
 * param :  无
 * return:  无
 */
static void ACS712Task(void)
{
    ACS712_Init(); // 初始化ACS712
    while (1)      // 无限循环
    {
        float current = Get_ACS712_Current();               // 获取ACS712电流值
        float voltage = Get_ACS712_Voltage();               // 获取ACS712电压值
        printf("ACS712 Current is : %f mA\n", current);     // 输出电流值
        printf("ACS712 Voltage is : %f V\n", voltage * 10); // 输出电压值
        sleep(1);                                           // 睡眠1秒
    }
}

/**
 * @brief ACS712函数，创建一个新的线程来执行ACS712Task函数。
 *
 */
void ACS712(void)
{
    osThreadAttr_t attr;
    IoTWatchDogDisable();
    attr.name = "adcTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 5 * 1024; // 堆栈大小5*1024，stack size 5*1024
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)ACS712Task, NULL, &attr) == NULL)
    {
        printf("[ACS712Task] Failed to create ACS712Task!\n");
    }
}

APP_FEATURE_INIT(ACS712);
