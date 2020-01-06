/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: MCU SDK内部公共定义和声明头文件
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

/* HiSlip数据帧格式长度定义 */
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

/* HiSlip EA格式字段相关宏定义 */
#define HISLIP_EA_MAX_LEN    2
#define HISLIP_EA_VALID_BITS 7
#define HISLIP_EA_BIT        0x80

/* HiSlip协议报文格式特殊字符 */
#define END     0xC0
#define ESC     0xDB
#define ESC_END 0xDC
#define ESC_ESC 0xDD

/* HiSlip消息类型 */
#define HISLIP_CMD_TYPE 0
#define HISLIP_ACK_TYPE 0x80

/* HiSlip首个数据包编号 */
#define HISLIP_FIRST_PKG_NUM 0
/* CRC码表长度 */
#define HISLIP_CRC_TABLE_LEN 256
/* 发送数据包最大长度 */
#define HISLIP_SEND_LEN_MAX 15872
/* HiSlip最大数据包序号 */
#define HISLIP_MAX_SEQ_NUM 127
/* HiSlip无消息接收超时时间(单位毫秒) */
#define HISLIP_NO_MSG_TIMEOUT 10000
/* HiSlip串口发送缓存大小 */
#define HISLIP_UART_BUF_SIZE 512
/* HiSlip最大缓存消息个数 */
#define HISLIP_UART_SKB_NUM 16

/* WiFi信号强度字段字节长度 */
#define WIFI_RSSI_LEN 4
/* 设备MAC地址字节长度 */
#define DEV_MAC_LEN 6
/* 设备SN最大长度 */
#define DEV_SN_MAX_LEN 40
/* 设备AC密钥长度 */
#define HLK_AC_LEN 48
/* 模组信息字段最大长度 */
#define MODULE_INFO_FIELD_MAX_LEN 1024
/* 等待ACK超时时间 */
#define ACK_TIMEOUT_VALUE 500
/* 字符串类型属性最大长度 */
#define STR_KEY_MAX_LEN 255
/* 从模组获取的UTC时间缓存的长度 */
#define UTC_TIME_BUF_LEN 7

/* MCU升级文件大小所占字节长度 */
#define OTA_BIN_SIZE_LEN 4
/* 模组向MCU单次发送升级包数据大小 */
#define OTA_ONE_PKG_LEN 100

/* ACK消息报文TLV字段数 */
#define ACK_MSG_TLV_NUM 1
/* 控制模组重启消息报文TLV字段数 */
#define MD_REBOOT_TLV_NUM 1
/* 获取模组信息消息报文TLV字段数 */
#define GET_MDINFO_TLV_NUM 1
/* 设置工作模式消息报文TLV字段数 */
#define SET_WM_TLV_NUM 1

/* 注册标记字段数 */
#define REG_FLAG_ITEM_NUM 1
/* 协议版本号字段数 */
#define PROFILE_VER_ITEM_NUM 1
/* 设备信息结构字段数 */
#define DEV_INFO_ITEM_NUM 6
/* 设备名称信息结构字段数 */
#define DEV_NAME_ITEM_NUM 2
/* 设备AC字段数 */
#define DEV_AC_ITEM_NUM 1
/* 设备BI字段数 */
#define DEV_BI_ITEM_NUM 1
/* 服务信息结构字段数 */
#define SVC_INFO_ITEM_NUM 3
/* 属性信息结构字段数 */
#define KEY_INFO_ITEM_NUM 4

/* 按服务上报属性值的时机 */
#define REPORT_LATER 0 /* 稍后上报 */
#define REPORT_NOW   1 /* 立即上报 */

/* 设备服务属性值是否变更标记 */
#define DEV_KEY_MASK_SET  1
#define DEV_KEY_MASK_USET 0
#define DEV_MAX_KEY_COUNT 128
#define UNSIGNED_CHAR_BITS 8
#define UNSIGNED_INT_BITS (sizeof(int) * UNSIGNED_CHAR_BITS)
#define UNSIGNED_INT_BYTES 4
#define LOW_BYTE_FLAG 0xFF

