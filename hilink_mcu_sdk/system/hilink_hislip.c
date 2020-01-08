/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: HiSlip串口通讯协议实现
 * Create: 2018-12-01
 */
#include "hilink_mcu.h"

/* CRC数值表 */
static const unsigned char g_hiSlipCrcTable[HISLIP_CRC_TABLE_LEN] = {
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
    0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
    0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
    0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
    0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
    0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
    0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
    0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
    0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
    0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
    0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};

/* 最后一次ACK消息信息 */
static LastAckInfo g_hiSlipLastAck;
/* HiSlip数据收发结构和回调 */
static HiSlipHandler g_hiSlipHandler;
/* HiSlip发送缓存区 */
static unsigned char g_hiSlipSendBuf[HISLIP_MAX_FRM_LEN] = {0};
/* UART缓存区 */
static unsigned char g_uartBuf[HISLIP_UART_BUF_SIZE] = {0};
/* UART处理结构 */
static UartHandler g_uartHandler;
/* 写缓存区序列号 */
static unsigned char g_bufWriteIndex = 0;
/* 读缓存区序列号 */
static unsigned char g_bufReadIndex = 0;
/* 数据帧长度 */
static unsigned short g_frameLen = 0;

/* 创建HiSlip数据帧, 入参sendInfo是待发送帧, 出参frame表示输出创建的数据帧流 */
static short HiSlipBuildFrame(HiSlipSend* sendInfo, unsigned char** frame)
{
    if ((sendInfo == NULL) || (frame == NULL)) {
        return HISLIP_ERR;
    }

    if (sendInfo->len > HISLIP_MAX_DATA_LEN) {
        return HISLIP_ERR;
    }

    unsigned short i;
    /* 发送缓存清空 */
    for (i = 0; i < HISLIP_MAX_FRM_LEN; i++) {
        *(g_hiSlipSendBuf + i) = 0;
    }

    unsigned short j = 0;
    /* 开始结束标记 */
    g_hiSlipSendBuf[j] = END;
    j++;

    unsigned char calCrc = 0;
    /* 包编号pkgNum */
    g_hiSlipSendBuf[j] = sendInfo->pkgNum;
    calCrc = g_hiSlipCrcTable[calCrc ^ g_hiSlipSendBuf[j]];
    j++;

    /* 序号Seq */
    if ((sendInfo->pkgNum & (~HISLIP_EA_BIT)) == HISLIP_FIRST_PKG_NUM) {
        g_hiSlipSendBuf[j] = sendInfo->seq;
        calCrc = g_hiSlipCrcTable[calCrc ^ g_hiSlipSendBuf[j]];
        sendInfo->frameSize = sendInfo->len + HISLIP_FRAME_FIELD_LEN;
        j++;
    } else {
        sendInfo->frameSize = sendInfo->len + HISLIP_FRAME_FIELD_LEN - HISLIP_SEQ_NUM_LEN;
    }

    /* 数据内容 */
    for (i = 0; i < sendInfo->len; i++) {
        g_hiSlipSendBuf[j] = *(sendInfo->data + i);
        calCrc = g_hiSlipCrcTable[calCrc ^ g_hiSlipSendBuf[j]];
        j++;
    }

    /* CRC */
    g_hiSlipSendBuf[j] = calCrc;
    j++;

    /* Flag */
    g_hiSlipSendBuf[j] = END;

    *frame = g_hiSlipSendBuf;
    return HISLIP_OK;
}

/* 发送由HiSlipBuildFrame函数创建的HiSlip帧数据流 */
static short HiSlipSendFrame(const unsigned char* data, unsigned short len)
{
    if ((data == NULL) || (len == 0) || (len > HISLIP_MAX_DATA_LEN)) {
        return HISLIP_ERR;
    }

    g_hiSlipHandler.sendInfo.data = data;
    g_hiSlipHandler.sendInfo.len = len;
    g_hiSlipHandler.sendInfo.frameSize = 0;

    unsigned char* buildFrame = NULL;
    if (HiSlipBuildFrame(&(g_hiSlipHandler.sendInfo), &buildFrame) != HISLIP_OK) {
        return HISLIP_ERR;
    }

    if (g_hiSlipHandler.ioSend(buildFrame, g_hiSlipHandler.sendInfo.frameSize) == 0) {
        return HISLIP_ERR;
    }

    return HISLIP_OK;
}

