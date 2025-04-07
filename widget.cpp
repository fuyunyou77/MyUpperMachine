#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
//    , greyLit(":/icon/grey_light.png")
//    , greenLit(":/icon/green_light.png")
//    , yellowLit(":/icon/yellow_light.png")
//    , redLit(":/icon/red_light.png")
{
    ui->setupUi(this);
    configManager = new ConfigManager;

    //连接json处理信号和槽函数
    connect(configManager,&ConfigManager::initJsonResult,this,&Widget::initJsonResultHandler);
    connect(configManager,&ConfigManager::readFromJsonFail,this,&Widget::readFromJsonFailHandler);
//    connect(configManager,&ConfigManager::readFromJsonSuccess,this,&Widget::readFromJsonSuccessHandler);
    connect(configManager,&ConfigManager::saveToJsonResult,this,&Widget::saveToJsonResultHandler);

    uiInit();//调用初始化函数初始化ui



    //日志区清空按钮
    connect(ui->logClearBtn,&QPushButton::clicked,[this]()
    {
        ui->logPlainTextEdit->clear();//清空日志区
    });
    //发送区清空按钮
    connect(ui->sendClearBtn,&QPushButton::clicked,[this]()
    {
        ui->sendTextEdit->clear();//清空发送区
    });

    connect(sendSelfDfnCmdBtn,&QPushButton::clicked,this,&Widget::sendSelfDfnCmdBtn_clicked);
    connect(saveSelfDfnCmdBtn,&QPushButton::clicked,this,&Widget::saveSelfDfnCmdBtn_clicked);
    connect(clearSelfDfnCmdBtn,&QPushButton::clicked,this,&Widget::clearSelfDfnCmdBtn_clicked);
    connect(addSelfDfnCmdBtn,&QPushButton::clicked,this,&Widget::addSelfDfnCmdBtn_clicked);

    connect(&timer, &QTimer::timeout,this,&Widget::get_devPhyParam_timeout);


    qDebug()<<"setLowPowMessFreq from lineedit :"<<ui->setLowPowMessFreqLineEdit->text();
/*还有许多按钮等widget没有被显式的连接相应的信号与槽，原因是定义了符合qt规则的标准槽函数
 * qt会直接将这些按钮的信号与槽函数默认隐式绑定，如果再显示的绑定反而会使信号重复触发
*/
}

Widget::~Widget()
{
    delete ui;
}

/**
 * @brief:ui初始化，设置label宽度，设置状态灯图片，初始化config.json文件，设置自定义命令模块的布局
 * @param:无
 * @retval:无
 */
void Widget::uiInit()
{
    greyLit.load(":/icon/grey_light.png");
    greenLit.load(":/icon/green_light.png");
    yellowLit.load(":/icon/yellow_light.png");
    redLit.load(":/icon/red_light.png");
    //设置状态提示label宽度
    QFontMetrics metrics(ui->stateLabel_1->font());
    int textWidth = metrics.horizontalAdvance("电量状态");
    ui->stateLabel_1->setFixedWidth(textWidth + 2); // 增加额外宽度
    ui->stateLabel_2->setFixedWidth(textWidth + 2); // 增加额外宽度
    ui->stateLabel_3->setFixedWidth(textWidth + 2); // 增加额外宽度

    //状态灯label上放上图片
    ui->devStateLitLabel->setPixmap(greyLit.scaled(60,60));//设备状态
    ui->netStateLitLabel ->setPixmap(greyLit.scaled(60,60));//网络状态
    ui->batStateLitLabel ->setPixmap(greyLit.scaled(60,60));//电量状态

    //设置模式控制按钮的选中状态
    ui->normalModeBtn->setCheckable(false);
    ui->lowPowerModeBtn->setCheckable(false);

    //设置自定义命令模块的布局
    SelfDfnCmdArealayout();

    //初始化json文件
    if(true==configManager->initJson())
    {
        //将预设值填入ui对应栏位
        ui->setVolThresholdBtnLineEdit->setText(QString::number( configManager->readFromJson("VolThreshold")));
        ui->setNormalMessFreqLineEdit->setText(QString::number( configManager->readFromJson("NormalMessFreq")));
        ui->setLowPowMessFreqLineEdit->setText(QString::number( configManager->readFromJson("LowPowMessFreq")));
    }
    else
    {
        QMessageBox::warning(this,"警告","config.json创建失败,请手动添加!");
    }

}

