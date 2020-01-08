/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: HiLink业务流程处理文件
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
 * 设置属性值变更标记.
 * 参数rptType表示属性上报类型(按属性或按服务).
 * 参数idx是待设置的属性索引下标, setMark是设置的标记值(0或1).
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
        changeMask[intPos] |= mark; /* 对应位设为1 */
    } else {
        changeMask[intPos] &= (~mark); /* 对应位设为0 */
    }
}

/*
 * 检查是否存在属性值变更, rptType表示属性上报类型(按属性或按服务).
 * 返回1表示存在属性变更, 返回0表示不存在属性变更.
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
        /* 存在变化的属性 */
        if (changeMask[i] != 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * 检查指定下标属性值是否变更.
 * 参数rptType表示属性上报类型(按属性或按服务), idx是待检查的下标.
 * 参数setMark表示检查变更后，将对应属性设置成setMark的标记值.
 * 返回1表示对应属性变更, 返回0表示对应属性未变更.
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

    /* 对应位属性变化, 与1表示取最低位 */
    if ((mark & 1) == DEV_KEY_MASK_SET) {
        if (setMark == DEV_KEY_MASK_USET) {
            mark = (1 << bitPos);
            /* 将对应位标记回0(未变化) */
            changeMask[intPos] &= (~mark);
        }
        return 1;
    }
    return 0;
}

/* 获取设备信息，返回设备信息结构指针 */
static void* HiLinkGetDeviceInfo(void)
{
    return (void*)&DEV_PROFILE_DEVINF;
}

/* 获取设备BI密钥，返回设备BI密钥字符串指针 */
static void* HiLinkGetDeviceBi(void)
{
    return (void*)DEV_PROFILE_BIRSA;
}

/* 获取设备AC信息，返回设备AC信息数组指针 */
static void* HiLinkGetDeviceAc(void)
{
    return (void*)DEV_PROFILE_ACVAL;
}

/* 获取设备类型及厂商英文名称，返回设备名称信息结构指针 */
static void* HiLinkGetDeviceEnName(void)
{
    return (void*)&DEV_PROFILE_ENNAME;
}

/* 获取设备Profile版本号，返回字符串指针 */
static void* HiLinkGetProfileVersion(void)
{
    return (void*)DEV_PROFILE_VER;
}

/*
 * 根据类型获取设备Profile信息列表.
 * 参数type表示要获取的设备Profile信息类型, size表示获取的信息列表大小.
 * 返回对应类型设备Profile信息列表指针.
 */
