/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK系统入口函数
 * Create: 2018-12-01
 */
#include "hilink_common.h"

/*
 * 系统入口函数, 开发者通过调用本函数集成MCU SDK主框架.
 * 开发者需要根据设备实际情况调用延时接口在主循环中的添加合适延时.
 */
void HiLinkMcuMain(void)
{
    /* HiSlip初始化，初始化HiSlip使用的内部全局数据 */
    HiLinkDevInit();

    while (1) {
        /* 接收处理HiSlip以及HiLink数据 */
        HiLinkMainProcess();

        /* 在此处调用延时接口添加合适的延时 */
    }
}