/* 各阶段处理函数个数 */
#define INIT_FUNC_NUM         2
#define REG_FUNC_NUM          7
#define SET_WORKMODE_FUNC_NUM 1
#define RPT_INIT_FUNC_NUM     1
#define IDLE_FUNC_NUM         2

/* TAG字段取值 */
#define HLK_DEV_USR_TAG_MIN 0x40
#define HLK_DEV_USR_TAG_MAX 0x7F
#define HLK_INVALID_VAL 0x4000U

/* 最大支持服务个数 */
#define HLK_MAX_SVCINST_CNT 16
/* 最大支持属性个数 */
#define HLK_MAX_KEYMAP_CNT 32
/* 最大支持TLV个数 */
#define HLK_MAX_TLVS_CNT 80

/* 命令发送数据缓存 */
#define HLK_SND_BUF_SIZE 640
/* 命令发送数据扩展缓存 */
#define HLK_EXT_BUF_SIZE 64

/* CMD命令类型 */
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

/* TAG属性类型 */
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

/* MCU SDK调试错误码 */
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

/* HiSlip错误码 */
typedef enum {
    HISLIP_SIZE_ERR    = -3, /* 入参长度错误 */
    HISLIP_RCV_NO_DATA = -2, /* 未收到数据 */
    HISLIP_ERR         = -1, /* 错误 */
    HISLIP_OK          = 0,  /* 正常 */
    HISLIP_ERR_MAX
} HiSlipErrCode;

/* 属性值数据类型 */
typedef enum {
    HLK_DATA_TYPE_STR  = 0x00, /* 字符类型 */
    HLK_DATA_TYPE_BOOL = 0x01, /* 布尔类型 */
    HLK_DATA_TYPE_INT  = 0x02, /* 整数类型 */
    HLK_DATA_TYPE_UNKNOWN = 0x7F
} HiLinkDataType;

/* HiLink返回值类型 */
typedef enum {
    HLK_RET_SFAIL     = -2,
    HLK_RET_ERR       = -1,
    HLK_RET_OK        = 0,
    HLK_RET_WACK      = 1,
    HLK_RET_ACK_TMOUT = 2,
    HLK_RET_NEXTSTAGE = 3
} HiLinkRetVal;

/* HiLink运行状态 */
typedef enum {
    HLK_RUN_STATE_INIT       = 0, /* 初始状态 */
    HLK_RUN_STATE_REGPROFILE = 1, /* 注册Profile信息 */
    HLK_RUN_STATE_SETWKMODE  = 2, /* 设置模组工作模式 */
    HLK_RUN_STATE_RPTINIVAL  = 3, /* 初始值上报 */
    HLK_RUN_STATE_IDLE       = 4, /* 空闲状态 */
    HLK_RUN_STATE_MAX        = 5
} HiLinkRunState;

/* ACK等待状态 */
typedef enum {
    ACK_STATE_WAITTING  = 0, /* 等待ACK */
    ACK_STATE_ACKED     = 1, /* 已收到ACK */
    ACK_STATE_ACKTIMOUT = 2, /* 等待ACK超时 */
    ACK_STATE_MAX       = 3
} HiLinkAckState;

/* 属性初始值上报状态 */
typedef enum {
    HLK_RPT_PROCESSING = 0, /* 正在上报 */
    HLK_RPT_DONE       = 1  /* 上报完成 */
} HiLinkRptState;

/* HiLink命令函数执行状态 */
typedef enum {
    FUNC_PROC_STATE_SNDCMD = 0, /* 发送状态 */
    FUNC_PROC_STATE_WACK   = 1, /* 等待ACK状态 */
    FUNC_PROC_STATE_MAX    = 2
} HiLinkCmdState;

/* 升级状态 */
typedef enum {
    HLK_UGD_NONE     = 0,
    HLK_UGD_DOWNLOAD = 1,
    HLK_UGD_FLASHOK  = 2
} HiLinkUpgradeState;

