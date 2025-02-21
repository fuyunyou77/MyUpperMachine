#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QProcess>
#include <QMessageBox>
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include <QDateTime>
#include <QtEndian>
#include <QFlags>
#include <QRegularExpression>
#include "cmdBuild.h"

#define CMD_PORT 21079//命令端口
#define DATA_PORT 21081//数据流端口
#define CMD_DEFAULT_DATA 0xFF//上位机发送命令包的默认数据内容
#define CMD_HEADER_LENGTH 12//命令包包头长度



enum IPv4ValidationFlag {
    AllowNormal           = 0x0001,  // 允许普通地址（默认包含）
    AllowLoopback         = 0x0002,  // 允许环回地址（127.0.0.0/8）
    AllowMulticast        = 0x0004,  // 允许多播地址（224.0.0.0/4）
    AllowLinkLocal        = 0x0008,  // 允许链路本地地址（169.254.0.0/16）
    AllowDocumentation    = 0x0010,  // 允许文档地址（192.0.2.0/24等）
    AllowZeroAddress      = 0x0020,  // 允许全零地址（0.0.0.0）
    AllowBroadcast        = 0x0040,  // 允许有限广播地址（255.255.255.255）
    AllowNetworkBroadcast = 0x0080,  // 允许网络/广播地址标志
    AllowAll              = 0x00FF   // 允许所有地址
};
Q_DECLARE_FLAGS(IPv4ValidationFlags, IPv4ValidationFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(IPv4ValidationFlags)

//定义TCP发送命令类型
enum TcpSendCmdType : uint8_t{
    TCP_UNANSWER_STATE=0x00,//无响应状态,初始默认状态,在该状态下上位机没有发出tcp请求,下位机不应该有tcp响应
    TCP_SEND_DEFAULT_STATE=0xFF,//默认回复状态，单片机只会回复一个字节的0或1
    TCP_SEND_SET_FACTORY_IP = 0xC1,//设置出厂ip
    TCP_SEND_FACTORY_CALIBRATION = 0xC2,//设置出厂校准
    TCP_SEND_DATA_COLLECTION = 0xC3,//采集命令
    TCP_SEND_SET_WORK_PARAMETER = 0xC4,//设置设备工作参数
    TCP_SEND_GET_WORK_PARAMETER=0xC5,//获取设备工作参数
    TCP_SEND_GET_SATELLITE_INFO=0xC6,//获取卫星信息
    TCP_SEND_GET_DEVICE_STATUS=0xC7,//获取设备状态信息
    TCP_SEND_NETWORK_TIME_SYNC = 0xC8,//网络时间同步
    TCP_SEND_FORCE_UPDATE_POSITION = 0xC9,//强制更新位置
    TCP_SEND_EXCHANGE_SOFTWARE_VERSION=0xCF,//双向发送软件版本
    TCP_SEND_REPORT_COLLECTION_DATA = 0xDA,//上报采集数据
    TCP_SEND_GET_PHY_PARAMETER=0XCA,//获取设备物理参数命令
    TCP_SEND_SET_DEV_WORKMODE=0xF1//设置设备工作模式
};

enum WorkMode : uint8_t {
    NORMAL_MODE=0x00,
    LOW_POWER_MODE=0x01
};

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT
    Q_ENUM(IPv4ValidationFlag)

public:
    CmdPacketHeader header;
    Widget(QWidget *parent = nullptr);
    ~Widget();



private slots:
    void on_normalModeBtn_clicked();//正常工作模式按钮
    void on_lowPowerModeBtn_clicked();//低功耗模式按钮
    void on_connectBtn_clicked();//开启TCP连接按钮
    void on_disconnectBtn_clicked();//断开TCP连接按钮

    void on_serverConnectted();//成功连接服务器
    void on_serverDisconnectted();//断开服务器连接
    void on_serverConnectError();

    void on_socketReadyRead(); // 处理下位机响应

    void on_sendBtn_clicked();//处理发送按钮按下槽函数

    void on_getDevPhyParaBtn_clicked();//获取设备物理参数信息按键槽函数
    void on_getSateInfoBtn_clicked();//获取设备卫星信息按键槽函数
    void on_getDevStateBtn_clicked();//查询设备状态槽函数
    void on_getDevWorkParaBtn_clicked();//查询设备工作参数槽函数
    void on_forceUpdateLocBtn_clicked();//强制更新位置槽函数

    void on_userDefCmdBtn_clicked();

private:
    Ui::Widget *ui;

    QTcpSocket *socket;
    QPixmap greyLit;
    QPixmap greenLit;
    QPixmap yellowLit;
    QString mask="255.255.255.0";
    uint8_t devID=0xff;

    //TODO:设备模式从设备获取更安全,设备出现故障一上电就是低功耗模式,那么这个预设就是有问题的
    WorkMode devStateSet=NORMAL_MODE;//设备上电是正常工作模式
    bool changeWorkModeFlag=false;
    //该值用来记录tcp发出的命令，在tcp数据接受函数中使用该标志量决定调用什么函数处理回复的消息
    TcpSendCmdType sendCmdFlag=TCP_UNANSWER_STATE;

    devPhysicsParameter phyPara;
    satelliteInfo sateInfo;
    devState devSta;
    softwareVersion sfVersion;
    devWorkParameter devWorkParam;

    void parseDefalutResponse(QByteArray response);
    //重载以处理不同的数据包
    void parseOtherResponse(QByteArray response,devPhysicsParameter *phyPara);
    void parseOtherResponse(QByteArray response,satelliteInfo *sateInfo);
    void parseOtherResponse(QByteArray response,devState *devSta);
    void parseOtherResponse(QByteArray response,softwareVersion *sfVersion);
    void parseOtherResponse(QByteArray response,devWorkParameter *devWorkParam);
    TcpSendCmdType CmdTcpType(uint8_t cmdHeader);
    bool isStringInvalid(QString sendText);

    //进行TCP连接前相关输入的检查
//    bool isIPv4Address(const QString &ip);
    bool isIPv4AddressEx(const QString &ip,
                        IPv4ValidationFlags flags,
                        quint32 network ,       // 网络地址（需配合掩码使用）
                        quint32 mask); // 子网掩码（默认不检查网络地址）
    bool isPortValid(const QString &port, bool allowZero);
    bool isDevIDValid(const QString &devID);
    bool isValidSubnetMask(const QString &input);
};
#endif // WIDGET_H
