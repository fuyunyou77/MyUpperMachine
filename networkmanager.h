#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QFlags>
#include <QObject>
#include <QTcpSocket>


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


class NetworkManager:public QObject
{
    Q_OBJECT
public:
    NetworkManager();
    QTcpSocket *socket;
    QString mask="255.255.255.0";
    uint8_t connectToHost(QString IP,QString Port);

private:
    QString hexToFormatStr(QByteArray);//将接收数据包格式化的函数,方便log打印和阅读
    TcpSendCmdType CmdTcpType(uint8_t cmdHeader);//判断发出的数据包的类型，与不同的命令相对应
    bool isStringInvalid(QString sendText);//判断作为TCP命令被发送的字符串是否非法

    //进行TCP连接前相关输入的检查
    //bool isIPv4Address(const QString &ip);//宽松的ipv4检查
    bool isIPv4AddressEx(const QString &ip,//FIXME:严格的ipv4检查(对于整个网段的ip无法判断)
                        IPv4ValidationFlags flags,//给出不同的IPv4ValidationFlag枚举类型的组合，以允许不同的ip
                        quint32 network ,       // 网络地址（需配合掩码使用）
                        quint32 mask); // 子网掩码（默认不检查网络地址）
    bool isPortValid(const QString &port, bool allowZero);//判断端口号是否合法
    bool isValidSubnetMask(const QString &input);//判断子网掩码是否合法

private slots:
    void on_serverConnectted();//TCP成功连接
    void on_serverDisconnectted();//TCP断开连接
    void on_serverConnectError();//TCP连接错误

    void on_socketReadyRead(); // 处理下位机响应报文，并传给相应的parse函数
};

#endif // NETWORKMANAGER_H
