/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK功能定义及控制接口声明
 * Create: 2018-12-01
 */
#ifndef _HILINK_DEVICE_H
#define _HILINK_DEVICE_H

#include "hilink_common.h"

#define DEV_PROFILE_VER     g_profileVer
#define DEV_PROFILE_DEVINF  g_devInfo
#define DEV_PROFILE_ENNAME  g_devEnName
#define DEV_PROFILE_ACVAL   g_ac
#define DEV_PROFILE_BIRSA   g_bi
#define DEV_PROFILE_SVCS    g_svcMap
#define DEV_PROFILE_KEYS    g_keyMap
#define DEV_PROFILE_VALS    g_msgTable

/* 服务映射ID定义 */
/* 注意, 服务映射ID宏定义的范围为: 0x40 ~ 0x7f 或 0x80 ~ 0x1FFF. */
#define SWITCH_SVC_MAP_ID  (0x40)
#define BRIGHTNESS_SVC_MAP_ID  (0x41)
#define WAKEUPTIME_SVC_MAP_ID  (0x42)
#define FADETIME_SVC_MAP_ID  (0x43)
#define COMMONEXECUTION1_SVC_MAP_ID  (0x44)

/* 属性映射ID定义 */
/* 注意, 属性映射ID宏定义的范围为: 0x40 ~ 0x7f 或 0x80 ~ 0x1FFF. */
#define SWITCH_SVC_ON_KEY_MAP_ID  (0x40)
#define BRIGHTNESS_SVC_BRIGHTNESS_KEY_MAP_ID  (0x41)
#define WAKEUPTIME_SVC_TIME_KEY_MAP_ID  (0x42)
#define FADETIME_SVC_TIME_KEY_MAP_ID  (0x43)
#define COMMONEXECUTION1_SVC_ACTION_KEY_MAP_ID  (0x44)


/* 设备profile版本号 */
extern const char* g_profileVer;

/* 设备信息 */
extern const DevInfo g_devInfo;

/* 厂商及设备类型名称 */
extern const DevEnName g_devEnName;

/* 设备AC信息 */
extern const unsigned char g_ac[HLK_AC_LEN];

/* 设备BI信息 */
extern const char* g_bi;

/* 服务列表 */
extern const SvcInfo g_svcMap[5];

/* 属性列表 */
extern const KeyInfo g_keyMap[5];

/* 服务属性回调表 */
extern HiLinkMsg g_msgTable[5];

/*
 * 功能: switch服务on属性控制回调函数
 * 参数: value 模组下发的属性值
 * 返回值: 0-成功, 非0-失败
 * 注意: 此函数由设备厂商实现, 实现硬件控制以及本地数据的更新.
 */
int SwitchOnCtrlFunc(int value);

/*
 * 功能: brightness服务brightness属性控制回调函数
 * 参数: value 模组下发的属性值
 * 返回值: 0-成功, 非0-失败
 * 注意: 此函数由设备厂商实现, 实现硬件控制以及本地数据的更新.
 */
int BrightnessBrightnessCtrlFunc(int value);

/*
 * 功能: wakeupTime服务time属性控制回调函数
 * 参数: value 模组下发的属性值
 * 返回值: 0-成功, 非0-失败
 * 注意: 此函数由设备厂商实现, 实现硬件控制以及本地数据的更新.
 */
int WakeupTimeTimeCtrlFunc(int value);

/*
 * 功能: fadeTime服务time属性控制回调函数
 * 参数: value 模组下发的属性值
 * 返回值: 0-成功, 非0-失败
 * 注意: 此函数由设备厂商实现, 实现硬件控制以及本地数据的更新.
 */
int FadeTimeTimeCtrlFunc(int value);

/*
 * 功能: commonExecution1服务action属性控制回调函数
 * 参数: value 模组下发的属性值
 * 返回值: 0-成功, 非0-失败
 * 注意: 此函数由设备厂商实现, 实现硬件控制以及本地数据的更新.
 */
int CommonExecution1ActionCtrlFunc(int value);

#endif /* _HILINK_DEVICE_H */
