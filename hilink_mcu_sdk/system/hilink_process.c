/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: HiLinkҵ�����̴����ļ�
 * Create: 2018-12-01
 */
#include "hilink_mcu.h"
#include "hilink_ota.h"
#include "hilink_device.h"

static short HiLinkCommonWaitAck(unsigned short cmd, unsigned long long startTick);
static short HiLinkReqGetWorkMode(void);
static short HiLinkReqGetNetStatus(void);
static short HiLinkReqRegStart(void);
static short HiLinkReqRegAcBi(void);
static short HiLinkReqRegDevInfo(void);
static short HiLinkReqRegSvcInfo(void);
static short HiLinkReqRegKeyMap(void);
static short HiLinkReqRegEnd(void);
static short HiLinkSetWorkMode(void);
static short HiLinkReportInitVals(void);
static short HiLinkReportProfileVals(void);

static unsigned int g_keyValChangeMark[DEV_MAX_KEY_COUNT / UNSIGNED_INT_BITS] = {0};
static unsigned int g_svcValChangeMark[DEV_MAX_KEY_COUNT / UNSIGNED_INT_BITS] = {0};
static HiLinkTlvType g_tlvArray[HLK_MAX_TLVS_CNT] = {0};
static unsigned char g_tmpSendBuf[HLK_SND_BUF_SIZE] = {0};
static unsigned char g_tmpExtendBuf[HLK_EXT_BUF_SIZE] = {0};

static unsigned char g_waitAckSeq = 0;
static unsigned short g_initSvcIndex = 0;
static ModuleWorkMode g_localWorkMode = WKMD_UNKONWN;
static ModuleNetStatus g_localNetStatus = NET_UNKONWN;
static HiLinkRunState g_hiLinkRunState = HLK_RUN_STATE_INIT;
static HiLinkCmdState g_hiLinkFuncState = FUNC_PROC_STATE_SNDCMD;
static HiLinkRptState g_initValReportState = HLK_RPT_PROCESSING;

static HiLinkReqFuncs g_initialFuncs[INIT_FUNC_NUM] = {
    { HLK_CMD_DEVCTL_QWM, HiLinkReqGetWorkMode, HiLinkCommonWaitAck },
    { HLK_CMD_DEVCTL_QNS, HiLinkReqGetNetStatus, HiLinkCommonWaitAck }
};

static HiLinkReqFuncs g_registerFuncs[REG_FUNC_NUM] = {
    { HLK_CMD_DEVREG_SETSTA, HiLinkReqRegStart, HiLinkCommonWaitAck },
    { HLK_CMD_DEVREG_DEVINFO, HiLinkReqRegAcBi, HiLinkCommonWaitAck },
    { HLK_CMD_DEVREG_DEVINFO, HiLinkReqRegDevInfo, HiLinkCommonWaitAck },
    { HLK_CMD_DEVREG_SVCINFO, HiLinkReqRegSvcInfo, HiLinkCommonWaitAck },
    { HLK_CMD_DEVREG_PROT, HiLinkReqRegKeyMap, HiLinkCommonWaitAck },
    { HLK_CMD_DEVREG_SETSTA, HiLinkReqRegEnd, HiLinkCommonWaitAck },
    { HLK_CMD_DEVCTL_QNS, HiLinkReqGetNetStatus, HiLinkCommonWaitAck }
};

static HiLinkReqFuncs g_setWorkModeFuncs[SET_WORKMODE_FUNC_NUM] = {
    { HLK_CMD_DEVCTL_WM, HiLinkSetWorkMode, HiLinkCommonWaitAck }
};

static HiLinkReqFuncs g_reportInitValFuncs[RPT_INIT_FUNC_NUM] = {
    { HLK_CMD_DEVUPD_STA, HiLinkReportInitVals, HiLinkCommonWaitAck }
};

static HiLinkReqFuncs g_idleStateFuncs[IDLE_FUNC_NUM] = {
    { HLK_CMD_DEVUPD_STA, HiLinkReportProfileVals, HiLinkCommonWaitAck },
    { HLK_CMD_DEVCTL_QNS, HiLinkReqGetNetStatus, HiLinkCommonWaitAck }
};

static HiLinkFuncMatrix g_functionMatrix[HLK_RUN_STATE_MAX] = {
    { g_initialFuncs, 0, INIT_FUNC_NUM },
    { g_registerFuncs, 0, REG_FUNC_NUM },
    { g_setWorkModeFuncs, 0, SET_WORKMODE_FUNC_NUM },
    { g_reportInitValFuncs, 0, RPT_INIT_FUNC_NUM },
    { g_idleStateFuncs, 0, IDLE_FUNC_NUM }
};

/*
 * ��������ֵ������.
 * ����rptType��ʾ�����ϱ�����(�����Ի򰴷���).
 * ����idx�Ǵ����õ����������±�, setMark�����õı��ֵ(0��1).
 */
static void HiLinkSetKeyValChangeMark(DEV_REPORT_TYPE rptType, unsigned short idx, int setMark)
{
    if (idx >= DEV_MAX_KEY_COUNT) {
        return;
    }

    unsigned short intPos = idx / UNSIGNED_INT_BITS;
    unsigned short bitPos = idx % UNSIGNED_INT_BITS;
    unsigned int mark = (1 << bitPos);
    unsigned int* changeMask = NULL;

    if (rptType == RPT_BY_KEY) {
        changeMask = (unsigned int*)g_keyValChangeMark;
    } else if (rptType == RPT_BY_SVC) {
        changeMask = (unsigned int*)g_svcValChangeMark;
    } else {
        return;
    }

    if (setMark == DEV_KEY_MASK_SET) {
        changeMask[intPos] |= mark; /* ��Ӧλ��Ϊ1 */
    } else {
        changeMask[intPos] &= (~mark); /* ��Ӧλ��Ϊ0 */
    }
}

/*
 * ����Ƿ��������ֵ���, rptType��ʾ�����ϱ�����(�����Ի򰴷���).
 * ����1��ʾ�������Ա��, ����0��ʾ���������Ա��.
 */
