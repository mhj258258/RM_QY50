/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK�豸Ӳ������ӿ�
 * Create: 2018-12-01
 */
#include "hilink_mcu.h"
#include "hilink_device.h"
#include "systick.h"
#include "uart.h"

/* ������ʵ�ֱ��ӿڻ�ȡ������ϵͳ��ǰʱ�����ֵ(��ʱ��tickֵ) */
unsigned long long HiLinkGetSysCurTime(void)
{
    unsigned long long curTime = 0;

    /* �ڴ˴����ʵ�ִ��� */
    /* ����ϵͳ�⺯����ȡϵͳʱ�Ӽ���ֵ */
	//---------by tomi----------
		curTime = Get_tick();
	
    return curTime;
}

/* ������ʵ�ֱ��ӿ�ͨ��UART��������ʽ����һ���ֽڵ����� */
void HiLinkUartSendOneByte(unsigned char data)
{
    /* �ڴ˴����ʵ�ִ��� */
    /*
     * ���þ��崮�ڷ��ͽӿڷ�������
     * ��Ӻ��ʵķ�����ʱ
     */
		//---------by tomi----------
		usart_data_transmit(EVAL_COM1, data);//uart0
	
    return;
}

/*
 * ������ʵ�ֱ�������ʼ�����з���ʵ������������ֵ.
 * ���ݵ�ǰʵ���豸״̬, ͨ������HiLinkUpdateKeyVal�ӿ����θ�g_msgTable�е�value�ֶθ�ֵ.
 */
void HiLinkInitProfileValue(void)
{
    /* �ڴ˴����ʵ�ִ��� */
    /*
     * ʵ�ֲ���ʾ��(�����ܵ�Ϊ��):
     * ��1��: ʵ�ֶ�ȡGPIO�ĺ���, ��ÿ��Ƶƿ��ص�GPIO�˿�ֵ;
     * ��2��: ����HiLinkUpdateKeyVal�������÷������Գ�ʼֵ��
     */
		//------------by tomi---------------
		//svc1������
		HiLinkUpdateKeyVal(1, 1, 0, NULL);//��Ŵ�1��ʼ��1��1�͸ñ��˿��ط��� on����
		//svc2������
		HiLinkUpdateKeyVal(2, 1, 100, NULL);
		//svc3������ʱ��
		HiLinkUpdateKeyVal(3, 1, 1800, NULL);
		//svc4������ʱ��
		HiLinkUpdateKeyVal(4, 1, 1800, NULL);
	
    return;
}

/*
 * ����û�������HiLinkGetModuleInfo�ӿڻ�ȡwifiģ����Ϣ, ����Ҫʵ�ֱ����������ص�ģ����Ϣ.
 * ���ص�ģ����Ϣ��:ģ��MAC��ַ(mac), Ӳ���汾��(hardVer), ����汾��(softVer), WiFiǿ��(rssi),
 * WiFi�ȵ�AP����(apName)
 */
void HiLinkNotifyModuleInfo(const char* mac, const char* hardVer,
    const char* softVer, int rssi, const char* apName)
{
    if ((mac == NULL) || (hardVer == NULL) || (softVer == NULL) || (apName == NULL)) {
        return;
    }

    /* �ڴ˴����ʵ�ִ���, ʹ�û�ȡ��ģ����Ϣ */

    return;
}

/*
 * ����û�������HiLinkGetUtcTime�ӿڻ�ȡWiFiģ��UTCʱ��, ����Ҫʵ�ֱ����������ص�ʱ��
 * ��Ϣbuf��7���ֽ���ɣ��ֱ��ʾ��(2�ֽ�)����(1�ֽ�)����(1�ֽ�)��ʱ(1�ֽ�)����(1�ֽ�)����(1�ֽ�)
 */
void HiLinkNotifyUtcTime(const unsigned char* buf, int len)
{
    if (len != UTC_TIME_BUF_LEN) {
        return;
    }

    /* �ڴ˴����ʵ�ִ���, ʹ�û�ȡ��ʱ����Ϣ, ����ֵ��ȡ�ο����� */
    /*
     * �꣺(buf[0] << 8) + buf[1]
     * �£�buf[2]
     * �գ�buf[3]
     * ʱ��buf[4]
     * �֣�buf[5]
     * �룺buf[6]
     */

}

/*
 * MCU SDK�ڲ��Ĵ�����Ϣ��ͨ�����������ݸ��豸.
 * �����߿���ѡ��ʵ�ֱ��ӿ�, ���ݴ�����errCode������Ӧ�Ĵ���.
 */
void HiLinkNotifyErrorInfo(unsigned int errCode)
{
    /* �ڴ˴����ʵ�ִ���, ��ӡ�����Ĵ�����errCode */

    return;
}

/*
 * ģ��֪ͨMCU�豸���ƶ�ɾ��ע��.
 * �����߿���ʵ�ֱ��ӿ�, ����豸���ƶ�ɾ��ע��ʱ���豸�����Ӧ����.
 */
void HiLinkDevRemoved(void)
{
    /* �ڴ˴�����豸���ƶ�ɾ��ע��ʱ��MCU�����߼� */

    return;
}

/*
 * ������ͨ��ʵ�ֱ��������豸SN�ŷ���¼��.
 * ���в���len��ʾSN����󳤶�39�ֽ�, ����sn��ʾ¼����豸SN��.
 * ��δʵ�ֱ��ӿڻ�snָ�뷵�ص��ַ�������Ϊ0ʱ, SDK��Ĭ��ʹ���豸mac��ַ��ΪSN��.
 */
void HiLinkGetDeviceSn(unsigned int len, char* sn)
{
    /* �ڴ˴����ʵ�ִ���, ��sn��ֵ��*sn�ش� */

    return;
}
