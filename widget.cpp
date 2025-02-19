#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , greyLit(":/icon/grey_light.png")
    , greenLit(":/icon/green_light.png")
    , yellowLit(":/icon/yellow_light.png")
{
    ui->setupUi(this);
    ui->normalModeBtn->setCheckable(false);
    ui->lowPowerModeBtn->setCheckable(false);

    socket = new QTcpSocket;//创建Socket对象

    //按钮上放上图片
    ui->devStateLitLabel->setPixmap(greyLit.scaled(60,60));
    ui->netStateLitLabel ->setPixmap(greyLit.scaled(60,60));

    //连接信号与槽
    connect(ui->normalModeBtn,&QPushButton::clicked,this,&Widget::on_normalModeBtn_clicked,Qt::UniqueConnection);//开启正常模式按钮
    connect(ui->lowPowerModeBtn,&QPushButton::clicked,this,&Widget::on_lowPowerModeBtn_clicked,Qt::UniqueConnection);//开启低功耗模式按钮

    //日志区清空按钮
    connect(ui->logClearBtn,&QPushButton::clicked,[this]()
    {
        ui->logTextEdit->clear();//清空日志区
    });

    //发送区清空按钮
    connect(ui->sendClearBtn,&QPushButton::clicked,[this]()
    {
        ui->sendTextEdit->clear();//清空发送区
    });
}

Widget::~Widget()
{
    delete ui;
}

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
            QMessageBox::information(this,"注意","当前已处于正常工作模式!");
            // 防止信号再次触发
            disconnect(ui->normalModeBtn, &QPushButton::clicked, this, &Widget::on_normalModeBtn_clicked);
            return;
        }

        //更改标志量设置
        devStateSet=NORMAL_MODE;
        sendCmdFlag=TCP_SEND_DEFAULT_STATE;
        changeWorkModeFlag=true;

        QByteArray packet;
        packet.append(buildCmdPktHeader(CMD_SET_WORK_MODE,DEV_DEFAULT_ID));
        // 包总长度（4字节，包头12B + 数据1B = 13 → 0x0D）
        uint32_t totalLength = 13;
        packet.append(reinterpret_cast<char*>(&totalLength), 4);
        // 数据内容（1字节）
        packet.append(static_cast<char>(NORMAL_MODE));

        //将数据通过tcp发出,根据返回值打印日志信息
        qint64 bytesWritten = socket->write(packet);
        if (bytesWritten == -1) {
            ui->logTextEdit->append(getTimestamp() + "发送失败: " + socket->errorString());
        } else {
            ui->logTextEdit->append(getTimestamp() + "设置为正常工作模式...");
        }
    }

    // 防止信号再次触发
    disconnect(ui->normalModeBtn, &QPushButton::clicked, this, &Widget::on_normalModeBtn_clicked);
}

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
            QMessageBox::information(this,"注意","当前已处于低功耗模式!");
            // 防止信号再次触发
            disconnect(ui->lowPowerModeBtn, &QPushButton::clicked, this, &Widget::on_lowPowerModeBtn_clicked);
            return;
        }
        //更改标志量设置
        devStateSet=LOW_POWER_MODE;
        sendCmdFlag=TCP_SEND_DEFAULT_STATE;
        changeWorkModeFlag=true;

        QByteArray packet;
        packet.append(buildCmdPktHeader(CMD_SET_WORK_MODE,DEV_DEFAULT_ID));
        // 包总长度（4字节，包头12B + 数据1B = 13 → 0x0D）
        uint32_t totalLength = 13;
        packet.append(reinterpret_cast<char*>(&totalLength), 4);
        // 数据内容（1字节）
        packet.append(static_cast<char>(LOW_POWER_MODE));

        //将数据通过tcp发出,根据返回值打印日志信息
        qint64 bytesWritten = socket->write(packet);
        if (bytesWritten == -1) {
            ui->logTextEdit->append(getTimestamp() + "发送失败: " + socket->errorString());
        } else {
            ui->logTextEdit->append(getTimestamp() + "设置为低功耗模式...");
        }
    }

    // 防止信号再次触发
    disconnect(ui->lowPowerModeBtn, &QPushButton::clicked, this, &Widget::on_lowPowerModeBtn_clicked);

}

