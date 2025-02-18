#ifndef CMDBUILD_H
#define CMDBUILD_H

#include <stdint.h>
#include <QString>
#include <QByteArray>
#include <QDateTime>

// 命令字枚举类型定义
enum CommandWord : uint8_t {
    CMD_SET_WORK_MODE = 0xF1,
    CMD_SET_FACTORY_IP = 0xC1,
    CMD_FACTORY_CALIBRATION = 0xC2,
    CMD_DATA_COLLECTION = 0xC3,
    CMD_SET_PARAMETERS = 0xC4,
    CMD_QUERY_PARAMETERS = 0xC5,
    CMD_QUERY_SATELLITE_INFO = 0xC6,
    CMD_QUERY_DEVICE_STATUS = 0xC7,
    CMD_NETWORK_TIME_SYNC = 0xC8,
    CMD_FORCE_UPDATE_POSITION = 0xC9,
    CMD_EXCHANGE_SOFTWARE_VERSION = 0xCF,
    CMD_REPORT_COLLECTION_DATA = 0xDA,
    CMD_GET_DEV_PHY_PARAMETERS = 0XCA
};

#pragma pack(push, 1)
//定义不同的结构体存储不同的回复数据
typedef struct {
    uint16_t startFlag;//开始标志
    CommandWord cmdWord;//命令字
    uint8_t devID;//设备ID
    uint32_t time;//时间
    uint32_t packetLength;//数据包长度(包头+数据)
} CmdPacketHeader;//命令数据包头

typedef struct {
    uint8_t workMode;//工作模式
    float current;//板卡电流
    uint8_t batPercent;//电池百分比
    float batVol;//电池电压
    float temperature;//板卡温度
} devPhysicsParameter;//设备物理参数

typedef struct {
    double longitude;//经度
    double latitude;//纬度
    float ellipsoidHeight;//椭球高
    float diffHeight;//高程差
    float horiNorthDire;//水平偏北方向
    float vertiPitchDire;//垂直俯仰方向
    float antennaDistance;//天线距离
    uint8_t positionType;//位置类型
    uint8_t GNSS_QualIndicator;//GNSS质量指标
} satelliteInfo;//卫星信息

typedef struct {
    uint8_t collectionState;//采集状态
    uint8_t timeState;//对时状态
    uint8_t satelliteState;//卫星状态
    uint8_t reserveWord;//预留字
    int32_t totalSpace;//总存储空间
    int32_t freeSpace;//可用存储空间
    float batVol;//电池电压
    float temperature;//板卡温度
} devState;//设备状态

typedef struct {
    //版本号为点分十进制,w1.w2.w3.w4
    uint16_t w1;
    uint16_t w2;
    uint16_t w3;
    uint16_t w4;
} softwareVersion;//软件版本
#pragma pack(pop)

QString getTimestamp();
QByteArray buildCmdPktHeader(CommandWord cmd,uint8_t devID);
QByteArray removeCmdPktHeader(QByteArray response,CmdPacketHeader *header);
#endif // CMDBUILD_H
