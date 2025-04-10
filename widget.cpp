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
    networkManager =new NetworkManager;
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

    //tcp连接状态处理函数
    connect(networkManager,&NetworkManager::HostConnectted,this,&Widget::HostConnecttedHandler);
    connect(networkManager,&NetworkManager::HostDisconnectted,this,&Widget::HostDisconnecttedHandler);
    connect(networkManager,&NetworkManager::HostConnectError,this,&Widget::HostConnectErrorHandler);

    connect(networkManager,&NetworkManager::WorkmodeSetResponse,this,&Widget::WorkmodeSetResponseHandler);
    connect(networkManager,&NetworkManager::PhyParamResponse,this,&Widget::PhyParamResponseHandler);

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
    qDebug()<<__FILE__<<__LINE__<<__func__;
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
    if(networkManager->socket->state()==QAbstractSocket::UnconnectedState)
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
        packet.append(networkManager->buildCmdPktHeader(CMD_SET_WORK_MODE));
        // 包总长度（4字节，包头12B + 数据1B = 13 → 0x0D）
        uint32_t totalLength = 13;
        packet.append(reinterpret_cast<char*>(&totalLength), 4);
        // 数据内容（1字节）
        packet.append(static_cast<char>(NORMAL_MODE));
//        packet.append(static_cast<char>(0x00));//00为正常模式

        //将数据通过tcp发出,根据返回值打印日志信息
        qint64 bytesWritten = networkManager->socket->write(packet);
        if (bytesWritten == -1) {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "发送失败: " + networkManager->socket->errorString());
        } else {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "发送成功！正在设置为正常工作模式...");
            QString hexPacket=networkManager->hexToFormatStr(packet);
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "Hex:"+hexPacket);

            //发送成功，更改相应的标志量设置
            devStateSet=NORMAL_MODE;
            networkManager->sendCmdFlag=TCP_SEND_DEFAULT_STATE;
            changeWorkModeFlag=true;
        }
    }
}



/*@brief：设置低功耗模式按钮槽函数：
 * 1.发送“低功耗模式设置命令”
 * 2.更新标志量
 *@param ：无
 *@retval：无
*/
void Widget::on_lowPowerModeBtn_clicked()
{

    if(networkManager->socket->state()==QAbstractSocket::UnconnectedState)
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
        packet.append(networkManager->buildCmdPktHeader(CMD_SET_WORK_MODE));
        // 包总长度（4字节，包头12B + 数据1B = 13 → 0x0D）
        uint32_t totalLength = 13;
        packet.append(reinterpret_cast<char*>(&totalLength), 4);
        // 数据内容（1字节）
        packet.append(static_cast<char>(LOW_POWER_MODE));
//        packet.append(static_cast<char>(0x01));//01为低功耗模式

        //将数据通过tcp发出,根据返回值打印日志信息
        qint64 bytesWritten = networkManager->socket->write(packet);
        if (bytesWritten == -1) {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "发送失败: " + networkManager->socket->errorString());
        } else {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "发送成功！正在设置为低功耗模式...");
            QString hexPacket=networkManager->hexToFormatStr(packet);
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "Hex:"+hexPacket);
            //更改标志量设置
            devStateSet=LOW_POWER_MODE;
            networkManager->sendCmdFlag=TCP_SEND_DEFAULT_STATE;
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

    if(QAbstractSocket::ConnectedState==networkManager->socket->state())
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
    if(!recvMask.isEmpty() && ! networkManager->isValidSubnetMask(recvMask))
    {
        QMessageBox::information(this,"注意","输入的子网掩码无效!");
        return;
    }
    else if(networkManager->isValidSubnetMask(recvMask))//如果子网掩码为空,使用接收到的子网掩码代替存储的子网掩码
    {
        networkManager->mask=recvMask;
    }

    //TODO:在此处处理子网掩码将Qstring类型转换为quint32传递给下面的ipv4验证代码
    if(!networkManager->isIPv4AddressEx(IP,
                        AllowNormal|AllowLoopback|AllowMulticast,
                        0xC0A80000,//子网网段,192.168.0.0
                        0xFFFFFF00//子网掩码,255.255.255.0
                        ))
    {
        QMessageBox::information(this,"注意","输入的IP地址无效!");
        return;
    }


    qDebug()<<"socket state:"<<networkManager->socket->state();
    //连接服务器
    networkManager->socket->connectToHost(QHostAddress(IP),port.toUShort());
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
    QAbstractSocket::SocketState state=networkManager->socket->state();

    if(QAbstractSocket::ConnectingState==state
       ||QAbstractSocket::HostLookupState==state)
    {
        networkManager->socket->abort();
        ui->logPlainTextEdit->appendPlainText(getTimestamp()+"已中断连接行为!");
        devStateSet=NORMAL_MODE;
    }
    else if(QAbstractSocket::ConnectedState==state)
    {
        networkManager->socket->disconnectFromHost();
        devStateSet=NORMAL_MODE;
    }
    else if(QAbstractSocket::UnconnectedState==state)
    {
        QMessageBox::information(this,"注意","未连接下位机!");
    }
}