void Widget::on_connectBtn_clicked()
{
    //从输入框获取ip地址和端口
    QString IP = ui->IPLineEdit->text();
    QString port = ui->PortLineEdit->text();
    QString recvMask=ui->MaskLineEdit->text();

    if(IP.isEmpty()||port.isEmpty())
    {
        QMessageBox::information(this,"注意","未输入IP地址或端口号!");
        return;
    }

    //连接服务器
    socket->connectToHost(QHostAddress(IP),port.toUShort());

    //断开socket旧有的连接成功信号与槽
    disconnect(socket,&QTcpSocket::connected,this,&Widget::on_serverConnectted);
    //连接socket的连接成功信号与槽
    connect(socket,&QTcpSocket::connected,this,&Widget::on_serverConnectted);

    //断开socket旧有的连接错误信号与槽
    disconnect(socket,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&Widget::on_serverConnectError);
    //连接socket的连接错误信号与槽
    connect(socket,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&Widget::on_serverConnectError);

}

void Widget::on_disconnectBtn_clicked()
{
    //断开socket旧有的断开连接信号与槽
    disconnect(socket,&QTcpSocket::disconnected,this,&Widget::on_serverDisconnnectted);
    //连接socket的断开连接信号与槽
    connect(socket,&QTcpSocket::disconnected,this,&Widget::on_serverDisconnnectted);

    socket->disconnectFromHost();
}

void Widget::on_serverConnectted()
{
    ui->normalModeBtn->setCheckable(true);
    ui->lowPowerModeBtn->setCheckable(true);
    ui->normalModeBtn->setChecked(true);

    ui->netStateLitLabel ->setPixmap(yellowLit.scaled(60,60));//设置指示灯为黄色常亮,表示连接
    ui->devStateLitLabel->setPixmap(greenLit.scaled(60,60));//初始连接板卡时，板卡一定为正常模式，设备状态显示绿灯
    //TODO:TCP连接成功后自动发起一次获取设备物理参数请求,获取其设备状态用于其他各项信息显示

    // 断开旧的 readyRead 信号连接，避免重复绑定
    disconnect(socket, &QTcpSocket::readyRead, this, &Widget::on_socketReadyRead);
    //连接socket接收数据信号与槽，如果板卡有回复信息，则触发on_socketReadyRead函数
    connect(socket, &QTcpSocket::readyRead, this, &Widget::on_socketReadyRead);

    //TODO:掩码需要可以自定义,此处实现需要修改
    ui->MaskLineEdit->setText(mask);//在掩码位置显示掩码

    //在日志栏打印信息
    QString logText=getTimestamp();
    logText.append("下位机连接成功!------>["+ui->IPLineEdit->text()+":"+ui->PortLineEdit->text()+"]");
    ui->logTextEdit->append(logText);
}

void Widget::on_serverDisconnnectted()
{
    ui->netStateLitLabel ->setPixmap(greyLit.scaled(60,60));//设置网络状态指示灯为灰色,表示断开连接
    ui->devStateLitLabel ->setPixmap(greyLit.scaled(60,60));//设置设备状态指示灯为灰色,表示断开连接

    //打印日志
    QString logText=getTimestamp();
    logText.append("下位机连接断开!--\\\\-->["+ui->IPLineEdit->text()+":"+ui->PortLineEdit->text()+"]");
    ui->logTextEdit->append(logText);
}

//TODO:
void Widget::on_serverConnectError()
{
    QMessageBox::information(this,"警告","TCP连接错误!");
    // 获取错误描述
    QString errorDescription = socket->errorString();
    ui->logTextEdit->append(getTimestamp()+"socketError:"+errorDescription);
}