/* 获取合法有效的序列号 */
static unsigned char HiSlipGetValidSeq(void)
{
    g_hiSlipHandler.sendInfo.autoSeq++;
    g_hiSlipHandler.sendInfo.autoSeq %= HISLIP_MAX_SEQ_NUM;
    return g_hiSlipHandler.sendInfo.autoSeq;
}

static short HiSlipCrcCheck(const unsigned char* data, unsigned short len)
{
    if ((data == NULL) || (len == 0)) {
        return HISLIP_ERR;
    }

    unsigned char i;
    unsigned char calCrc = 0;

    for (i = 0; i < (len - HISLIP_FCS_LEN); i++) {
        calCrc = g_hiSlipCrcTable[calCrc ^ (*data)];
        data++;
    }

    if (calCrc == (*data)) {
        return HISLIP_OK;
    }

    return HISLIP_ERR;
}

static void HiSlipHandleAck(unsigned char seq, unsigned short cmd, unsigned char* data, unsigned short len)
{
    if (data == NULL) {
        return;
    }

    g_hiSlipLastAck.cmd = cmd;
    g_hiSlipLastAck.seq = seq & (~HISLIP_EA_BIT);

    if (g_hiSlipHandler.rcvInfo.ackFunc != NULL) {
        g_hiSlipHandler.rcvInfo.ackFunc(seq & (~HISLIP_EA_BIT), cmd, data, len);
    }
}

/* 处理WiFi模组发来的的数据包 */
static short HiSlipProcessPkg(unsigned char* data, unsigned short len)
{
    if ((data == NULL) || (len == 0)) {
        return HISLIP_ERR;
    }

    unsigned short receiveLen = 0;
    unsigned char* tmp = data;
    unsigned char packageNum = *tmp;
    tmp += HISLIP_PKG_NUM_LEN;

    if ((packageNum & HISLIP_EA_BIT) == 0) {
        /* 不是第一个包的编号 */
        return HISLIP_ERR;
    }

    unsigned char seq = *tmp;
    tmp += HISLIP_SEQ_NUM_LEN;
    short cmd = HiSlipParseDataEA(tmp, &receiveLen);
    if (cmd == HISLIP_ERR) {
        return HISLIP_ERR;
    }

    if (seq == g_hiSlipHandler.rcvInfo.curSeq) {
        if ((seq & HISLIP_EA_BIT) != HISLIP_ACK_TYPE) {
            HiSlipSendAckMsg(seq, (unsigned short)cmd, HISLIP_OK);
        }
        return HISLIP_OK;
    }

    g_hiSlipHandler.rcvInfo.curSeq = seq;
    tmp += receiveLen;
    unsigned short valLen = ((len - HISLIP_PKG_NUM_LEN) - HISLIP_SEQ_NUM_LEN) - receiveLen;

    if ((seq & HISLIP_EA_BIT) == HISLIP_ACK_TYPE) {
        HiSlipHandleAck((seq & (~HISLIP_EA_BIT)), (unsigned short)cmd, tmp, valLen);
    } else {
        if (g_hiSlipHandler.rcvInfo.cmdFunc != NULL) {
            g_hiSlipHandler.rcvInfo.cmdFunc(seq & (~HISLIP_EA_BIT), (unsigned short)cmd, tmp, valLen);
        }
    }

    return HISLIP_OK;
}

static unsigned short HiSlipUartWrite(const unsigned char* buf, unsigned short size)
{
    if ((buf == NULL) || (size < HISLIP_FRAME_MIN_LEN)) {
        return 0;
    }

    HiLinkUartSendOneByte(*buf);
    buf++;

    unsigned short len = 0;
    while (len < (size - HISLIP_FLAG_LEN)) {
        switch (*buf) {
            case END:
                HiLinkUartSendOneByte(ESC);
                HiLinkUartSendOneByte(ESC_END);
                break;
            case ESC:
                HiLinkUartSendOneByte(ESC);
                HiLinkUartSendOneByte(ESC_ESC);
                break;
            default:
                HiLinkUartSendOneByte(*buf);
                break;
        }
        buf++;
        len++;
    }

    HiLinkUartSendOneByte(*buf);
    return size;
}

