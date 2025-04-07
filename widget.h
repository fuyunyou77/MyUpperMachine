#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QProcess>
#include <QMessageBox>
#include <QDateTime>
#include <QRegularExpression>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTimer>
#include "cmdBuild.h"
#include "configmanager.h"
#include "networkmanager.h"

//定义设备工作模式
enum WorkMode : uint8_t {
    NORMAL_MODE=0x00,//正常工作模式
    LOW_POWER_MODE=0x01,//低功耗模式
    DATA_CONLLECT_START_MODE=0x02,//数采启动中
    UNKNOWN_MODE=0xFF//未知模式
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
private:
    Ui::Widget *ui;
    ConfigManager *configManager;

    QPixmap greyLit;
    QPixmap greenLit;
    QPixmap yellowLit;
    QPixmap redLit;


    bool timer_on_flag=false;
    bool timer_stop_flag=false;

    //TODO:设备模式从设备获取更安全,设备出现故障一上电就是低功耗模式,那么这个预设就是有问题的
    WorkMode devStateSet=UNKNOWN_MODE;//设备上电是正常工作模式
    bool changeWorkModeFlag=false;

    //定时获取设备状态信息的定时器
    QTimer timer;
    //创建表组件，存储自定义命令
    QTableWidget * tableWidget= new QTableWidget(0,2,this);

    //创建输入框,保存按钮，新增按钮
    QLineEdit *descriptionEdit = new QLineEdit(this);
    QPushButton *saveSelfDfnCmdBtn = new QPushButton("保存", this);
    QPushButton *addSelfDfnCmdBtn = new QPushButton("新增",this);

    //创建发送和清除按钮
    QPushButton *sendSelfDfnCmdBtn = new QPushButton("发送选中命令", this);
    QPushButton *clearSelfDfnCmdBtn=new QPushButton("清除选中命令", this);

    //该值用来记录tcp发出的命令，在tcp数据接受函数中使用该标志量决定调用什么函数处理回复的消息
    TcpSendCmdType sendCmdFlag=TCP_UNANSWER_STATE;

    //定义各种结构体用来存储和解析下位机数据包内容
    devPhysicsParameter phyPara;//设备物理参数结构体
    satelliteInfo sateInfo;//卫星信息结构体
    devState devSta;//设备状态结构体
    softwareVersion sfVersion;//软件版本结构体
    devWorkParameter devWorkParam;//设备工作参数结构体


    /***********ui布局函数************/
    void uiInit();//初始化ui中的内容,获取一次设备参数更新到对应的栏位
    void SelfDfnCmdArealayout(void);//设置自定义命令区域的ui布局

    /***********逻辑控制函数************/
    //处理不同的命令对应的数据包
    //处理默认数据包，数据部分只有0或1的数据TCP response被称为默认数据包，可以统一处理
    void parseDefalutResponse(QByteArray response);
    //重载以处理其他数据部分不同的数据包，与不同的结构体相对应
    void parseOtherResponse(QByteArray response,devPhysicsParameter *phyPara);
    void parseOtherResponse(QByteArray response,satelliteInfo *sateInfo);
    void parseOtherResponse(QByteArray response,devState *devSta);
    void parseOtherResponse(QByteArray response,softwareVersion *sfVersion);
    void parseOtherResponse(QByteArray response,devWorkParameter *devWorkParam);

    void setGetDevInfoFreq();//设置自动获取设备物理信息频次

private slots:
    void on_normalModeBtn_clicked();//正常工作模式按钮
    void on_lowPowerModeBtn_clicked();//低功耗模式按钮
    void on_connectBtn_clicked();//开启TCP连接按钮
    void on_disconnectBtn_clicked();//断开TCP连接按钮

    void on_sendBtn_clicked();//处理发送按钮按下槽函数

    void on_getDevPhyParaBtn_clicked();//获取设备物理参数信息按键槽函数
    void on_getSateInfoBtn_clicked();//获取设备卫星信息按键槽函数
    void on_getDevStateBtn_clicked();//查询设备状态槽函数
    void on_getDevWorkParaBtn_clicked();//查询设备工作参数槽函数
    void on_forceUpdateLocBtn_clicked();//强制更新位置槽函数

    void on_userDefCmdBtn_clicked();//切换为用户自定义按键页面

    /***动态创建的widget不能使用qt的connect slot by name，命名也不能按照那个规则命名，否则即使***/
    void saveSelfDfnCmdBtn_clicked();//自定义命令保存按钮槽函数
    void addSelfDfnCmdBtn_clicked();//增加自定义命令按钮槽函数
    void sendSelfDfnCmdBtn_clicked();//自定义命令发送按钮槽函数
    void clearSelfDfnCmdBtn_clicked();//清除选中自定义命令按钮槽函数

    void on_setVolThresholdBtn_clicked();//设置电量阈值槽按钮槽函数
    void on_setNormalMessFreqBtn_clicked();//设置正常模式信息获取频次按钮槽函数
    void on_setLowPowMessFreqBtn_clicked();//设置低功耗模式信息获取频次按钮槽函数
    void get_devPhyParam_timeout();//定时获取设备信息槽函数

    void on_stopTimerBtn_clicked();

    /**处理config.json相关信号的槽函数**/
    void initJsonResultHandler(const QString &result);
    void saveToJsonResultHandler(const QString &result)
    {
        QMessageBox::information(this,"注意",result);
    }
    void readFromJsonFailHandler(const QString &result);
//    void readFromJsonSuccessHandler(float result);


};

#endif // WIDGET_H