void Widget::SelfDfnCmdArealayout(void)
{
    //创建布局
    QVBoxLayout *selfDfnCmdVlayout = new QVBoxLayout(ui->SelfDefineCmdArea);
    QHBoxLayout *selfDfnCmdHlayout1 = new QHBoxLayout();
    QHBoxLayout *selfDfnCmdHlayout2 = new QHBoxLayout();

    //设置表头
    tableWidget->setHorizontalHeaderLabels(QStringList()<<"描述"<<"命令");
    tableWidget->horizontalHeader()->setStretchLastSection(true);

    //将保存按键和输入栏放入水平布局1中
    selfDfnCmdHlayout1->addWidget(descriptionEdit);
    selfDfnCmdHlayout1->addWidget(saveSelfDfnCmdBtn);
    selfDfnCmdHlayout1->addWidget(addSelfDfnCmdBtn);

    //将发送和清除按钮放入水平布局2中
    selfDfnCmdHlayout2->addWidget(sendSelfDfnCmdBtn);
    selfDfnCmdHlayout2->addWidget(clearSelfDfnCmdBtn);

    // 将命令表和两个水平布局添加到垂直布局中
    selfDfnCmdVlayout->addWidget(tableWidget);
    selfDfnCmdVlayout->addLayout(selfDfnCmdHlayout1);
    selfDfnCmdVlayout->addLayout(selfDfnCmdHlayout2);

    //将垂直布局添加到SelfDfnCmdArea中
    ui->SelfDefineCmdArea->setLayout(selfDfnCmdVlayout);

}

/**
 * @brief：设置普通模式按钮槽函数：
 * 1.发送“正常模式设置命令”
 * 2.更新标志量
 *@param ：无
 *@retval：无
*/
void Widget::on_normalModeBtn_clicked()
{
    if(socket->state()==QAbstractSocket::UnconnectedState)
    {
        qDebug()<<"normalModeBtn";
        QMessageBox::information(this,"错误","未连接单片机");
    }
    else
    {
        ui->normalModeBtn->setChecked(true);
        ui->lowPowerModeBtn->setChecked(false);

        if(NORMAL_MODE==devStateSet)
        {
            QMessageBox::information(this,"注意","请勿重复操作!");
            return;
        }

        //定义要发送的数据包
        QByteArray packet;

        //开始构造数据包
        packet.append(buildCmdPktHeader(CMD_SET_WORK_MODE,devID));
        // 包总长度（4字节，包头12B + 数据1B = 13 → 0x0D）
        uint32_t totalLength = 13;
        packet.append(reinterpret_cast<char*>(&totalLength), 4);
        // 数据内容（1字节）
        packet.append(static_cast<char>(NORMAL_MODE));
//        packet.append(static_cast<char>(0x00));//00为正常模式

        //将数据通过tcp发出,根据返回值打印日志信息
        qint64 bytesWritten = socket->write(packet);
        if (bytesWritten == -1) {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "发送失败: " + socket->errorString());
        } else {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "发送成功！正在设置为正常工作模式...");
            QString hexPacket=hexToFormatStr(packet);
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "Hex:"+hexPacket);

            //发送成功，更改相应的标志量设置
            devStateSet=NORMAL_MODE;
            sendCmdFlag=TCP_SEND_DEFAULT_STATE;
            changeWorkModeFlag=true;
        }
    }
}

/**
 * @brief:将输入的二级制串转换为 以空格分割字节的 全大写的 格式化字符串,方便log打印和阅读
 * eg:(QByteArray)0x123456ef->(QString)12 34 56 EF
 * @param:QByteArray packet 数据包
 * @retval:QString 格式化的数据包字符串
*/
QString Widget::hexToFormatStr(QByteArray packet)
{
    QString formatPacket=packet.toHex().toUpper();
    formatPacket=formatPacket.replace(QRegularExpression("(..)"),"\\1 ").trimmed();
    return formatPacket;
}

/*@brief：设置低功耗模式按钮槽函数：
 * 1.发送“低功耗模式设置命令”
 * 2.更新标志量
 *@param ：无
 *@retval：无
*/
void Widget::on_lowPowerModeBtn_clicked()
{

    if(socket->state()==QAbstractSocket::UnconnectedState)
    {
        qDebug()<<"lowPowerModeBtn";
        QMessageBox::information(this,"错误","未连接单片机");
    }
    else
    {
        ui->lowPowerModeBtn->setChecked(true);
        ui->normalModeBtn->setChecked(false);

        if(LOW_POWER_MODE==devStateSet)
        {
            QMessageBox::information(this,"注意","请勿重复操作!");
            return;
        }

        QByteArray packet;
        packet.append(buildCmdPktHeader(CMD_SET_WORK_MODE,devID));
        // 包总长度（4字节，包头12B + 数据1B = 13 → 0x0D）
        uint32_t totalLength = 13;
        packet.append(reinterpret_cast<char*>(&totalLength), 4);
        // 数据内容（1字节）
        packet.append(static_cast<char>(LOW_POWER_MODE));
//        packet.append(static_cast<char>(0x01));//01为低功耗模式

        //将数据通过tcp发出,根据返回值打印日志信息
        qint64 bytesWritten = socket->write(packet);
        if (bytesWritten == -1) {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "发送失败: " + socket->errorString());
        } else {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "发送成功！正在设置为低功耗模式...");
            QString hexPacket=hexToFormatStr(packet);
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "Hex:"+hexPacket);
            //更改标志量设置
            devStateSet=LOW_POWER_MODE;
            sendCmdFlag=TCP_SEND_DEFAULT_STATE;
            changeWorkModeFlag=true;
        }
    }
}

