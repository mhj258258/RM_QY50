/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK���ܶ��弰�豸���ƽӿ�
 * Create: 2018-12-01
 */
#include "hilink_device.h"
#include "hilink_mcu.h"
#include "led.h"
#include "uart.h"
/* �豸profile�汾�Ŷ��� */
const char* g_profileVer = "VC001.01.01";

/* �豸��Ϣ���� */
const DevInfo g_devInfo = {
    "",  /* �豸SN, ��ͨ��HiLinkGetDeviceSn�ӿ�¼��, ��δ��д��Ĭ����WiFiģ���MAC��ַ�Զ����� */
    "20Y0",  /* �豸ID */
    "qy_sw",  /* �豸�ͺ� */
    "01B",  /* �豸����ID */
    "498",  /* �豸������ID */
    "1.0.0"  /* �豸MCU�汾��, �뿪���߰���ʵ�ʰ汾����д */
};

/* �豸���ƶ��� */
const DevEnName g_devEnName = {
    "Lamp",  /* �豸�������� */
    "Xclea"  /* �������� */
};

/* �豸ACֵ���� */
const unsigned char g_ac[HLK_AC_LEN] = {
    0x2B, 0x24, 0x3A, 0x5B, 0x2C, 0x43, 0x52, 0x3D,
    0x22, 0x72, 0x30, 0x6B, 0x2C, 0x2A, 0x6C, 0x32,
    0x90, 0xF4, 0x2D, 0x15, 0x7D, 0xC3, 0xAA, 0x89,
    0xB8, 0x37, 0x29, 0x3F, 0x02, 0xAC, 0x65, 0xB4,
    0xA6, 0xA9, 0xA2, 0x69, 0x65, 0xCE, 0x42, 0xD0,
    0xAB, 0x01, 0x0A, 0xBB, 0x9B, 0x6A, 0x04, 0x41
};

/* �豸BIֵ���� */
const char* g_bi = "65455BDDE3818F44D1D2D177622EAD9D3369EF729F02BD7EF1570F517B931DEE" \
    "D061263932806EB6F80F085D7D67178D4A8F3738931EA0F208948EFCCCA236DD" \
    "5168209466DDB2F2947B7E11C7E46CA40BD55524E631FB955FAD33ED71F530F3" \
    "CFA9B93D5DB20C4F26353BB955FB57202275268667643A45DF07F83414D91FED" \
    "14A17EAA6867120D1E93FA197B563B25239C6A9101010F2E3CB9CD4474052377" \
    "306B62B2860B94F0B50AFB35E461261A0C866E2C75AFB12F33B56B9035670493" \
    "93A98D508CBBC88D5017E9F1F8DD88685EE16FBB8BDA5D919F367FF8F163FF9E" \
    "714D28128740402ED46CD92D4236D1A2DB9564F7A0FF12E6F86A9B6CC94D4548";

/* �����б��� */
const SvcInfo g_svcMap[] = {
    { SWITCH_SVC_MAP_ID, "switch", "switch" },
    { BRIGHTNESS_SVC_MAP_ID, "brightness", "brightness" },
    { WAKEUPTIME_SVC_MAP_ID, "wakeupTime", "wakeupTime" },
    { FADETIME_SVC_MAP_ID, "fadeTime", "fadeTime" },
    { COMMONEXECUTION1_SVC_MAP_ID, "commonExecution1", "commonExecution1" }
};

/* �����б��� */
const KeyInfo g_keyMap[] = {
    { SWITCH_SVC_ON_KEY_MAP_ID, "switch", "on", HLK_DATA_TYPE_BOOL },
    { BRIGHTNESS_SVC_BRIGHTNESS_KEY_MAP_ID, "brightness", "brightness", HLK_DATA_TYPE_INT },
    { WAKEUPTIME_SVC_TIME_KEY_MAP_ID, "wakeupTime", "time", HLK_DATA_TYPE_INT },
    { FADETIME_SVC_TIME_KEY_MAP_ID, "fadeTime", "time", HLK_DATA_TYPE_INT },
    { COMMONEXECUTION1_SVC_ACTION_KEY_MAP_ID, "commonExecution1", "action", HLK_DATA_TYPE_INT }
};

/* �������Իص����� */
HiLinkMsg g_msgTable[] = {
    { SWITCH_SVC_MAP_ID, SWITCH_SVC_ON_KEY_MAP_ID, 0, SwitchOnCtrlFunc },
    { BRIGHTNESS_SVC_MAP_ID, BRIGHTNESS_SVC_BRIGHTNESS_KEY_MAP_ID, 0, BrightnessBrightnessCtrlFunc },
    { WAKEUPTIME_SVC_MAP_ID, WAKEUPTIME_SVC_TIME_KEY_MAP_ID, 0, WakeupTimeTimeCtrlFunc },
    { FADETIME_SVC_MAP_ID, FADETIME_SVC_TIME_KEY_MAP_ID, 0, FadeTimeTimeCtrlFunc },
    { COMMONEXECUTION1_SVC_MAP_ID, COMMONEXECUTION1_SVC_ACTION_KEY_MAP_ID, 0, CommonExecution1ActionCtrlFunc }
};

/* �����Ƿ������Կ��ƻص��������� */

/*
 * ����: switch����on���Կ��ƻص�����
 * ����: value ģ���·�������ֵ
 * ����ֵ: 0-�ɹ�, ��0-ʧ��
 * ע��: �˺������豸����ʵ��, ʵ��Ӳ�������Լ��������ݵĸ���.
 */