/* maxSize表示保存读取数据的缓存的最大长度 */
static unsigned short HiSlipUartRead(unsigned char* outBuf, unsigned short maxSize)
{
    if ((outBuf == NULL) || (maxSize == 0)) {
        return 0;
    }

    unsigned short readLen = 0;
    unsigned char i;
    for (i = g_bufReadIndex; i < HISLIP_UART_SKB_NUM; i++) {
        if (g_uartHandler.skb[i].len != 0) {
            readLen = g_uartHandler.skb[i].len;
            g_bufReadIndex = i;
            break;
        }
    }

    if ((readLen == 0) || (g_bufReadIndex >= HISLIP_UART_SKB_NUM)) {
        return 0;
    } else {
        if ((g_uartHandler.skb[g_bufReadIndex].len < HISLIP_FRAME_MIN_LEN) ||
            (g_uartHandler.skb[g_bufReadIndex].len > maxSize)) {
            g_uartHandler.skb[g_bufReadIndex].len = 0;
            g_bufReadIndex = (g_bufReadIndex + 1) % HISLIP_UART_SKB_NUM;
            return 0;
        }
    }

    unsigned short start = g_uartHandler.skb[g_bufReadIndex].startPos;
    unsigned short j;
    if ((start + readLen) <= g_uartHandler.bufSize) {
        for (j = 0; j < readLen; j++) {
            *(outBuf + j) = *(g_uartHandler.buf + start + j);
        }
    } else { /* 帧交叠 */
        unsigned short leftLen = g_uartHandler.bufSize - start;
        unsigned short rightIdx = 0;
        for (j = 0; j < readLen; j++) {
            if (j < leftLen) {
                *(outBuf + j) = *(g_uartHandler.buf + start + j);
                continue;
            }

            *(outBuf + j) = *(g_uartHandler.buf + rightIdx);
            rightIdx++;
        }
        g_uartHandler.readOverlap = 0;
    }

    g_uartHandler.skb[g_bufReadIndex].len = 0;
    g_bufReadIndex = (g_bufReadIndex + 1) % HISLIP_UART_SKB_NUM;
    return readLen;
}

/* 调用硬件IO接口(串口)发送数据 */
static unsigned short HiSlipIoSendData(unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len == 0)) {
        return 0;
    }
    return HiSlipUartWrite(buf, len);
}

/* 通过硬件IO接口(串口)接收数据 */
static unsigned short HiSlipIoRcvData(unsigned char* buf, unsigned short len)
{
    if ((buf == NULL) || (len == 0)) {
        return 0;
    }
    return HiSlipUartRead(buf, len);
}

/*
 * 将整型数据data转换填充成EA结构数据.
 * 出参outLen表示转换后的EA数据长度, 出参outData表示转换后的EA数据.
 * 注意outData可以为NULL, 仅返回outLen, 获取整数转换EA后所占的字节数.
 */
void HiSlipFillDataEA(unsigned short data, unsigned short* outLen, unsigned char* outData)
{
    unsigned char tmp;
    if (data < HISLIP_EA_BIT) {
        tmp = (unsigned char)data;
        tmp = tmp | HISLIP_EA_BIT;
        if (outLen != NULL) {
            *outLen = 1;
        }
        if (outData != NULL) {
            *outData = tmp;
        }
    } else {
        tmp = (unsigned char)(data >> HISLIP_EA_VALID_BITS);
        /* 数据大小超过了HiSlip协议EA结构支持的最大值 */
        if (tmp >= HISLIP_EA_BIT) {
            *outLen = 0;
            return;
        }
        if (outData != NULL) {
            tmp = tmp & (~HISLIP_EA_BIT);
            /* 0表示EA结构的第1个字节 */
            outData[0] = tmp;
            tmp = (unsigned char)(data & (~HISLIP_EA_BIT));
            tmp = tmp | HISLIP_EA_BIT;
            /* 1表示EA结构的第2个字节 */
            outData[1] = tmp;
        }
        if (outLen != NULL) {
            *outLen = HISLIP_EA_MAX_LEN;
        }
    }
}

/* 解析EA结构数据, 返回解析后的整型数据 */
short HiSlipParseDataEA(const unsigned char* data, unsigned short* len)
{
    if ((data == NULL) || (len == NULL)) {
        return HISLIP_ERR;
    }

    short outData;
    /* 0表示EA结构的第1个字节 */
    unsigned char tmp = data[0];

    if ((tmp & HISLIP_EA_BIT) != 0) {
        tmp = tmp & (~HISLIP_EA_BIT);
        outData = tmp;
        *len = 1;
    } else {
        outData = tmp << HISLIP_EA_VALID_BITS;
        /* 1表示EA结构的第2个字节 */
        tmp = data[1];

        /* EA结构最大支持2个字节 */
        if ((tmp & HISLIP_EA_BIT) == 0) {
            return HISLIP_ERR;
        }

        tmp = tmp & (~HISLIP_EA_BIT);
        outData += tmp;
        *len = HISLIP_EA_MAX_LEN;
    }

    return outData;
}