/*@brief：连接下位机按钮槽函数：
 * 1.判断输入的IP，端口号等内容是否合法
 * 2.建立与下位机的TCP连接
 * 2.更新标志量
 *@param ：无
 *@retval：无
*/
void Widget::on_connectBtn_clicked()
{
    //从输入框获取ip地址和端口
    QString IP = ui->IPLineEdit->text();
    QString port = ui->PortLineEdit->text();
    QString recvMask=ui->MaskLineEdit->text();
//    QString recvID=ui->devIDLineEdit->text();
    QString recvID="255";

    if(QAbstractSocket::ConnectedState==socket->state())
    {
        QMessageBox::information(this,"注意","已经连接下位机,请先断开连接!");
        return;
    }

    if(IP.isEmpty()||port.isEmpty())
    {
        QMessageBox::information(this,"注意","未输入IP地址或端口号!");
        return;
    }

    //子网掩码可以为空,此时使用默认的子网掩码255.255.255.0
    if(!recvMask.isEmpty() && !isValidSubnetMask(recvMask))
    {
        QMessageBox::information(this,"注意","输入的子网掩码无效!");
        return;
    }
    else if(isValidSubnetMask(recvMask))//如果子网掩码为空,使用接收到的子网掩码代替存储的子网掩码
    {
        mask=recvMask;
    }

    //TODO:在此处处理子网掩码将Qstring类型转换为quint32传递给下面的ipv4验证代码
    if(!isIPv4AddressEx(IP,
                        AllowNormal|AllowLoopback|AllowMulticast,
                        0xC0A80000,//子网网段,192.168.0.0
                        0xFFFFFF00//子网掩码,255.255.255.0
                        ))
    {
        QMessageBox::information(this,"注意","输入的IP地址无效!");
        return;
    }


    qDebug()<<"socket state:"<<socket->state();
    //连接服务器
    socket->connectToHost(QHostAddress(IP),port.toUShort());
    ui->logPlainTextEdit->appendPlainText(getTimestamp()+"正在进行TCP连接...");

}



/**
 * @brief：下位机断开连接按钮槽函数：
 * 1.如果已连接下位机，断开与下位机的TCP连接
 * 2.如果正在尝试连接下位机，终止正在进行的连接操作
 *@param ：无
 *@retval：无
*/
void Widget::on_disconnectBtn_clicked()
{  
    QAbstractSocket::SocketState state=socket->state();

    if(QAbstractSocket::ConnectingState==state
       ||QAbstractSocket::HostLookupState==state)
    {
        socket->abort();
        ui->logPlainTextEdit->appendPlainText(getTimestamp()+"已中断连接行为!");
        devStateSet=NORMAL_MODE;
    }
    else if(QAbstractSocket::ConnectedState==state)
    {
        socket->disconnectFromHost();
        devStateSet=NORMAL_MODE;
    }
    else if(QAbstractSocket::UnconnectedState==state)
    {
        QMessageBox::information(this,"注意","未连接下位机!");
    }
}


void Widget::parseDefalutResponse(QByteArray response)
{
    sendCmdFlag = TCP_UNANSWER_STATE;
    uint8_t tcpRespond = static_cast<uint8_t>(response.at(0));
    QString logText = getTimestamp();

    switch (tcpRespond) {

    case 0:
//    case 2:
        logText += "设置成功!";
        if(NORMAL_MODE==devStateSet)//正常工作模式设置成功
        {
            //与工作模式切换相关的ui变化只有在确定发出来工作模式切换请求的情况下(标志量为真)才进行
            if(true==changeWorkModeFlag)
            {
                ui->devStateLitLabel->setPixmap(greenLit.scaled(60,60));//设备状态指示灯变为绿色
                ui->normalModeBtn->setChecked(true);
                ui->lowPowerModeBtn->setChecked(false);
                changeWorkModeFlag=false;
            }
        }else if(LOW_POWER_MODE==devStateSet)//低功耗模式设置成功
        {
            if(true==changeWorkModeFlag)
            {
                ui->devStateLitLabel->setPixmap(yellowLit.scaled(60,60));//设备状态指示灯变为黄色
                ui->normalModeBtn->setChecked(false);
                ui->lowPowerModeBtn->setChecked(true);
                changeWorkModeFlag=false;
            }
        }

        break;
    case 1:
        logText += "设置失败!";

        if(NORMAL_MODE==devStateSet)//正常模式设置失败
        {
            if(true==changeWorkModeFlag)
            {
                ui->devStateLitLabel->setPixmap(yellowLit.scaled(60,60));//设备状态指示灯变为黄色
                ui->normalModeBtn->setChecked(false);
                ui->lowPowerModeBtn->setChecked(true);
                changeWorkModeFlag=false;
                devStateSet=LOW_POWER_MODE;
            }
        }
        else if(LOW_POWER_MODE==devStateSet)//低功耗模式设置失败
        {
            if(true==changeWorkModeFlag)
            {
                ui->devStateLitLabel->setPixmap(greenLit.scaled(60,60));//设备状态指示灯变为绿色
                ui->normalModeBtn->setChecked(true);
                ui->lowPowerModeBtn->setChecked(false);
                changeWorkModeFlag=false;
                devStateSet=NORMAL_MODE;
            }
        }
        //TODO:设备状态设置成功时,状态可知,可是没有一个参数用来表示设备当前的工作状态
        break;
//    case 2:
//        logText += "数采启动中,请稍后操作...";
//        if(NORMAL_MODE==devStateSet)//正常模式设置失败
//        {
//            if(true==changeWorkModeFlag)
//            {
//                ui->devStateLitLabel->setPixmap(yellowLit.scaled(60,60));//设备状态指示灯变为黄色
//                ui->normalModeBtn->setChecked(false);
//                ui->lowPowerModeBtn->setChecked(true);
//                changeWorkModeFlag=false;
//                devStateSet=LOW_POWER_MODE;
//            }
//        }
//        else if(LOW_POWER_MODE==devStateSet)//低功耗模式设置失败
//        {
//            if(true==changeWorkModeFlag)
//            {
//                ui->devStateLitLabel->setPixmap(greenLit.scaled(60,60));//设备状态指示灯变为绿色
//                ui->normalModeBtn->setChecked(true);
//                ui->lowPowerModeBtn->setChecked(false);
//                changeWorkModeFlag=false;
//                devStateSet=NORMAL_MODE;
//            }
//        }
//        break;
    default:
        logText += "未知响应!";
        break;
    }

    QString hexResponse=response.toHex();
    logText.append("(Hex:"+hexResponse+")");

    ui->logPlainTextEdit->appendPlainText(logText); // 记录日志
}