static int HiLinkHasKeyValChanged(DEV_REPORT_TYPE rptType)
{
    unsigned int* changeMask = NULL;
    if (rptType == RPT_BY_KEY) {
        changeMask = (unsigned int*)g_keyValChangeMark;
    } else if (rptType == RPT_BY_SVC) {
        changeMask = (unsigned int*)g_svcValChangeMark;
    } else {
        return 0;
    }

    unsigned char i;
    for (i = 0; i < (DEV_MAX_KEY_COUNT / UNSIGNED_INT_BITS); i++) {
        /* ���ڱ仯������ */
        if (changeMask[i] != 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * ���ָ���±�����ֵ�Ƿ���.
 * ����rptType��ʾ�����ϱ�����(�����Ի򰴷���), idx�Ǵ������±�.
 * ����setMark��ʾ������󣬽���Ӧ�������ó�setMark�ı��ֵ.
 * ����1��ʾ��Ӧ���Ա��, ����0��ʾ��Ӧ����δ���.
 */
static int HiLinkCheckChangedKeyVal(DEV_REPORT_TYPE rptType, unsigned short idx, int setMark)
{
    if (idx >= DEV_MAX_KEY_COUNT) {
        return 0;
    }

    unsigned int* changeMask = NULL;
    if (rptType == RPT_BY_KEY) {
        changeMask = (unsigned int*)g_keyValChangeMark;
    } else if (rptType == RPT_BY_SVC) {
        changeMask = (unsigned int*)g_svcValChangeMark;
    } else {
        return 0;
    }

    unsigned short intPos = idx / UNSIGNED_INT_BITS;
    unsigned short bitPos = idx % UNSIGNED_INT_BITS;
    unsigned int mark = changeMask[intPos] >> bitPos;

    /* ��Ӧλ���Ա仯, ��1��ʾȡ���λ */
    if ((mark & 1) == DEV_KEY_MASK_SET) {
        if (setMark == DEV_KEY_MASK_USET) {
            mark = (1 << bitPos);
            /* ����Ӧλ��ǻ�0(δ�仯) */
            changeMask[intPos] &= (~mark);
        }
        return 1;
    }
    return 0;
}

/* ��ȡ�豸��Ϣ�������豸��Ϣ�ṹָ�� */
static void* HiLinkGetDeviceInfo(void)
{
    return (void*)&DEV_PROFILE_DEVINF;
}

/* ��ȡ�豸BI��Կ�������豸BI��Կ�ַ���ָ�� */
static void* HiLinkGetDeviceBi(void)
{
    return (void*)DEV_PROFILE_BIRSA;
}

/* ��ȡ�豸AC��Ϣ�������豸AC��Ϣ����ָ�� */
static void* HiLinkGetDeviceAc(void)
{
    return (void*)DEV_PROFILE_ACVAL;
}

/* ��ȡ�豸���ͼ�����Ӣ�����ƣ������豸������Ϣ�ṹָ�� */
static void* HiLinkGetDeviceEnName(void)
{
    return (void*)&DEV_PROFILE_ENNAME;
}

/* ��ȡ�豸Profile�汾�ţ������ַ���ָ�� */
static void* HiLinkGetProfileVersion(void)
{
    return (void*)DEV_PROFILE_VER;
}

/*
 * �������ͻ�ȡ�豸Profile��Ϣ�б�.
 * ����type��ʾҪ��ȡ���豸Profile��Ϣ����, size��ʾ��ȡ����Ϣ�б��С.
 * ���ض�Ӧ�����豸Profile��Ϣ�б�ָ��.
 */
static void* HiLinkGetProfileInfoList(ProfileInfoType type, unsigned short* size)
{
    if (size == NULL) {
        return NULL;
    }

    unsigned short count = 0;
    void* retAddr = NULL;

    switch (type) {
        case TYPE_SVC_MAP: /* ������Ϣ�б� */
            count = sizeof(DEV_PROFILE_SVCS) / sizeof(DEV_PROFILE_SVCS[0]);
            retAddr = (void*)DEV_PROFILE_SVCS;
            break;
        case TYPE_KEY_MAP: /* ������Ϣ�б� */
            count = sizeof(DEV_PROFILE_KEYS) / sizeof(DEV_PROFILE_KEYS[0]);
            retAddr = (void*)DEV_PROFILE_KEYS;
            break;
        case TYPE_MSG_TABLE: /* ��������ӳ��� */
            count = sizeof(DEV_PROFILE_VALS) / sizeof(DEV_PROFILE_VALS[0]);
            retAddr = (void*)DEV_PROFILE_VALS;
            break;
        default:
            return NULL;
    }

    *size = count;
    return retAddr;
}

static void HiLinkSetLocalNetStatus(ModuleNetStatus netStatus)
{
    g_localNetStatus = netStatus;
}

ModuleNetStatus HiLinkGetLocalNetStatus(void)
{
    return g_localNetStatus;
}

static void HiLinkSetLocalWorkMode(ModuleWorkMode workMode)
{
    g_localWorkMode = workMode;
}

static ModuleWorkMode HiLinkGetLocalWorkMode(void)
{
    return g_localWorkMode;
}

/* ��ȡȫ�����ݷ��ͻ��� */
static unsigned char* HiLinkGetDataSendBuf(void)
{
    return g_tmpSendBuf;
}

/* ��ȡȫ����չ���ݷ��ͻ��� */
static unsigned char* HiLinkGetExtendBuf(void)
{
    return g_tmpExtendBuf;
}

/* ���õ�ǰ���ϱ���ʼֵ�ķ����±� */
static void HiLinkSetCurInitSvcIndex(unsigned short idx)
{
    g_initSvcIndex = idx;
}

/* ��ȡ��ǰ���ϱ���ʼֵ�����±� */
static unsigned short HiLinkGetCurInitSvcIndex(void)
{
    return g_initSvcIndex;
}

/* ��ȡ���ݷ�����ʱtlv���� */
static HiLinkTlvType* HiLinkGetTmpTlvArray(void)
{
    return g_tlvArray;
}

/* ��¼��ǰ�ȴ��ظ�ACK����ϢSeq */
void HiLinkSetCurWaitAckSeq(unsigned char seq)
{
    g_waitAckSeq = seq;
}

/* ��ȡ��ǰ�ȴ��ظ�ACK����ϢSeq */
static unsigned char HiLinkGetCurWaitAckSeq(void)
{
    return g_waitAckSeq;
}

/*
 * ����������ִ��״̬.
 * ״̬��Ϊ����״̬�͵ȴ�ACK�ظ�״̬.
 */
static void HiLinkSetFuncProcState(HiLinkCmdState state)
{
    if (state <= FUNC_PROC_STATE_MAX) {
        g_hiLinkFuncState = state;
    }
}

/* ��ȡ��ǰ������ִ��״̬ */
static HiLinkCmdState HiLinkGetFuncProcState(void)
{
    return g_hiLinkFuncState;
}

/* ���ó�ʼ����ֵ�ϱ�״̬ */
static void HiLinkSetInitValsReportState(HiLinkRptState state)
{
    g_initValReportState = state;
}

/* ��ȡ��ǰ��ʼ����ֵ�ϱ�״̬ */
static HiLinkRptState HiLinkGetInitValsReportState(void)
{
    return g_initValReportState;
}

/*
 * �޸ĵ�ǰ�������б�ִ�к����±�, funcMatrix��ʾ���޸ĵĺ����б�.
 * ����HLK_RET_OK���±��1, ����HLK_RET_NEXTSTAGE���±��0��ִ�е���һ��.
 */
static short HiLinkChangeMatrixFuncIndex(HiLinkFuncMatrix* funcMatrix)
{
    if (funcMatrix != NULL) {
        funcMatrix->curFuncIdx++;
        if (funcMatrix->curFuncIdx == funcMatrix->funcNum) {
            funcMatrix->curFuncIdx = 0;
            return HLK_RET_NEXTSTAGE;
        }
        return HLK_RET_OK;
    }
    return HLK_RET_ERR;
}

/* ��ȡ��ǰ�������б�ִ�к����±� */
static short HiLinkGetCurMatrixFuncIndex(const HiLinkFuncMatrix* funcMatrix)
{
    if (funcMatrix != NULL) {
        return (short)(funcMatrix->curFuncIdx);
    }
    return HLK_RET_ERR;
}

/* ���ÿ�������״̬�µ�ǰ����ĺ����±� */
static void HiLinkSetCurMatrixFuncIndex(unsigned short idx)
{
    g_functionMatrix[HLK_RUN_STATE_IDLE].curFuncIdx = idx;
}

static void HiLinkSetCurRunState(HiLinkRunState state)
{
    if (state < HLK_RUN_STATE_MAX) {
        g_hiLinkRunState = state;
    }
}

static HiLinkRunState HiLinkGetCurRunState(void)
{
    return g_hiLinkRunState;
}

/*
 * ���rcvFlag��ʾ���ݽ��ձ��(�Ƿ���յ�����).
 * ����noDataFlag��ʾ�Ƿ�ʱ��δ���յ����ݱ��.
 * ����ʱ��û�����ݽ���ʱ, MCU��Ҫ��ģ�鷢�Ͳ�ѯ����״̬����.
 */
static void HiLinkGetNoDataFlag(short rcvFlag, unsigned char* noDataFlag)
{
    /* unsigned long long�������ֵ���㳡��Ҫ��, ���ᷴת */
    static unsigned long long lastReceiveTick = 0;
    unsigned long long curTick = HiLinkGetSysCurTime();

    if (rcvFlag == HISLIP_RCV_NO_DATA) {
        if (lastReceiveTick == 0) {
            /* ��¼���һ�ν��յ����ݰ���ʱ�� */
            lastReceiveTick = curTick;
        }
        /* ��ʱ��û�����ݰ����� */
        if ((curTick > lastReceiveTick) && ((curTick - lastReceiveTick) >= HISLIP_NO_MSG_TIMEOUT)) {
            if (noDataFlag != NULL) {
                *noDataFlag = 1;
            }
            lastReceiveTick = 0;
        }
    } else {
        lastReceiveTick = 0;
    }
}

/* ��ȡ����ֵ��Ӧ���������� */
static HiLinkDataType HiLinkGetKeyDataType(unsigned short keyMapId)
{
    unsigned short keyNum = 0;
    KeyInfo* keys = (KeyInfo*)HiLinkGetProfileInfoList(TYPE_KEY_MAP, &keyNum);
    if (keys == NULL) {
        return HLK_DATA_TYPE_UNKNOWN;
    }

    unsigned short i;
    for (i = 0; i < keyNum; i++) {
        if (keys[i].keyMapId == keyMapId) {
            return keys[i].dataType;
        }
    }

    return HLK_DATA_TYPE_UNKNOWN;
}

/*
 * ��䷢����Ϣ����.
 * outData��ʾ������ݱ���, outLen��ʾ������ݱ��ĳ���.
 */
static int HiLinkGeneralFillCmdPkg(unsigned short cmd, const HiLinkTlvType* tlvs,
    unsigned int tlvNum, unsigned char* outData, unsigned short* outLen)
{
    if ((outData == NULL) ||
        (outLen == NULL) ||
        ((tlvs != NULL) && (tlvNum == 0)) ||
        ((tlvs == NULL) && (tlvNum != 0))) {
        return HLK_RET_ERR;
    }

    unsigned char* msg = outData;
    unsigned short readLen = 0;

    /* ���cmd�ֶ� */
    HiSlipFillDataEA(cmd, &readLen, msg);
    unsigned short sendLen = readLen;
    msg += readLen;

    if ((tlvs == NULL) && (tlvNum == 0)) {
        *outLen = sendLen;
        return HLK_RET_OK;
    }

    /* ���ָ���п�, ����pclint�澯 */
    if (tlvs == NULL) {
        return HLK_RET_ERR;
    }

    /* ���tlv���� */
    unsigned int i;
    for (i = 0; i < tlvNum; i++) {
        /* ���tag�ֶ� */
        if (tlvs[i].tag >= HLK_INVALID_VAL) {
            HiLinkNotifyErrorInfo(HLK_ECODE_PROFILE_TAG_ERR);
            return HLK_RET_ERR;
        }
        HiSlipFillDataEA(tlvs[i].tag, &readLen, msg);
        sendLen += readLen;
        msg += readLen;

        /* ���len�ֶ� */
        if (tlvs[i].len >= HLK_INVALID_VAL) {
            HiLinkNotifyErrorInfo(HLK_ECODE_PROFILE_LEN_ERR);
            return HLK_RET_ERR;
        }
        HiSlipFillDataEA(tlvs[i].len, &readLen, msg);
        sendLen += readLen;
        msg += readLen;

        /* ���val�ֶ� */
        unsigned char* val = tlvs[i].val;
        unsigned short len = tlvs[i].len;
        if ((len > 0) && (val != NULL)) {
            if ((sendLen + len) > HLK_SND_BUF_SIZE) {
                return HLK_RET_ERR;
            }
            unsigned short j;
            for (j = 0; j < len; j++) {
                *(msg + j) = *(val + j);
            }
            msg += len;
            sendLen += len;
        }
    }

    *outLen = sendLen;
    return HLK_RET_OK;
}

/*
 * ͨ������ͽӿ�.
 * ackSeq��ʾACK���к�.
 */
void HiLinkGeneralCmdSend(unsigned short cmd, const HiLinkTlvType* tlvs,
    unsigned short tlvNum, unsigned char ackSeq)
{
    unsigned short dataLen = 0;
    unsigned char* tmpBuf = HiLinkGetDataSendBuf();
    if (tmpBuf == NULL) {
        return;
    }

    /* tlvsΪNULL�ǺϷ�ֵ */
    int ret = HiLinkGeneralFillCmdPkg(cmd, tlvs, tlvNum, tmpBuf, &dataLen);
    if (ret == HLK_RET_OK) {
        if (ackSeq == 0) {
            HiSlipSendData(tmpBuf, dataLen, HISLIP_CMD_TYPE);
        } else {
            HiSlipSendData(tmpBuf, dataLen, ackSeq);
        }
    }
}

/* ��������Ϣ�б�tlv */
static int HiLinkFillSvcMapTlvs(unsigned short* cnt, HiLinkTlvType* tlvs)
{
    unsigned short i;
    unsigned short svcNum = 0;
    SvcInfo* svcList = (SvcInfo*)HiLinkGetProfileInfoList(TYPE_SVC_MAP, &svcNum);
    unsigned char* valBuf = HiLinkGetExtendBuf();
    if ((cnt == NULL) || (tlvs == NULL) || (svcList == NULL) || (valBuf == NULL)) {
        return HLK_RET_ERR;
    }

    /* �������չ������� */
    for (i = 0; i < HLK_EXT_BUF_SIZE; i++) {
        *(valBuf + i) = 0;
    }

    if ((svcNum > HLK_MAX_SVCINST_CNT) || ((svcNum * SVC_INFO_ITEM_NUM) > HLK_MAX_TLVS_CNT)) {
        HiLinkNotifyErrorInfo(HLK_ECODE_TLVARRAY_SIZE_SVCSMALL);
        return HLK_RET_ERR;
    }

    int offset = 0;
    unsigned short tagLen = 0;
    for (i = 0; i < svcNum; i++) {
        if ((svcList[i].svcMapId >= HLK_INVALID_VAL) ||
            (svcList[i].svcType == NULL) ||
            (svcList[i].svcId == NULL)) {
            HiLinkNotifyErrorInfo(HLK_ECODE_PROFILE_SVCINST_ERR);
            return HLK_RET_ERR;
        }

        char idxOffset = 0;

        /* ��1���ֶ�svcMapId */
        tlvs[i * SVC_INFO_ITEM_NUM].tag = HLK_TAG_PROT_SVCINF;
        HiSlipFillDataEA(svcList[i].svcMapId, &tagLen, (valBuf + offset));
        tlvs[i * SVC_INFO_ITEM_NUM].len = tagLen;
        tlvs[i * SVC_INFO_ITEM_NUM].val = valBuf + offset;
        idxOffset++;
        offset += tagLen;

        /* ��2���ֶ�svcType */
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].tag = HLK_TAG_PROT_SVCINF;
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].len = strlen(svcList[i].svcType);
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].val = (unsigned char*)svcList[i].svcType;
        idxOffset++;

        /* ��3���ֶ�svcId */
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].tag = HLK_TAG_PROT_SVCINF;
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].len = strlen(svcList[i].svcId);
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].val = (unsigned char*)svcList[i].svcId;
    }

    *cnt = svcNum * SVC_INFO_ITEM_NUM;
    return HLK_RET_OK;
}

