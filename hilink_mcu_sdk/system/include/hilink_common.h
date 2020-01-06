/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK�ڲ��������������ͷ�ļ�
 * Create: 2018-12-01
 */
#ifndef _HILINK_COMMON_H
#define _HILINK_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef NULL
#define NULL 0
#endif

/* HiSlip����֡��ʽ���ȶ��� */
#define HISLIP_FRAME_MIN_LEN    3
#define HISLIP_FLAG_LEN         2
#define HISLIP_PARAM_VAL_LEN    1
#define HISLIP_PKG_NUM_LEN      1
#define HISLIP_SEQ_NUM_LEN      1
#define HISLIP_FCS_LEN          1
#define HISLIP_FRAME_FIELD_LEN  5
#define HISLIP_MAX_FRM_DATA_LEN 123
#define HISLIP_MAX_DATA_LEN     124
#define HISLIP_MAX_FRM_INFO_LEN 126
#define HISLIP_MAX_FRM_LEN      128

/* HiSlip EA��ʽ�ֶ���غ궨�� */
#define HISLIP_EA_MAX_LEN    2
#define HISLIP_EA_VALID_BITS 7
#define HISLIP_EA_BIT        0x80

/* HiSlipЭ�鱨�ĸ�ʽ�����ַ� */
#define END     0xC0
#define ESC     0xDB
#define ESC_END 0xDC
#define ESC_ESC 0xDD

/* HiSlip��Ϣ���� */
#define HISLIP_CMD_TYPE 0
#define HISLIP_ACK_TYPE 0x80

/* HiSlip�׸����ݰ���� */
#define HISLIP_FIRST_PKG_NUM 0
/* CRC����� */
#define HISLIP_CRC_TABLE_LEN 256
/* �������ݰ���󳤶� */
#define HISLIP_SEND_LEN_MAX 15872
/* HiSlip������ݰ���� */
#define HISLIP_MAX_SEQ_NUM 127
/* HiSlip����Ϣ���ճ�ʱʱ��(��λ����) */
#define HISLIP_NO_MSG_TIMEOUT 10000
/* HiSlip���ڷ��ͻ����С */
#define HISLIP_UART_BUF_SIZE 512
/* HiSlip��󻺴���Ϣ���� */
#define HISLIP_UART_SKB_NUM 16

/* WiFi�ź�ǿ���ֶ��ֽڳ��� */
#define WIFI_RSSI_LEN 4
/* �豸MAC��ַ�ֽڳ��� */
#define DEV_MAC_LEN 6
/* �豸SN��󳤶� */
#define DEV_SN_MAX_LEN 40
/* �豸AC��Կ���� */
#define HLK_AC_LEN 48
/* ģ����Ϣ�ֶ���󳤶� */
#define MODULE_INFO_FIELD_MAX_LEN 1024
/* �ȴ�ACK��ʱʱ�� */
#define ACK_TIMEOUT_VALUE 500
/* �ַ�������������󳤶� */
#define STR_KEY_MAX_LEN 255
/* ��ģ���ȡ��UTCʱ�仺��ĳ��� */
#define UTC_TIME_BUF_LEN 7

/* MCU�����ļ���С��ռ�ֽڳ��� */
#define OTA_BIN_SIZE_LEN 4
/* ģ����MCU���η������������ݴ�С */
#define OTA_ONE_PKG_LEN 100

/* ACK��Ϣ����TLV�ֶ��� */
#define ACK_MSG_TLV_NUM 1
/* ����ģ��������Ϣ����TLV�ֶ��� */
#define MD_REBOOT_TLV_NUM 1
/* ��ȡģ����Ϣ��Ϣ����TLV�ֶ��� */
#define GET_MDINFO_TLV_NUM 1
/* ���ù���ģʽ��Ϣ����TLV�ֶ��� */
#define SET_WM_TLV_NUM 1

/* ע�����ֶ��� */
#define REG_FLAG_ITEM_NUM 1
/* Э��汾���ֶ��� */
#define PROFILE_VER_ITEM_NUM 1
/* �豸��Ϣ�ṹ�ֶ��� */
#define DEV_INFO_ITEM_NUM 6
/* �豸������Ϣ�ṹ�ֶ��� */
#define DEV_NAME_ITEM_NUM 2
/* �豸AC�ֶ��� */
#define DEV_AC_ITEM_NUM 1
/* �豸BI�ֶ��� */
#define DEV_BI_ITEM_NUM 1
/* ������Ϣ�ṹ�ֶ��� */
#define SVC_INFO_ITEM_NUM 3
/* ������Ϣ�ṹ�ֶ��� */
#define KEY_INFO_ITEM_NUM 4