//实现接收数据包解析,将结构体指针与数据包对齐
void Widget::parseOtherResponse(QByteArray response, devPhysicsParameter *phyPara)
{
    qDebug()<< "get phy param response sendcmdflag:"<<sendCmdFlag;
    sendCmdFlag = TCP_UNANSWER_STATE;
    if(response.size()< static_cast<int>(sizeof(devPhysicsParameter)))
    {
        ui->logPlainTextEdit->appendPlainText(getTimestamp()+"下位机响应回复物理参数数据包长度有误!");
//        QMessageBox::information(this,"警告","下位机响应回复物理参数数据包长度有误!");
        return;
    }
    else
    {

        qDebug() << "Size of devPhysicsParameter:" << sizeof(devPhysicsParameter);
        memcpy(phyPara,response.constData(),sizeof(devPhysicsParameter));

        //日志区打印设备物理参数
        ui->logPlainTextEdit->appendPlainText(getTimestamp()+"获取板卡物理参数如下:");

        ui->logPlainTextEdit->appendPlainText("工作模式: " + QString::number(phyPara->workMode));

        qDebug() << "phyPara->batPercent:"<<phyPara->batPercent;
        //限定电压的最大最小值，大于最大值
        if(phyPara->batVol>12.48f)
        {
            phyPara->batPercent=100;
        }else if(phyPara->batVol<10.74f)
        {
            phyPara->batPercent=0;
        }

        ui->logPlainTextEdit->appendPlainText("电池百分比: " + QString::number(phyPara->batPercent) + " %");
        ui->logPlainTextEdit->appendPlainText("电池电压: " + QString::number(phyPara->batVol, 'f', 2) + " V");
        ui->logPlainTextEdit->appendPlainText("板卡温度: " + QString::number(phyPara->temperature, 'f', 2) + " °C");
        ui->logPlainTextEdit->appendPlainText("电池电压 (hex): " + QString::number(*reinterpret_cast<uint32_t*>(&phyPara->batVol), 16));
        ui->logPlainTextEdit->appendPlainText("板卡温度 (hex): " + QString::number(*reinterpret_cast<uint32_t*>(&phyPara->temperature), 16));

        //将物理参数显示在对应的文本框
        ui->BatVolLineEdit->setText(QString::number(phyPara->batVol, 'f', 2) + " V");//显示电池电压
        if((float)(phyPara->batPercent)<=configManager->readFromJson("VolThreshold"))
        {
            ui->BatPercentLineEdit->setText(QString::number(phyPara->batPercent) + " %");//显示电池百分比，并显示报警文字
            ui->batStateLitLabel->setPixmap(redLit.scaled(60,60));
        }
        else
        {
            ui->BatPercentLineEdit->setText(QString::number(phyPara->batPercent) + " %");//显示电池百分比
            ui->batStateLitLabel->setPixmap(greenLit.scaled(60,60));
        }
        //功耗限制在16.2w
        if(16.2<=(phyPara->current)*(phyPara->batVol))
        {
            phyPara->current=16.2/phyPara->batVol;
        }
        ui->CurrentLineEdit->setText(QString::number(phyPara->current, 'f', 2) + " A");//显示板卡电流
        ui->TemperLineEdit->setText(QString::number(phyPara->temperature, 'f', 2) + " °C");//显示板卡温度
        ui->logPlainTextEdit->appendPlainText("板卡电流: " + QString::number(phyPara->current, 'f', 2) + " A");
        ui->normalModeBtn->setCheckable(true);
        ui->lowPowerModeBtn->setCheckable(true);

        if(NORMAL_MODE==phyPara->workMode||DATA_CONLLECT_START_MODE==phyPara->workMode)
        {
            timer.setInterval((int)(1000*configManager->readFromJson("NormalMessFreq")));//重新设置自动获取参数间隔
            ui->devStateLitLabel->setPixmap(greenLit.scaled(60,60));//设备状态指示灯变为绿色
            ui->normalModeBtn->setChecked(true);
            ui->lowPowerModeBtn->setChecked(false);
            devStateSet=NORMAL_MODE;
        }
        else if(LOW_POWER_MODE==phyPara->workMode)
        {
            timer.setInterval((int)(1000*configManager->readFromJson("LowPowMessFreq")));//重新设置自动获取参数间隔
            ui->devStateLitLabel->setPixmap(yellowLit.scaled(60,60));//设备状态指示灯变为黄色
            ui->normalModeBtn->setChecked(false);
            ui->lowPowerModeBtn->setChecked(true);
            devStateSet=LOW_POWER_MODE;
        }

        if(!timer_on_flag&&!timer_stop_flag)
        {
            qDebug()<<"timer_on_flag"<<timer_on_flag;
            setGetDevInfoFreq();//开启定时器，定时获取设备参数
            timer_on_flag=true;
        }

    }

}

