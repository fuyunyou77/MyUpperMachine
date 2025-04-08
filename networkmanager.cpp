#include "networkmanager.h"
#include <QHostAddress>
#include <QtEndian>
#include <QRegularExpression>
#include <stdint.h>
#include <QString>
#include <QByteArray>
#include <QDateTime>

NetworkManager::NetworkManager()
{
    //创建Socket对象
    socket = new QTcpSocket;

    //连接 socket的连接成功信号 与槽
    connect(socket,&QTcpSocket::connected,this,&NetworkManager::on_serverConnectted);
    //连接 socket的断开连接信号 与槽
    connect(socket,&QTcpSocket::disconnected,this,&NetworkManager::on_serverDisconnectted);
    //连接 socket接收数据信号 与槽，如果板卡有回复信息，则触发on_socketReadyRead函数
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::on_socketReadyRead);
    //连接 socket的连接错误信号 与槽
    connect(socket,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&NetworkManager::on_serverConnectError);
}

//bool NetworkManager::isIPv4Address(const QString &ip)
//{
//    QHostAddress addr;
//    return addr.setAddress(ip) &&
//           (addr.protocol() == QAbstractSocket::IPv4Protocol);
//}

//判断输入的IPv4地址是否合法,宽松验证,允许带前导0的格式(192.01.1.001)
//FIXME:这里的合法性判断有问题,对于192.168.0.0这一类代表一整个网段的ip无法被过滤
bool NetworkManager::isIPv4AddressEx(const QString &ip,
                    IPv4ValidationFlags flags = AllowNormal|AllowLoopback,
                    quint32 network = 0xC0A80000,       // 网络地址（需配合掩码使用）
                    quint32 mask = 0xFFFFFF00) // 子网掩码（默认不检查网络地址）
{
    // 基础格式验证
    QHostAddress addr;
    if (!addr.setAddress(ip) || addr.protocol() != QAbstractSocket::IPv4Protocol) {
        return false;
    }

    // 严格四段格式检查（允许前导零但必须四段）
    QStringList parts = ip.split('.');
    if (parts.size() != 4) return false; // 必须四段

    // 检查每段是否为0-255的数字（允许前导零）
    QRegularExpression octetRegex("^0$|^[1-9]\\d?$|^1\\d{2}$|^2[0-4]\\d$|^25[0-5]$");
    for (const QString &part : parts) {
        if (!octetRegex.match(part).hasMatch()) return false;
    }

    // 转换为32位无符号整数（网络字节序）
    quint32 ipv4 = addr.toIPv4Address();
    const quint32 ipv4HostOrder = qToBigEndian(ipv4); // 转换为大端序便于位运算

    // 1. 全零地址检查
    if (ipv4HostOrder == 0) {
        qDebug()<<"全零地址检查";
        return flags.testFlag(AllowZeroAddress);
    }

    // 2. 环回地址检查（127.0.0.0/8）
    if ((ipv4HostOrder & 0xFF000000) == 0x7F000000) {
        qDebug()<<"环回地址检查";
        return flags.testFlag(AllowLoopback);
    }

    // 3. 多播地址检查（224.0.0.0/4）
    if ((ipv4HostOrder & 0xF0000000) == 0xE0000000) {
        qDebug()<<"多播地址检查";
        return flags.testFlag(AllowMulticast);
    }

    // 4. 链路本地地址（169.254.0.0/16）
    if ((ipv4HostOrder & 0xFFFF0000) == 0xA9FE0000) {
        qDebug()<<"链路本地地址检查";
        return flags.testFlag(AllowLinkLocal);
    }

    // 5. 文档地址检查（TEST-NET-1/2/3）
    const quint32 testNet1 = 0xC0000200; // 192.0.2.0/24
    const quint32 testNet2 = 0xC6336400; // 198.51.100.0/24
    const quint32 testNet3 = 0xCB007100; // 203.0.113.0/24
    if ((ipv4HostOrder & 0xFFFFFF00) == testNet1 ||
        (ipv4HostOrder & 0xFFFFFF00) == testNet2 ||
        (ipv4HostOrder & 0xFFFFFF00) == testNet3) {
        qDebug()<<"文档地址检查";
        return flags.testFlag(AllowDocumentation);
    }

    // 6. 有限广播地址（255.255.255.255）
    if (ipv4HostOrder == 0xFFFFFFFF) {
        qDebug()<<"有限广播地址检查";
        return flags.testFlag(AllowBroadcast);
    }

    // 7. 网络地址检查（需提供网络地址和掩码）
    if (mask != 0xFFFFFFFF) { // 只要掩码有效就检查
        quint32 networkHostOrder = qToBigEndian(network);
        if ((ipv4HostOrder & mask) == (networkHostOrder & mask)) {
            quint32 hostPart = ipv4HostOrder & (~mask);
            if (hostPart == 0 || hostPart == (~mask)) {
                // 必须显式允许网络/广播地址
                qDebug()<<"网络地址检查";
                return flags.testFlag(AllowNetworkBroadcast);
            }
        }
    }

    // 8. 普通地址检查（排除其他特殊地址）
    if (flags.testFlag(AllowNormal)) {
        // 排除已被其他标志处理的情况
        const bool isSpecialAddress =
            (ipv4HostOrder == 0) ||
            ((ipv4HostOrder & 0xFF000000) == 0x7F000000) ||
            ((ipv4HostOrder & 0xF0000000) == 0xE0000000) ||
            ((ipv4HostOrder & 0xFFFF0000) == 0xA9FE0000) ||
            (ipv4HostOrder == 0xFFFFFFFF);
        qDebug()<<"普通地址检查";
        return !isSpecialAddress; // 只有非特殊地址才返回true
    }

    return false;
}

