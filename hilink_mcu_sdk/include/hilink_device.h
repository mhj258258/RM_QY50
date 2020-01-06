/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK���ܶ��弰���ƽӿ�����
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

/* ����ӳ��ID���� */
/* ע��, ����ӳ��ID�궨��ķ�ΧΪ: 0x40 ~ 0x7f �� 0x80 ~ 0x1FFF. */
#define SWITCH_SVC_MAP_ID  (0x40)
#define BRIGHTNESS_SVC_MAP_ID  (0x41)
#define WAKEUPTIME_SVC_MAP_ID  (0x42)
#define FADETIME_SVC_MAP_ID  (0x43)
#define COMMONEXECUTION1_SVC_MAP_ID  (0x44)

/* ����ӳ��ID���� */
/* ע��, ����ӳ��ID�궨��ķ�ΧΪ: 0x40 ~ 0x7f �� 0x80 ~ 0x1FFF. */
#define SWITCH_SVC_ON_KEY_MAP_ID  (0x40)
#define BRIGHTNESS_SVC_BRIGHTNESS_KEY_MAP_ID  (0x41)
#define WAKEUPTIME_SVC_TIME_KEY_MAP_ID  (0x42)
#define FADETIME_SVC_TIME_KEY_MAP_ID  (0x43)
#define COMMONEXECUTION1_SVC_ACTION_KEY_MAP_ID  (0x44)


/* �豸profile�汾�� */
extern const char* g_profileVer;

/* �豸��Ϣ */
extern const DevInfo g_devInfo;

/* ���̼��豸�������� */
extern const DevEnName g_devEnName;

/* �豸AC��Ϣ */
extern const unsigned char g_ac[HLK_AC_LEN];

/* �豸BI��Ϣ */
extern const char* g_bi;

/* �����б� */
extern const SvcInfo g_svcMap[5];

/* �����б� */
extern const KeyInfo g_keyMap[5];

/* �������Իص��� */
extern HiLinkMsg g_msgTable[5];

/*
 * ����: switch����on���Կ��ƻص�����
 * ����: value ģ���·�������ֵ
 * ����ֵ: 0-�ɹ�, ��0-ʧ��
 * ע��: �˺������豸����ʵ��, ʵ��Ӳ�������Լ��������ݵĸ���.
 */
int SwitchOnCtrlFunc(int value);

/*
 * ����: brightness����brightness���Կ��ƻص�����
 * ����: value ģ���·�������ֵ
 * ����ֵ: 0-�ɹ�, ��0-ʧ��
 * ע��: �˺������豸����ʵ��, ʵ��Ӳ�������Լ��������ݵĸ���.
 */
int BrightnessBrightnessCtrlFunc(int value);

/*
 * ����: wakeupTime����time���Կ��ƻص�����
 * ����: value ģ���·�������ֵ
 * ����ֵ: 0-�ɹ�, ��0-ʧ��
 * ע��: �˺������豸����ʵ��, ʵ��Ӳ�������Լ��������ݵĸ���.
 */
int WakeupTimeTimeCtrlFunc(int value);

/*
 * ����: fadeTime����time���Կ��ƻص�����
 * ����: value ģ���·�������ֵ
 * ����ֵ: 0-�ɹ�, ��0-ʧ��
 * ע��: �˺������豸����ʵ��, ʵ��Ӳ�������Լ��������ݵĸ���.
 */
int FadeTimeTimeCtrlFunc(int value);

/*
 * ����: commonExecution1����action���Կ��ƻص�����
 * ����: value ģ���·�������ֵ
 * ����ֵ: 0-�ɹ�, ��0-ʧ��
 * ע��: �˺������豸����ʵ��, ʵ��Ӳ�������Լ��������ݵĸ���.
 */
int CommonExecution1ActionCtrlFunc(int value);

#endif /* _HILINK_DEVICE_H */