static void* HiLinkGetProfileInfoList(ProfileInfoType type, unsigned short* size)
{
    if (size == NULL) {
        return NULL;
    }

    unsigned short count = 0;
    void* retAddr = NULL;

    switch (type) {
        case TYPE_SVC_MAP: /* 服务信息列表 */
            count = sizeof(DEV_PROFILE_SVCS) / sizeof(DEV_PROFILE_SVCS[0]);
            retAddr = (void*)DEV_PROFILE_SVCS;
            break;
        case TYPE_KEY_MAP: /* 属性信息列表 */
            count = sizeof(DEV_PROFILE_KEYS) / sizeof(DEV_PROFILE_KEYS[0]);
            retAddr = (void*)DEV_PROFILE_KEYS;
            break;
        case TYPE_MSG_TABLE: /* 服务属性映射表 */
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

/* 获取全局数据发送缓存 */
static unsigned char* HiLinkGetDataSendBuf(void)
{
    return g_tmpSendBuf;
}

/* 获取全局扩展数据发送缓存 */
static unsigned char* HiLinkGetExtendBuf(void)
{
    return g_tmpExtendBuf;
}

/* 设置当前待上报初始值的服务下标 */
static void HiLinkSetCurInitSvcIndex(unsigned short idx)
{
    g_initSvcIndex = idx;
}

/* 获取当前待上报初始值服务下标 */
static unsigned short HiLinkGetCurInitSvcIndex(void)
{
    return g_initSvcIndex;
}

/* 获取数据发送临时tlv数组 */
static HiLinkTlvType* HiLinkGetTmpTlvArray(void)
{
    return g_tlvArray;
}

/* 记录当前等待回复ACK的消息Seq */
void HiLinkSetCurWaitAckSeq(unsigned char seq)
{
    g_waitAckSeq = seq;
}

/* 获取当前等待回复ACK的消息Seq */
static unsigned char HiLinkGetCurWaitAckSeq(void)
{
    return g_waitAckSeq;
}

/*
 * 设置请求函数执行状态.
 * 状态分为发送状态和等待ACK回复状态.
 */
static void HiLinkSetFuncProcState(HiLinkCmdState state)
{
    if (state <= FUNC_PROC_STATE_MAX) {
        g_hiLinkFuncState = state;
    }
}

/* 获取当前请求函数执行状态 */
static HiLinkCmdState HiLinkGetFuncProcState(void)
{
    return g_hiLinkFuncState;
}

/* 设置初始属性值上报状态 */
static void HiLinkSetInitValsReportState(HiLinkRptState state)
{
    g_initValReportState = state;
}

/* 获取当前初始属性值上报状态 */
static HiLinkRptState HiLinkGetInitValsReportState(void)
{
    return g_initValReportState;
}

/*
 * 修改当前请求函数列表执行函数下标, funcMatrix表示待修改的函数列表.
 * 返回HLK_RET_OK则下标加1, 返回HLK_RET_NEXTSTAGE则下标归0并执行到下一轮.
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

/* 获取当前请求函数列表执行函数下标 */
static short HiLinkGetCurMatrixFuncIndex(const HiLinkFuncMatrix* funcMatrix)
{
    if (funcMatrix != NULL) {
        return (short)(funcMatrix->curFuncIdx);
    }
    return HLK_RET_ERR;
}

/* 设置空闲运行状态下当前处理的函数下标 */
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
 * 入参rcvFlag表示数据接收标记(是否接收到数据).
 * 出参noDataFlag表示是否长时间未接收到数据标记.
 * 当长时间没有数据接收时, MCU需要给模组发送查询网络状态命令.
 */
static void HiLinkGetNoDataFlag(short rcvFlag, unsigned char* noDataFlag)
{
    /* unsigned long long类型最大值满足场景要求, 不会反转 */
    static unsigned long long lastReceiveTick = 0;
    unsigned long long curTick = HiLinkGetSysCurTime();

    if (rcvFlag == HISLIP_RCV_NO_DATA) {
        if (lastReceiveTick == 0) {
            /* 记录最后一次接收到数据包的时间 */
            lastReceiveTick = curTick;
        }
        /* 长时间没有数据包接收 */
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

/* 获取属性值对应的数据类型 */
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
 * 填充发送消息报文.
 * outData表示输出数据报文, outLen表示输出数据报文长度.
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

    /* 填充cmd字段 */
    HiSlipFillDataEA(cmd, &readLen, msg);
    unsigned short sendLen = readLen;
    msg += readLen;

    if ((tlvs == NULL) && (tlvNum == 0)) {
        *outLen = sendLen;
        return HLK_RET_OK;
    }

    /* 添加指针判空, 消除pclint告警 */
    if (tlvs == NULL) {
        return HLK_RET_ERR;
    }

    /* 填充tlv数据 */
    unsigned int i;
    for (i = 0; i < tlvNum; i++) {
        /* 填充tag字段 */
        if (tlvs[i].tag >= HLK_INVALID_VAL) {
            HiLinkNotifyErrorInfo(HLK_ECODE_PROFILE_TAG_ERR);
            return HLK_RET_ERR;
        }
        HiSlipFillDataEA(tlvs[i].tag, &readLen, msg);
        sendLen += readLen;
        msg += readLen;

        /* 填充len字段 */
        if (tlvs[i].len >= HLK_INVALID_VAL) {
            HiLinkNotifyErrorInfo(HLK_ECODE_PROFILE_LEN_ERR);
            return HLK_RET_ERR;
        }
        HiSlipFillDataEA(tlvs[i].len, &readLen, msg);
        sendLen += readLen;
        msg += readLen;

        /* 填充val字段 */
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
 * 通用命令发送接口.
 * ackSeq表示ACK序列号.
 */
void HiLinkGeneralCmdSend(unsigned short cmd, const HiLinkTlvType* tlvs,
    unsigned short tlvNum, unsigned char ackSeq)
{
    unsigned short dataLen = 0;
    unsigned char* tmpBuf = HiLinkGetDataSendBuf();
    if (tmpBuf == NULL) {
        return;
    }

    /* tlvs为NULL是合法值 */
    int ret = HiLinkGeneralFillCmdPkg(cmd, tlvs, tlvNum, tmpBuf, &dataLen);
    if (ret == HLK_RET_OK) {
        if (ackSeq == 0) {
            HiSlipSendData(tmpBuf, dataLen, HISLIP_CMD_TYPE);
        } else {
            HiSlipSendData(tmpBuf, dataLen, ackSeq);
        }
    }
}

/* 填充服务信息列表tlv */
static int HiLinkFillSvcMapTlvs(unsigned short* cnt, HiLinkTlvType* tlvs)
{
    unsigned short i;
    unsigned short svcNum = 0;
    SvcInfo* svcList = (SvcInfo*)HiLinkGetProfileInfoList(TYPE_SVC_MAP, &svcNum);
    unsigned char* valBuf = HiLinkGetExtendBuf();
    if ((cnt == NULL) || (tlvs == NULL) || (svcList == NULL) || (valBuf == NULL)) {
        return HLK_RET_ERR;
    }

    /* 命令发送扩展缓存清空 */
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

        /* 第1个字段svcMapId */
        tlvs[i * SVC_INFO_ITEM_NUM].tag = HLK_TAG_PROT_SVCINF;
        HiSlipFillDataEA(svcList[i].svcMapId, &tagLen, (valBuf + offset));
        tlvs[i * SVC_INFO_ITEM_NUM].len = tagLen;
        tlvs[i * SVC_INFO_ITEM_NUM].val = valBuf + offset;
        idxOffset++;
        offset += tagLen;

        /* 第2个字段svcType */
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].tag = HLK_TAG_PROT_SVCINF;
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].len = strlen(svcList[i].svcType);
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].val = (unsigned char*)svcList[i].svcType;
        idxOffset++;

        /* 第3个字段svcId */
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].tag = HLK_TAG_PROT_SVCINF;
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].len = strlen(svcList[i].svcId);
        tlvs[i * SVC_INFO_ITEM_NUM + idxOffset].val = (unsigned char*)svcList[i].svcId;
    }

    *cnt = svcNum * SVC_INFO_ITEM_NUM;
    return HLK_RET_OK;
}