void Widget::parseOtherResponse(QByteArray response, satelliteInfo *satInfo)
{
    if(response.size()< static_cast<int>(sizeof(satelliteInfo)))
    {
        QMessageBox::information(this,"警告","下位机响应回复卫星信息数据包长度有误!");
        return;
    }
    else
    {
        memcpy(satInfo,response.constData(),sizeof(satelliteInfo));


        ui->logPlainTextEdit->appendPlainText(getTimestamp()+"获取卫星信息如下:");

        ui->logPlainTextEdit->appendPlainText("经度: " + QString::number(satInfo->longitude, 'f', 6));
        ui->logPlainTextEdit->appendPlainText("纬度: " + QString::number(satInfo->latitude, 'f', 6));
        ui->logPlainTextEdit->appendPlainText("椭球高: " + QString::number(satInfo->ellipsoidHeight, 'f', 2) + " m");
        ui->logPlainTextEdit->appendPlainText("高程差: " + QString::number(satInfo->diffHeight, 'f', 2) + " m");
        ui->logPlainTextEdit->appendPlainText("水平偏北方向: " + QString::number(satInfo->horiNorthDire, 'f', 2) + " °");
        ui->logPlainTextEdit->appendPlainText("垂直俯仰方向: " + QString::number(satInfo->vertiPitchDire, 'f', 2) + " °");
        ui->logPlainTextEdit->appendPlainText("天线距离: " + QString::number(satInfo->antennaDistance, 'f', 2) + " m");
        ui->logPlainTextEdit->appendPlainText("位置类型: " + QString::number(satInfo->positionType));
        ui->logPlainTextEdit->appendPlainText("GNSS质量指标: " + QString::number(satInfo->GNSS_QualIndicator));
    }
}

void Widget::parseOtherResponse(QByteArray response, devState *devSta)
{
    if(response.size()< static_cast<int>(sizeof(devState)))
    {
        QMessageBox::information(this,"警告","下位机响应回复设备状态数据包长度有误!");
        return;
    }
    else
    {
        memcpy(devSta,response.constData(),sizeof(devState));
        ui->logPlainTextEdit->appendPlainText(getTimestamp()+"获取设备状态信息如下:");

        ui->logPlainTextEdit->appendPlainText("采集状态: " + QString::number(devSta->collectionState));
        ui->logPlainTextEdit->appendPlainText("对时状态: " + QString::number(devSta->timeState));
        ui->logPlainTextEdit->appendPlainText("卫星状态: " + QString::number(devSta->satelliteState));
        ui->logPlainTextEdit->appendPlainText("预留字: " + QString::number(devSta->reserveWord));
        ui->logPlainTextEdit->appendPlainText("总存储空间: " + QString::number(devSta->totalSpace) + " B");
        ui->logPlainTextEdit->appendPlainText("可用存储空间: " + QString::number(devSta->freeSpace) + " B");
        ui->logPlainTextEdit->appendPlainText("电池电压: " + QString::number(devSta->batVol, 'f', 2) + " V");
        ui->logPlainTextEdit->appendPlainText("板卡温度: " + QString::number(devSta->temperature, 'f', 2) + " °C");
    }
}

void Widget::parseOtherResponse(QByteArray response, softwareVersion *softVer)
{
    if(response.size()< static_cast<int>(sizeof(devPhysicsParameter)))
    {
        QMessageBox::information(this,"警告","下位机响应回复软件版本数据包长度有误!");
        return;
    }
    else
    {
        memcpy(softVer,response.constData(),sizeof(softwareVersion));

        QString versionStr = QString("%1.%2.%3.%4")
                                 .arg(softVer->w1)
                                 .arg(softVer->w2)
                                 .arg(softVer->w3)
                                 .arg(softVer->w4);
        ui->logPlainTextEdit->appendPlainText(getTimestamp()+"软件版本: " + versionStr);
    }
}