/* ���������Ϣ�б�tlv */
static int HiLinkFillKeyMapTlvs(unsigned short* cnt, HiLinkTlvType* tlvs)
{
    unsigned short i;
    unsigned short keyNum = 0;
    KeyInfo* keys = (KeyInfo*)HiLinkGetProfileInfoList(TYPE_KEY_MAP, &keyNum);
    unsigned char* valBuf = HiLinkGetExtendBuf();
    if ((cnt == NULL) || (tlvs == NULL) || (keys == NULL) || (valBuf == NULL)) {
        return HLK_RET_ERR;
    }

    /* �������չ������� */
    for (i = 0; i < HLK_EXT_BUF_SIZE; i++) {
        *(valBuf + i) = 0;
    }

    if ((keyNum > HLK_MAX_KEYMAP_CNT) || ((keyNum * KEY_INFO_ITEM_NUM) > HLK_MAX_TLVS_CNT)) {
        HiLinkNotifyErrorInfo(HLK_ECODE_TLVARRAY_SIZE_KEYSMALL);
        return HLK_RET_ERR;
    }

    int offset = 0;
    unsigned short tagLen = 0;
    for (i = 0; i < keyNum; i++) {
        if ((keys[i].keyMapId >= HLK_INVALID_VAL) ||
            (keys[i].svcType == NULL) ||
            (keys[i].keyName == NULL)) {
            HiLinkNotifyErrorInfo(HLK_ECODE_PROFILE_KEYMAP_ERR);
            return HLK_RET_ERR;
        }

        char idxOffset = 0;

        /* ��1���ֶ�keyMapId */
        tlvs[i * KEY_INFO_ITEM_NUM].tag = HLK_TAG_PROT_ITEM;
        HiSlipFillDataEA(keys[i].keyMapId, &tagLen, (valBuf + offset));
        tlvs[i * KEY_INFO_ITEM_NUM].len = tagLen;
        tlvs[i * KEY_INFO_ITEM_NUM].val = valBuf + offset;
        idxOffset++;
        offset += tagLen;

        /* ��2���ֶ�svcType */
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].tag = HLK_TAG_PROT_ITEM;
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].len = strlen(keys[i].svcType);
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].val = (unsigned char*)keys[i].svcType;
        idxOffset++;

        /* ��3���ֶ�keyName */
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].tag = HLK_TAG_PROT_ITEM;
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].len = strlen(keys[i].keyName);
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].val = (unsigned char*)keys[i].keyName;
        idxOffset++;

        /* ��4���ֶ�dataType */
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].tag = HLK_TAG_PROTKIND;
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].len = 1;
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].val = (unsigned char*)&keys[i].dataType;

    }

    *cnt = keyNum * KEY_INFO_ITEM_NUM;
    return HLK_RET_OK;
}

static void HiLinkIntToCharArray(int val, int len, unsigned char* buf)
{
    if ((buf == NULL) || (len < UNSIGNED_INT_BYTES)) {
        return;
    }

    unsigned int value = (unsigned int)val;
    unsigned char idx = 0;
    buf[idx] = (unsigned char)((value >> ((UNSIGNED_INT_BYTES - 1) * UNSIGNED_CHAR_BITS)) & LOW_BYTE_FLAG);
    idx++;
    buf[idx] = (unsigned char)((value >> (((UNSIGNED_INT_BYTES - idx) - 1) * UNSIGNED_CHAR_BITS)) & LOW_BYTE_FLAG);
    idx++;
    buf[idx] = (unsigned char)((value >> UNSIGNED_CHAR_BITS) & LOW_BYTE_FLAG);
    idx++;
    buf[idx] = (unsigned char)(value & LOW_BYTE_FLAG);
}

static void HiLinkCharArrayToInt(const unsigned char* buf, int len, int* value)
{
    if ((buf == NULL) || (value == NULL) || (len < UNSIGNED_INT_BYTES)) {
        return;
    }

    unsigned char idx = 0;
    int val = buf[idx] << ((UNSIGNED_INT_BYTES - 1) * UNSIGNED_CHAR_BITS);
    idx++;
    val += buf[idx] << (((UNSIGNED_INT_BYTES - idx) - 1) * UNSIGNED_CHAR_BITS);
    idx++;
    val += buf[idx] << UNSIGNED_CHAR_BITS;
    idx++;
    val += buf[idx];
    *value = val;
}

/*
 * �����������tlv�б�.
 * ����tlvs����������tlv�б�, keyNum��ʾ������Ŀ.
 */