/* �������ϱ�����ֵ��ʱ�� */
#define REPORT_LATER 0 /* �Ժ��ϱ� */
#define REPORT_NOW   1 /* �����ϱ� */

/* �豸��������ֵ�Ƿ������ */
#define DEV_KEY_MASK_SET  1
#define DEV_KEY_MASK_USET 0
#define DEV_MAX_KEY_COUNT 128
#define UNSIGNED_CHAR_BITS 8
#define UNSIGNED_INT_BITS (sizeof(int) * UNSIGNED_CHAR_BITS)
#define UNSIGNED_INT_BYTES 4
#define LOW_BYTE_FLAG 0xFF

/* ���׶δ��������� */
#define INIT_FUNC_NUM         2
#define REG_FUNC_NUM          7
#define SET_WORKMODE_FUNC_NUM 1
#define RPT_INIT_FUNC_NUM     1
#define IDLE_FUNC_NUM         2

/* TAG�ֶ�ȡֵ */
#define HLK_DEV_USR_TAG_MIN 0x40
#define HLK_DEV_USR_TAG_MAX 0x7F
#define HLK_INVALID_VAL 0x4000U

/* ���֧�ַ������ */
#define HLK_MAX_SVCINST_CNT 16
/* ���֧�����Ը��� */
#define HLK_MAX_KEYMAP_CNT 32
/* ���֧��TLV���� */
#define HLK_MAX_TLVS_CNT 80

/* ��������ݻ��� */
#define HLK_SND_BUF_SIZE 640
/* �����������չ���� */
#define HLK_EXT_BUF_SIZE 64

/* CMD�������� */
#define HLK_CMD_DEVCTL_WM       0x01
#define HLK_CMD_DEVCTL_RB       0x02
#define HLK_CMD_DEVCTL_RST      0x03
#define HLK_CMD_DEVCTL_QWM      0x04
#define HLK_CMD_DEVCTL_QNS      0x05
#define HLK_CMD_DEVCTL_QRT      0x06
#define HLK_CMD_MDLRSP_WM       0x07
#define HLK_CMD_MDLRSP_RST      0x08
#define HLK_CMD_MDLRSP_RNS      0x09
#define HLK_CMD_MDLRSP_UGD      0x0A
#define HLK_CMD_DEVUPD_STA      0x0B
#define HLK_CMD_DEVCTL_FRLT     0x0C
#define HLK_CMD_MDCTL_DEV_PROT  0x0D
#define HLK_CMD_MDCTL_DEV_QPROT 0x0E
#define HLK_CMD_DEV_QUPG        0x10
#define HLK_CMD_DEV_SUPG        0x11
#define HLK_CMD_DEV_TRANSING    0x12
#define HLK_CMD_DEV_EUPG        0x13
#define HLK_CMD_DEVREG_SETSTA   0x2000
#define HLK_CMD_DEVREG_DEVINFO  0x2001
#define HLK_CMD_DEVREG_PROT     0x2002
#define HLK_CMD_DEVREG_SVCINFO  0x2003
#define HLK_CMD_DEVQRY_FINFOS   0x2004