/* 填充属性信息列表tlv */
static int HiLinkFillKeyMapTlvs(unsigned short* cnt, HiLinkTlvType* tlvs)
{
    unsigned short i;
    unsigned short keyNum = 0;
    KeyInfo* keys = (KeyInfo*)HiLinkGetProfileInfoList(TYPE_KEY_MAP, &keyNum);
    unsigned char* valBuf = HiLinkGetExtendBuf();
    if ((cnt == NULL) || (tlvs == NULL) || (keys == NULL) || (valBuf == NULL)) {
        return HLK_RET_ERR;
    }

    /* 命令发送扩展缓存清空 */
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

        /* 第1个字段keyMapId */
        tlvs[i * KEY_INFO_ITEM_NUM].tag = HLK_TAG_PROT_ITEM;
        HiSlipFillDataEA(keys[i].keyMapId, &tagLen, (valBuf + offset));
        tlvs[i * KEY_INFO_ITEM_NUM].len = tagLen;
        tlvs[i * KEY_INFO_ITEM_NUM].val = valBuf + offset;
        idxOffset++;
        offset += tagLen;

        /* 第2个字段svcType */
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].tag = HLK_TAG_PROT_ITEM;
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].len = strlen(keys[i].svcType);
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].val = (unsigned char*)keys[i].svcType;
        idxOffset++;

        /* 第3个字段keyName */
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].tag = HLK_TAG_PROT_ITEM;
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].len = strlen(keys[i].keyName);
        tlvs[i * KEY_INFO_ITEM_NUM + idxOffset].val = (unsigned char*)keys[i].keyName;
        idxOffset++;

        /* 第4个字段dataType */
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
 * 填充服务的属性tlv列表.
 * 参数tlvs是填充的数据tlv列表, keyNum表示属性数目.
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
        /* keyMapId为invalid值表示填充模组GET命令或上报全部服务属性值的返回报文 */
        if ((keyVals[i].msgInfo.svcMapId != svcMapId) ||
            ((keyVals[i].msgInfo.keyMapId != keyMapId) && (keyMapId != HLK_INVALID_VAL))) {
            continue;
        }

        tlvs[num].tag = keyVals[i].msgInfo.keyMapId; /* Tag字段 */
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
                tlvs[num].len = sizeof(int); /* Length字段 */
                HiLinkIntToCharArray(keyVals[i].msgInfo.value, (HLK_EXT_BUF_SIZE - offset), &valBuf[offset]);
                tlvs[num].val = valBuf + offset; /* Value字段 */
                offset += UNSIGNED_INT_BYTES; /* int类型所占字节数 */
                break;
            case HLK_DATA_TYPE_STR:
                tlvs[num].len = strlen((char*)((long)(keyVals[i].msgInfo.value))); /* Length字段 */
                tlvs[num].val = (unsigned char*)((long)(keyVals[i].msgInfo.value)); /* Value字段 */
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
 * 解析tlv结构的通用接口.
 * 返回非NULL表示下一个tlv起始位置, 返回NULL表示解析错误.
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

    /* 返回下一个TLV起始地址 */
    return msg;
}

