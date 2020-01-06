/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK设备硬件适配接口声明
 * Create: 2018-12-01
 */
#ifndef _HILINK_MCU_H
#define _HILINK_MCU_H

#include "hilink_common.h"

/* 提供开发者调用的接口 */

/*
 * 系统入口函数, 开发者通过调用本函数集成MCU SDK主框架.
 * 开发者需要根据设备实际情况调用延时接口在主循环中的添加合适延时.
 */
void HiLinkMcuMain(void);

/*
 * 通过串口接收单字节数据, 参数data表示接收的单字节数据.
 * 开发者需要在串口接收中断处理函数中调用本函数.
 */
void HiLinkUartRcvOneByte(unsigned char data);

/* 开发者如果需要重启wifi模组, 可在任何地方调用本函数 */
void HiLinkModuleReboot(void);

/* 开发者如果需要重置wifi模组, 可在任何地方调用本函数 */
void HiLinkModuleReset(void);

/* 开发者调用本函数获取模组当前的联网状态 */
ModuleNetStatus HiLinkGetLocalNetStatus(void);

/*
 * 开发者调用本接口获取模组的WiFi信息, 参数表示WiFi热点的名称.
 * 若需要获取当前设备连接的WiFi热点的信息, 参数ssid请传入NULL (多数情况下都传入NULL).
 * 开发者需要实现hilink_mcu.c中的HiLinkNotifyModuleInfo函数来处理接收到的模组信息.
 */
void HiLinkGetModuleInfo(char* ssid);

/*
 * 开发者调用本接口获取模组的UTC时间.
 * 开发者需要实现hilink_mcu.c中的HiLinkNotifyUtcTime函数来处理接收到的UTC时间.
 */
void HiLinkGetUtcTime(void);

/*
 * 开发者按属性维度上报本地服务状态时, 可以调用本接口上报单个服务属性值.
 * 参数val是上报的整型或布尔类型属性值, 若要上报字符类型数据, 则该参数填0.
 * 参数str是上报的字符类型属性值, 若要上报整型或布尔类型数据, 则该参数填NULL.
 */
int HiLinkUpdateKeyVal(unsigned short svcMapId, unsigned short keyMapId, int val, const char* str);

/*
 * 开发者需要按服务维度上报本地服务状态时, 需要调用本接口依次上报对应服务的所有属性值.
 * 参数val是上报的整型或布尔类型属性值, 若要上报字符类型数据, 则该参数填0.
 * 参数str是上报的字符类型属性值, 若要上报整型或布尔类型数据, 则该参数填NULL.
 * 参数rptFlag是上报时机的标记, 除最后一个上报属性外, rptFlag取值为REPORT_LATER表示稍后上报,
 * 对最后一个上报的属性, rptFlag取值为REPORT_NOW表示立即上报.
 */
int HiLinkUpdateSvcVal(unsigned short svcMapId, unsigned short keyMapId, int val, const char* str, char rptFlag);

/* 以下是需要开发者实现的接口 */

/* 开发者实现本接口获取并返回系统当前时间毫秒值(或时钟tick值) */
unsigned long long HiLinkGetSysCurTime(void);

/* 开发者实现本接口通过UART串口阻塞式发送一个字节的数据 */
void HiLinkUartSendOneByte(unsigned char data);

/*
 * 开发者实现本函数初始化所有服务实例的所有属性值.
 * 根据当前实际设备状态, 通过调用HiLinkUpdateKeyVal接口依次给g_msgTable中的value字段赋值.
 */
void HiLinkInitProfileValue(void);

/*
 * 如果用户调用了HiLinkGetModuleInfo接口获取wifi模组信息, 则需要实现本函数处理返回的模组信息.
 * 返回的模组信息有:模组MAC地址(mac), 硬件版本号(hardVer), 软件版本号(softVer), WiFi强度(rssi),
 * WiFi热点AP名称(apName)
 */
void HiLinkNotifyModuleInfo(const char* mac, const char* hardVer,
    const char* softVer, int rssi, const char* apName);

/*
 * 如果用户调用了HiLinkGetUtcTime接口获取wifi模组UTC时间, 则需要实现本函数处理返回的时间
 * 信息buf由7个字节组成，分别表示年(2字节)、月(1字节)、日(1字节)、时(1字节)、分(1字节)、秒(1字节)
 */
void HiLinkNotifyUtcTime(const unsigned char* buf, int len);

/*
 * MCU SDK内部的错误信息会通过本函数传递给设备.
 * 开发者可以选择实现本接口, 根据错误码errCode进行相应的处理.
 */
void HiLinkNotifyErrorInfo(unsigned int errCode);

/*
 * 模组通知MCU设备被云端删除注册.
 * 开发者可以实现本接口, 添加设备被云端删除注册时的设备侧的相应处理.
 */
void HiLinkDevRemoved(void);

/*
 * 开发者通过实现本函数将设备SN号返回录入.
 * 其中参数len表示SN的最大长度39字节, 出参sn表示录入的设备SN号.
 * 若未实现本接口或sn指针返回的字符串长度为0时, SDK将默认使用设备mac地址作为SN号.
 */
void HiLinkGetDeviceSn(unsigned int len, char* sn);

#endif /* _HILINK_MCU_H */