void Widget::parseOtherResponse(QByteArray response, devWorkParameter *devWorkParam)
{
    if (response.size() < static_cast<int>(sizeof(devWorkParameter))) {
        QMessageBox::information(this, "警告", "下位机响应回复设备工作参数数据包长度有误!");
        return;
    } else {
        memcpy(devWorkParam, response.constData(), sizeof(devWorkParameter));
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "获取设备工作参数如下:");

        ui->logPlainTextEdit->appendPlainText("采样频率: " + QString::number(devWorkParam->sampleFreq) + " Hz");
        ui->logPlainTextEdit->appendPlainText("卫星类型: " + QString::number(devWorkParam->sateType));

        // 打印通道量程类型
        ui->logPlainTextEdit->appendPlainText("通道量程类型:");
        ui->logPlainTextEdit->appendPlainText("  通道0: " + QString::number(devWorkParam->rangeTypeChannel0));
        ui->logPlainTextEdit->appendPlainText("  通道1: " + QString::number(devWorkParam->rangeTypeChannel1));
        ui->logPlainTextEdit->appendPlainText("  通道2: " + QString::number(devWorkParam->rangeTypeChannel2));
        ui->logPlainTextEdit->appendPlainText("  通道3: " + QString::number(devWorkParam->rangeTypeChannel3));
        ui->logPlainTextEdit->appendPlainText("  通道4: " + QString::number(devWorkParam->rangeTypeChannel4));
        ui->logPlainTextEdit->appendPlainText("  通道5: " + QString::number(devWorkParam->rangeTypeChannel5));
        ui->logPlainTextEdit->appendPlainText("  通道6: " + QString::number(devWorkParam->rangeTypeChannel6));
        ui->logPlainTextEdit->appendPlainText("  通道7: " + QString::number(devWorkParam->rangeTypeChannel7));

        // 打印通道信号类型
        ui->logPlainTextEdit->appendPlainText("通道信号类型:");
        ui->logPlainTextEdit->appendPlainText("  通道0: " + QString::number(devWorkParam->signalTypeChannel0));
        ui->logPlainTextEdit->appendPlainText("  通道1: " + QString::number(devWorkParam->signalTypeChannel1));
        ui->logPlainTextEdit->appendPlainText("  通道2: " + QString::number(devWorkParam->signalTypeChannel2));
        ui->logPlainTextEdit->appendPlainText("  通道3: " + QString::number(devWorkParam->signalTypeChannel3));
        ui->logPlainTextEdit->appendPlainText("  通道4: " + QString::number(devWorkParam->signalTypeChannel4));
        ui->logPlainTextEdit->appendPlainText("  通道5: " + QString::number(devWorkParam->signalTypeChannel5));
        ui->logPlainTextEdit->appendPlainText("  通道6: " + QString::number(devWorkParam->signalTypeChannel6));
        ui->logPlainTextEdit->appendPlainText("  通道7: " + QString::number(devWorkParam->signalTypeChannel7));
    }
}

void Widget::on_sendBtn_clicked()
{
    if(socket->state()==QAbstractSocket::UnconnectedState)
    {
        qDebug()<<"sendBtn";
        QMessageBox::information(this,"错误","未连接单片机");
    }
    else
    {
        QString sendText = ui->sendTextEdit->toPlainText().remove(' ');

        //判断要发送的字符串是否非法
        if(1==isStringInvalid(sendText))
        {
            return;
        }

        // 提取第一个字节作为命令
        QString cmdHeaderStr = sendText.left(2);
        bool ok;
        uint8_t cmdHeader = cmdHeaderStr.toUInt(&ok, 16);
        if (!ok)
        {
            QMessageBox::information(this, "错误", "无法解析命令，请检查输入！");
            return;
        }

        //判断发送命令对应的TCP类型
        sendCmdFlag=CmdTcpType(cmdHeader);
        if(TCP_UNANSWER_STATE==sendCmdFlag)
        {
            return;
        }

        // 构建数据包
        QByteArray packet;
        packet.append(buildCmdPktHeader((CommandWord)cmdHeader,devID)); // 使用提取的命令头

        // 将剩余的十六进制字符串转换为字节数组
        QByteArray dataBytes;
        for (int i = 2; i < sendText.size(); i += 2)
        {
            QString byteStr = sendText.mid(i, 2);
            bool ok;
            uint8_t byteValue = byteStr.toUInt(&ok, 16);
            if (ok)
            {
                dataBytes.append(byteValue);
            }
            else
            {
                QMessageBox::information(this, "错误", "无法解析部分十六进制数据，请检查输入！");
                return;
            }
        }

        uint32_t totalLength = CMD_HEADER_LENGTH + dataBytes.size(); // 包总长度（包头4字节 + 数据N字节）
        packet.append(reinterpret_cast<char*>(&totalLength), 4);
        packet.append(dataBytes);

        // 将数据通过 TCP 发出
        qint64 bytesWritten = socket->write(packet);

        if (bytesWritten == -1)
        {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "发送失败: " + socket->errorString());
        }
        else
        {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "成功发送: " + sendText);
        }
    }
}

//判断字符串是否非法
bool Widget::isStringInvalid(QString sendText)
{
    if(sendText.isEmpty())
    {
        QMessageBox::information(this,"提示","发送区为空，请输入内容！");
        return true;
    }

    // 检查输入是否为合法的十六进制字符串
    bool isValidHex = true;
    for (int i = 0; i < sendText.size(); ++i)
    {
        if (!sendText.at(i).isDigit() && !sendText.at(i).isLetter() ||
            (sendText.at(i).toUpper() > 'F' && sendText.at(i).toUpper() < 'A'))
        {
            isValidHex = false;
            break;
        }
    }

    if (!isValidHex || sendText.size() % 2 != 0)
    {
        QMessageBox::information(this, "错误", "请输入有效的十六进制字符串（偶数长度）！");
        return true;
    }

    return false;
}