void Widget::on_sendBtn_clicked()
{
    qDebug()<<"sendBtn";
    QString sendText = ui->sendTextEdit->toPlainText().remove(' ');

    uint8_t result=networkManager->sendCmdToHost(sendText);

    switch (result) {
    case 0:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "成功发送: " + sendText);
        if(TCP_SEND_GET_PHY_PARAMETER==networkManager->sendCmdFlag)
        {
            ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在获取设备物理参数...");
        }
        break;

    case 1:
        QMessageBox::information(this,"错误","未连接单片机");
        break;
    case 2:
        QMessageBox::information(this, "错误", "请输入有效的十六进制字符串（偶数长度）！");
        break;

    case 3:
        QMessageBox::information(this, "错误", "命令的内容由Qstring转换成uint8_t类型失败，请检查输入！");
        break;
    case 4:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "发送失败: " + networkManager->socket->errorString());
        break;

    case -1:
        QMessageBox::information(this,"警告","不支持手动发送命令设置工作模式, 请使用模式按键!");
        break;

    case -2:
        QMessageBox::information(this,"错误","发送的命令不在命令集之中,请检查输入！");
        break;

    default:
        break;
    }
}



//获取当前时间戳用于日志打印,格式"yyyy-MM-dd HH:mm:ss"
QString Widget::getTimestamp()
{
    QString timestamp= QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    timestamp.append("|| ");
    return timestamp;
}


//“获取设备物理参数信息”命令按钮槽函数
void Widget::on_getDevPhyParaBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("ca ff");
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

//下位机连接成功处理槽函数
void Widget::HostConnecttedHandler(qint64 bytesWritten)
{
    ui->netStateLitLabel ->setPixmap(greenLit.scaled(60,60));//网络指示灯为黄色常亮,表示连接
    //TODO:掩码需要可以自定义,此处实现需要修改
    ui->MaskLineEdit->setText(networkManager->mask);//在掩码位置显示掩码
    //在日志栏打印信息
    QString logText=getTimestamp();
    logText.append("下位机连接成功!------>["+ui->IPLineEdit->text()+":"+ui->PortLineEdit->text()+"]");
    ui->logPlainTextEdit->appendPlainText(logText);
    if (bytesWritten == -1)
    {
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "获取设备初始物理参数失败：" + networkManager->socket->errorString());
        networkManager->sendCmdFlag=TCP_UNANSWER_STATE;
    }
    else
    {
        networkManager->sendCmdFlag=TCP_SEND_GET_PHY_PARAMETER;
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "获取设备初始物理参数成功！" );
    }


}

//下位机断开连接处理槽函数
void Widget::HostDisconnecttedHandler()
{
    ui->normalModeBtn->setChecked(false);
    ui->lowPowerModeBtn->setChecked(false);
    ui->normalModeBtn->setCheckable(false);
    ui->lowPowerModeBtn->setCheckable(false);
    ui->netStateLitLabel ->setPixmap(greyLit.scaled(60,60));//设置网络状态指示灯为灰色,表示断开连接
    ui->devStateLitLabel ->setPixmap(greyLit.scaled(60,60));//设置设备状态指示灯为灰色,表示断开连接
    ui->batStateLitLabel ->setPixmap(greyLit.scaled(60,60));//设置电池状态指示灯为灰色,表示断开连接

    timer.stop();
    timer_on_flag=false;
    timer_stop_flag=false;
    //TODO:下位机断开连接，相关的标志量要全部清空

    ui->BatPercentLineEdit->clear();
    ui->BatVolLineEdit->clear();
    ui->CurrentLineEdit->clear();
    ui->TemperLineEdit->clear();

    //打印日志
    QString logText=getTimestamp();
    logText.append("下位机连接断开!--\\\\-->["+ui->IPLineEdit->text()+":"+ui->PortLineEdit->text()+"]");
    ui->logPlainTextEdit->appendPlainText(logText);
}