/* TAG�������� */
#define HLK_TAG_OP_RLT       0x00
#define HLK_TAG_WK_MD        0x01
#define HLK_TAG_RB_RSN       0x02
#define HLK_TAG_MD_NST       0x03
#define HLK_TAG_UGDST        0x04
#define HLK_TAG_UGDPGS       0x05
#define HLK_TAG_UTC          0x06
#define HLK_TAG_PROT_ITEM    0x07
#define HLK_TAG_PROTKIND     0x08
#define HLK_TAG_PROT_SVCINF  0x09
#define HLK_TAG_PROFILE_STR  0x0A
#define HLK_TAG_PROFILE_TREE 0x0B
#define HLK_TAG_PROFILE_TIME 0x0C
#define HLK_TAG_DEV_SVER     0x10
#define HLK_TAG_DEV_BIN_SIZE 0x11
#define HLK_TAG_DEV_BIN_DATA 0x12
#define HLK_TAG_DEV_BIN_CHK  0x13
#define HLK_TAG_PROFILE_VER  0x2000
#define HLK_TAG_WIFI_SSID    0x2001
#define HLK_TAG_WIFI_PWD     0x2002
#define HLK_TAG_WIFI_RAND    0x2003
#define HLK_TAG_NTP          0x2004
#define HLK_TAG_REGSTA       0x2005
#define HLK_TAG_PROT_PRDTID  0x2006
#define HLK_TAG_PROT_DEVSN   0x2007
#define HLK_TAG_PROT_PRDTMD  0x2008
#define HLK_TAG_PROT_PRDTTP  0x2009
#define HLK_TAG_PROT_DTENN   0x200A
#define HLK_TAG_PROT_MANU    0x200B
#define HLK_TAG_PROT_MAUNENN 0x200C
#define HLK_TAG_PROT_CA      0x200D
#define HLK_TAG_PROT_AC      0x200E
#define HLK_TAG_PROT_BIRSA   0x200F
#define HLK_TAG_FINFO_MAC    0x2010
#define HLK_TAG_FINFO_HW     0x2011
#define HLK_TAG_FINFO_FW     0x2012
#define HLK_TAG_FINFO_APRSSI 0x2013
#define HLK_TAG_FINFO_AP     0x2014
#define HLK_TAG_SDK_TMPD     0x2015

/* MCU SDK���Դ����� */
#define HLK_ECODE_RCV_TAG_ERR            0x01
#define HLK_ECODE_RCV_LEN_ERR            0x02
#define HLK_ECODE_PROFILE_TAG_ERR        0x03
#define HLK_ECODE_PROFILE_LEN_ERR        0x04
#define HLK_ECODE_TLVARRAY_SIZE_SVCSMALL 0x05
#define HLK_ECODE_PROFILE_SVCINST_ERR    0x06
#define HLK_ECODE_TLVARRAY_SIZE_KEYSMALL 0x07
#define HLK_ECODE_PROFILE_KEYMAP_ERR     0x08
#define HLK_ECODE_EXT_BUF_SIZE_SMALL     0x09
#define HLK_ECODE_KEYMAP_DATA_TYPE_ERR   0x0A
#define HLK_ECODE_RCV_WKMODE_ERR         0x10
#define HLK_ECODE_RCV_NETSTA_ERR         0x11
#define HLK_ECODE_RCV_UNKNOWN_CMD        0x12
#define HLK_ECODE_NO_BUF_TO_STORE_STR    0x13

/* HiSlip������ */
typedef enum {
    HISLIP_SIZE_ERR    = -3, /* ��γ��ȴ��� */
    HISLIP_RCV_NO_DATA = -2, /* δ�յ����� */
    HISLIP_ERR         = -1, /* ���� */
    HISLIP_OK          = 0,  /* ���� */
    HISLIP_ERR_MAX
} HiSlipErrCode;

/* ����ֵ�������� */
typedef enum {
    HLK_DATA_TYPE_STR  = 0x00, /* �ַ����� */
    HLK_DATA_TYPE_BOOL = 0x01, /* �������� */
    HLK_DATA_TYPE_INT  = 0x02, /* �������� */
    HLK_DATA_TYPE_UNKNOWN = 0x7F
} HiLinkDataType;

/* HiLink����ֵ���� */
typedef enum {
    HLK_RET_SFAIL     = -2,
    HLK_RET_ERR       = -1,
    HLK_RET_OK        = 0,
    HLK_RET_WACK      = 1,
    HLK_RET_ACK_TMOUT = 2,
    HLK_RET_NEXTSTAGE = 3
} HiLinkRetVal;

/* HiLink����״̬ */
typedef enum {
    HLK_RUN_STATE_INIT       = 0, /* ��ʼ״̬ */
    HLK_RUN_STATE_REGPROFILE = 1, /* ע��Profile��Ϣ */
    HLK_RUN_STATE_SETWKMODE  = 2, /* ����ģ�鹤��ģʽ */
    HLK_RUN_STATE_RPTINIVAL  = 3, /* ��ʼֵ�ϱ� */
    HLK_RUN_STATE_IDLE       = 4, /* ����״̬ */
    HLK_RUN_STATE_MAX        = 5
} HiLinkRunState;

/* ACK�ȴ�״̬ */
typedef enum {
    ACK_STATE_WAITTING  = 0, /* �ȴ�ACK */
    ACK_STATE_ACKED     = 1, /* ���յ�ACK */
    ACK_STATE_ACKTIMOUT = 2, /* �ȴ�ACK��ʱ */
    ACK_STATE_MAX       = 3
} HiLinkAckState;

