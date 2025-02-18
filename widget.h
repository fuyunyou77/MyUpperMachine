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
#include "cmdBuild.h"

#define CMD_PORT 21079//命令端口
#define DATA_PORT 21081//数据流端口
#define CMD_DEFAULT_DATA 0xFF//上位机发送命令包的默认数据内容
#define CMD_HEADER_LENGTH 12//命令包包头长度
#define DEV_DEFAULT_ID 0xFF//默认设备ID

//定义TCP发送命令类型
enum TcpSendCmdType : uint8_t{
    TCP_UNANSWER_STATE=0x00,//无响应状态,初始默认状态,在该状态下上位机没有发出tcp请求,下位机不应该有tcp响应
    TCP_SEND_DEFAULT_STATE=0xFF,//默认回复状态，单片机只会回复一个字节的0或1
    TCP_SEND_GET_DEV_PARAMETER=0XCA,//获取设备物理参数命令，需要在tcp数据接收函数中调用命令解析函数
    TCP_SEND_GET_WORK_PARAMETER=0xC5,//获取设备工作参数
    TCP_SEND_GET_SATELLITE_INFO=0xC6,//获取卫星信息
    TCP_SEND_GET_DEVICE_STATUS=0xC7,//获取设备状态信息
    TCP_EXCHANGE_SOFTWARE_VERSION=0xCF//双向发送软件版本

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
    void on_serverDisconnnectted();//断开服务器连接
    void on_serverConnectError();

    void on_socketReadyRead(); // 处理下位机响应

    void on_sendBtn_clicked();//处理发送按钮按下槽函数

    void on_getDevPhyParaBtn_clicked();//获取设备物理参数信息按键槽函数
    void on_getSateInfoBtn_clicked();//获取设备卫星信息按键槽函数
    void on_getDevStateBtn_clicked();//查询设备状态槽函数
    void on_getDevWorkParaBtn_clicked();//查询设备工作参数槽函数
    void on_forceUpdateLocBtn_clicked();//强制更新位置槽函数

private:
    Ui::Widget *ui;

    QTcpSocket *socket;
    QPixmap greyLit;
    QPixmap greenLit;
    QPixmap yellowLit;
    QString mask="255.255.255.0";

    //TODO:设备模式从设备获取更安全,设备出现故障一上电就是低功耗模式,那么这个预设就是有问题的
    WorkMode devStateSet=NORMAL_MODE;//设备上电是正常工作模式
    //该值用来记录tcp发出的命令，在tcp数据接受函数中使用该标志量决定调用什么函数处理回复的消息
    TcpSendCmdType sendCmdFlag=TCP_UNANSWER_STATE;

    devPhysicsParameter phyPara;
    satelliteInfo sateInfo;
    devState devSta;
    softwareVersion sfVersion;

    void parseDefalutResponse(QByteArray response);
    //重载以处理不同的数据包
    void parseOtherResponse(QByteArray response,devPhysicsParameter *phyPara);
    void parseOtherResponse(QByteArray response,satelliteInfo *sateInfo);
    void parseOtherResponse(QByteArray response,devState *devSta);
    void parseOtherResponse(QByteArray response,softwareVersion *sfVersion);
    TcpSendCmdType CmdTcpType(uint8_t cmdHeader);
};


#endif // WIDGET_H