//获取当前时间戳用于日志打印,格式"yyyy-MM-dd HH:mm:ss"
QString getTimestamp()
{
    QString timestamp= QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    timestamp.append("|| ");
    return timestamp;
}

//判断发送的命令是否是命令集中的数据
//TODO：可以在case分支中增加判断发送数据合法性的代码，需要额外增加
TcpSendCmdType Widget::CmdTcpType(uint8_t cmdHeader)
{
    switch (cmdHeader)
    {
    case TCP_SEND_SET_DEV_WORKMODE:
        QMessageBox::information(this,"警告","不支持发送命令设置工作模式, 请使用模式按键!");
//        changeWorkModeFlag = true;
//        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送设置设备工作模式命令...");
        return TCP_UNANSWER_STATE;//TODO:返回值后续可能会更改

    case TCP_SEND_SET_FACTORY_IP:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送设置出厂ip命令...");
        return TCP_SEND_DEFAULT_STATE;

    case TCP_SEND_FACTORY_CALIBRATION:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送设置出厂校准命令...");
        return TCP_SEND_DEFAULT_STATE;

    case TCP_SEND_DATA_COLLECTION:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送采集命令...");
        return TCP_SEND_DEFAULT_STATE;

    case TCP_SEND_SET_WORK_PARAMETER:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送设置设备工作参数命令...");
        return TCP_SEND_DEFAULT_STATE;

    case TCP_SEND_NETWORK_TIME_SYNC:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送网络时间同步命令...");
        return TCP_SEND_DEFAULT_STATE;

    case TCP_SEND_FORCE_UPDATE_POSITION:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送强制更新位置命令...");
        return TCP_SEND_DEFAULT_STATE;

    case TCP_SEND_GET_PHY_PARAMETER:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送获取设备物理参数命令...");
        return TCP_SEND_GET_PHY_PARAMETER;

    case TCP_SEND_GET_WORK_PARAMETER:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送获取设备工作参数命令...");
        return TCP_SEND_GET_WORK_PARAMETER;

    case TCP_SEND_GET_SATELLITE_INFO:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送获取卫星信息命令...");
        return TCP_SEND_GET_SATELLITE_INFO;

    case TCP_SEND_GET_DEVICE_STATUS:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送获取设备状态信息命令...");
        return TCP_SEND_GET_DEVICE_STATUS;

    case TCP_SEND_EXCHANGE_SOFTWARE_VERSION:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送双向发送软件版本命令...");
        return TCP_SEND_EXCHANGE_SOFTWARE_VERSION;

    case TCP_SEND_REPORT_COLLECTION_DATA:
        //ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送上报采集数据命令...");
        QMessageBox::information(this, "抱歉", "该命令的功能尚未实现!请使用其他命令尝试");
        return TCP_UNANSWER_STATE;//TODO:返回值需要修改

    default:
        QMessageBox::information(this,"错误","发送的命令不在命令集之中,请检查输入！");
        return TCP_UNANSWER_STATE;
    }
}

//去除收到的数据包的包头,只留下数据部分
QByteArray removeCmdPktHeader(QByteArray response,CmdPacketHeader *header)
{
    QByteArray headerBytes = response.left(12);
    memcpy(header, headerBytes.constData(), sizeof(CmdPacketHeader));
    response = response.mid(12);

    return response;
}

//“获取设备物理参数信息”命令按钮槽函数
void Widget::on_getDevPhyParaBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("ca ff");
}

//“获取设备卫星信息”命令按钮槽函数
void Widget::on_getSateInfoBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("c6 ff");
}

//“查询设备状态命令”按钮槽函数
void Widget::on_getDevStateBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("c7 ff");
}

//发送“查询设备工作参数”命令按钮槽函数
void Widget::on_getDevWorkParaBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("c5 ff");
}

//发送“强制更新位置”命令按钮槽函数
void Widget::on_forceUpdateLocBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("c9 ff");
}

//切换自定义命令页面槽函数
void Widget::on_userDefCmdBtn_clicked()
{
    int nextIndex = (ui->cmdStackedWidget->currentIndex()+1)%ui->cmdStackedWidget->count();//FIXME:此处按键状态判断使用的是硬编码,如果后续在stacked widget中添加新页面,需要修改这里的逻辑

    if(1==nextIndex)
    {
        ui->userDefCmdBtn->setCheckable(true);
        ui->userDefCmdBtn->setChecked(true);
    }
    else
    {
        ui->userDefCmdBtn->setChecked(false);
    }
    ui->cmdStackedWidget->setCurrentIndex(nextIndex);
}