/* ���Գ�ʼֵ�ϱ�״̬ */
typedef enum {
    HLK_RPT_PROCESSING = 0, /* �����ϱ� */
    HLK_RPT_DONE       = 1  /* �ϱ���� */
} HiLinkRptState;

/* HiLink�����ִ��״̬ */
typedef enum {
    FUNC_PROC_STATE_SNDCMD = 0, /* ����״̬ */
    FUNC_PROC_STATE_WACK   = 1, /* �ȴ�ACK״̬ */
    FUNC_PROC_STATE_MAX    = 2
} HiLinkCmdState;

/* ����״̬ */
typedef enum {
    HLK_UGD_NONE     = 0,
    HLK_UGD_DOWNLOAD = 1,
    HLK_UGD_FLASHOK  = 2
} HiLinkUpgradeState;

/* ����״̬ */
typedef enum {
    HLK_HW_NOTCON = 0,
    HLK_HW_CONOK  = 1,
    HLK_HW_CONMAX
} HiLinkConnectStatus;

/* HiLink������� */
typedef enum {
    HLK_RSLT_OK  = 0, /* �����ɹ�  */
    HLK_RSLT_NOK = 1  /* ����ʧ�� */
} HiLinkOperateResult;

/* ģ�鹤��ģʽ */
typedef enum {
    WKMD_UNKONWN = 0,
    WKMD_OFFLINE = 1,
    WKMD_ONLINE  = 2,
    WKMD_AUTOAP  = 3,
    WKMD_MAX
} ModuleWorkMode;

/* ģ������״̬ */
typedef enum {
    NET_NOAP    = 0, /* δ���AP��Ϣ, ������ */
    NET_NOTCONN = 1, /* �ѻ��AP��Ϣ, δ����AP */
    NET_CONWIFI = 2, /* ������AP, δ�����Ʒ����� */
    NET_ONLINE  = 3, /* �������Ʒ����� */
    NET_UNKONWN = 4  /* δ���AP��Ϣ, δ���� */
} ModuleNetStatus;

/* ģ������ԭ�� */
typedef enum {
    HLK_RB_MDERR = 1,
    HLK_RB_DTERR = 2
} ModuleRebootReason;

/* Profileע��״̬ */
typedef enum {
    HLK_REG_START = 0,
    HLK_REG_END   = 1,
    HLK_REG_MAX   = 2
} ProfileRegStatus;

/* Profile��Ϣ�б����� */
typedef enum {
    TYPE_SVC_MAP   = 0,
    TYPE_KEY_MAP   = 1,
    TYPE_MSG_TABLE = 2
} ProfileInfoType;

/* ����ֵ�ϱ����� */
typedef enum {
    RPT_BY_KEY = 0, /* �������ϱ� */
    RPT_BY_SVC = 1  /* �������ϱ� */
} DEV_REPORT_TYPE;

/* �豸��Ϣ */
typedef struct {
    char* sn;        /* �豸SN */
    char* prodId;    /* �豸ID */
    char* devModel;  /* �豸�ͺ� */
    char* devTypeId; /* �豸����ID */
    char* manuId;    /* ������ID */
    char* mcuVer;    /* �豸MCU�İ汾�� */
} DevInfo;

/* �豸������Ϣ */
typedef struct {
    char* devTypeName; /* �豸����Ӣ������ */
    char* manuName;    /* ������Ӣ������ */
} DevEnName;

/* ������Ϣ */
typedef struct {
    unsigned short svcMapId;
    char* svcType;
    char* svcId;
} SvcInfo;

/* ������Ϣ */
typedef struct {
    unsigned short keyMapId;
    char* svcType;
    char* keyName;
    HiLinkDataType dataType;
} KeyInfo;

/* ����ʵ������ֵ */
typedef struct {
    unsigned short svcMapId;
    unsigned short keyMapId;
    int value;
} MsgInfo;

/* ����ʵ�����Կ��ƻص� */
typedef int (*MsgCtrlFunc)(int msgVal);

/* �豸���ƻص��� */
typedef struct {
    MsgInfo msgInfo;
    MsgCtrlFunc msgFunc;
} HiLinkMsg;

/* HiSlip���������Ϣ�ص� */
typedef void (*ProcessRcvFunc)(unsigned char seq, unsigned short cmd, unsigned char* buf, unsigned short len);

/* ���һ��ACK��Ϣ */
typedef struct {
    unsigned short cmd;
    unsigned char  seq;
} LastAckInfo;