/* 功能: 发送消息上报具体服务属性值 */
static void HiLinkReportSvcKeyVals(unsigned short svcMapId, unsigned short keyMapId)
{
    HiLinkTlvType* msg = HiLinkGetTmpTlvArray();
    if (msg == NULL) {
        return;
    }

    unsigned short keyNum = 0;
    unsigned short dataLen = 0;
    /* 1表示从第2个TLV开始填充属性信息 */
    int ret = HiLinkFillSvcTlvs(svcMapId, keyMapId, &msg[1], &keyNum, &dataLen);
    if (ret == HLK_RET_OK) {
        /* 0表示第1个TLV填充服务映射ID字段 */
        msg[0].tag = svcMapId;
        msg[0].len = dataLen;
        msg[0].val = NULL;
        HiLinkGeneralCmdSend(HLK_CMD_DEVUPD_STA, msg, (keyNum + 1), HISLIP_CMD_TYPE);
    }
}

/* 根据下标索引, 上报变化的属性值 */
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
 * 处理等待命令ACK响应.
 * 参数startTick表示开始等待ACK的时间计数值.
 * 返回值ACK_STATE_WAITTING表示正在等待ACK,
 * ACK_STATE_ACKED表示已收到ACK, ACK_STATE_ACKTIMOUT表示等待ACK超时.
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
    /* 查询工作模式, 没有payload */
    HiLinkGeneralCmdSend(HLK_CMD_DEVCTL_QWM, NULL, 0, HISLIP_CMD_TYPE);
    return HLK_RET_OK;
}

static short HiLinkReqGetNetStatus(void)
{
    /* 查询网络状态, 没有payload */
    HiLinkGeneralCmdSend(HLK_CMD_DEVCTL_QNS, NULL, 0, HISLIP_CMD_TYPE);
    return HLK_RET_OK;
}

static short HiLinkReqRegStart(void)
{
    HiLinkTlvType msg[REG_FLAG_ITEM_NUM + PROFILE_VER_ITEM_NUM]; /* 设备注册状态和协议版本号 */
    ProfileRegStatus regStatus = HLK_REG_START;

    unsigned char* profileVer = (unsigned char*)HiLinkGetProfileVersion();
    if (profileVer == NULL) {
        return HLK_RET_ERR;
    }

    /* 0表示第1个字段: 设备注册状态(开始) */
    msg[0].tag = HLK_TAG_REGSTA;
    msg[0].len = HISLIP_PARAM_VAL_LEN;
    msg[0].val = (unsigned char*)&regStatus;

    /* 1表示第2个字段: profileVer */
    msg[1].tag = HLK_TAG_PROFILE_VER;
    msg[1].len = strlen((const char*)profileVer);
    msg[1].val = profileVer;

    /* 设备注册状态和协议版本号两个TLV字段 */
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

    /* 0表示第1个字段: BI */
    msg[0].tag = HLK_TAG_PROT_BIRSA;
    msg[0].len = strlen((const char*)bi);
    msg[0].val = bi;

    /* 1表示第2个字段: AC */
    msg[1].tag = HLK_TAG_PROT_AC;
    msg[1].len = HLK_AC_LEN;
    msg[1].val = ac;

    /* TLV个数是设备AC信息和设备BI信息两个TLV字段 */
    HiLinkGeneralCmdSend(HLK_CMD_DEVREG_DEVINFO, msg, (DEV_AC_ITEM_NUM + DEV_BI_ITEM_NUM), HISLIP_CMD_TYPE);
    return HLK_RET_OK;
}