//判断输入的端口号是否合法,默认0端口合法
bool NetworkManager::isPortValid(const QString &port, bool allowZero = true)
{
    QString trimmed = port.trimmed();
    if (trimmed.isEmpty()) return false;

    bool ok;
    quint16 portNum = trimmed.toUShort(&ok, 10); // 强制十进制转换
    return ok && (allowZero || portNum > 0);
}

//判断子网掩码是否合法
bool NetworkManager::isValidSubnetMask(const QString &input)
{
    QString str = input.trimmed();

    // 检查CIDR表示法（/0 - /32）
    if (str.startsWith("/")) {
        bool ok;
        int cidr = str.mid(1).toInt(&ok);
        return ok && cidr >= 0 && cidr <= 32;
    }

    // 检查点分十进制格式
    QStringList parts = str.split('.');
    if (parts.size() != 4) return false;

    quint32 mask = 0;
    for (const QString &part : parts) {
        bool ok;
        quint8 octet = part.toUShort(&ok);
        if (!ok || octet > 255) return false;
        mask = (mask << 8) | octet; // 组合为32位整数
    }

    // 特殊值处理（全0无效）
    if (mask == 0) return false;

    // 位运算验证连续1的掩码
    quint32 inverted = ~mask + 1;
    return (inverted & (inverted - 1)) == 0;
}

// 构造数据包头
QByteArray buildCmdPktHeader(CommandWord cmd,uint8_t devID) {
    QByteArray packetHeader;

    // 包头标识符(2字节)：0x7E58
    packetHeader.append(0x7E);
    packetHeader.append(0x58);

    // 命令字(1字节)，从枚举类型中获取
    packetHeader.append(static_cast<uint8_t>(cmd));

    // 设备ID（固定值0xFF，1字节）
    //TODO:后续设备应该为设备IP的后2位
    packetHeader.append(devID);

    // 当前时间戳(4字节)
    uint32_t timestamp = static_cast<uint32_t>(QDateTime::currentSecsSinceEpoch());
    packetHeader.append(reinterpret_cast<char*>(&timestamp), 4);

    return packetHeader;
}


