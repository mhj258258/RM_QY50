/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK�豸��������ӿ�
 * Create: 2018-12-01
 */
#include "hilink_ota.h"

/*
 * �ú����ɳ��̻򿪷��߰�����ѡ��ʵ��, ����ģ�鴫�ݹ����İ汾�ż���ذ��Ƿ���Ҫ����.
 * ����version��ʾ���������°汾��, len�ǰ汾�ų���.
 * ����HLK_RET_OK��ʾ��Ҫ����, ����HLK_RET_ERR��ʾ����Ҫ��������ִ���.
 */
short HiLinkOtaCheckVer(const unsigned char* version, unsigned short len)
{
    if ((version == NULL) || (len == 0)) {
        return HLK_RET_ERR;
    }

    /* ���ڴ˴�ʵ�ּ��汾 */

    return HLK_RET_OK;
}

/*
 * �ú����ɳ��̻򿪷��߰�����ѡ��ʵ��, У����������С, ��У������, ��������������.
 * ����binSize��ʾ����������Ĵ�С.
 */
short HiLinkOtaStart(unsigned int binSize)
{
    if (binSize == 0) {
        return HLK_RET_ERR;
    }

    /* ���ڴ˴�ʵ��У����������С���������� */

    return HLK_RET_OK;
}

/*
 * �ú����ɳ��̻򿪷��߰�����ѡ��ʵ��, �����������ݰ�.
 * ���ݰ��ְ�����, ÿ�δ���100���ֽ�, ���պ���ִ����������(д��flash��).
 * ����pkg��ʾ�������ְ�����, len��ʾ���ηְ��ĳ���.
 */
short HiLinkOtaRcvPkg(const unsigned char* pkg, unsigned short len)
{
    if ((pkg == NULL) || (len == 0)) {
        return HLK_RET_ERR;
    }

    /* ���ڴ˴�ʵ�ֽ����������Ͱ汾д�� */

    return HLK_RET_OK;
}

/*
 * �ú����ɳ��̻򿪷��߰�����ѡ��ʵ��, ���������У��.
 * ����checkSum��ʾ��������sha-256У���, len��У��͵ĳ���.
 * ��������Ҫ�����յ���У���checkSum�뱾�ؽ��յ��������������У��ͽ��бȽ�.
 */
short HiLinkOtaEnd(const unsigned char* checkSum, unsigned short len)
{
    if ((checkSum == NULL) || (len == 0)) {
        return HLK_RET_ERR;
    }

    /* ���ڴ˴�ʵ��������У�� */

    return HLK_RET_OK;
}