static short HiLinkReqRegDevInfo(void)
{
    HiLinkTlvType msg[DEV_INFO_ITEM_NUM + DEV_NAME_ITEM_NUM];
    char devSn[DEV_SN_MAX_LEN] = {0};
    char msgIdx = 0; /* 消息TLV数组下标 */
    DevInfo* devInfo = (DevInfo*)HiLinkGetDeviceInfo();
    if (devInfo == NULL) {
        return HLK_RET_ERR;
    }
    DevEnName* devNameEn = (DevEnName*)HiLinkGetDeviceEnName();
    if (devNameEn == NULL) {
        return HLK_RET_ERR;
    }

    /* 第1个字段: SN */
    msg[msgIdx].tag = HLK_TAG_PROT_DEVSN;
    /* devInfo->sn初始化为空字符串"", 不会为NULL */
    if (strlen(devInfo->sn) == 0) {
        /* profile中未定义sn时从提供给厂商的接口中获取 */
        HiLinkGetDeviceSn(DEV_SN_MAX_LEN, devSn);
        msg[msgIdx].len = strlen(devSn);
        msg[msgIdx].val = (unsigned char *)devSn;
    } else {
        msg[msgIdx].len = strlen(devInfo->sn);
        msg[msgIdx].val = (unsigned char *)devInfo->sn;
    }
    msgIdx++;

    /* 第2个字段: prodId */
    msg[msgIdx].tag = HLK_TAG_PROT_PRDTID;
    msg[msgIdx].len = strlen(devInfo->prodId);
    msg[msgIdx].val = (unsigned char*)devInfo->prodId;
    msgIdx++;

    /* 第3个字段: devModel */
    msg[msgIdx].tag = HLK_TAG_PROT_PRDTMD;
    msg[msgIdx].len = strlen(devInfo->devModel);
    msg[msgIdx].val = (unsigned char*)devInfo->devModel;
    msgIdx++;

    /* 第4个字段: devTypeId */
    msg[msgIdx].tag = HLK_TAG_PROT_PRDTTP;
    msg[msgIdx].len = strlen(devInfo->devTypeId);
    msg[msgIdx].val = (unsigned char*)devInfo->devTypeId;
    msgIdx++;

    /* 第5个字段: manuId */
    msg[msgIdx].tag = HLK_TAG_PROT_MANU;
    msg[msgIdx].len = strlen(devInfo->manuId);
    msg[msgIdx].val = (unsigned char*)devInfo->manuId;
    msgIdx++;

    /* 第6个字段: mcuVer */
    msg[msgIdx].tag = HLK_TAG_DEV_SVER;
    msg[msgIdx].len = strlen(devInfo->mcuVer);
    msg[msgIdx].val = (unsigned char*)devInfo->mcuVer;
    msgIdx++;

    /* 第7个字段: devTypeName */
    msg[msgIdx].tag = HLK_TAG_PROT_DTENN;
    msg[msgIdx].len = strlen(devNameEn->devTypeName);
    msg[msgIdx].val = (unsigned char*)devNameEn->devTypeName;
    msgIdx++;

    /* 第8个字段: manuName */
    msg[msgIdx].tag = HLK_TAG_PROT_MAUNENN;
    msg[msgIdx].len = strlen(devNameEn->manuName);
    msg[msgIdx].val = (unsigned char*)devNameEn->manuName;

    /* TLV个数是设备信息字段数和设备名称信息字段数之和 */
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
    /* 消息只有1个TLV字段, 设备注册状态(结束) */
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
 * 上报服务属性初始值.
 * 其中返回值HLK_RET_NEXTSTAGE表示系统状态进入到下一阶段.
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

    /* 上报服务所有属性值 */
    HiLinkReportSvcKeyVals(services[idx].svcMapId, HLK_INVALID_VAL);
    HiLinkSetCurInitSvcIndex(idx + 1); /* 下一个服务 */
    return HLK_RET_OK;
}

/* 主动上报变化的服务属性值 */
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
            /* 上报变化的属性 */
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
            /* 上报有变化的服务的所有属性 */
            if (HiLinkCheckChangedKeyVal(RPT_BY_SVC, i, DEV_KEY_MASK_USET) == 1) {
                HiLinkReportSvcKeyVals(services[i].svcMapId, HLK_INVALID_VAL);
                return HLK_RET_OK;
            }
        }
    }

    return HLK_RET_ERR;
}

/* 处理运行状态变更, 参数表示当前阶段执行函数及下标 */
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

/* 处理系统运行状态的变更 */
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
                /* 切换到注册状态 */
                HiLinkSetCurRunState(HLK_RUN_STATE_REGPROFILE);
                break;
            }
            if ((isOfflineState == true) || (isResetState == true)) {
                /* 模组在线, 设备重启, 需要重新上报初始值 */
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
            /* 初始值上报完成, 切换成idle状态 */
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

/* 执行不同系统状态下的处理函数 */
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
            /* 切换到下一状态 */
            HiLinkChangeRunState(workMode, netStatus);
        }
    }

    return ret;
}

/* 处理IDLE状态的处理函数, 其中rcvFlag表示是否接收到消息的标记 */
static short HiLinkProcessIdleState(HiLinkFuncMatrix* funcMatrix, short rcvFlag)
{
    short ret = HLK_RET_OK;
    unsigned char noDataFlag = 0;
    HiLinkGetNoDataFlag(rcvFlag, &noDataFlag);

    /* 长时间未接收到数据包 */
    if (noDataFlag != 0) {
        /* 1表示查询模组网络状态 */
        HiLinkSetCurMatrixFuncIndex(1);
        ret = HiLinkProcessMaxtrixFuncs(funcMatrix);
        if (ret != HLK_RET_WACK) {
            noDataFlag = 0;
        }
    } else {
        /* 0表示更新属性值状态 */
        HiLinkSetCurMatrixFuncIndex(0);
        ret = HiLinkProcessMaxtrixFuncs(funcMatrix);
    }

    return ret;
}

/* 处理内部状态机, 其中rcvFlag表示是否接收到消息的标记 */
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

