/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK设备升级适配接口声明
 * Create: 2018-12-01
 */
#ifndef _HILINK_OTA_H
#define _HILINK_OTA_H

#include "hilink_common.h"

/*
 * 该函数由厂商或开发者按需求选择实现, 根据模组传递过来的版本号检查电控板是否需要升级.
 * 参数version表示服务器最新版本号, len是版本号长度.
 * 返回HLK_RET_OK表示需要升级, 返回HLK_RET_ERR表示不需要升级或出现错误.
 */
short HiLinkOtaCheckVer(const unsigned char* version, unsigned short len);

/*
 * 该函数由厂商或开发者按需求选择实现, 校验升级包大小, 若校验正常, 则启动升级过程.
 * 参数binSize表示软件升级包的大小.
 */
short HiLinkOtaStart(unsigned int binSize);

/*
 * 该函数由厂商或开发者按需求选择实现, 接收升级数据包.
 * 数据包分包传输, 每次传输100个字节, 接收后请执行升级操作(写入flash等).
 * 参数pkg表示升级包分包数据, len表示本次分包的长度.
 */
short HiLinkOtaRcvPkg(const unsigned char* pkg, unsigned short len);

/*
 * 该函数由厂商或开发者按需求选择实现, 完成升级包校验.
 * 参数checkSum表示升级包的sha-256校验和, len是校验和的长度.
 * 开发者需要将接收到的校验和checkSum与本地接收的升级包计算出的校验和进行比较.
 */
short HiLinkOtaEnd(const unsigned char* checkSum, unsigned short len);

#endif /* _HILINK_OTA_H */