/* 获取最后一次ACK消息的信息(cmd, seq) */
void HiSlipGetLastAckInfo(LastAckInfo* ackInfo)
{
    if (ackInfo != NULL) {
        ackInfo->cmd = g_hiSlipLastAck.cmd;
        ackInfo->seq = g_hiSlipLastAck.seq;
    }
}

/* 发送命令ACK消息, 报文只包含1个TLV结构 */
void HiSlipSendAckMsg(unsigned char seq, unsigned short cmd, unsigned char result)
{
    HiLinkTlvType msg;
    msg.tag = HLK_TAG_OP_RLT;
    msg.len = HISLIP_PARAM_VAL_LEN;
    msg.val = &result;
    HiLinkGeneralCmdSend(cmd, &msg, ACK_MSG_TLV_NUM, (seq | HISLIP_ACK_TYPE));
}

/* HiSlip数据发送, 参数cr表示发送的消息类型(Cmd或Ack) */
void HiSlipSendData(unsigned char* data, unsigned short len, unsigned char cr)
{
    if ((data == NULL) || (len == 0) || (len > HISLIP_SEND_LEN_MAX)) {
        return;
    }

    unsigned char* tmpData = data;
    unsigned short left = len;
    unsigned char seq = 0;
    unsigned char packageNum = 0;

    if ((cr & HISLIP_ACK_TYPE) != 0) {
        seq = cr;
    } else {
        seq = HiSlipGetValidSeq();
        seq |= cr;
    }

    g_hiSlipHandler.sendInfo.seq = seq;

    if (left > HISLIP_MAX_FRM_DATA_LEN) {
        g_hiSlipHandler.sendInfo.pkgNum = packageNum;
        if (HiSlipSendFrame(tmpData, HISLIP_MAX_FRM_DATA_LEN) != HISLIP_OK) {
            return;
        }
        packageNum++;
        tmpData += HISLIP_MAX_FRM_DATA_LEN;
        left -= HISLIP_MAX_FRM_DATA_LEN;

        while (left > HISLIP_MAX_DATA_LEN) {
            g_hiSlipHandler.sendInfo.pkgNum = packageNum;
            if (HiSlipSendFrame(tmpData, HISLIP_MAX_DATA_LEN) != HISLIP_OK) {
                return;
            }
            packageNum++;
            tmpData += HISLIP_MAX_DATA_LEN;
            left -= HISLIP_MAX_DATA_LEN;
        }
    }

    g_hiSlipHandler.sendInfo.pkgNum = packageNum | HISLIP_EA_BIT;

    if (HiSlipSendFrame(tmpData, left) == HISLIP_ERR) {
        return;
    }

    if ((cr & HISLIP_ACK_TYPE) == 0) {
        /* 存储等待ACK的消息seq */
        HiLinkSetCurWaitAckSeq(seq);
    }
}

/* 接收并处理数据报文, 其中返回值HISLIP_RCV_NO_DATA表示没有数据接收 */
short HiSlipRcvData(unsigned char* data, unsigned char len)
{
    if ((data == NULL) || (len == 0)) {
        return HISLIP_ERR;
    }

    /* 接收缓存清空 */
    unsigned char i;
    for (i = 0; i < len; i++) {
        *(data + i) = 0;
    }

    short ret = HISLIP_OK;
    unsigned short receiveLen = g_hiSlipHandler.ioRcv(data, len);
    if (receiveLen == 0) {
        return HISLIP_RCV_NO_DATA;
    } else {
        if (HiSlipCrcCheck(data, receiveLen) == HISLIP_ERR) {
            return HISLIP_ERR;
        }
        /* 此处receiveLen > 0, HISLIP_FCS_LEN值为1, 可确保receiveLen >= HISLIP_FCS_LEN */
        ret = HiSlipProcessPkg(data, (receiveLen - HISLIP_FCS_LEN));
    }

    return ret;
}