/* 处理模组侧PUT命令的设备控制消息 */
static void HiLinkPutSvcKeyVal(unsigned char* message, unsigned short len)
{
    if ((message == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    /* 获取服务mapid */
    HiLinkTlvType parseMsg = { 0, 0, NULL };
    unsigned char* next = HiLinkGeneralParseTlv(message, &parseMsg);
    if (next == NULL) {
        return;
    }
    unsigned short svcMapId = parseMsg.tag;
    next = parseMsg.val;

    /* 获取属性mapid */
    if (HiLinkGeneralParseTlv(next, &parseMsg) == NULL) {
        return;
    }
    unsigned short keyMapId = parseMsg.tag;

    /* 获取消息内容 */
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
            /* int类型需要将网络字节序的4个字节数据转换成整型 */
            HiLinkCharArrayToInt(parseMsg.val, parseMsg.len, &value);
            keyVals[i].msgFunc(value);
        } else if ((dataType == HLK_DATA_TYPE_STR) && (parseMsg.len > 0) && (parseMsg.len <= STR_KEY_MAX_LEN)) {
            char* strAddr = (char*)malloc(parseMsg.len + 1);
            if (strAddr == NULL) {
                return;
            }
            /* malloc内存后紧接着赋值, 无需重复清零初始化 */
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

/* 处理模组侧GET命令的服务属性值查询消息 */
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

    /* 1表示从第2个TLV开始填充属性信息列表 */
    int ret = HiLinkFillSvcTlvs(svcMapId, HLK_INVALID_VAL, &package[1], &keyNum, &dataLen);
    if (ret == HLK_RET_OK) {
        /* 0表示第1个TLV填充服务映射ID字段 */
        package[0].tag = svcMapId;
        package[0].len = dataLen;
        package[0].val = NULL;
        HiLinkGeneralCmdSend(HLK_CMD_MDCTL_DEV_QPROT, package, (keyNum + 1), (seq | HISLIP_ACK_TYPE));
    }
}

/* 处理函数矩阵重新初始化 */
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
 * 模组通知MCU设备被云端删除注册.
 * 设备删除成功会调用HiLinkDevRemoved函数做后续处理.
 */
static void HiLinkFactoryResetResult(const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    unsigned short readLen = 0;
    int offset = 0;
    /* Tag字段 */
    short data = HiSlipParseDataEA(buf, &readLen);
    offset += readLen;
    if ((data != HLK_TAG_OP_RLT) || (offset >= len)) {
        return;
    }

    /* Length字段 */
    data = HiSlipParseDataEA((buf + offset), &readLen);
    offset += readLen;
    if ((data != HISLIP_PARAM_VAL_LEN) || (offset >= len)) {
        return;
    }

    /* 获取操作结果值, 占用一个字节 */
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

/* 跳转注册处理函数下标为查询模组网络状态 */
static void HiLinkSetRegFuncIndex(void)
{
    g_functionMatrix[HLK_RUN_STATE_REGPROFILE].curFuncIdx =
        g_functionMatrix[HLK_RUN_STATE_REGPROFILE].funcNum - 1;
}

/* 检查设备profile是否注册标识 */
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

/* 模组通知电控板升级状态和速率 */
static void HiLinkNotifUpgradeInfo(unsigned char seq, unsigned short cmd,
    const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len == 0)) {
        return;
    }
    HiSlipSendAckMsg(seq, cmd, HLK_RET_OK);
}

/*
 * 根据传递过来的版本号检查电控板是否需要升级, 检查完成后, 调用HiSlipSendAckMsg函数
 * 上报确认消息. 若需要升级, HiSlipSendAckMsg函数的第三个参数result则置为0; 若不需要
 * 升级, 则将该参数result置为非0值.
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
 * 启动升级后, 调用HiSlipSendAckMsg函数上报确认消息. 启动升级成功, HiSlipSendAckMsg
 * 函数的第三个参数result则置为0; 若启动升级不成功, 则将参数result置为非0值.
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

    /* size变量是int类型, 需要将网络字节序的4个字节数据转换成整型 */
    int size = 0;
    HiLinkCharArrayToInt((buf + offset), length, &size);
    ret = HiLinkOtaStart((unsigned int)size);
    HiSlipSendAckMsg(seq, cmd, ret);
}

/*
 * 如果接收的数据包序列号不连续, 出现错误时，则调用HiSlip_send_ack_msg函数上报确认消息,
 * 第三个参数result则置为非0; 否则将参数result置为0.
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
 * 如果接收的校验和与本地计算的校验和不相同, 出现错误时, 则调用HiSlip_send_ack_msg函数
 * 上报确认消息, 第三个参数result则置为非0; 否则将参数result置为0.
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

/* 释放模组信息内存, 注意在异常分支返回前也要调用本函数释放模组信息内存 */
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
 * 解析模组WiFi信息的MAC地址字段.
 * 返回字段偏移量或返回操作失败HLK_RET_ERR.
 */