int SwitchOnCtrlFunc(int value)
{
    /* ���ģ���·�������ֵ */
     int val = value;

    /* ��������ֵʵ���豸����, ��������: */

    // 1.���ݻ�ȡ������ֵ, ʵ���豸����, ����:
    // printf("SwitchOnCtrlFunc, on: %d\r\n", val);
    // 2.���������ϱ�Ȩ�ޣ��ɵ�������ӿ������ϱ���ǰ����
    // HiLinkUpdateKeyVal(XXX_SVC_MAP_ID, XXX_SVC_XXX_KEY_MAP_ID, val, NULL);
	
		USART1_DEBUG("\r\n val :%d\r\n",val);
		//--------by tomi--------
		if(val == 1)
		{
			USART1_DEBUG("\r\n LED_ON \r\n");
			gd_eval_led_on(LED2);
		}
		else
		{
			USART1_DEBUG("\r\n LED_OFF \r\n");
			gd_eval_led_off(LED2);
		}
		
		HiLinkUpdateKeyVal(SWITCH_SVC_MAP_ID, SWITCH_SVC_ON_KEY_MAP_ID, val, NULL);
    return 0;
}

/*
 * ����: brightness����brightness���Կ��ƻص�����
 * ����: value ģ���·�������ֵ
 * ����ֵ: 0-�ɹ�, ��0-ʧ��
 * ע��: �˺������豸����ʵ��, ʵ��Ӳ�������Լ��������ݵĸ���.
 */
int BrightnessBrightnessCtrlFunc(int value)
{
    /* ���ģ���·�������ֵ */
     int val = value;

    /* ��������ֵʵ���豸����, ��������: */

    // 1.���ݻ�ȡ������ֵ, ʵ���豸����, ����:
    // printf("BrightnessBrightnessCtrlFunc, brightness: %d\r\n", val);
    // 2.���������ϱ�Ȩ�ޣ��ɵ�������ӿ������ϱ���ǰ����
    // HiLinkUpdateKeyVal(XXX_SVC_MAP_ID, XXX_SVC_XXX_KEY_MAP_ID, val, NULL);
		USART1_DEBUG("\r\n BrightnessBrightnessCtrlFunc val = %d \r\n",val);
    return 0;
}

/*
 * ����: wakeupTime����time���Կ��ƻص�����
 * ����: value ģ���·�������ֵ
 * ����ֵ: 0-�ɹ�, ��0-ʧ��
 * ע��: �˺������豸����ʵ��, ʵ��Ӳ�������Լ��������ݵĸ���.
 */
int WakeupTimeTimeCtrlFunc(int value)
{
    /* ���ģ���·�������ֵ */
     int val = value;

    /* ��������ֵʵ���豸����, ��������: */

    // 1.���ݻ�ȡ������ֵ, ʵ���豸����, ����:
    // printf("WakeupTimeTimeCtrlFunc, time: %d\r\n", val);
    // 2.���������ϱ�Ȩ�ޣ��ɵ�������ӿ������ϱ���ǰ����
    // HiLinkUpdateKeyVal(XXX_SVC_MAP_ID, XXX_SVC_XXX_KEY_MAP_ID, val, NULL);
		USART1_DEBUG("\r\n WakeupTimeTimeCtrlFunc val = %d \r\n",val);
    return 0;
}

/*
 * ����: fadeTime����time���Կ��ƻص�����
 * ����: value ģ���·�������ֵ
 * ����ֵ: 0-�ɹ�, ��0-ʧ��
 * ע��: �˺������豸����ʵ��, ʵ��Ӳ�������Լ��������ݵĸ���.
 */
int FadeTimeTimeCtrlFunc(int value)
{
    /* ���ģ���·�������ֵ */
     int val = value;

    /* ��������ֵʵ���豸����, ��������: */

    // 1.���ݻ�ȡ������ֵ, ʵ���豸����, ����:
    // printf("FadeTimeTimeCtrlFunc, time: %d\r\n", val);
    // 2.���������ϱ�Ȩ�ޣ��ɵ�������ӿ������ϱ���ǰ����
    // HiLinkUpdateKeyVal(XXX_SVC_MAP_ID, XXX_SVC_XXX_KEY_MAP_ID, val, NULL);
		USART1_DEBUG("\r\n FadeTimeTimeCtrlFunc val = %d \r\n",val);
    return 0;
}

/*
 * ����: commonExecution1����action���Կ��ƻص�����
 * ����: value ģ���·�������ֵ
 * ����ֵ: 0-�ɹ�, ��0-ʧ��
 * ע��: �˺������豸����ʵ��, ʵ��Ӳ�������Լ��������ݵĸ���.
 */
int CommonExecution1ActionCtrlFunc(int value)
{
    /* ���ģ���·�������ֵ */
    int val = value;

    /* ��������ֵʵ���豸����, ��������: */

    // 1.���ݻ�ȡ������ֵ, ʵ���豸����, ����:
    // printf("CommonExecution1ActionCtrlFunc, action: %d\r\n", val);
    // 2.���������ϱ�Ȩ�ޣ��ɵ�������ӿ������ϱ���ǰ����
    // HiLinkUpdateKeyVal(XXX_SVC_MAP_ID, XXX_SVC_XXX_KEY_MAP_ID, val, NULL);
		USART1_DEBUG("\r\n CommonExecution1ActionCtrlFunc val = %d \r\n",val);
    return 0;
}