static int HiLinkFillSvcTlvs(unsigned short svcMapId, unsigned short keyMapId,
    HiLinkTlvType* tlvs, unsigned short* keyNum, unsigned short* dataLen)
{
    unsigned short cnt = 0;
    unsigned char* valBuf = HiLinkGetExtendBuf();
    HiLinkMsg* keyVals = (HiLinkMsg*)HiLinkGetProfileInfoList(TYPE_MSG_TABLE, &cnt);

    if ((tlvs == NULL) || (keyNum == NULL) || (dataLen == NULL) || (keyVals == NULL) || (valBuf == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short num = 0;
    unsigned short readLen = 0;
    unsigned short outLen = 0;
    int offset = 0;
    unsigned short i;

    for (i = 0; i < cnt; i++) {
        /* keyMapIdΪinvalidֵ��ʾ���ģ��GET������ϱ�ȫ����������ֵ�ķ��ر��� */
        if ((keyVals[i].msgInfo.svcMapId != svcMapId) ||
            ((keyVals[i].msgInfo.keyMapId != keyMapId) && (keyMapId != HLK_INVALID_VAL))) {
            continue;
        }

        tlvs[num].tag = keyVals[i].msgInfo.keyMapId; /* Tag�ֶ� */
        HiSlipFillDataEA(tlvs[num].tag, &readLen, NULL);
        outLen += readLen;

        HiLinkDataType dataType = HiLinkGetKeyDataType(keyVals[i].msgInfo.keyMapId);
        switch (dataType) {
            case HLK_DATA_TYPE_INT:
            case HLK_DATA_TYPE_BOOL:
                if (offset >= HLK_EXT_BUF_SIZE) {
                    HiLinkNotifyErrorInfo(HLK_ECODE_EXT_BUF_SIZE_SMALL);
                    return HLK_RET_ERR;
                }
                tlvs[num].len = sizeof(int); /* Length�ֶ� */
                HiLinkIntToCharArray(keyVals[i].msgInfo.value, (HLK_EXT_BUF_SIZE - offset), &valBuf[offset]);
                tlvs[num].val = valBuf + offset; /* Value�ֶ� */
                offset += UNSIGNED_INT_BYTES; /* int������ռ�ֽ��� */
                break;
            case HLK_DATA_TYPE_STR:
                tlvs[num].len = strlen((char*)((long)(keyVals[i].msgInfo.value))); /* Length�ֶ� */
                tlvs[num].val = (unsigned char*)((long)(keyVals[i].msgInfo.value)); /* Value�ֶ� */
                break;
            default:
                HiLinkNotifyErrorInfo(HLK_ECODE_KEYMAP_DATA_TYPE_ERR);
                return HLK_RET_ERR;
        }

        HiSlipFillDataEA(tlvs[num].len, &readLen, NULL);
        outLen += (readLen + tlvs[num].len);
        num++;
        if (num > HLK_MAX_TLVS_CNT) {
            return HLK_RET_ERR;
        }

        if (keyMapId != HLK_INVALID_VAL) {
            break;
        }
    }

    *keyNum = num;
    *dataLen = outLen;
    return HLK_RET_OK;
}

/*
 * ����tlv�ṹ��ͨ�ýӿ�.
 * ���ط�NULL��ʾ��һ��tlv��ʼλ��, ����NULL��ʾ��������.
 */
static unsigned char* HiLinkGeneralParseTlv(unsigned char* inBuf, HiLinkTlvType* tlv)
{
    if (inBuf == NULL) {
        return NULL;
    }

    unsigned short readLen = 0;
    unsigned char* val = NULL;
    unsigned char* msg = inBuf;
    short tag = HiSlipParseDataEA(msg, &readLen);
    if (tag == HISLIP_ERR) {
        HiLinkNotifyErrorInfo(HLK_ECODE_RCV_TAG_ERR);
        return NULL;
    }
    msg += readLen;
    readLen = 0;

    short len = HiSlipParseDataEA(msg, &readLen);
    if (len == HISLIP_ERR) {
        HiLinkNotifyErrorInfo(HLK_ECODE_RCV_LEN_ERR);
        return NULL;
    }
    msg += readLen;
    if (len > 0) {
        val = msg;
        msg += len;
    }

    if (tlv != NULL) {
        tlv->tag = (unsigned short)tag;
        tlv->len = (unsigned short)len;
        tlv->val = val;
    }

    /* ������һ��TLV��ʼ��ַ */
    return msg;
}

/* ����: ������Ϣ�ϱ������������ֵ */
static void HiLinkReportSvcKeyVals(unsigned short svcMapId, unsigned short keyMapId)
{
    HiLinkTlvType* msg = HiLinkGetTmpTlvArray();
    if (msg == NULL) {
        return;
    }

    unsigned short keyNum = 0;
    unsigned short dataLen = 0;
    /* 1��ʾ�ӵ�2��TLV��ʼ���������Ϣ */
    int ret = HiLinkFillSvcTlvs(svcMapId, keyMapId, &msg[1], &keyNum, &dataLen);
    if (ret == HLK_RET_OK) {
        /* 0��ʾ��1��TLV������ӳ��ID�ֶ� */
        msg[0].tag = svcMapId;
        msg[0].len = dataLen;
        msg[0].val = NULL;
        HiLinkGeneralCmdSend(HLK_CMD_DEVUPD_STA, msg, (keyNum + 1), HISLIP_CMD_TYPE);
    }
}

/* �����±�����, �ϱ��仯������ֵ */
static void HiLinkReportChangedKeyVals(unsigned short idx)
{
    unsigned short cnt = 0;
    HiLinkMsg* msg = (HiLinkMsg*)HiLinkGetProfileInfoList(TYPE_MSG_TABLE, &cnt);
    if ((msg == NULL) || (idx >= cnt)) {
        return;
    }

    HiLinkReportSvcKeyVals(msg[idx].msgInfo.svcMapId, msg[idx].msgInfo.keyMapId);
}

/*
 * ����ȴ�����ACK��Ӧ.
 * ����startTick��ʾ��ʼ�ȴ�ACK��ʱ�����ֵ.
 * ����ֵACK_STATE_WAITTING��ʾ���ڵȴ�ACK,
 * ACK_STATE_ACKED��ʾ���յ�ACK, ACK_STATE_ACKTIMOUT��ʾ�ȴ�ACK��ʱ.
 */
static short HiLinkCommonWaitAck(unsigned short cmd, unsigned long long startTick)
{
    unsigned long long curTick = HiLinkGetSysCurTime();
    if ((curTick > startTick) && ((curTick - startTick) >= ACK_TIMEOUT_VALUE)) {
        return ACK_STATE_ACKTIMOUT;
    }

    LastAckInfo lastAck = { 0, 0 };
    HiSlipGetLastAckInfo(&lastAck);
    unsigned char seq = HiLinkGetCurWaitAckSeq();
    if ((lastAck.cmd == cmd) && (lastAck.seq == seq)) {
        return ACK_STATE_ACKED;
    }

    return ACK_STATE_WAITTING;
}

static short HiLinkReqGetWorkMode(void)
{
    /* ��ѯ����ģʽ, û��payload */
    HiLinkGeneralCmdSend(HLK_CMD_DEVCTL_QWM, NULL, 0, HISLIP_CMD_TYPE);
    return HLK_RET_OK;
}

static short HiLinkReqGetNetStatus(void)
{
    /* ��ѯ����״̬, û��payload */
    HiLinkGeneralCmdSend(HLK_CMD_DEVCTL_QNS, NULL, 0, HISLIP_CMD_TYPE);
    return HLK_RET_OK;
}

static short HiLinkReqRegStart(void)
{
    HiLinkTlvType msg[REG_FLAG_ITEM_NUM + PROFILE_VER_ITEM_NUM]; /* �豸ע��״̬��Э��汾�� */
    ProfileRegStatus regStatus = HLK_REG_START;

    unsigned char* profileVer = (unsigned char*)HiLinkGetProfileVersion();
    if (profileVer == NULL) {
        return HLK_RET_ERR;
    }

    /* 0��ʾ��1���ֶ�: �豸ע��״̬(��ʼ) */
    msg[0].tag = HLK_TAG_REGSTA;
    msg[0].len = HISLIP_PARAM_VAL_LEN;
    msg[0].val = (unsigned char*)&regStatus;

    /* 1��ʾ��2���ֶ�: profileVer */
    msg[1].tag = HLK_TAG_PROFILE_VER;
    msg[1].len = strlen((const char*)profileVer);
    msg[1].val = profileVer;

    /* �豸ע��״̬��Э��汾������TLV�ֶ� */
    HiLinkGeneralCmdSend(HLK_CMD_DEVREG_SETSTA, msg, (REG_FLAG_ITEM_NUM + PROFILE_VER_ITEM_NUM), HISLIP_CMD_TYPE);
    return HLK_RET_OK;
}

static short HiLinkReqRegAcBi(void)
{
    HiLinkTlvType msg[DEV_AC_ITEM_NUM + DEV_BI_ITEM_NUM];
    unsigned char* bi = (unsigned char*)HiLinkGetDeviceBi();
    unsigned char* ac = (unsigned char*)HiLinkGetDeviceAc();

    if ((bi == NULL) || (ac == NULL)) {
        return HLK_RET_ERR;
    }

    /* 0��ʾ��1���ֶ�: BI */
    msg[0].tag = HLK_TAG_PROT_BIRSA;
    msg[0].len = strlen((const char*)bi);
    msg[0].val = bi;

    /* 1��ʾ��2���ֶ�: AC */
    msg[1].tag = HLK_TAG_PROT_AC;
    msg[1].len = HLK_AC_LEN;
    msg[1].val = ac;

    /* TLV�������豸AC��Ϣ���豸BI��Ϣ����TLV�ֶ� */
    HiLinkGeneralCmdSend(HLK_CMD_DEVREG_DEVINFO, msg, (DEV_AC_ITEM_NUM + DEV_BI_ITEM_NUM), HISLIP_CMD_TYPE);
    return HLK_RET_OK;
}

static short HiLinkReqRegDevInfo(void)
{
    HiLinkTlvType msg[DEV_INFO_ITEM_NUM + DEV_NAME_ITEM_NUM];
    char devSn[DEV_SN_MAX_LEN] = {0};
    char msgIdx = 0; /* ��ϢTLV�����±� */
    DevInfo* devInfo = (DevInfo*)HiLinkGetDeviceInfo();
    if (devInfo == NULL) {
        return HLK_RET_ERR;
    }
    DevEnName* devNameEn = (DevEnName*)HiLinkGetDeviceEnName();
    if (devNameEn == NULL) {
        return HLK_RET_ERR;
    }

    /* ��1���ֶ�: SN */
    msg[msgIdx].tag = HLK_TAG_PROT_DEVSN;
    /* devInfo->sn��ʼ��Ϊ���ַ���"", ����ΪNULL */
    if (strlen(devInfo->sn) == 0) {
        /* profile��δ����snʱ���ṩ�����̵Ľӿ��л�ȡ */
        HiLinkGetDeviceSn(DEV_SN_MAX_LEN, devSn);
        msg[msgIdx].len = strlen(devSn);
        msg[msgIdx].val = (unsigned char *)devSn;
    } else {
        msg[msgIdx].len = strlen(devInfo->sn);
        msg[msgIdx].val = (unsigned char *)devInfo->sn;
    }
    msgIdx++;

    /* ��2���ֶ�: prodId */
    msg[msgIdx].tag = HLK_TAG_PROT_PRDTID;
    msg[msgIdx].len = strlen(devInfo->prodId);
    msg[msgIdx].val = (unsigned char*)devInfo->prodId;
    msgIdx++;

    /* ��3���ֶ�: devModel */
    msg[msgIdx].tag = HLK_TAG_PROT_PRDTMD;
    msg[msgIdx].len = strlen(devInfo->devModel);
    msg[msgIdx].val = (unsigned char*)devInfo->devModel;
    msgIdx++;

    /* ��4���ֶ�: devTypeId */
    msg[msgIdx].tag = HLK_TAG_PROT_PRDTTP;
    msg[msgIdx].len = strlen(devInfo->devTypeId);
    msg[msgIdx].val = (unsigned char*)devInfo->devTypeId;
    msgIdx++;

    /* ��5���ֶ�: manuId */
    msg[msgIdx].tag = HLK_TAG_PROT_MANU;
    msg[msgIdx].len = strlen(devInfo->manuId);
    msg[msgIdx].val = (unsigned char*)devInfo->manuId;
    msgIdx++;

    /* ��6���ֶ�: mcuVer */
    msg[msgIdx].tag = HLK_TAG_DEV_SVER;
    msg[msgIdx].len = strlen(devInfo->mcuVer);
    msg[msgIdx].val = (unsigned char*)devInfo->mcuVer;
    msgIdx++;

    /* ��7���ֶ�: devTypeName */
    msg[msgIdx].tag = HLK_TAG_PROT_DTENN;
    msg[msgIdx].len = strlen(devNameEn->devTypeName);
    msg[msgIdx].val = (unsigned char*)devNameEn->devTypeName;
    msgIdx++;

    /* ��8���ֶ�: manuName */
    msg[msgIdx].tag = HLK_TAG_PROT_MAUNENN;
    msg[msgIdx].len = strlen(devNameEn->manuName);
    msg[msgIdx].val = (unsigned char*)devNameEn->manuName;

    /* TLV�������豸��Ϣ�ֶ������豸������Ϣ�ֶ���֮�� */
    HiLinkGeneralCmdSend(HLK_CMD_DEVREG_DEVINFO, msg, (DEV_INFO_ITEM_NUM + DEV_NAME_ITEM_NUM), HISLIP_CMD_TYPE);
    return HLK_RET_OK;
}

static short HiLinkReqRegSvcInfo(void)
{
    HiLinkTlvType* serviceInfo = HiLinkGetTmpTlvArray();
    if (serviceInfo == NULL) {
        return HLK_RET_ERR;
    }

    unsigned short itemNum = 0;
    int ret = HiLinkFillSvcMapTlvs(&itemNum, serviceInfo);
    if (ret == HLK_RET_OK) {
        HiLinkGeneralCmdSend(HLK_CMD_DEVREG_SVCINFO, serviceInfo, itemNum, HISLIP_CMD_TYPE);
        return HLK_RET_OK;
    }
    return HLK_RET_ERR;
}

static short HiLinkReqRegKeyMap(void)
{
    HiLinkTlvType* msg = HiLinkGetTmpTlvArray();
    if (msg == NULL) {
        return HLK_RET_ERR;
    }

    unsigned short itemNum = 0;
    int ret = HiLinkFillKeyMapTlvs(&itemNum, msg);
    if (ret == HLK_RET_OK) {
        HiLinkGeneralCmdSend(HLK_CMD_DEVREG_PROT, msg, itemNum, HISLIP_CMD_TYPE);
        return HLK_RET_OK;
    }
    return HLK_RET_ERR;
}

static short HiLinkReqRegEnd(void)
{
    /* ��Ϣֻ��1��TLV�ֶ�, �豸ע��״̬(����) */
    ProfileRegStatus regStatus = HLK_REG_END;
    HiLinkTlvType msg;
    msg.tag = HLK_TAG_REGSTA;
    msg.len = 1;
    msg.val = (unsigned char*)&regStatus;
    HiLinkGeneralCmdSend(HLK_CMD_DEVREG_SETSTA, &msg, REG_FLAG_ITEM_NUM, HISLIP_CMD_TYPE);
    return HLK_RET_OK;
}

static short HiLinkSetWorkMode(void)
{
    unsigned char workMode = 0;
    ModuleNetStatus netStatus = HiLinkGetLocalNetStatus();
    switch (netStatus) {
        case NET_NOAP:
            workMode = (unsigned char)WKMD_AUTOAP;
            break;
        case NET_NOTCONN:
            workMode = (unsigned char)WKMD_ONLINE;
            break;
        default:
            workMode = (unsigned char)HiLinkGetLocalWorkMode();
            break;
    }

    HiLinkTlvType msg;
    msg.tag = HLK_TAG_WK_MD;
    msg.len = HISLIP_PARAM_VAL_LEN;
    msg.val = &workMode;
    HiLinkGeneralCmdSend(HLK_CMD_DEVCTL_WM, &msg, SET_WM_TLV_NUM, HISLIP_CMD_TYPE);
    return HLK_RET_OK;
}

/*
 * �ϱ��������Գ�ʼֵ.
 * ���з���ֵHLK_RET_NEXTSTAGE��ʾϵͳ״̬���뵽��һ�׶�.
 */
static short HiLinkReportInitVals(void)
{
    unsigned short itemNum = 0;
    unsigned short idx = HiLinkGetCurInitSvcIndex();
    SvcInfo* services = (SvcInfo*)HiLinkGetProfileInfoList(TYPE_SVC_MAP, &itemNum);
    if (services == NULL) {
        return HLK_RET_ERR;
    }
    if (idx == itemNum) {
        HiLinkSetInitValsReportState(HLK_RPT_DONE);
        return HLK_RET_NEXTSTAGE;
    }

    /* �ϱ�������������ֵ */
    HiLinkReportSvcKeyVals(services[idx].svcMapId, HLK_INVALID_VAL);
    HiLinkSetCurInitSvcIndex(idx + 1); /* ��һ������ */
    return HLK_RET_OK;
}

/* �����ϱ��仯�ķ�������ֵ */
static short HiLinkReportProfileVals(void)
{
    unsigned short msgNum = 0;
    unsigned short svcNum = 0;
    unsigned short i;

    if (HiLinkHasKeyValChanged(RPT_BY_KEY) == 1) {
        void* tmp = HiLinkGetProfileInfoList(TYPE_MSG_TABLE, &msgNum);
        if (tmp == NULL) {
            return HLK_RET_ERR;
        }
        for (i = 0; i < msgNum; i++) {
            /* �ϱ��仯������ */
            if (HiLinkCheckChangedKeyVal(RPT_BY_KEY, i, DEV_KEY_MASK_USET) == 1) {
                HiLinkReportChangedKeyVals(i);
                return HLK_RET_OK;
            }
        }
    }

    if (HiLinkHasKeyValChanged(RPT_BY_SVC) == 1) {
        SvcInfo* services = (SvcInfo*)HiLinkGetProfileInfoList(TYPE_SVC_MAP, &svcNum);
        if (services == NULL) {
            return HLK_RET_ERR;
        }
        for (i = 0; i < svcNum; i++) {
            /* �ϱ��б仯�ķ������������ */
            if (HiLinkCheckChangedKeyVal(RPT_BY_SVC, i, DEV_KEY_MASK_USET) == 1) {
                HiLinkReportSvcKeyVals(services[i].svcMapId, HLK_INVALID_VAL);
                return HLK_RET_OK;
            }
        }
    }

    return HLK_RET_ERR;
}

/* ��������״̬���, ������ʾ��ǰ�׶�ִ�к������±� */
static short HiLinkProcessStageFunc(const HiLinkReqFuncs* stageFunc, unsigned short idx)
{
    short ret = HLK_RET_OK;
    static unsigned long long curTick = 0;

    if (stageFunc == NULL) {
        return HLK_RET_ERR;
    }

    HiLinkCmdState cmdState = HiLinkGetFuncProcState();
    if (cmdState == FUNC_PROC_STATE_SNDCMD) {
        ret = stageFunc[idx].sendReqFunc();
        if (ret == HLK_RET_NEXTSTAGE) {
            ret = HLK_RET_OK;
        } else if (ret == HLK_RET_OK) {
            HiLinkSetFuncProcState(FUNC_PROC_STATE_WACK);
            ret = HLK_RET_WACK;
            curTick = HiLinkGetSysCurTime();
        } else {
            ret = HLK_RET_SFAIL;
        }
    } else if (cmdState == FUNC_PROC_STATE_WACK) {
        ret = stageFunc[idx].processAckState(stageFunc[idx].cmd, curTick);
        switch (ret) {
            case ACK_STATE_WAITTING:
                ret = HLK_RET_WACK;
                break;
            case ACK_STATE_ACKED:
                ret = HLK_RET_OK;
                HiLinkSetFuncProcState(FUNC_PROC_STATE_SNDCMD);
                break;
            case ACK_STATE_ACKTIMOUT:
                ret = HLK_RET_ACK_TMOUT;
                HiLinkSetFuncProcState(FUNC_PROC_STATE_SNDCMD);
                break;
            default:
                ret = HLK_RET_ERR;
                HiLinkSetFuncProcState(FUNC_PROC_STATE_SNDCMD);
                break;
        }
    }

    return ret;
}

/* ����ϵͳ����״̬�ı�� */
static void HiLinkChangeRunState(ModuleWorkMode workMode, ModuleNetStatus netStatus)
{
    HiLinkRptState reportState = HLK_RPT_PROCESSING;
    HiLinkRunState runState = HiLinkGetCurRunState();
    bool isInitState = (workMode == WKMD_OFFLINE) && (netStatus == NET_UNKONWN);
    bool isOfflineState = (workMode == WKMD_OFFLINE) && ((netStatus == NET_NOAP) || (netStatus == NET_NOTCONN));
    bool isResetState = (workMode == WKMD_ONLINE) || (workMode == WKMD_AUTOAP);

    switch (runState) {
        case HLK_RUN_STATE_INIT:
            if (isInitState == true) {
                /* �л���ע��״̬ */
                HiLinkSetCurRunState(HLK_RUN_STATE_REGPROFILE);
                break;
            }
            if ((isOfflineState == true) || (isResetState == true)) {
                /* ģ������, �豸����, ��Ҫ�����ϱ���ʼֵ */
                HiLinkSetCurRunState(HLK_RUN_STATE_RPTINIVAL);
            }
            break;
        case HLK_RUN_STATE_REGPROFILE:
            if (isOfflineState == true) {
                HiLinkSetCurRunState(HLK_RUN_STATE_RPTINIVAL);
            }
            break;
        case HLK_RUN_STATE_SETWKMODE:
            HiLinkSetCurRunState(HLK_RUN_STATE_IDLE);
            break;
        case HLK_RUN_STATE_RPTINIVAL:
            /* ��ʼֵ�ϱ����, �л���idle״̬ */
            reportState = HiLinkGetInitValsReportState();
            if (reportState == HLK_RPT_DONE) {
                HiLinkSetInitValsReportState(HLK_RPT_PROCESSING);
                HiLinkSetCurRunState(HLK_RUN_STATE_SETWKMODE);
            }
            break;
        case HLK_RUN_STATE_IDLE:
            HiLinkSetCurRunState(HLK_RUN_STATE_INIT);
            break;
        default:
            break;
    }
}

/* ִ�в�ͬϵͳ״̬�µĴ����� */
static short HiLinkProcessMaxtrixFuncs(HiLinkFuncMatrix* funcMatrix)
{
    ModuleWorkMode workMode;
    ModuleNetStatus netStatus;

    if (funcMatrix == NULL) {
        return HLK_RET_ERR;
    }

    short funcIdx = HiLinkGetCurMatrixFuncIndex(funcMatrix);
    if ((funcIdx < 0) || (funcIdx >= funcMatrix->funcNum)) {
        return HLK_RET_ERR;
    }

    short ret = HiLinkProcessStageFunc(funcMatrix->reqFuncs, (unsigned short)funcIdx);
    if (ret == HLK_RET_OK) {
        ret = HiLinkChangeMatrixFuncIndex(funcMatrix);
        if (ret == HLK_RET_NEXTSTAGE) {
            workMode = HiLinkGetLocalWorkMode();
            netStatus = HiLinkGetLocalNetStatus();
            /* �л�����һ״̬ */
            HiLinkChangeRunState(workMode, netStatus);
        }
    }

    return ret;
}

/* ����IDLE״̬�Ĵ�����, ����rcvFlag��ʾ�Ƿ���յ���Ϣ�ı�� */
static short HiLinkProcessIdleState(HiLinkFuncMatrix* funcMatrix, short rcvFlag)
{
    short ret = HLK_RET_OK;
    unsigned char noDataFlag = 0;
    HiLinkGetNoDataFlag(rcvFlag, &noDataFlag);

    /* ��ʱ��δ���յ����ݰ� */
    if (noDataFlag != 0) {
        /* 1��ʾ��ѯģ������״̬ */
        HiLinkSetCurMatrixFuncIndex(1);
        ret = HiLinkProcessMaxtrixFuncs(funcMatrix);
        if (ret != HLK_RET_WACK) {
            noDataFlag = 0;
        }
    } else {
        /* 0��ʾ��������ֵ״̬ */
        HiLinkSetCurMatrixFuncIndex(0);
        ret = HiLinkProcessMaxtrixFuncs(funcMatrix);
    }

    return ret;
}

/* �����ڲ�״̬��, ����rcvFlag��ʾ�Ƿ���յ���Ϣ�ı�� */
static short HiLinkProcessInternalStates(short rcvFlag)
{
    short ret = HLK_RET_ERR;
    HiLinkRunState runState = HiLinkGetCurRunState();

    switch (runState) {
        case HLK_RUN_STATE_INIT:
        case HLK_RUN_STATE_REGPROFILE:
        case HLK_RUN_STATE_SETWKMODE:
        case HLK_RUN_STATE_RPTINIVAL:
            ret = HiLinkProcessMaxtrixFuncs(&g_functionMatrix[runState]);
            break;
        case HLK_RUN_STATE_IDLE:
            ret = HiLinkProcessIdleState(&g_functionMatrix[runState], rcvFlag);
            break;
        default:
            break;
    }

    return ret;
}

/* ����ģ���PUT������豸������Ϣ */
static void HiLinkPutSvcKeyVal(unsigned char* message, unsigned short len)
{
    if ((message == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    /* ��ȡ����mapid */
    HiLinkTlvType parseMsg = { 0, 0, NULL };
    unsigned char* next = HiLinkGeneralParseTlv(message, &parseMsg);
    if (next == NULL) {
        return;
    }
    unsigned short svcMapId = parseMsg.tag;
    next = parseMsg.val;

    /* ��ȡ����mapid */
    if (HiLinkGeneralParseTlv(next, &parseMsg) == NULL) {
        return;
    }
    unsigned short keyMapId = parseMsg.tag;

    /* ��ȡ��Ϣ���� */
    unsigned short msgNum = 0;
    HiLinkMsg* keyVals = (HiLinkMsg*)HiLinkGetProfileInfoList(TYPE_MSG_TABLE, &msgNum);
    if (keyVals == NULL) {
        return;
    }

    unsigned short i;
    for (i = 0; i < msgNum; i++) {
        if ((keyVals[i].msgInfo.keyMapId != keyMapId) || (keyVals[i].msgInfo.svcMapId != svcMapId)) {
            continue;
        }
        if (keyVals[i].msgFunc == NULL) {
            return;
        }
        HiLinkDataType dataType = HiLinkGetKeyDataType(keyMapId);
        bool isValueType = (dataType == HLK_DATA_TYPE_INT) || (dataType == HLK_DATA_TYPE_BOOL);
        if ((isValueType == true) && (parseMsg.len >= UNSIGNED_INT_BYTES)) {
            int value = 0;
            /* int������Ҫ�������ֽ����4���ֽ�����ת�������� */
            HiLinkCharArrayToInt(parseMsg.val, parseMsg.len, &value);
            keyVals[i].msgFunc(value);
        } else if ((dataType == HLK_DATA_TYPE_STR) && (parseMsg.len > 0) && (parseMsg.len <= STR_KEY_MAX_LEN)) {
            char* strAddr = (char*)malloc(parseMsg.len + 1);
            if (strAddr == NULL) {
                return;
            }
            /* malloc�ڴ������Ÿ�ֵ, �����ظ������ʼ�� */
            unsigned short j;
            for (j = 0; j < parseMsg.len; j++) {
                *(strAddr + j) = *(parseMsg.val + j);
            }
            *(strAddr + parseMsg.len) = 0;
            keyVals[i].msgFunc((int)(long)strAddr);
            free(strAddr);
        } else {
            return;
        }
        break;
    }
}

/* ����ģ���GET����ķ�������ֵ��ѯ��Ϣ */
static void HiLinkGetSvcKeyVals(unsigned char seq, unsigned char* message, unsigned short len)
{
    if ((message == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    unsigned short svcMapId = 0;
    unsigned short keyNum = 0;
    unsigned short dataLen = 0;
    HiLinkTlvType service = { 0, 0, NULL };
    unsigned char* next = HiLinkGeneralParseTlv(message, &service);
    if (next != NULL) {
        svcMapId = service.tag;
    }

    HiLinkTlvType* package = HiLinkGetTmpTlvArray();
    if (package == NULL) {
        return;
    }

    /* 1��ʾ�ӵ�2��TLV��ʼ���������Ϣ�б� */
    int ret = HiLinkFillSvcTlvs(svcMapId, HLK_INVALID_VAL, &package[1], &keyNum, &dataLen);
    if (ret == HLK_RET_OK) {
        /* 0��ʾ��1��TLV������ӳ��ID�ֶ� */
        package[0].tag = svcMapId;
        package[0].len = dataLen;
        package[0].val = NULL;
        HiLinkGeneralCmdSend(HLK_CMD_MDCTL_DEV_QPROT, package, (keyNum + 1), (seq | HISLIP_ACK_TYPE));
    }
}

/* �������������³�ʼ�� */
static void HiLinkFuncMatrixReinit(void)
{
    unsigned char i;
    for (i = 0; i < HLK_RUN_STATE_MAX; i++) {
        g_functionMatrix[i].curFuncIdx = 0;
    }
}

static void HiLinkUpdateWorkMode(const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    unsigned short readLen = 0;
    int offset = 0;
    short data = HiSlipParseDataEA(buf, &readLen);
    offset += readLen;
    switch (data) {
        case HLK_TAG_WK_MD:
            break;
        case HLK_TAG_OP_RLT:
            data = HiSlipParseDataEA(buf + offset, &readLen);
            offset += (readLen + data);
            data = HiSlipParseDataEA(buf + offset, &readLen);
            offset += readLen;
            if (data != HLK_TAG_WK_MD) {
                HiLinkNotifyErrorInfo(HLK_ECODE_RCV_WKMODE_ERR);
                return;
            }
            break;
        default:
            return;
    }

    data = HiSlipParseDataEA(buf + offset, &readLen);
    offset += readLen;
    if ((data != HISLIP_PARAM_VAL_LEN) || ((offset + HISLIP_PARAM_VAL_LEN) > len)) {
        return;
    }

    ModuleWorkMode workMode = (ModuleWorkMode)(*(buf + offset));
    switch (workMode) {
        case WKMD_OFFLINE:
        case WKMD_AUTOAP:
        case WKMD_ONLINE:
            HiLinkSetLocalWorkMode(workMode);
            break;
        case WKMD_UNKONWN:
        default:
            return;
    }

    if ((HiLinkGetLocalWorkMode() == WKMD_OFFLINE) && (HiLinkGetCurRunState() != HLK_RUN_STATE_INIT)) {
        HiLinkSetCurRunState(HLK_RUN_STATE_INIT);
        HiLinkSetFuncProcState(FUNC_PROC_STATE_SNDCMD);
        HiLinkSetCurInitSvcIndex(0);
        HiLinkSetInitValsReportState(HLK_RPT_PROCESSING);
        HiSlipUartInit();
        HiLinkFuncMatrixReinit();
    }
}

/*
 * ģ��֪ͨMCU�豸���ƶ�ɾ��ע��.
 * �豸ɾ���ɹ������HiLinkDevRemoved��������������.
 */
static void HiLinkFactoryResetResult(const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    unsigned short readLen = 0;
    int offset = 0;
    /* Tag�ֶ� */
    short data = HiSlipParseDataEA(buf, &readLen);
    offset += readLen;
    if ((data != HLK_TAG_OP_RLT) || (offset >= len)) {
        return;
    }

    /* Length�ֶ� */
    data = HiSlipParseDataEA((buf + offset), &readLen);
    offset += readLen;
    if ((data != HISLIP_PARAM_VAL_LEN) || (offset >= len)) {
        return;
    }

    /* ��ȡ�������ֵ, ռ��һ���ֽ� */
    HiLinkOperateResult result = (HiLinkOperateResult)(*(buf + offset));
    if (result == HLK_RSLT_OK) {
        HiLinkDevRemoved();
    }
}

static void HiLinkUpdateNetStatus(const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    unsigned short readLen = 0;
    int offset = 0;
    short data = HiSlipParseDataEA(buf, &readLen);
    offset += readLen;
    switch (data) {
        case HLK_TAG_MD_NST:
            break;
        case HLK_TAG_OP_RLT:
            data = HiSlipParseDataEA((buf + offset), &readLen);
            offset += (readLen + data);
            data = HiSlipParseDataEA((buf + offset), &readLen);
            offset += readLen;
            if (data != HLK_TAG_MD_NST) {
                HiLinkNotifyErrorInfo(HLK_ECODE_RCV_NETSTA_ERR);
                return;
            }
            break;
        default:
            return;
    }

    data = HiSlipParseDataEA((buf + offset), &readLen);
    offset += readLen;
    if ((data != HISLIP_PARAM_VAL_LEN) || ((offset + HISLIP_PARAM_VAL_LEN) > len)) {
        return;
    }

    ModuleNetStatus netStatus = (ModuleNetStatus)(*(buf + offset));
    HiLinkSetLocalNetStatus(netStatus);
}

/* ��תע�ᴦ�����±�Ϊ��ѯģ������״̬ */
static void HiLinkSetRegFuncIndex(void)
{
    g_functionMatrix[HLK_RUN_STATE_REGPROFILE].curFuncIdx =
        g_functionMatrix[HLK_RUN_STATE_REGPROFILE].funcNum - 1;
}

/* ����豸profile�Ƿ�ע���ʶ */
static void HiLinkCheckRegFlag(const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    unsigned short readLen = 0;
    short dataLen = 0;
    short tag = 0;
    int offset = 0;

    while (offset < len) {
        tag = HiSlipParseDataEA((buf + offset), &readLen);
        offset += readLen;
        if ((readLen == 0) || (offset >= len)) {
            break;
        }

        dataLen = HiSlipParseDataEA((buf + offset), &readLen);
        offset += readLen;
        if ((readLen == 0) || (offset >= len)) {
            break;
        }

        if (tag != HLK_TAG_OP_RLT) {
            offset += dataLen;
            continue;
        }

        if (buf[offset] != 0) {
            HiLinkSetRegFuncIndex();
        }
        offset += dataLen;
    }
}

/* ģ��֪ͨ��ذ�����״̬������ */
static void HiLinkNotifUpgradeInfo(unsigned char seq, unsigned short cmd,
    const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len == 0)) {
        return;
    }
    HiSlipSendAckMsg(seq, cmd, HLK_RET_OK);
}

/*
 * ���ݴ��ݹ����İ汾�ż���ذ��Ƿ���Ҫ����, �����ɺ�, ����HiSlipSendAckMsg����
 * �ϱ�ȷ����Ϣ. ����Ҫ����, HiSlipSendAckMsg�����ĵ���������result����Ϊ0; ������Ҫ
 * ����, �򽫸ò���result��Ϊ��0ֵ.
 */
static void HiLinkCheckUpgrade(unsigned char seq, unsigned short cmd,
    const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    short ret = HLK_RET_ERR;
    int offset = 0;
    unsigned short readLen = 0;

    short tag = HiSlipParseDataEA(buf, &readLen);
    offset += readLen;
    if (tag != HLK_TAG_DEV_SVER) {
        HiSlipSendAckMsg(seq, cmd, ret);
        return;
    }

    short length = HiSlipParseDataEA((buf + offset), &readLen);
    offset += readLen;
    if ((length <= 0) || ((offset + length) > len)) {
        HiSlipSendAckMsg(seq, cmd, ret);
        return;
    }

    ret = HiLinkOtaCheckVer((buf + offset), (unsigned short)length);
    if (ret == HLK_RET_ERR) {
        HiSlipSendAckMsg(seq, cmd, ret);
        return;
    }

    HiSlipSendAckMsg(seq, cmd, ret);
}

/*
 * ����������, ����HiSlipSendAckMsg�����ϱ�ȷ����Ϣ. ���������ɹ�, HiSlipSendAckMsg
 * �����ĵ���������result����Ϊ0; �������������ɹ�, �򽫲���result��Ϊ��0ֵ.
 */
static void HiLinkStartUpgrade(unsigned char seq, unsigned short cmd,
    const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    short ret = HLK_RET_ERR;
    int offset = 0;
    unsigned short readLen = 0;
    short tag = HiSlipParseDataEA(buf, &readLen);
    offset += readLen;
    if (tag != HLK_TAG_DEV_BIN_SIZE) {
        HiSlipSendAckMsg(seq, cmd, ret);
        return;
    }

    short length = HiSlipParseDataEA((buf + offset), &readLen);
    offset += readLen;
    if ((length != OTA_BIN_SIZE_LEN) || ((offset + OTA_BIN_SIZE_LEN) > len)) {
        HiSlipSendAckMsg(seq, cmd, ret);
        return;
    }

    /* size������int����, ��Ҫ�������ֽ����4���ֽ�����ת�������� */
    int size = 0;
    HiLinkCharArrayToInt((buf + offset), length, &size);
    ret = HiLinkOtaStart((unsigned int)size);
    HiSlipSendAckMsg(seq, cmd, ret);
}

/*
 * ������յ����ݰ����кŲ�����, ���ִ���ʱ�������HiSlip_send_ack_msg�����ϱ�ȷ����Ϣ,
 * ����������result����Ϊ��0; ���򽫲���result��Ϊ0.
 */
static void HiLinkRcvUpgradeData(unsigned char seq, unsigned short cmd,
    const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    short ret = HLK_RET_ERR;
    int offset = 0;
    unsigned short readLen = 0;
    short tag = HiSlipParseDataEA(buf, &readLen);
    offset += readLen;
    if (tag != HLK_TAG_DEV_BIN_DATA) {
        HiSlipSendAckMsg(seq, cmd, ret);
        return;
    }

    short length = HiSlipParseDataEA((buf + offset), &readLen);
    offset += readLen;
    if ((length <= 0) || (length > OTA_ONE_PKG_LEN) || ((offset + length) > len)) {
        HiSlipSendAckMsg(seq, cmd, ret);
        return;
    }

    ret = HiLinkOtaRcvPkg((buf + offset), (unsigned short)length);
    if (ret == HLK_RET_ERR) {
        HiSlipSendAckMsg(seq, cmd, ret);
        return;
    }

    HiSlipSendAckMsg(seq, cmd, ret);
}

/*
 * ������յ�У����뱾�ؼ����У��Ͳ���ͬ, ���ִ���ʱ, �����HiSlip_send_ack_msg����
 * �ϱ�ȷ����Ϣ, ����������result����Ϊ��0; ���򽫲���result��Ϊ0.
 */
static void HiLinkEndUpgrade(unsigned char seq, unsigned short cmd,
    const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    short ret = HLK_RET_ERR;
    int offset = 0;
    unsigned short readLen = 0;
    short tag = HiSlipParseDataEA(buf, &readLen);
    offset += readLen;
    if (tag != HLK_TAG_DEV_BIN_CHK) {
        HiSlipSendAckMsg(seq, cmd, ret);
        return;
    }

    short length = HiSlipParseDataEA((buf + offset), &readLen);
    offset += readLen;
    if ((length <= 0) || ((offset + length) > len)) {
        HiSlipSendAckMsg(seq, cmd, ret);
        return;
    }

    ret = HiLinkOtaEnd((buf + offset), (unsigned short)length);
    HiSlipSendAckMsg(seq, cmd, ret);
}

void HiLinkCmdProcess(unsigned char seq, unsigned short cmd, unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len == 0)) {
        return;
    }

    switch (cmd) {
        case HLK_CMD_MDLRSP_WM:
            HiSlipSendAckMsg(seq, cmd, HISLIP_OK);
            HiLinkUpdateWorkMode(buf, len);
            break;
        case HLK_CMD_MDLRSP_RST:
            HiSlipSendAckMsg(seq, cmd, HISLIP_OK);
            HiLinkFactoryResetResult(buf, len);
            break;
        case HLK_CMD_MDLRSP_RNS:
            HiSlipSendAckMsg(seq, cmd, HISLIP_OK);
            HiLinkUpdateNetStatus(buf, len);
            break;
        case HLK_CMD_MDCTL_DEV_PROT:
            HiSlipSendAckMsg(seq, cmd, HISLIP_OK);
            HiLinkPutSvcKeyVal(buf, len);
            break;
        case HLK_CMD_DEV_QUPG:
            HiLinkCheckUpgrade(seq, cmd, buf, len);
            break;
        case HLK_CMD_MDLRSP_UGD:
            HiLinkNotifUpgradeInfo(seq, cmd, buf, len);
            break;
        case HLK_CMD_DEV_SUPG:
            HiLinkStartUpgrade(seq, cmd, buf, len);
            break;
        case HLK_CMD_DEV_TRANSING:
            HiLinkRcvUpgradeData(seq, cmd, buf, len);
            break;
        case HLK_CMD_DEV_EUPG:
            HiLinkEndUpgrade(seq, cmd, buf, len);
            break;
        case HLK_CMD_MDCTL_DEV_QPROT:
            HiLinkGetSvcKeyVals(seq, buf, len);
            break;
        default:
            HiLinkNotifyErrorInfo(HLK_ECODE_RCV_UNKNOWN_CMD);
            break;
    }
    return;
}

/* �ͷ�ģ����Ϣ�ڴ�, ע�����쳣��֧����ǰҲҪ���ñ������ͷ�ģ����Ϣ�ڴ� */
static void HiLinkFreeModuleInfo(ModuleWifiInfo* moduleInfo)
{
    if (moduleInfo == NULL) {
        return;
    }

    if (moduleInfo->mac != NULL) {
        free(moduleInfo->mac);
        moduleInfo->mac = NULL;
    }

    if (moduleInfo->hardVer != NULL) {
        free(moduleInfo->hardVer);
        moduleInfo->hardVer = NULL;
    }

    if (moduleInfo->softVer != NULL) {
        free(moduleInfo->softVer);
        moduleInfo->softVer = NULL;
    }

    moduleInfo->rssi = 0;

    if (moduleInfo->apName != NULL) {
        free(moduleInfo->apName);
        moduleInfo->apName = NULL;
    }
}

/*
 * ����ģ��WiFi��Ϣ��MAC��ַ�ֶ�.
 * �����ֶ�ƫ�����򷵻ز���ʧ��HLK_RET_ERR.
 */
static int HiLinkParseMac(const unsigned char* buf, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* MAC:tag�ֶ� */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_FINFO_MAC) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* MAC:length�ֶ� */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if (length != DEV_MAC_LEN) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* MAC:value�ֶ� */
    moduleInfo->mac = (char*)malloc(DEV_MAC_LEN);
    if (moduleInfo->mac == NULL) {
        return HLK_RET_ERR;
    }
    /* malloc�ڴ������Ÿ�ֵ, �����ظ������ʼ�� */
    unsigned char i;
    for (i = 0; i < DEV_MAC_LEN; i++) {
        *(moduleInfo->mac + i) = *(buf + offset + i);
    }
    offset += DEV_MAC_LEN;
    return offset;
}

/*
 * ����ģ��WiFi��Ϣ��Ӳ���汾���ֶ�.
 * �����ֶ�ƫ�����򷵻ز���ʧ��HLK_RET_ERR.
 */
static int HiLinkParseHardVer(const unsigned char* buf, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* HardwareVer:tag�ֶ� */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_FINFO_HW) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* HardwareVer:length�ֶ� */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if ((length <= 0) || (length >= MODULE_INFO_FIELD_MAX_LEN)) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* HardwareVer:value�ֶ� */
    moduleInfo->hardVer = (char*)malloc(length + 1);
    if (moduleInfo->hardVer == NULL) {
        return HLK_RET_ERR;
    }
    /* malloc�ڴ������Ÿ�ֵ, �����ظ������ʼ�� */
    short i;
    for (i = 0; i < length; i++) {
        *(moduleInfo->hardVer + i) = *(buf + offset + i);
    }
    *(moduleInfo->hardVer + length) = 0;
    offset += length;
    return offset;
}

