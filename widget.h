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

    Widget(QWidget *parent = nullptr);
    ~Widget();
private:
    Ui::Widget *ui;
    ConfigManager *configManager;
    NetworkManager *networkManager;
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




    /***********ui布局函数************/
    void uiInit();//初始化ui中的内容,获取一次设备参数更新到对应的栏位
    void SelfDfnCmdArealayout(void);//设置自定义命令区域的ui布局

    /***********逻辑控制函数************/

    void setGetDevInfoFreq();//设置自动获取设备物理信息频次

private slots:
    void on_normalModeBtn_clicked();//正常工作模式按钮
    void on_lowPowerModeBtn_clicked();//低功耗模式按钮
    void on_connectBtn_clicked();//开启TCP连接按钮
    void on_disconnectBtn_clicked();//断开TCP连接按钮

    void on_sendBtn_clicked();//处理发送按钮按下槽函数

    void on_getDevPhyParaBtn_clicked();//获取设备物理参数信息按键槽函数


    void on_userDefCmdBtn_clicked();//切换为用户自定义按键页面

    /***动态创建的widget不能使用qt的connect slot by name，命名也不能按照那个规则命名，否则即使***/
    void saveSelfDfnCmdBtn_clicked();//自定义命令保存按钮槽函数
    void addSelfDfnCmdBtn_clicked();//增加自定义命令按钮槽函数
    void sendSelfDfnCmdBtn_clicked();//自定义命令发送按钮槽函数
    void clearSelfDfnCmdBtn_clicked();//清除选中自定义命令按钮槽函数

    void on_setVolThresholdBtn_clicked();//设置电量阈值槽按钮槽函数
    void on_setNormalMessFreqBtn_clicked();//设置正常模式信息获取频次按钮槽函数
    void on_setLowPowMessFreqBtn_clicked();//设置低功耗模式信息获取频次按钮槽函数


    /********tcp响应处理槽函数********/
    void HostConnecttedHandler(qint64 bytesWritten);
    void HostDisconnecttedHandler();
    void HostConnectErrorHandler();
    void TcpHexResponseHandler(QByteArray response);
    void WorkmodeSetResponseHandler(QByteArray response);
    void PhyParamResponseHandler(QByteArray response,devPhysicsParameter *phyPara);


    /**处理config.json相关信号的槽函数**/
    void initJsonResultHandler(const QString &result);
    void saveToJsonResultHandler(const QString &result)
    {
        QMessageBox::information(this,"注意",result);
    }
    void readFromJsonFailHandler(const QString &result);

    QString getTimestamp();
    void on_stopTimerBtn_clicked();
    void get_devPhyParam_timeout();//定时获取设备信息槽函数
};

#endif // WIDGET_H