/**
 * @brief：下位机连接成功信号对应的槽函数：
 * 1.更新按钮的checked状态
 * 2.更新指示灯状态
 * 3.打印日志
 * 4.将子网掩码显示到对应位置
 *@param ：无
 *@retval：无
*/
void NetworkManager::on_serverConnectted()
{
//    ui->normalModeBtn->setCheckable(true);
//    ui->lowPowerModeBtn->setCheckable(true);
//    ui->normalModeBtn->setChecked(true);
//    ui->devStateLitLabel->setPixmap(greenLit.scaled(60,60));//初始连接板卡时，板卡一定为正常模式，设备状态显示绿灯

    ui->netStateLitLabel ->setPixmap(greenLit.scaled(60,60));//网络指示灯为黄色常亮,表示连接

    //在日志栏打印信息
    QString logText=getTimestamp();
    logText.append("下位机连接成功!------>["+ui->IPLineEdit->text()+":"+ui->PortLineEdit->text()+"]");
    ui->logPlainTextEdit->appendPlainText(logText);

    //TCP连接成功后自动发起一次获取设备物理参数请求,获取其设备状态用于其他各项信息显示
    QByteArray packet;
    packet.append(buildCmdPktHeader(CMD_GET_DEV_PHY_PARAMETERS,devID));
    // 包总长度（4字节，包头12B + 数据1B = 13 → 0x0D）
    uint32_t totalLength = 13;
    packet.append(reinterpret_cast<char*>(&totalLength), 4);
    // 数据内容（1字节）
    packet.append(0xff);
    qDebug()<<"packet:"<<hexToFormatStr(packet);
    qint64 bytesWritten =socket->write(packet);

    if (bytesWritten == -1)
    {
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "获取设备初始物理参数失败：" + socket->errorString());
        sendCmdFlag=TCP_UNANSWER_STATE;
    }
    else
    {
        sendCmdFlag=TCP_SEND_GET_PHY_PARAMETER;
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "获取设备初始物理参数成功！" );
    }
    //TODO:掩码需要可以自定义,此处实现需要修改
    ui->MaskLineEdit->setText(mask);//在掩码位置显示掩码
}

/**
 * @brief：下位机断开连接信号对应的槽函数：
 * 1.更新按钮的checked状态
 * 2.更新指示灯状态
 * 3.打印日志
 *@param ：无
 *@retval：无
*/
void NetworkManager::on_serverDisconnectted()
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

/**
 * @brief：下位机连接错误信号对应的槽函数：
 * 1.弹窗警告并打印错误日志
 * @param ：无
 * @retval：无
*/
void NetworkManager::on_serverConnectError()
{
    QMessageBox::information(this,"警告","TCP连接错误!");
    // 获取错误描述
    QString errorDescription = socket->errorString();
    ui->logPlainTextEdit->appendPlainText(getTimestamp()+"socketError:"+errorDescription);
}

/**
 * @brief：接收原始TCP数据包的槽函数：
 * 1.在日志中打印原始的tcp数据包内容（hex）
 * 2.处理数据包，并分发给相应的解析函数解析内容
 * @param ：无
 * @retval：无
*/
void NetworkManager::on_socketReadyRead()
{
    QByteArray response = socket->readAll();

    //将获取的响应直接在log中打印出来(hex形式)
    QString hexResponse=hexToFormatStr(response);
    ui->logPlainTextEdit->appendPlainText(getTimestamp()+"接收到原始数据\n(Hex:"+hexResponse+")");

    if(TCP_UNANSWER_STATE==sendCmdFlag)
    {
        //FIXME:下位机多次上报有时会出现在TCP_UNANSWER_STATE下响应，需要确认自动获取参数时的sendCmdFlag置位操作
//        QMessageBox::information(this,"警告","下位机在无TCP请求时进行了响应\n请确认下位机是否正常工作!");
        ui->logPlainTextEdit->appendPlainText(getTimestamp()+"下位机在无TCP请求时进行了响应\n请确认下位机是否正常工作!");
        return;
    }

    if (response.size() >= 12) {

        //去除收到的数据包头,存放在header中,其他功能可能会用
        qDebug()<<"response:"<<response.toHex();
        response=removeCmdPktHeader(response,&header);

        /*根据上位机发送给下位机命令的不同，sendCmdFlag会在发送命令时被赋给不同的值，可选值由TcpSendCmdType枚举类型约束。
         *接收到TCP响应后，数据进入该函数被解析，根据sendCmdFlag的不同，进入不同的分支被解析。分别实现不同的解析函数。
         *parseDefalutResponse（）处理默认响应（默认响应是指TCP数据部分只回复0或1的响应），其他复杂的响应由各种重载的parseOtherResponse（）函数解析
         */
        switch (sendCmdFlag) {
            case TCP_SEND_DEFAULT_STATE://默认响应
                parseDefalutResponse(response);
                break;

            case TCP_SEND_GET_PHY_PARAMETER://获取物理参数响应
                qDebug() << "处理设备物理参数响应!";
                parseOtherResponse(response, &phyPara);

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

            case TCP_SEND_EXCHANGE_SOFTWARE_VERSION://双向发送软件版本
                //TODO:实现逻辑需求
                break;

            case TCP_SEND_SET_DEV_WORKMODE://双向发送软件版本
                qDebug() << "处理设备模式设置响应!";
                parseDefalutResponse(response);
                sendCmdFlag = TCP_UNANSWER_STATE;
                break;

            default:
                qDebug()<<"上位机处于异常的TCP接受状态!无法解析数据包!";
                qDebug() << "sendCmdFlag:" << sendCmdFlag;
//                QMessageBox::information(this, "警告", "上位机处于异常的TCP接受状态!无法解析数据包!");
                break;
        }
    }
    else
    {
        qDebug()<<"下位机响应数据无效!查看日志输出获取详细信息";
        //QMessageBox::information(this,"警告","下位机响应数据无效!查看日志输出获取详细信息");
    }
}