//自定义命令保存按钮槽函数
void Widget::saveSelfDfnCmdBtn_clicked()
{
    qDebug()<<"save cmd btn clicked";
    //获取当前选中的行
    int row = tableWidget->currentRow();


    //如果没有行被选中，则在表格末尾添加新行
    if(-1==row)
    {
        QMessageBox::information(this,"注意","请选中一行进行保存");
        return;
    }

    //获取命令内容和描述
    QString command = tableWidget->item(row,0)?tableWidget->item(row,0)->text():"cmd";
    QString description = descriptionEdit->text();

    // 设置命令和描述到表格中
    tableWidget->setItem(row, 0, new QTableWidgetItem(command));
    tableWidget->setItem(row, 1, new QTableWidgetItem(description));

    // 清空输入框
    descriptionEdit->clear();
}

void Widget::addSelfDfnCmdBtn_clicked()
{
    int row = tableWidget->rowCount();
    tableWidget->insertRow(row);
}

//自定义命令发送按钮槽函数
void Widget::sendSelfDfnCmdBtn_clicked()
{

}

//清除选中自定义命令按钮槽函数
void Widget::clearSelfDfnCmdBtn_clicked()
{

}



/**
 * @brief：设置电压阈值按钮槽函数
 * @param：无
 * @retval：无
*/
void Widget::on_setVolThresholdBtn_clicked()
{
    // 获取 QLineEdit 中的数据
    QString data = ui->setVolThresholdBtnLineEdit->text();
    float currentData=phyPara.batPercent;
    qDebug()<<"data:"<<data<<"\ncurrentData:"<<currentData;
    configManager->saveToJson("VolThreshold", data);
    //WARNING：未判断toFloat是否成功
    if(QAbstractSocket::ConnectedState==socket->state())
    {

        if(currentData<=(data.toFloat()))//当前电量小于阈值
        {
            ui->batStateLitLabel->setPixmap(redLit.scaled(60,60));//红灯
        }
        else
        {
            ui->batStateLitLabel->setPixmap(greenLit.scaled(60,60));//绿灯
        }
    }

}

void Widget::on_setNormalMessFreqBtn_clicked()
{
    // 获取 QLineEdit 中的数据
    QString data = ui->setNormalMessFreqLineEdit->text();
    configManager->saveToJson("NormalMessFreq", data);

    //设置定时时间后更新定时器间隔
    setGetDevInfoFreq();

}

void Widget::on_setLowPowMessFreqBtn_clicked()
{
    // 获取 QLineEdit 中的数据
    QString data = ui->setLowPowMessFreqLineEdit->text();
    configManager->saveToJson("LowPowMessFreq", data);

    //设置定时时间后更新定时器间隔
    setGetDevInfoFreq();
}


/**
 * @brief：根据设备当前工作模式设置自动获取设备物理信息频次，并开启定时器
 * @param：无
 * @retval：无
 */
void Widget::setGetDevInfoFreq()
{
    float time=2;
    if(LOW_POWER_MODE==devStateSet)
    {
        qDebug()<<"timer interval mode: Low pwr";
        time=configManager->readFromJson("LowPowMessFreq");
    }else if(NORMAL_MODE==devStateSet||DATA_CONLLECT_START_MODE==devStateSet)
    {
        qDebug()<<"timer interval mode: normal";
        time=configManager->readFromJson("NormalMessFreq");
    }
    qDebug()<<"timer interval:"<<time;
    timer.setInterval((int)(1000*time));//将秒转换为ms

    // 启动定时器
    timer.start();
    timer_stop_flag=false;
}

//WARNING:定时获取设备信息和手动获取，以及其他TCP命令有可能冲突，可能涉及线程安全问题
/**
 * @brief:定时获取设备信息槽函数
 * @param:无
 * @retval：无
*/
void Widget::get_devPhyParam_timeout()
{
    //通过tcp发送获取信息命令即可
    QByteArray packet;
    packet.append(buildCmdPktHeader(CMD_GET_DEV_PHY_PARAMETERS,devID));
    // 包总长度（4字节，包头12B + 数据1B = 13 → 0x0D）
    uint32_t totalLength = 13;
    packet.append(reinterpret_cast<char*>(&totalLength), 4);
    // 数据内容（1字节）
    packet.append(0xff);
    qDebug()<<"packet:"<<hexToFormatStr(packet);
    qint64 bytesWritten =socket->write(packet);
    sendCmdFlag=TCP_SEND_GET_PHY_PARAMETER;
    if (bytesWritten == -1)
    {
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "自动获取设备物理参数失败：" + socket->errorString());
        sendCmdFlag=TCP_UNANSWER_STATE;
    }
    else
    {
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "自动获取设备物理参数成功！" );
    }
}

/**
 * @brief：停止自动获取设备信息槽函数
 * @param：无
 * @retval：无
*/
void Widget::on_stopTimerBtn_clicked()
{
    timer.stop();
    timer_on_flag=false;
    timer_stop_flag=true;
}

void Widget::initJsonResultHandler(const QString &result)
{
    qDebug()<<"initJsonResultHandler";
    ui->logPlainTextEdit->appendPlainText(getTimestamp() +result);
}

void Widget::readFromJsonFailHandler(const QString &result)
{
    qDebug()<<"readFromJsonFailHandler";
    ui->logPlainTextEdit->appendPlainText(getTimestamp() +result);
}