void HiSlipUartInit(void)
{
    g_uartHandler.writePos = 0;
    g_uartHandler.readOverlap = 0;
    g_uartHandler.isEsc = 0;
    g_uartHandler.buf = g_uartBuf;
    g_uartHandler.bufSize = sizeof(g_uartBuf);
    g_bufWriteIndex = 0;
    g_bufReadIndex = 0;

    /* 串口消息列表缓存清空 */
    unsigned char i;
    for (i = 0; i < HISLIP_UART_SKB_NUM; i++) {
        g_uartHandler.skb[i].startPos = 0;
        g_uartHandler.skb[i].len = 0;
    }
}

void HiSlipInit(void)
{
    g_hiSlipHandler.ioSend = HiSlipIoSendData;
    g_hiSlipHandler.ioRcv = HiSlipIoRcvData;
    g_hiSlipHandler.rcvInfo.cmdFunc = HiLinkCmdProcess;
    g_hiSlipHandler.rcvInfo.ackFunc = HiLinkAckProcess;
    g_hiSlipHandler.rcvInfo.ackSem = 0;
    g_hiSlipHandler.sendInfo.seq = 0;
    g_hiSlipHandler.sendInfo.autoSeq = 0;

    HiSlipUartInit();
}

/* 处理帧起始或结束标识END */
static void HiSlipProcessEnd(void)
{
    if (g_frameLen != 0) {
        g_uartHandler.skb[g_bufWriteIndex].len = g_frameLen;
        g_bufWriteIndex = (g_bufWriteIndex + 1) % HISLIP_UART_SKB_NUM;

        if (g_bufWriteIndex == g_bufReadIndex) {
            /* 写位置达到了未读位置, 读位置移动到下一个读位置 */
            g_uartHandler.skb[g_bufReadIndex].len = 0;
            g_bufReadIndex = (g_bufReadIndex + 1) % HISLIP_UART_SKB_NUM;
        }

        g_uartHandler.skb[g_bufWriteIndex].startPos = g_uartHandler.writePos;
        g_frameLen = 0;
    }
}

/*
 * 通过串口接收单字节数据, 参数data表示接收的单字节数据.
 * 开发者需要在串口接收中断处理函数中调用本函数.
 */
void HiLinkUartRcvOneByte(unsigned char data)
{
    unsigned char receiveData = data;

    /* 1表示上一个字节收到的是ESC字符 */
    if (g_uartHandler.isEsc == 1) {
        /* 紧接着ESC不是ESC_END或ESC_ESC, 丢弃当前帧 */
        if ((receiveData != ESC_END) && (receiveData != ESC_ESC)) {
            g_uartHandler.skb[g_bufWriteIndex].len = 0;
            g_uartHandler.skb[g_bufWriteIndex].startPos = g_uartHandler.writePos;
            g_frameLen = 0;
            g_uartHandler.isEsc = 0;
        }
    }

    switch (receiveData) {
        case END:
            HiSlipProcessEnd();
            return;
        case ESC:
            /* 设置收到ESC字符标记 */
            g_uartHandler.isEsc = 1;
            break;
        case ESC_END:
            if (g_uartHandler.isEsc == 1) {
                receiveData = END;
                g_uartHandler.isEsc = 0;
            }
            g_frameLen++;
            g_uartHandler.buf[g_uartHandler.writePos] = receiveData;
            g_uartHandler.writePos++;
            break;
        case ESC_ESC:
            if (g_uartHandler.isEsc == 1) {
                receiveData = ESC;
                g_uartHandler.isEsc = 0;
            }
            g_frameLen++;
            g_uartHandler.buf[g_uartHandler.writePos] = receiveData;
            g_uartHandler.writePos++;
            break;
        default:
            g_frameLen++;
            g_uartHandler.buf[g_uartHandler.writePos] = receiveData;
            g_uartHandler.writePos++;
            break;
    }

    if (g_uartHandler.writePos == g_uartHandler.bufSize) {
        g_uartHandler.writePos = 0;
        g_uartHandler.readOverlap = 1;
    }

    if ((g_uartHandler.readOverlap == 1) &&
        (g_uartHandler.writePos == g_uartHandler.skb[g_bufReadIndex].startPos)) {
        g_uartHandler.skb[g_bufReadIndex].len = 0;
        g_bufReadIndex = (g_bufReadIndex + 1) % HISLIP_UART_SKB_NUM;
    }
}