static int HiLinkParseMac(const unsigned char* buf, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* MAC:tag字段 */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_FINFO_MAC) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* MAC:length字段 */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if (length != DEV_MAC_LEN) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* MAC:value字段 */
    moduleInfo->mac = (char*)malloc(DEV_MAC_LEN);
    if (moduleInfo->mac == NULL) {
        return HLK_RET_ERR;
    }
    /* malloc内存后紧接着赋值, 无需重复清零初始化 */
    unsigned char i;
    for (i = 0; i < DEV_MAC_LEN; i++) {
        *(moduleInfo->mac + i) = *(buf + offset + i);
    }
    offset += DEV_MAC_LEN;
    return offset;
}

/*
 * 解析模组WiFi信息的硬件版本号字段.
 * 返回字段偏移量或返回操作失败HLK_RET_ERR.
 */
static int HiLinkParseHardVer(const unsigned char* buf, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* HardwareVer:tag字段 */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_FINFO_HW) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* HardwareVer:length字段 */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if ((length <= 0) || (length >= MODULE_INFO_FIELD_MAX_LEN)) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* HardwareVer:value字段 */
    moduleInfo->hardVer = (char*)malloc(length + 1);
    if (moduleInfo->hardVer == NULL) {
        return HLK_RET_ERR;
    }
    /* malloc内存后紧接着赋值, 无需重复清零初始化 */
    short i;
    for (i = 0; i < length; i++) {
        *(moduleInfo->hardVer + i) = *(buf + offset + i);
    }
    *(moduleInfo->hardVer + length) = 0;
    offset += length;
    return offset;
}

/*
 * 解析模组WiFi信息的软件版本号字段.
 * 返回字段偏移量或返回操作失败HLK_RET_ERR.
 */
static int HiLinkParseSoftVer(const unsigned char* buf, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* SoftwareVer:tag字段 */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_FINFO_FW) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* SoftwareVer:length字段 */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if ((length <= 0) || (length >= MODULE_INFO_FIELD_MAX_LEN)) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* SoftwareVer:value字段 */
    moduleInfo->softVer = (char*)malloc(length + 1);
    if (moduleInfo->softVer == NULL) {
        return HLK_RET_ERR;
    }
    /* malloc内存后紧接着赋值, 无需重复清零初始化 */
    short i;
    for (i = 0; i < length; i++) {
        *(moduleInfo->softVer + i) = *(buf + offset + i);
    }
    *(moduleInfo->softVer + length) = 0;
    offset += length;
    return offset;
}

/*
 * 解析模组WiFi信息的RSSI字段.
 * 返回字段偏移量或返回操作失败HLK_RET_ERR.
 */
static int HiLinkParseRssi(const unsigned char* buf, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* Rssi:tag字段 */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_FINFO_APRSSI) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* Rssi:length字段 */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if (length != WIFI_RSSI_LEN) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* Rssi:value rssi是int类型, 需要将网络字节序的4个字节数据转换成整型 */
    int value = 0;
    HiLinkCharArrayToInt((buf + offset), length, &value);
    moduleInfo->rssi = value;
    offset += length;
    return offset;
}

/*
 * 解析模组WiFi信息的AP热点名称字段.
 * 返回字段偏移量或返回操作失败HLK_RET_ERR.
 */
static int HiLinkParseApName(const unsigned char* buf, ModuleWifiInfo* moduleInfo)
{
    if ((buf == NULL) || (moduleInfo == NULL)) {
        return HLK_RET_ERR;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* ApName:tag字段 */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_FINFO_AP) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* ApName:length字段 */
    short length = HiSlipParseDataEA((buf + offset), &readLen);
    if ((length <= 0) || (length >= MODULE_INFO_FIELD_MAX_LEN)) {
        return HLK_RET_ERR;
    }
    offset += readLen;

    /* ApName:value字段 */
    moduleInfo->apName = (char*)malloc(length + 1);
    if (moduleInfo->apName == NULL) {
        return HLK_RET_ERR;
    }
    /* malloc内存后紧接着赋值, 无需重复清零初始化 */
    short i;
    for (i = 0; i < length; i++) {
        *(moduleInfo->apName + i) = *(buf + offset + i);
    }
    *(moduleInfo->apName + length) = 0;
    offset += length;
    return offset;
}

/*
 * 解析模组返回的模组WiFi信息.
 * 在HiLinkHandleModuleInfo函数中, 需先调用本函数解析数据包.
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
 * 处理模组传来的wifi模组信息.
 * 如果用户调用了HiLinkGetModuleInfo获取wifi模组信息, 在本函数中处理返回的模组信息.
 */