/*
 * ����ģ��WiFi��Ϣ������汾���ֶ�.
 * �����ֶ�ƫ�����򷵻ز���ʧ��HLK_RET_ERR.
 */
static int HiLinkParseSoftVer(const unsigned char* buf, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* SoftwareVer:tag�ֶ� */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_FINFO_FW) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* SoftwareVer:length�ֶ� */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if ((length <= 0) || (length >= MODULE_INFO_FIELD_MAX_LEN)) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* SoftwareVer:value�ֶ� */
    moduleInfo->softVer = (char*)malloc(length + 1);
    if (moduleInfo->softVer == NULL) {
        return HLK_RET_ERR;
    }
    /* malloc�ڴ������Ÿ�ֵ, �����ظ������ʼ�� */
    short i;
    for (i = 0; i < length; i++) {
        *(moduleInfo->softVer + i) = *(buf + offset + i);
    }
    *(moduleInfo->softVer + length) = 0;
    offset += length;
    return offset;
}

/*
 * ����ģ��WiFi��Ϣ��RSSI�ֶ�.
 * �����ֶ�ƫ�����򷵻ز���ʧ��HLK_RET_ERR.
 */
static int HiLinkParseRssi(const unsigned char* buf, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* Rssi:tag�ֶ� */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_FINFO_APRSSI) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* Rssi:length�ֶ� */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if (length != WIFI_RSSI_LEN) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* Rssi:value rssi��int����, ��Ҫ�������ֽ����4���ֽ�����ת�������� */
    int value = 0;
    HiLinkCharArrayToInt((buf + offset), length, &value);
    moduleInfo->rssi = value;
    offset += length;
    return offset;
}