void Widget::on_socketReadyRead()
{
    QByteArray response = socket->readAll();

    //将获取的响应直接在log中打印出来(hex形式)
    QString hexResponse=response.toHex();
    QString logText = getTimestamp();
    logText.append("接收到原始数据(Hex:"+hexResponse+")");
    ui->logTextEdit->append(logText); // 记录日志

    if(TCP_UNANSWER_STATE==sendCmdFlag)
    {
        QMessageBox::information(this,"警告","下位机在无TCP请求时进行了响应\n请确认下位机是否正常工作!");
        return;
    }

    if (response.size() >= 12) {

        //去除收到的数据包头,存放在header中,其他功能可能会用
        qDebug()<<"response:"<<response.toHex();
        response=removeCmdPktHeader(response,&header);
        qDebug()<<"response:"<<response.toHex();

        /*只有当上位机发送获取设备物理参数命令后sendCmdFlag才会被置为TCP_SEND_GET_DEV_PARAMETER
        从而进入该分支处理物理参数包,其他模式都是默认模式,只会回复0或1*/
        switch (sendCmdFlag) {
            case TCP_SEND_DEFAULT_STATE://默认响应
                parseDefalutResponse(response);
                sendCmdFlag = TCP_UNANSWER_STATE;
                break;

            case TCP_SEND_GET_DEV_PARAMETER://获取物理参数响应
                qDebug() << "处理设备物理参数响应!";
                parseOtherResponse(response, &phyPara);
                sendCmdFlag = TCP_UNANSWER_STATE;
                break;

            case TCP_SEND_GET_WORK_PARAMETER://获取工作参数响应
                // TODO: 根据需求实现工作参数解析逻辑
                break;

            case TCP_SEND_GET_SATELLITE_INFO://获取卫星信息响应
                // TODO: 根据需求实现卫星信息解析逻辑
                break;

            case TCP_SEND_GET_DEVICE_STATUS://获取设备状态信息响应
                //TODO:实现逻辑需求
                break;

            case TCP_EXCHANGE_SOFTWARE_VERSION://双向发送软件版本
                //TODO:实现逻辑需求
                break;

            default:
                qDebug() << "sendCmdFlag:" << sendCmdFlag;
                QMessageBox::information(this, "警告", "上位机处于异常的TCP接受状态!无法解析数据包!");
                break;
        }
    }
    else
    {
        QMessageBox::information(this,"警告","下位机响应数据无效!查看日志输出获取详细信息");
    }
}

void Widget::parseDefalutResponse(QByteArray response)
{
    uint8_t tcpRespond = static_cast<uint8_t>(response.at(0));
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
        }
        else if(LOW_POWER_MODE==devStateSet)//低功耗模式设置成功
        {
            if(true==changeWorkModeFlag)
            {
                ui->devStateLitLabel->setPixmap(greyLit.scaled(60,60));//设备状态指示灯变为灰色
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
                ui->devStateLitLabel->setPixmap(greyLit.scaled(60,60));//设备状态指示灯变为灰色
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
    case 2:
        logText += "数采系统正在启动中...";
        break;
    default:
        logText += "未知响应!";
        break;
    }

    QString hexResponse=response.toHex();
    logText.append("(Hex:"+hexResponse+")");

    ui->logTextEdit->append(logText); // 记录日志
}

//实现接收数据包解析,将结构体指针与数据包对齐
//TODO:根据协议规定,计算物理参数,浮点数转换有误,原因未知
void Widget::parseOtherResponse(QByteArray response,devPhysicsParameter *phyPara)
{
    if(response.size()< static_cast<int>(sizeof(devPhysicsParameter)))
    {
        QMessageBox::information(this,"警告","下位机响应回复物理参数数据包长度有误!");
        return;
    }
    else
    {
        qDebug() << "Size of devPhysicsParameter:" << sizeof(devPhysicsParameter);
        memcpy(phyPara,response.constData(),sizeof(devPhysicsParameter));

        //日志区打印设备物理参数
        ui->logTextEdit->append(getTimestamp()+"获取板卡物理参数如下:");

        ui->logTextEdit->append("工作模式: " + QString::number(phyPara->workMode));
        ui->logTextEdit->append("板卡电流: " + QString::number(phyPara->current, 'f', 2) + " A");
        ui->logTextEdit->append("电池百分比: " + QString::number(phyPara->batPercent) + " %");
        ui->logTextEdit->append("电池电压: " + QString::number(phyPara->batVol, 'f', 2) + " V");
        ui->logTextEdit->append("板卡温度: " + QString::number(phyPara->temperature, 'f', 2) + " °C");
//        ui->logTextEdit->append("电池电压 (hex): " + QString::number(*reinterpret_cast<uint32_t*>(&phyPara->batVol), 16));
//        ui->logTextEdit->append("板卡温度 (hex): " + QString::number(*reinterpret_cast<uint32_t*>(&phyPara->temperature), 16));

        //将物理参数显示在对应的文本框
        ui->BatVolLineEdit->setText(QString::number(phyPara->batVol, 'f', 2) + " V");//显示电池电压
        ui->BatPercentLineEdit->setText(QString::number(phyPara->batPercent) + " %");//显示电池百分比
        ui->CurrentLineEdit->setText(QString::number(phyPara->current, 'f', 2) + " A");//显示板卡电流
        ui->TemperLineEdit->setText(QString::number(phyPara->temperature, 'f', 2) + " °C");//显示板卡温度
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
        ui->logTextEdit->append(getTimestamp()+"获取卫星信息如下:");

        ui->logTextEdit->append("经度: " + QString::number(satInfo->longitude, 'f', 6));
        ui->logTextEdit->append("纬度: " + QString::number(satInfo->latitude, 'f', 6));
        ui->logTextEdit->append("椭球高: " + QString::number(satInfo->ellipsoidHeight, 'f', 2) + " m");
        ui->logTextEdit->append("高程差: " + QString::number(satInfo->diffHeight, 'f', 2) + " m");
        ui->logTextEdit->append("水平偏北方向: " + QString::number(satInfo->horiNorthDire, 'f', 2) + " °");
        ui->logTextEdit->append("垂直俯仰方向: " + QString::number(satInfo->vertiPitchDire, 'f', 2) + " °");
        ui->logTextEdit->append("天线距离: " + QString::number(satInfo->antennaDistance, 'f', 2) + " m");
        ui->logTextEdit->append("位置类型: " + QString::number(satInfo->positionType));
        ui->logTextEdit->append("GNSS质量指标: " + QString::number(satInfo->GNSS_QualIndicator));
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
        ui->logTextEdit->append(getTimestamp()+"获取设备状态信息如下:");

        ui->logTextEdit->append("采集状态: " + QString::number(devSta->collectionState));
        ui->logTextEdit->append("对时状态: " + QString::number(devSta->timeState));
        ui->logTextEdit->append("卫星状态: " + QString::number(devSta->satelliteState));
        ui->logTextEdit->append("预留字: " + QString::number(devSta->reserveWord));
        ui->logTextEdit->append("总存储空间: " + QString::number(devSta->totalSpace) + " B");
        ui->logTextEdit->append("可用存储空间: " + QString::number(devSta->freeSpace) + " B");
        ui->logTextEdit->append("电池电压: " + QString::number(devSta->batVol, 'f', 2) + " V");
        ui->logTextEdit->append("板卡温度: " + QString::number(devSta->temperature, 'f', 2) + " °C");
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
        ui->logTextEdit->append(getTimestamp()+"软件版本: " + versionStr);
    }
}

