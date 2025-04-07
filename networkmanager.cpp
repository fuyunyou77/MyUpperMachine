#include "networkmanager.h"
#include <QHostAddress>
#include <QtEndian>
#include <QRegularExpression>
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