/*
 * ����ģ��WiFi��Ϣ��AP�ȵ������ֶ�.
 * �����ֶ�ƫ�����򷵻ز���ʧ��HLK_RET_ERR.
 */
static int HiLinkParseApName(const unsigned char* buf, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* ApName:tag�ֶ� */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_FINFO_AP) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* ApName:length�ֶ� */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if ((length <= 0) || (length >= MODULE_INFO_FIELD_MAX_LEN)) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* ApName:value�ֶ� */
    moduleInfo->apName = (char*)malloc(length + 1);
    if (moduleInfo->apName == NULL) {
        return HLK_RET_ERR;
    }
    /* malloc�ڴ������Ÿ�ֵ, �����ظ������ʼ�� */
    short i;
    for (i = 0; i < length; i++) {
        *(moduleInfo->apName + i) = *(buf + offset + i);
    }
    *(moduleInfo->apName + length) = 0;
    offset += length;
    return offset;
}

/*
 * ����ģ�鷵�ص�ģ��WiFi��Ϣ.
 * ��HiLinkHandleModuleInfo������, ���ȵ��ñ������������ݰ�.
 */
static int HiLinkParseModuleInfo(const unsigned char* buf, int len, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    int offset = 0;
    int ret = HiLinkParseMac(buf, moduleInfo);
    if (ret == HLK_RET_ERR) {
        HiLinkFreeModuleInfo(moduleInfo);
        return HLK_RET_ERR;
    }
    offset += ret;

    ret = HiLinkParseHardVer((buf + offset), moduleInfo);
    if (ret == HLK_RET_ERR) {
        HiLinkFreeModuleInfo(moduleInfo);
        return HLK_RET_ERR;
    }
    offset += ret;

    ret = HiLinkParseSoftVer((buf + offset), moduleInfo);
    if (ret == HLK_RET_ERR) {
        HiLinkFreeModuleInfo(moduleInfo);
        return HLK_RET_ERR;
    }
    offset += ret;

    ret = HiLinkParseRssi((buf + offset), moduleInfo);
    if (ret == HLK_RET_ERR) {
        HiLinkFreeModuleInfo(moduleInfo);
        return HLK_RET_ERR;
    }
    offset += ret;

    ret = HiLinkParseApName((buf + offset), moduleInfo);
    if (ret == HLK_RET_ERR) {
        HiLinkFreeModuleInfo(moduleInfo);
        return HLK_RET_ERR;
    }

    return HLK_RET_OK;
}