void Widget::parseOtherResponse(QByteArray response, devWorkParameter *devWorkParam)
{
    if (response.size() < static_cast<int>(sizeof(devWorkParameter))) {
        QMessageBox::information(this, "警告", "下位机响应回复设备工作参数数据包长度有误!");
        return;
    } else {
        memcpy(devWorkParam, response.constData(), sizeof(devWorkParameter));
        ui->logTextEdit->append(getTimestamp() + "获取设备工作参数如下:");

        ui->logTextEdit->append("采样频率: " + QString::number(devWorkParam->sampleFreq) + " Hz");
        ui->logTextEdit->append("卫星类型: " + QString::number(devWorkParam->sateType));

        // 打印通道量程类型
        ui->logTextEdit->append("通道量程类型:");
        ui->logTextEdit->append("  通道0: " + QString::number(devWorkParam->rangeTypeChannel0));
        ui->logTextEdit->append("  通道1: " + QString::number(devWorkParam->rangeTypeChannel1));
        ui->logTextEdit->append("  通道2: " + QString::number(devWorkParam->rangeTypeChannel2));
        ui->logTextEdit->append("  通道3: " + QString::number(devWorkParam->rangeTypeChannel3));
        ui->logTextEdit->append("  通道4: " + QString::number(devWorkParam->rangeTypeChannel4));
        ui->logTextEdit->append("  通道5: " + QString::number(devWorkParam->rangeTypeChannel5));
        ui->logTextEdit->append("  通道6: " + QString::number(devWorkParam->rangeTypeChannel6));
        ui->logTextEdit->append("  通道7: " + QString::number(devWorkParam->rangeTypeChannel7));

        // 打印通道信号类型
        ui->logTextEdit->append("通道信号类型:");
        ui->logTextEdit->append("  通道0: " + QString::number(devWorkParam->signalTypeChannel0));
        ui->logTextEdit->append("  通道1: " + QString::number(devWorkParam->signalTypeChannel1));
        ui->logTextEdit->append("  通道2: " + QString::number(devWorkParam->signalTypeChannel2));
        ui->logTextEdit->append("  通道3: " + QString::number(devWorkParam->signalTypeChannel3));
        ui->logTextEdit->append("  通道4: " + QString::number(devWorkParam->signalTypeChannel4));
        ui->logTextEdit->append("  通道5: " + QString::number(devWorkParam->signalTypeChannel5));
        ui->logTextEdit->append("  通道6: " + QString::number(devWorkParam->signalTypeChannel6));
        ui->logTextEdit->append("  通道7: " + QString::number(devWorkParam->signalTypeChannel7));
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
        if(isStringInvalid(sendText))
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
        //TODO:这一部分数据可以提取成函数,用于判断数据包类型
        sendCmdFlag=CmdTcpType(cmdHeader);

        if(TCP_UNANSWER_STATE==sendCmdFlag)
        {
            QMessageBox::information(this,"错误","发送的命令不在命令集之中,请检查输入！");
            return;
        }

        // 构建数据包
        QByteArray packet;
        packet.append(buildCmdPktHeader((CommandWord)cmdHeader,DEV_DEFAULT_ID)); // 使用提取的命令头

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
            ui->logTextEdit->append(getTimestamp() + "发送失败: " + socket->errorString());
        }
        else
        {
            ui->logTextEdit->append(getTimestamp() + "成功发送: " + sendText);
        }
    }
}