/* HiSlip������Ϣ */
typedef struct {
    unsigned int   ackSem;
    unsigned char  curSeq;
    ProcessRcvFunc cmdFunc;
    ProcessRcvFunc ackFunc;
} HiSlipReceive;

/* HiSlip������Ϣ */
typedef struct {
    const unsigned char* data;
    unsigned short len;
    unsigned short frameSize;
    unsigned char seq;
    unsigned char autoSeq;
    unsigned char pkgNum;
} HiSlipSend;

/* HiSlip IO�ص� */
typedef unsigned short (*HiSlipIoFunc)(unsigned char* data, unsigned short len);

/* HiSlip�����շ��ͻص� */
typedef struct {
    HiSlipSend    sendInfo;
    HiSlipReceive rcvInfo;
    HiSlipIoFunc  ioSend;
    HiSlipIoFunc  ioRcv;
} HiSlipHandler;

/* ������ */
typedef short (*RequestFunc)(void);

/* ACK������ */
typedef short (*ProcessAckState)(unsigned short cmd, unsigned long long startTick);

/* TLV��ʽ���� */
typedef struct {
    unsigned short tag;
    unsigned short len;
    unsigned char* val;
} HiLinkTlvType;

/* ���������� */
typedef struct {
    unsigned short cmd;
    RequestFunc sendReqFunc;
    ProcessAckState processAckState;
} HiLinkReqFuncs;

/* �������� */
typedef struct {
    const HiLinkReqFuncs* reqFuncs;
    unsigned short curFuncIdx;
    unsigned short funcNum;
} HiLinkFuncMatrix;

/* Uart������ */
typedef struct {
    unsigned short startPos;
    unsigned short len;
} UartMsgInfo;

/* Uart���ڴ��� */
typedef struct {
    unsigned short writePos;
    unsigned short readOverlap;
    unsigned char  isEsc;
    unsigned char* buf;
    unsigned short bufSize;
    UartMsgInfo    skb[HISLIP_UART_SKB_NUM];
} UartHandler;

/* WiFiģ����Ϣ */
typedef struct {
    char* mac;
    char* hardVer;
    char* softVer;
    int   rssi;
    char* apName;
} ModuleWifiInfo;

/* ��¼��ǰ�ȴ��ظ�ACK����ϢSeq�� */
void HiLinkSetCurWaitAckSeq(unsigned char seq);

/*
 * ͨ������ͽӿ�.
 * ackSeq��ʾACK���к�.
 */
void HiLinkGeneralCmdSend(unsigned short cmd, const HiLinkTlvType* tlvs,
    unsigned short tlvNum, unsigned char ackSeq);

void HiLinkCmdProcess(unsigned char seq, unsigned short cmd, unsigned char* buf, unsigned short len);

void HiLinkAckProcess(unsigned char seq, unsigned short cmd, unsigned char* buf, unsigned short len);

/* HiLink���̴߳����� */
void HiLinkMainProcess(void);

/*
 * ����������dataת������EA�ṹ����.
 * ����outLen��ʾת�����EA���ݳ���, ����outData��ʾת�����EA����.
 * ע��outData����ΪNULL, ������outLen, ��ȡ����ת��EA����ռ���ֽ���.
 */
void HiSlipFillDataEA(unsigned short data, unsigned short* outLen, unsigned char* outData);

/* ����EA�ṹ����, ���ؽ�������������� */
short HiSlipParseDataEA(const unsigned char* data, unsigned short* len);

/* ��ȡ���һ��ACK��Ϣ����Ϣ(cmd, seq) */
void HiSlipGetLastAckInfo(LastAckInfo* ackInfo);

/* ��������ACK��Ϣ, ����ֻ����1��TLV�ṹ */
void HiSlipSendAckMsg(unsigned char seq, unsigned short cmd, unsigned char result);

/* HiSlip���ݷ���, ����cr��ʾ���͵���Ϣ����(Cmd��Ack) */
void HiSlipSendData(unsigned char* data, unsigned short len, unsigned char cr);

/* ���ղ��������ݱ���, ���з���ֵHISLIP_RCV_NO_DATA��ʾû�����ݽ��� */
short HiSlipRcvData(unsigned char* data, unsigned char len);

void HiSlipUartInit(void);

void HiSlipInit(void);

/* HiSlip��ʼ��&�������Գ�ʼ�� */
void HiLinkDevInit(void);

#endif /* _HILINK_COMMON_H */