//判断发送的命令是否是命令集中的数据
//TODO：可以在case分支中增加判断发送数据合法性的代码，需要额外增加
TcpSendCmdType NetworkManager::CmdTcpType(uint8_t cmdHeader)
{
    switch (cmdHeader)
    {
    case TCP_SEND_SET_DEV_WORKMODE:
        QMessageBox::information(this,"警告","不支持发送命令设置工作模式, 请使用模式按键!");
        return TCP_UNANSWER_STATE;//TODO:返回值后续可能会更改

    case TCP_SEND_GET_PHY_PARAMETER:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送获取设备物理参数命令...");
        return TCP_SEND_GET_PHY_PARAMETER;

    case TCP_SEND_GET_DEVICE_STATUS:
        ui->logPlainTextEdit->appendPlainText(getTimestamp() + "正在发送获取设备状态信息命令...");
        return TCP_SEND_GET_DEVICE_STATUS;

    default:
        QMessageBox::information(this,"错误","发送的命令不在命令集之中,请检查输入！");
        return TCP_UNANSWER_STATE;
    }
}



/**
 * @brief:将输入的二级制串转换为 以空格分割字节的 全大写的 格式化字符串,方便log打印和阅读
 * eg:(QByteArray)0x123456ef->(QString)12 34 56 EF
 * @param:QByteArray packet 数据包
 * @retval:QString 格式化的数据包字符串
*/
QString NetworkManager::hexToFormatStr(QByteArray packet)
{
    QString formatPacket=packet.toHex().toUpper();
    formatPacket=formatPacket.replace(QRegularExpression("(..)"),"\\1 ").trimmed();
    return formatPacket;
}

//判断字符串是否非法
bool NetworkManager::isStringInvalid(QString sendText)
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

//去除收到的数据包的包头,只留下数据部分
QByteArray removeCmdPktHeader(QByteArray response,CmdPacketHeader *header)
{
    QByteArray headerBytes = response.left(12);
    memcpy(header, headerBytes.constData(), sizeof(CmdPacketHeader));
    response = response.mid(12);

    return response;
}


void NetworkManager::parseDefalutResponse(QByteArray response)
{
    sendCmdFlag = TCP_UNANSWER_STATE;
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

    default:
        logText += "未知响应!";
        break;
    }

    QString hexResponse=response.toHex();
    logText.append("(Hex:"+hexResponse+")");

    ui->logPlainTextEdit->appendPlainText(logText); // 记录日志
}

//实现接收数据包解析,将结构体指针与数据包对齐
void NetworkManager::parseOtherResponse(QByteArray response, devPhysicsParameter *phyPara)
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