/* 连接状态 */
typedef enum {
    HLK_HW_NOTCON = 0,
    HLK_HW_CONOK  = 1,
    HLK_HW_CONMAX
} HiLinkConnectStatus;

/* HiLink操作结果 */
typedef enum {
    HLK_RSLT_OK  = 0, /* 操作成功  */
    HLK_RSLT_NOK = 1  /* 操作失败 */
} HiLinkOperateResult;

/* 模组工作模式 */
typedef enum {
    WKMD_UNKONWN = 0,
    WKMD_OFFLINE = 1,
    WKMD_ONLINE  = 2,
    WKMD_AUTOAP  = 3,
    WKMD_MAX
} ModuleWorkMode;

/* 模组网络状态 */
typedef enum {
    NET_NOAP    = 0, /* 未获得AP信息, 配网中 */
    NET_NOTCONN = 1, /* 已获得AP信息, 未连接AP */
    NET_CONWIFI = 2, /* 已连接AP, 未连接云服务器 */
    NET_ONLINE  = 3, /* 已连接云服务器 */
    NET_UNKONWN = 4  /* 未获得AP信息, 未配网 */
} ModuleNetStatus;

/* 模组重启原因 */
typedef enum {
    HLK_RB_MDERR = 1,
    HLK_RB_DTERR = 2
} ModuleRebootReason;

/* Profile注册状态 */
typedef enum {
    HLK_REG_START = 0,
    HLK_REG_END   = 1,
    HLK_REG_MAX   = 2
} ProfileRegStatus;

/* Profile信息列表类型 */
typedef enum {
    TYPE_SVC_MAP   = 0,
    TYPE_KEY_MAP   = 1,
    TYPE_MSG_TABLE = 2
} ProfileInfoType;

/* 属性值上报类型 */
typedef enum {
    RPT_BY_KEY = 0, /* 按属性上报 */
    RPT_BY_SVC = 1  /* 按服务上报 */
} DEV_REPORT_TYPE;

/* 设备信息 */
typedef struct {
    char* sn;        /* 设备SN */
    char* prodId;    /* 设备ID */
    char* devModel;  /* 设备型号 */
    char* devTypeId; /* 设备类型ID */
    char* manuId;    /* 制造商ID */
    char* mcuVer;    /* 设备MCU的版本号 */
} DevInfo;

/* 设备名称信息 */
typedef struct {
    char* devTypeName; /* 设备类型英文名称 */
    char* manuName;    /* 制造商英文名称 */
} DevEnName;

/* 服务信息 */
typedef struct {
    unsigned short svcMapId;
    char* svcType;
    char* svcId;
} SvcInfo;

/* 属性信息 */
typedef struct {
    unsigned short keyMapId;
    char* svcType;
    char* keyName;
    HiLinkDataType dataType;
} KeyInfo;

/* 服务实例属性值 */
typedef struct {
    unsigned short svcMapId;
    unsigned short keyMapId;
    int value;
} MsgInfo;

/* 服务实例属性控制回调 */
typedef int (*MsgCtrlFunc)(int msgVal);

/* 设备控制回调表 */
typedef struct {
    MsgInfo msgInfo;
    MsgCtrlFunc msgFunc;
} HiLinkMsg;

/* HiSlip处理接收消息回调 */
typedef void (*ProcessRcvFunc)(unsigned char seq, unsigned short cmd, unsigned char* buf, unsigned short len);

/* 最后一次ACK信息 */
typedef struct {
    unsigned short cmd;
    unsigned char  seq;
} LastAckInfo;

/* HiSlip接收消息 */
typedef struct {
    unsigned int   ackSem;
    unsigned char  curSeq;
    ProcessRcvFunc cmdFunc;
    ProcessRcvFunc ackFunc;
} HiSlipReceive;