//判断字符串是否有效
bool Widget::isStringInvalid(QString sendText)
{
    if(sendText.isEmpty())
    {
        QMessageBox::information(this,"提示","发送区为空，请输入内容！");
        // 防止信号再次触发
        disconnect(ui->normalModeBtn, &QPushButton::clicked, this, &Widget::on_sendBtn_clicked);
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
        // 防止信号再次触发
        disconnect(ui->normalModeBtn, &QPushButton::clicked, this, &Widget::on_sendBtn_clicked);
        return true;
    }

    return false;
}

//bool Widget::isNotInCmdSet(TcpSendCmdType sendCmdFlag)
//{
//    switch (sendCmdFlag) {
//    case TCP_SEND_DEFAULT_STATE:

//        break;

//    }
//}

//获取当前时间戳用于日志打印,格式"yyyy-MM-dd HH:mm:ss"
QString getTimestamp()
{
    QString timestamp= QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    timestamp.append("|| ");
    return timestamp;
}

//判断发送的命令是否是命令集中的数据
TcpSendCmdType Widget::CmdTcpType(uint8_t cmdHeader)
{
    if(0xF1==cmdHeader||//0/1
       0xC1==cmdHeader||//0/1
       0xC2==cmdHeader||//0/1
       0xC3==cmdHeader||//0/1
       0xC4==cmdHeader||//0/1
       0xC8==cmdHeader||//0/1
       0xC9==cmdHeader)//0/1
        //下位机回复如果是0或1的命令返回该参数
        return TCP_SEND_DEFAULT_STATE;
    else if(0xCA==cmdHeader)
        return TCP_SEND_GET_DEV_PARAMETER;
    else if(0xC5==cmdHeader)
        return TCP_SEND_GET_WORK_PARAMETER;
    else if(0xC6==cmdHeader)
        return TCP_SEND_GET_SATELLITE_INFO;
    else if(0xC7==cmdHeader)
        return TCP_SEND_GET_DEVICE_STATUS;
    else if(0xCF==cmdHeader)
        return TCP_SEND_EXCHANGE_SOFTWARE_VERSION;
    else if(0xDA==cmdHeader)
    {
        //TODO:0xDA是采集数据上报的命令,走的是另外的端口后续或许需要额外的实现方法
        QMessageBox::information(this,"抱歉","该命令的功能尚未实现!请使用其他命令尝试");
        return TCP_SEND_REPORT_COLLECTION_DATA;
    }
    else
        return TCP_UNANSWER_STATE;
}

//去除收到的数据包的包头,只留下数据部分
QByteArray removeCmdPktHeader(QByteArray response,CmdPacketHeader *header)
{
    QByteArray headerBytes = response.left(12);
    memcpy(header, headerBytes.constData(), sizeof(CmdPacketHeader));
    response = response.mid(12);

    return response;
}

//获取设备物理参数信息按键槽函数
void Widget::on_getDevPhyParaBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("ca ff");
}

//获取设备卫星信息按键槽函数
void Widget::on_getSateInfoBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("c6 ff");
}

//查询设备状态槽函数
void Widget::on_getDevStateBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("c7 ff");
}

//查询设备工作参数槽函数
void Widget::on_getDevWorkParaBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("c5 ff");
}

//强制更新位置槽函数
void Widget::on_forceUpdateLocBtn_clicked()
{
    ui->sendTextEdit->clear();
    ui->sendTextEdit->setText("c9 ff");
}