/*
 * ����ģ�鴫����wifiģ����Ϣ.
 * ����û�������HiLinkGetModuleInfo��ȡwifiģ����Ϣ, �ڱ������д����ص�ģ����Ϣ.
 */
static void HiLinkHandleModuleInfo(const unsigned char* buf, int len)
{
    if ((buf == NULL) || (len == 0)) {
        return;
    }

    ModuleWifiInfo moduleInfo = { NULL, NULL, NULL, 0, NULL };
    /* ģ����Ϣ���� */
    if (HiLinkParseModuleInfo(buf, len, &moduleInfo) != HLK_RET_OK) {
        return;
    }

    /* ����������ģ����Ϣ֪ͨ����ذ�mcu */
    HiLinkNotifyModuleInfo(moduleInfo.mac, moduleInfo.hardVer, moduleInfo.softVer,
        moduleInfo.rssi, moduleInfo.apName);

    /* �ͷ�ģ����Ϣ�ڴ� */
    HiLinkFreeModuleInfo(&moduleInfo);
}

/* ����ģ�鴫����UTCʱ����Ϣ. */
static void HiLinkUpdateUtcTime(const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* tag�ֶ� */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_UTC) {
        return;
    }
    offset += readLen;

    /* length�ֶ� */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if ((length != UTC_TIME_BUF_LEN) || (len < (offset + readLen + UTC_TIME_BUF_LEN))) {
        return;
    }
    offset += readLen;

    HiLinkNotifyUtcTime(buf + offset, length);
}

