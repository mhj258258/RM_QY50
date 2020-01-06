/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK�豸Ӳ������ӿ�����
 * Create: 2018-12-01
 */
#ifndef _HILINK_MCU_H
#define _HILINK_MCU_H

#include "hilink_common.h"

/* �ṩ�����ߵ��õĽӿ� */

/*
 * ϵͳ��ں���, ������ͨ�����ñ���������MCU SDK�����.
 * ��������Ҫ�����豸ʵ�����������ʱ�ӿ�����ѭ���е���Ӻ�����ʱ.
 */
void HiLinkMcuMain(void);

/*
 * ͨ�����ڽ��յ��ֽ�����, ����data��ʾ���յĵ��ֽ�����.
 * ��������Ҫ�ڴ��ڽ����жϴ������е��ñ�����.
 */
void HiLinkUartRcvOneByte(unsigned char data);

/* �����������Ҫ����wifiģ��, �����κεط����ñ����� */
void HiLinkModuleReboot(void);

/* �����������Ҫ����wifiģ��, �����κεط����ñ����� */
void HiLinkModuleReset(void);

/* �����ߵ��ñ�������ȡģ�鵱ǰ������״̬ */
ModuleNetStatus HiLinkGetLocalNetStatus(void);

/*
 * �����ߵ��ñ��ӿڻ�ȡģ���WiFi��Ϣ, ������ʾWiFi�ȵ������.
 * ����Ҫ��ȡ��ǰ�豸���ӵ�WiFi�ȵ����Ϣ, ����ssid�봫��NULL (��������¶�����NULL).
 * ��������Ҫʵ��hilink_mcu.c�е�HiLinkNotifyModuleInfo������������յ���ģ����Ϣ.
 */
void HiLinkGetModuleInfo(char* ssid);

/*
 * �����ߵ��ñ��ӿڻ�ȡģ���UTCʱ��.
 * ��������Ҫʵ��hilink_mcu.c�е�HiLinkNotifyUtcTime������������յ���UTCʱ��.
 */
void HiLinkGetUtcTime(void);

/*
 * �����߰�����ά���ϱ����ط���״̬ʱ, ���Ե��ñ��ӿ��ϱ�������������ֵ.
 * ����val���ϱ������ͻ򲼶���������ֵ, ��Ҫ�ϱ��ַ���������, ��ò�����0.
 * ����str���ϱ����ַ���������ֵ, ��Ҫ�ϱ����ͻ򲼶���������, ��ò�����NULL.
 */
int HiLinkUpdateKeyVal(unsigned short svcMapId, unsigned short keyMapId, int val, const char* str);

/*
 * ��������Ҫ������ά���ϱ����ط���״̬ʱ, ��Ҫ���ñ��ӿ������ϱ���Ӧ�������������ֵ.
 * ����val���ϱ������ͻ򲼶���������ֵ, ��Ҫ�ϱ��ַ���������, ��ò�����0.
 * ����str���ϱ����ַ���������ֵ, ��Ҫ�ϱ����ͻ򲼶���������, ��ò�����NULL.
 * ����rptFlag���ϱ�ʱ���ı��, �����һ���ϱ�������, rptFlagȡֵΪREPORT_LATER��ʾ�Ժ��ϱ�,
 * �����һ���ϱ�������, rptFlagȡֵΪREPORT_NOW��ʾ�����ϱ�.
 */
int HiLinkUpdateSvcVal(unsigned short svcMapId, unsigned short keyMapId, int val, const char* str, char rptFlag);

/* ��������Ҫ������ʵ�ֵĽӿ� */

/* ������ʵ�ֱ��ӿڻ�ȡ������ϵͳ��ǰʱ�����ֵ(��ʱ��tickֵ) */
unsigned long long HiLinkGetSysCurTime(void);

/* ������ʵ�ֱ��ӿ�ͨ��UART��������ʽ����һ���ֽڵ����� */
void HiLinkUartSendOneByte(unsigned char data);

/*
 * ������ʵ�ֱ�������ʼ�����з���ʵ������������ֵ.
 * ���ݵ�ǰʵ���豸״̬, ͨ������HiLinkUpdateKeyVal�ӿ����θ�g_msgTable�е�value�ֶθ�ֵ.
 */
void HiLinkInitProfileValue(void);

/*
 * ����û�������HiLinkGetModuleInfo�ӿڻ�ȡwifiģ����Ϣ, ����Ҫʵ�ֱ����������ص�ģ����Ϣ.
 * ���ص�ģ����Ϣ��:ģ��MAC��ַ(mac), Ӳ���汾��(hardVer), ����汾��(softVer), WiFiǿ��(rssi),
 * WiFi�ȵ�AP����(apName)
 */
void HiLinkNotifyModuleInfo(const char* mac, const char* hardVer,
    const char* softVer, int rssi, const char* apName);

/*
 * ����û�������HiLinkGetUtcTime�ӿڻ�ȡwifiģ��UTCʱ��, ����Ҫʵ�ֱ����������ص�ʱ��
 * ��Ϣbuf��7���ֽ���ɣ��ֱ��ʾ��(2�ֽ�)����(1�ֽ�)����(1�ֽ�)��ʱ(1�ֽ�)����(1�ֽ�)����(1�ֽ�)
 */
void HiLinkNotifyUtcTime(const unsigned char* buf, int len);

/*
 * MCU SDK�ڲ��Ĵ�����Ϣ��ͨ�����������ݸ��豸.
 * �����߿���ѡ��ʵ�ֱ��ӿ�, ���ݴ�����errCode������Ӧ�Ĵ���.
 */
void HiLinkNotifyErrorInfo(unsigned int errCode);

/*
 * ģ��֪ͨMCU�豸���ƶ�ɾ��ע��.
 * �����߿���ʵ�ֱ��ӿ�, ����豸���ƶ�ɾ��ע��ʱ���豸�����Ӧ����.
 */
void HiLinkDevRemoved(void);

/*
 * ������ͨ��ʵ�ֱ��������豸SN�ŷ���¼��.
 * ���в���len��ʾSN����󳤶�39�ֽ�, ����sn��ʾ¼����豸SN��.
 * ��δʵ�ֱ��ӿڻ�snָ�뷵�ص��ַ�������Ϊ0ʱ, SDK��Ĭ��ʹ���豸mac��ַ��ΪSN��.
 */
void HiLinkGetDeviceSn(unsigned int len, char* sn);

#endif /* _HILINK_MCU_H */
