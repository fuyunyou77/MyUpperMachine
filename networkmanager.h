#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QFlags>
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

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
    TCP_SEND_GET_PHY_PARAMETER=0XCA,//获取设备物理参数命令
    TCP_SEND_SET_DEV_WORKMODE=0xF1//设置设备工作模式
};

// 命令字枚举类型定义
enum CommandWord : uint8_t {
    CMD_SET_WORK_MODE = 0xF1,
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
} CmdPacketHeader;//命令数据包头结构体

typedef struct {
    uint8_t workMode;//工作模式
    float current;//板卡电流
    uint8_t batPercent;//电池百分比
    float batVol;//电池电压
    float temperature;//板卡温度
} devPhysicsParameter;//设备物理参数结构体
#pragma pack(pop)

class NetworkManager:public QObject
{
    Q_OBJECT
public:
    NetworkManager();
    QTcpSocket *socket;
    QString mask="255.255.255.0";
    CmdPacketHeader header;
    devPhysicsParameter phyPara;//设备物理参数结构体

    uint8_t connectToHost(QString IP,QString Port);
    uint8_t sendCmdToHost(QString sendText);
    QByteArray buildCmdPktHeader(CommandWord cmd);
    QString hexToFormatStr(QByteArray);//将接收数据包格式化的函数,方便log打印和阅读
    qint64 tcp_getDevPhyParam();
    //该值用来记录tcp发出的命令，在tcp数据接受函数中使用该标志量决定调用什么函数处理回复的消息
    TcpSendCmdType sendCmdFlag=TCP_UNANSWER_STATE;

    bool isStringInvalid(QString sendText);//判断作为TCP命令被发送的字符串是否非法

    //进行TCP连接前相关输入的检查
    //bool isIPv4Address(const QString &ip);//宽松的ipv4检查
    bool isIPv4AddressEx(const QString &ip,//FIXME:严格的ipv4检查(对于整个网段的ip无法判断)
                        IPv4ValidationFlags flags,//给出不同的IPv4ValidationFlag枚举类型的组合，以允许不同的ip
                        quint32 network ,       // 网络地址（需配合掩码使用）
                        quint32 mask); // 子网掩码（默认不检查网络地址）
    bool isPortValid(const QString &port, bool allowZero);//判断端口号是否合法
    bool isValidSubnetMask(const QString &input);//判断子网掩码是否合法

signals:
    void HostConnectted(qint64 bytesWritten);
    void HostDisconnectted();
    void HostConnectError();
    void TcpHexResponse(QByteArray response);
    void WorkmodeSetResponse(QByteArray response);
    void PhyParamResponse(QByteArray response,devPhysicsParameter *phyPara);

private:


    uint8_t CmdTcpType(uint8_t cmdHeader);//判断发出的数据包的类型，与不同的命令相对应
    QByteArray removeCmdPktHeader(QByteArray response,CmdPacketHeader *header);

private slots:
    void on_hostConnectted();//TCP成功连接
    void on_hostDisconnectted();//TCP断开连接
    void on_hostConnectError();//TCP连接错误

    void on_socketReadyRead(); // 处理下位机响应报文，并传给相应的parse函数

    //处理不同的命令对应的数据包
    //处理默认数据包，数据部分只有0或1的数据TCP response被称为默认数据包，可以统一处理
    void parseWorkmodeSetResponse(QByteArray response);
    //重载以处理其他数据部分不同的数据包，与不同的结构体相对应
    void parsePhyParamResponse(QByteArray response,devPhysicsParameter *phyPara);
};

#endif // NETWORKMANAGER_H