/* HiSlip发送消息 */
typedef struct {
    const unsigned char* data;
    unsigned short len;
    unsigned short frameSize;
    unsigned char seq;
    unsigned char autoSeq;
    unsigned char pkgNum;
} HiSlipSend;

/* HiSlip IO回调 */
typedef unsigned short (*HiSlipIoFunc)(unsigned char* data, unsigned short len);

/* HiSlip数据收发和回调 */
typedef struct {
    HiSlipSend    sendInfo;
    HiSlipReceive rcvInfo;
    HiSlipIoFunc  ioSend;
    HiSlipIoFunc  ioRcv;
} HiSlipHandler;

/* 请求函数 */
typedef short (*RequestFunc)(void);

/* ACK处理函数 */
typedef short (*ProcessAckState)(unsigned short cmd, unsigned long long startTick);

/* TLV格式类型 */
typedef struct {
    unsigned short tag;
    unsigned short len;
    unsigned char* val;
} HiLinkTlvType;

/* 请求函数类型 */
typedef struct {
    unsigned short cmd;
    RequestFunc sendReqFunc;
    ProcessAckState processAckState;
} HiLinkReqFuncs;

/* 函数矩阵 */
typedef struct {
    const HiLinkReqFuncs* reqFuncs;
    unsigned short curFuncIdx;
    unsigned short funcNum;
} HiLinkFuncMatrix;

/* Uart缓存区 */
typedef struct {
    unsigned short startPos;
    unsigned short len;
} UartMsgInfo;

/* Uart串口处理 */
typedef struct {
    unsigned short writePos;
    unsigned short readOverlap;
    unsigned char  isEsc;
    unsigned char* buf;
    unsigned short bufSize;
    UartMsgInfo    skb[HISLIP_UART_SKB_NUM];
} UartHandler;

/* WiFi模组信息 */
typedef struct {
    char* mac;
    char* hardVer;
    char* softVer;
    int   rssi;
    char* apName;
} ModuleWifiInfo;

/* 记录当前等待回复ACK的消息Seq号 */
void HiLinkSetCurWaitAckSeq(unsigned char seq);

/*
 * 通用命令发送接口.
 * ackSeq表示ACK序列号.
 */
void HiLinkGeneralCmdSend(unsigned short cmd, const HiLinkTlvType* tlvs,
    unsigned short tlvNum, unsigned char ackSeq);

void HiLinkCmdProcess(unsigned char seq, unsigned short cmd, unsigned char* buf, unsigned short len);

void HiLinkAckProcess(unsigned char seq, unsigned short cmd, unsigned char* buf, unsigned short len);

/* HiLink主线程处理函数 */
void HiLinkMainProcess(void);

/*
 * 将整型数据data转换填充成EA结构数据.
 * 出参outLen表示转换后的EA数据长度, 出参outData表示转换后的EA数据.
 * 注意outData可以为NULL, 仅返回outLen, 获取整数转换EA后所占的字节数.
 */
void HiSlipFillDataEA(unsigned short data, unsigned short* outLen, unsigned char* outData);

/* 解析EA结构数据, 返回解析后的整型数据 */
short HiSlipParseDataEA(const unsigned char* data, unsigned short* len);

/* 获取最后一次ACK消息的信息(cmd, seq) */
void HiSlipGetLastAckInfo(LastAckInfo* ackInfo);

/* 发送命令ACK消息, 报文只包含1个TLV结构 */
void HiSlipSendAckMsg(unsigned char seq, unsigned short cmd, unsigned char result);

/* HiSlip数据发送, 参数cr表示发送的消息类型(Cmd或Ack) */
void HiSlipSendData(unsigned char* data, unsigned short len, unsigned char cr);

/* 接收并处理数据报文, 其中返回值HISLIP_RCV_NO_DATA表示没有数据接收 */
short HiSlipRcvData(unsigned char* data, unsigned char len);

void HiSlipUartInit(void);

void HiSlipInit(void);

/* HiSlip初始化&服务属性初始化 */
void HiLinkDevInit(void);

#endif /* _HILINK_COMMON_H */