static void HiLinkHandleModuleInfo(const unsigned char* buf, int len)
{
    if ((buf == NULL) || (len == 0)) {
        return;
    }

    ModuleWifiInfo moduleInfo = { NULL, NULL, NULL, 0, NULL };
    /* 模组信息解析 */
    if (HiLinkParseModuleInfo(buf, len, &moduleInfo) != HLK_RET_OK) {
        return;
    }

    /* 将解析出的模组信息通知到电控板mcu */
    HiLinkNotifyModuleInfo(moduleInfo.mac, moduleInfo.hardVer, moduleInfo.softVer,
        moduleInfo.rssi, moduleInfo.apName);

    /* 释放模组信息内存 */
    HiLinkFreeModuleInfo(&moduleInfo);
}

/* 处理模组传来的UTC时间信息. */
static void HiLinkUpdateUtcTime(const unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len < HISLIP_FRAME_MIN_LEN)) {
        return;
    }

    unsigned short readLen = 0;
    int offset = 0;

    /* tag字段 */
    short tag = HiSlipParseDataEA(buf, &readLen);
    if (tag != HLK_TAG_UTC) {
        return;
    }
    offset += readLen;

    /* length字段 */
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

/* HiSlip初始化&服务属性初始化 */
void HiLinkDevInit(void)
{
    HiSlipInit();
    HiLinkInitProfileValue();
}

/* HiLink主线程处理函数 */
void HiLinkMainProcess(void)
{
    unsigned char tempData[HISLIP_MAX_FRM_INFO_LEN] = {0};
    short ret = HiSlipRcvData(tempData, sizeof(tempData));
    /* 主线程循环中不关心内部状态机执行的返回值, 此处不需判断 */
    (void)HiLinkProcessInternalStates(ret);
}

/* 开发者如果需要重启wifi模组, 可在任何地方调用本函数 */
void HiLinkModuleReboot(void)
{
    /* 重启命令数据内容只包含1个TLV结构 */
    unsigned char result = HLK_RB_MDERR;
    HiLinkTlvType msg;
    msg.tag = HLK_TAG_RB_RSN;
    msg.len = HISLIP_PARAM_VAL_LEN;
    msg.val = &result;
    HiLinkGeneralCmdSend(HLK_CMD_DEVCTL_RB, &msg, MD_REBOOT_TLV_NUM, HISLIP_CMD_TYPE);
}

/* 开发者如果需要重置wifi模组, 可在任何地方调用本函数 */
void HiLinkModuleReset(void)
{
    HiLinkGeneralCmdSend(HLK_CMD_DEVCTL_RST, NULL, 0, HISLIP_CMD_TYPE);
}

/*
 * 开发者调用本接口获取模组的WiFi信息, 参数表示WiFi热点的名称.
 * 若需要获取当前设备连接的WiFi热点的信息, 参数ssid请传入NULL (多数情况下都传入NULL).
 * 开发者需要实现hilink_mcu.c中的HiLinkNotifyModuleInfo函数来处理接收到的模组信息.
 */
void HiLinkGetModuleInfo(char* ssid)
{
    /* 获取模组WiFi信息命令数据内容只包含1个TLV结构, 0表示下标 */
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
 * 开发者调用本接口获取模组的UTC时间.
 * 开发者需要实现hilink_mcu.c中的HiLinkNotifyUtcTime函数来处理接收到的UTC时间.
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
 * 电控板上报单个服务属性值.
 * 参数val是上报的整型或布尔类型属性值, 若要上报字符类型数据, 则该参数填0.
 * 参数str是上报的字符类型属性值, 若要上报整型或布尔类型数据, 则该参数填NULL.
 * 返回消息列表属性下标或操作失败返回HLK_RET_ERR.
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
 * 开发者按属性维度上报本地服务状态时, 可以调用本接口上报单个服务属性值.
 * 参数val是上报的整型或布尔类型属性值, 若要上报字符类型数据, 则该参数填0.
 * 参数str是上报的字符类型属性值, 若要上报整型或布尔类型数据, 则该参数填NULL.
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
 * 开发者需要按服务维度上报本地服务状态时, 需要调用本接口依次上报对应服务的所有属性值.
 * 参数val是上报的整型或布尔类型属性值, 若要上报字符类型数据, 则该参数填0.
 * 参数str是上报的字符类型属性值, 若要上报整型或布尔类型数据, 则该参数填NULL.
 * 参数rptFlag是上报时机的标记, 除最后一个上报属性外, rptFlag取值为REPORT_LATER表示稍后上报,
 * 对最后一个上报的属性, rptFlag取值为REPORT_NOW表示立即上报.
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