void HiLinkAckProcess(unsigned char seq, unsigned short cmd, unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len == 0)) {
        return;
    }

    switch (cmd) {
        case HLK_CMD_DEVCTL_QWM:
            HiLinkUpdateWorkMode(buf, len);
            break;
        case HLK_CMD_DEVCTL_QNS:
            HiLinkUpdateNetStatus(buf, len);
            break;
        case HLK_CMD_DEVCTL_QRT:
            HiLinkUpdateUtcTime(buf, len);
            break;
        case HLK_CMD_DEVREG_SETSTA:
            HiLinkCheckRegFlag(buf, len);
            break;
        case HLK_CMD_DEVQRY_FINFOS:
            HiLinkHandleModuleInfo(buf, len);
            break;
        default:
            break;
    }
    return;
}

/* HiSlip��ʼ��&�������Գ�ʼ�� */
void HiLinkDevInit(void)
{
    HiSlipInit();
    HiLinkInitProfileValue();
}

/* HiLink���̴߳����� */
void HiLinkMainProcess(void)
{
    unsigned char tempData[HISLIP_MAX_FRM_INFO_LEN] = {0};
    short ret = HiSlipRcvData(tempData, sizeof(tempData));
    /* ���߳�ѭ���в������ڲ�״̬��ִ�еķ���ֵ, �˴������ж� */
    (void)HiLinkProcessInternalStates(ret);
}

/* �����������Ҫ����wifiģ��, �����κεط����ñ����� */
void HiLinkModuleReboot(void)
{
    /* ����������������ֻ����1��TLV�ṹ */
    unsigned char result = HLK_RB_MDERR;
    HiLinkTlvType msg;
    msg.tag = HLK_TAG_RB_RSN;
    msg.len = HISLIP_PARAM_VAL_LEN;
    msg.val = &result;
    HiLinkGeneralCmdSend(HLK_CMD_DEVCTL_RB, &msg, MD_REBOOT_TLV_NUM, HISLIP_CMD_TYPE);
}

/* �����������Ҫ����wifiģ��, �����κεط����ñ����� */
void HiLinkModuleReset(void)
{
    HiLinkGeneralCmdSend(HLK_CMD_DEVCTL_RST, NULL, 0, HISLIP_CMD_TYPE);
}

/*
 * �����ߵ��ñ��ӿڻ�ȡģ���WiFi��Ϣ, ������ʾWiFi�ȵ������.
 * ����Ҫ��ȡ��ǰ�豸���ӵ�WiFi�ȵ����Ϣ, ����ssid�봫��NULL (��������¶�����NULL).
 * ��������Ҫʵ��hilink_mcu.c�е�HiLinkNotifyModuleInfo������������յ���ģ����Ϣ.
 */
void HiLinkGetModuleInfo(char* ssid)
{
    /* ��ȡģ��WiFi��Ϣ������������ֻ����1��TLV�ṹ, 0��ʾ�±� */
    HiLinkTlvType msg;
    msg.tag = HLK_TAG_FINFO_AP;
    if (ssid == NULL) {
        msg.len = 0;
        msg.val = NULL;
    } else {
        msg.len = strlen(ssid);
        msg.val = (unsigned char*)ssid;
    }
    HiLinkGeneralCmdSend(HLK_CMD_DEVQRY_FINFOS, &msg, GET_MDINFO_TLV_NUM, HISLIP_CMD_TYPE);
}

/*
 * �����ߵ��ñ��ӿڻ�ȡģ���UTCʱ��.
 * ��������Ҫʵ��hilink_mcu.c�е�HiLinkNotifyUtcTime������������յ���UTCʱ��.
 */
void HiLinkGetUtcTime(void)
{
    HiLinkGeneralCmdSend(HLK_CMD_DEVCTL_QRT, NULL, 0, HISLIP_CMD_TYPE);
}

static short HiLinkGetSvcIdxBySvcMapId(unsigned short svcMapId)
{
    unsigned short itemNum = 0;
    SvcInfo* services = (SvcInfo*)HiLinkGetProfileInfoList(TYPE_SVC_MAP, &itemNum);
    if (services == NULL) {
        return HLK_RET_ERR;
    }

    unsigned short i;
    for (i = 0; i < itemNum; i++) {
        if (services[i].svcMapId == svcMapId) {
            return (short)i;
        }
    }
    return HLK_RET_ERR;
}

/*
 * ��ذ��ϱ�������������ֵ.
 * ����val���ϱ������ͻ򲼶���������ֵ, ��Ҫ�ϱ��ַ���������, ��ò�����0.
 * ����str���ϱ����ַ���������ֵ, ��Ҫ�ϱ����ͻ򲼶���������, ��ò�����NULL.
 * ������Ϣ�б������±�����ʧ�ܷ���HLK_RET_ERR.
 */
static short HiLinkUpdateOneKeyVal(unsigned short svcMapId, unsigned short keyMapId, int val, const char* str)
{
    unsigned short cnt = 0;
    KeyInfo* keys = (KeyInfo*)HiLinkGetProfileInfoList(TYPE_KEY_MAP, &cnt);
    if (keys == NULL) {
        return HLK_RET_ERR;
    }

    HiLinkMsg* keyVals = (HiLinkMsg*)HiLinkGetProfileInfoList(TYPE_MSG_TABLE, &cnt);
    if (keyVals == NULL) {
        return HLK_RET_ERR;
    }

    HiLinkDataType dataType = HLK_DATA_TYPE_UNKNOWN;
    unsigned short i;
    for (i = 0; i < cnt; i++) {
        if (keys[i].keyMapId == keyMapId) {
            dataType = keys[i].dataType;
            break;
        }
    }

    for (i = 0; i < cnt; i++) {
        if ((keyVals[i].msgInfo.svcMapId != svcMapId) ||
            (keyVals[i].msgInfo.keyMapId != keyMapId)) {
            continue;
        }

        if ((dataType == HLK_DATA_TYPE_INT) || (dataType == HLK_DATA_TYPE_BOOL)) {
            keyVals[i].msgInfo.value = val;
            return (short)i;
        } else if (dataType == HLK_DATA_TYPE_STR) {
            if (str == NULL) {
                return HLK_RET_ERR;
            }

            if (keyVals[i].msgInfo.value == 0) {
                HiLinkNotifyErrorInfo(HLK_ECODE_NO_BUF_TO_STORE_STR);
                return HLK_RET_ERR;
            }

            char* strAddr = (char*)((long)(keyVals[i].msgInfo.value));
            unsigned short length = strlen(str);
            unsigned short j;
            for (j = 0; j < length; j++) {
                *(strAddr + j) = *(str + j);
            }
            return (short)i;
        } else {
            return HLK_RET_ERR;
        }
    }

    return HLK_RET_ERR;
}

/*
 * �����߰�����ά���ϱ����ط���״̬ʱ, ���Ե��ñ��ӿ��ϱ�������������ֵ.
 * ����val���ϱ������ͻ򲼶���������ֵ, ��Ҫ�ϱ��ַ���������, ��ò�����0.
 * ����str���ϱ����ַ���������ֵ, ��Ҫ�ϱ����ͻ򲼶���������, ��ò�����NULL.
 */
int HiLinkUpdateKeyVal(unsigned short svcMapId, unsigned short keyMapId, int val, const char* str)
{
    if ((str != NULL) && (strlen(str) > STR_KEY_MAX_LEN)) {
        return HLK_RET_ERR;
    }

    short keyIdx = HiLinkUpdateOneKeyVal(svcMapId, keyMapId, val, str);
    if (keyIdx != HLK_RET_ERR) {
        HiLinkSetKeyValChangeMark(RPT_BY_KEY, (unsigned short)keyIdx, DEV_KEY_MASK_SET);
        return HLK_RET_OK;
    }
    return HLK_RET_ERR;
}

/*
 * ��������Ҫ������ά���ϱ����ط���״̬ʱ, ��Ҫ���ñ��ӿ������ϱ���Ӧ�������������ֵ.
 * ����val���ϱ������ͻ򲼶���������ֵ, ��Ҫ�ϱ��ַ���������, ��ò�����0.
 * ����str���ϱ����ַ���������ֵ, ��Ҫ�ϱ����ͻ򲼶���������, ��ò�����NULL.
 * ����rptFlag���ϱ�ʱ���ı��, �����һ���ϱ�������, rptFlagȡֵΪREPORT_LATER��ʾ�Ժ��ϱ�,
 * �����һ���ϱ�������, rptFlagȡֵΪREPORT_NOW��ʾ�����ϱ�.
 */
int HiLinkUpdateSvcVal(unsigned short svcMapId, unsigned short keyMapId, int val, const char* str, char rptFlag)
{
    if ((rptFlag != REPORT_NOW) && (rptFlag != REPORT_LATER)) {
        return HLK_RET_ERR;
    }

    if ((str != NULL) && (strlen(str) > STR_KEY_MAX_LEN)) {
        return HLK_RET_ERR;
    }

    short ret = HiLinkUpdateOneKeyVal(svcMapId, keyMapId, val, str);
    if (ret != HLK_RET_ERR) {
        if (rptFlag == REPORT_NOW) {
            short svcIdx = HiLinkGetSvcIdxBySvcMapId(svcMapId);
            if (svcIdx != HLK_RET_ERR) {
                HiLinkSetKeyValChangeMark(RPT_BY_SVC, (unsigned short)svcIdx, DEV_KEY_MASK_SET);
                return HLK_RET_OK;
            }
        } else {
            return HLK_RET_OK;
        }
    }
    return HLK_RET_ERR;
}
