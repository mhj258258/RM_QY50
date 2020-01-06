/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK设备硬件适配接口
 * Create: 2018-12-01
 */
#include "hilink_mcu.h"
#include "hilink_device.h"
#include "systick.h"
#include "uart.h"

/* 开发者实现本接口获取并返回系统当前时间毫秒值(或时钟tick值) */
unsigned long long HiLinkGetSysCurTime(void)
{
    unsigned long long curTime = 0;

    /* 在此处添加实现代码 */
    /* 调用系统库函数获取系统时钟计数值 */
	//---------by tomi----------
		curTime = Get_tick();
	
    return curTime;
}

/* 开发者实现本接口通过UART串口阻塞式发送一个字节的数据 */
void HiLinkUartSendOneByte(unsigned char data)
{
    /* 在此处添加实现代码 */
    /*
     * 调用具体串口发送接口发送数据
     * 添加合适的发送延时
     */
		//---------by tomi----------
		usart_data_transmit(EVAL_COM1, data);//uart0
	
    return;
}

/*
 * 开发者实现本函数初始化所有服务实例的所有属性值.
 * 根据当前实际设备状态, 通过调用HiLinkUpdateKeyVal接口依次给g_msgTable中的value字段赋值.
 */
void HiLinkInitProfileValue(void)
{
    /* 在此处添加实现代码 */
    /*
     * 实现步骤示例(以智能灯为例):
     * 第1步: 实现读取GPIO的函数, 获得控制灯开关的GPIO端口值;
     * 第2步: 调用HiLinkUpdateKeyVal函数设置服务属性初始值。
     */
		//------------by tomi---------------
		//svc1：开关
		HiLinkUpdateKeyVal(1, 1, 0, NULL);//编号从1开始，1，1就该表了开关服务 on属性
		//svc2：亮度
		HiLinkUpdateKeyVal(2, 1, 100, NULL);
		//svc3：渐亮时长
		HiLinkUpdateKeyVal(3, 1, 1800, NULL);
		//svc4：渐暗时长
		HiLinkUpdateKeyVal(4, 1, 1800, NULL);
	
    return;
}

/*
 * 如果用户调用了HiLinkGetModuleInfo接口获取wifi模组信息, 则需要实现本函数处理返回的模组信息.
 * 返回的模组信息有:模组MAC地址(mac), 硬件版本号(hardVer), 软件版本号(softVer), WiFi强度(rssi),
 * WiFi热点AP名称(apName)
 */
void HiLinkNotifyModuleInfo(const char* mac, const char* hardVer,
    const char* softVer, int rssi, const char* apName)
{
    if ((mac == NULL) || (hardVer == NULL) || (softVer == NULL) || (apName == NULL)) {
        return;
    }

    /* 在此处添加实现代码, 使用获取的模组信息 */

    return;
}

/*
 * 如果用户调用了HiLinkGetUtcTime接口获取WiFi模组UTC时间, 则需要实现本函数处理返回的时间
 * 信息buf由7个字节组成，分别表示年(2字节)、月(1字节)、日(1字节)、时(1字节)、分(1字节)、秒(1字节)
 */
void HiLinkNotifyUtcTime(const unsigned char* buf, int len)
{
    if (len != UTC_TIME_BUF_LEN) {
        return;
    }

    /* 在此处添加实现代码, 使用获取的时间信息, 具体值获取参考如下 */
    /*
     * 年：(buf[0] << 8) + buf[1]
     * 月：buf[2]
     * 日：buf[3]
     * 时：buf[4]
     * 分：buf[5]
     * 秒：buf[6]
     */

}

/*
 * MCU SDK内部的错误信息会通过本函数传递给设备.
 * 开发者可以选择实现本接口, 根据错误码errCode进行相应的处理.
 */
void HiLinkNotifyErrorInfo(unsigned int errCode)
{
    /* 在此处添加实现代码, 打印传来的错误码errCode */

    return;
}

/*
 * 模组通知MCU设备被云端删除注册.
 * 开发者可以实现本接口, 添加设备被云端删除注册时的设备侧的相应处理.
 */
void HiLinkDevRemoved(void)
{
    /* 在此处添加设备被云端删除注册时的MCU处理逻辑 */

    return;
}

/*
 * 开发者通过实现本函数将设备SN号返回录入.
 * 其中参数len表示SN的最大长度39字节, 出参sn表示录入的设备SN号.
 * 若未实现本接口或sn指针返回的字符串长度为0时, SDK将默认使用设备mac地址作为SN号.
 */
void HiLinkGetDeviceSn(unsigned int len, char* sn)
{
    /* 在此处添加实现代码, 将sn赋值给*sn回传 */

    return;
}
