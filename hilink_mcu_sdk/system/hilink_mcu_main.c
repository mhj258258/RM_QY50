/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDKϵͳ��ں���
 * Create: 2018-12-01
 */
#include "hilink_common.h"

/*
 * ϵͳ��ں���, ������ͨ�����ñ���������MCU SDK�����.
 * ��������Ҫ�����豸ʵ�����������ʱ�ӿ�����ѭ���е���Ӻ�����ʱ.
 */
void HiLinkMcuMain(void)
{
    /* HiSlip��ʼ������ʼ��HiSlipʹ�õ��ڲ�ȫ������ */
    HiLinkDevInit();

    while (1) {
        /* ���մ���HiSlip�Լ�HiLink���� */
        HiLinkMainProcess();

        /* �ڴ˴�������ʱ�ӿ���Ӻ��ʵ���ʱ */
    }
}