//下位机连接错误槽函数
void Widget::HostConnectErrorHandler()
{
    QMessageBox::information(this,"警告","TCP连接错误!");
    // 获取错误描述
    QString errorDescription = networkManager->socket->errorString();
    ui->logPlainTextEdit->appendPlainText(getTimestamp()+"socketError:"+errorDescription);
}


void Widget::TcpHexResponseHandler(QByteArray response)
{
    //将获取的响应直接在log中打印出来(hex形式)
    QString hexResponse=networkManager->hexToFormatStr(response);
    ui->logPlainTextEdit->appendPlainText(getTimestamp()+"接收到原始数据\n(Hex:"+hexResponse+")");
    if(TCP_UNANSWER_STATE==networkManager->sendCmdFlag)
    {
        //FIXME:下位机多次上报有时会出现在TCP_UNANSWER_STATE下响应，需要确认自动获取参数时的sendCmdFlag置位操作
//        QMessageBox::information(this,"警告","下位机在无TCP请求时进行了响应\n请确认下位机是否正常工作!");
        ui->logPlainTextEdit->appendPlainText(getTimestamp()+"下位机在无TCP请求时进行了响应\n请确认下位机是否正常工作!");
        return;
    }
}

//工作模式设置响应处理槽函数
void Widget::WorkmodeSetResponseHandler(QByteArray response)
{
    uint8_t tcpRespond = static_cast<uint8_t>(response.at(0));
    networkManager->sendCmdFlag= TCP_UNANSWER_STATE;
    QString logText = getTimestamp();

    switch (tcpRespond) {
    case 0:
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

        break;

    default:
        logText += "未知响应!";
        break;
    }

    QString hexResponse=response.toHex();
    logText.append("(Hex:"+hexResponse+")");

    ui->logPlainTextEdit->appendPlainText(logText); // 记录日志
}

//设备物理参数响应处理槽函数,实现接收数据包解析,将结构体指针与数据包对齐
void Widget::PhyParamResponseHandler(QByteArray response,  devPhysicsParameter *phyPara)
{
    qDebug()<< "get phy param response sendcmdflag:"<<networkManager->sendCmdFlag;
    networkManager->sendCmdFlag = TCP_UNANSWER_STATE;
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

        ui->CurrentLineEdit->setText(QString::number(phyPara->current, 'f', 2) + " A");//显示板卡电流
        ui->TemperLineEdit->setText(QString::number(phyPara->temperature, 'f', 2) + " °C");//显示板卡温度
        ui->logPlainTextEdit->appendPlainText("板卡电流: " + QString::number(phyPara->current, 'f', 2) + " A");
        ui->normalModeBtn->setCheckable(true);
        ui->lowPowerModeBtn->setCheckable(true);

        if(NORMAL_MODE==phyPara->workMode)
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

/**
 * @brief：设置电压阈值按钮槽函数
 * @param：无
 * @retval：无
*/
void Widget::on_setVolThresholdBtn_clicked()
{
    // 获取 QLineEdit 中的数据
    QString data = ui->setVolThresholdBtnLineEdit->text();
    float currentData=networkManager->phyPara.batPercent;
    qDebug()<<"data:"<<data<<"\ncurrentData:"<<currentData;
    configManager->saveToJson("VolThreshold", data);
    //WARNING：未判断toFloat是否成功
    if(QAbstractSocket::ConnectedState==networkManager->socket->state())
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
    packet.append(networkManager->buildCmdPktHeader(CMD_GET_DEV_PHY_PARAMETERS));
    // 包总长度（4字节，包头12B + 数据1B = 13 → 0x0D）
    uint32_t totalLength = 13;
    packet.append(reinterpret_cast<char*>(&totalLength), 4);
    // 数据内容（1字节）
    packet.append(0xff);
    qDebug()<<"packet:"<<networkManager->hexToFormatStr(packet);
    qint64 bytesWritten =networkManager->socket->write(packet);
    networkManager->sendCmdFlag=TCP_SEND_GET_PHY_PARAMETER;


    if (bytesWritten == -1)
    {
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "自动获取设备物理参数失败：" + networkManager->socket->errorString());
        networkManager->sendCmdFlag=TCP_UNANSWER_STATE;
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
