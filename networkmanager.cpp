#include "networkmanager.h"
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
    connect(socket,&QTcpSocket::connected,this,&NetworkManager::on_hostConnectted);
    //连接 socket的断开连接信号 与槽
    connect(socket,&QTcpSocket::disconnected,this,&NetworkManager::on_hostDisconnectted);
    //连接 socket接收数据信号 与槽，如果板卡有回复信息，则触发on_socketReadyRead函数
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::on_socketReadyRead);
    //连接 socket的连接错误信号 与槽
    connect(socket,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&NetworkManager::on_hostConnectError);
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
QByteArray NetworkManager::buildCmdPktHeader(CommandWord cmd) {
    QByteArray packetHeader;

    // 包头标识符(2字节)：0x7E58
    packetHeader.append(0x7E);
    packetHeader.append(0x58);

    // 命令字(1字节)，从枚举类型中获取
    packetHeader.append(static_cast<uint8_t>(cmd));

    // 设备ID（固定值0xFF，1字节）
    packetHeader.append(0xff);

    // 当前时间戳(4字节)
    uint32_t timestamp = static_cast<uint32_t>(QDateTime::currentSecsSinceEpoch());
    packetHeader.append(reinterpret_cast<char*>(&timestamp), 4);

    return packetHeader;
}

/**
 * @brief:通过tcp发送获取设备物理参数命令
*/
qint64 NetworkManager::tcp_getDevPhyParam()
{
    QByteArray packet;
    packet.append(buildCmdPktHeader(CMD_GET_DEV_PHY_PARAMETERS));
    // 包总长度（4字节，包头12B + 数据1B = 13 → 0x0D）
    uint32_t totalLength = 13;
    packet.append(reinterpret_cast<char*>(&totalLength), 4);
    // 数据内容（1字节）
    packet.append(0xff);
    qDebug()<<"packet:"<<hexToFormatStr(packet);
    return socket->write(packet);
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
void NetworkManager::on_hostConnectted()
{

    //TCP连接成功后自动发起一次获取设备物理参数请求,获取其设备状态用于其他各项信息显示
    qint64 bytesWritten=tcp_getDevPhyParam();
    //将发送tcp命令的结果作为信号的参数发出
    emit HostConnectted(bytesWritten);

}

/**
 * @brief：下位机断开连接信号对应的槽函数：
 * 1.更新按钮的checked状态
 * 2.更新指示灯状态
 * 3.打印日志
 *@param ：无
 *@retval：无
*/
void NetworkManager::on_hostDisconnectted()
{
    emit HostDisconnectted();

}

/**
 * @brief：下位机连接错误信号对应的槽函数：
 * 1.弹窗警告并打印错误日志
 * @param ：无
 * @retval：无
*/
void NetworkManager::on_hostConnectError()
{
    emit HostConnectError();

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

    emit TcpHexResponse(response);



    if (response.size() >= 12) {

        //去除收到的数据包头,存放在header中,其他功能可能会用
        qDebug()<<"response:"<<response.toHex();
        response=removeCmdPktHeader(response,&header);

        /*根据上位机发送给下位机命令的不同，sendCmdFlag会在发送命令时被赋给不同的值，可选值由TcpSendCmdType枚举类型约束。
         *接收到TCP响应后，数据进入该函数被解析，根据sendCmdFlag的不同，进入不同的分支被解析。分别实现不同的解析函数。
         *parseWorkmodeSetResponse（）处理默认响应（默认响应是指TCP数据部分只回复0或1的响应），其他复杂的响应由各种重载的parsePhyParamResponse（）函数解析
         */
        switch (sendCmdFlag) {
            case TCP_SEND_DEFAULT_STATE://默认响应
                qDebug() << "处理设备默认响应!";
                parseWorkmodeSetResponse(response);
                break;

            case TCP_SEND_GET_PHY_PARAMETER://获取物理参数响应
                qDebug() << "处理物理参数响应!";
                parsePhyParamResponse(response, &phyPara);
                break;

            case TCP_SEND_SET_DEV_WORKMODE://双向发送软件版本
                qDebug() << "处理模式设置响应!";
                parseWorkmodeSetResponse(response);
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
uint8_t NetworkManager::CmdTcpType(uint8_t cmdHeader)
{
    switch (cmdHeader)
    {
    case TCP_SEND_SET_DEV_WORKMODE:
        sendCmdFlag=TCP_UNANSWER_STATE;

        return -1;

    case TCP_SEND_GET_PHY_PARAMETER:
        sendCmdFlag=TCP_SEND_GET_PHY_PARAMETER;
        return 0;

    default:

        sendCmdFlag=TCP_UNANSWER_STATE;
        return -2;
    }
}

/**
 * @brief:向下位机发送命令
 * @param:QString sendText 要发送的命令字符串
 * @retval:返回发送的结果
 * 0：发送成功
 * 1：未连接下位机，发送失败
 * 2：发送的字符串不是合法字符串
 * 3：命令的内容由Qstring转换成uint8_t类型失败
 * 4：tcp发送失败
 * -1：不支持手动发送命令设置工作模式, 请使用模式按键!
 * -2：发送的命令不在命令集之中
 */
uint8_t NetworkManager::sendCmdToHost(QString sendText)
{
    if(socket->state()==QAbstractSocket::UnconnectedState)
    {
        return 1;
    }
    else
    {
        //判断要发送的字符串是否非法
        if(true==isStringInvalid(sendText))
        {
            return 2;
        }

        // 提取第一个字节作为命令
        QString cmdHeaderStr = sendText.left(2);
        bool ok;
        uint8_t cmdHeader = cmdHeaderStr.toUInt(&ok, 16);
        if (!ok)
        {
            return 3;
        }

        // 构建数据包
        QByteArray packet;
        packet.append(buildCmdPktHeader((CommandWord)cmdHeader)); // 使用提取的命令头

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
                return 3;
            }
        }

        //判断发送命令对应的TCP类型
        uint8_t result=CmdTcpType(cmdHeader);
        if(TCP_UNANSWER_STATE==sendCmdFlag)
        {
            return result;
        }

        uint32_t totalLength = CMD_HEADER_LENGTH + dataBytes.size(); // 包总长度（包头4字节 + 数据N字节）
        packet.append(reinterpret_cast<char*>(&totalLength), 4);
        packet.append(dataBytes);

        // 将数据通过 TCP 发出
        qint64 bytesWritten = socket->write(packet);

        if (bytesWritten == -1)
        {
            return 4;
        }
        else
        {
            return 0;
        }
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

        return true;
    }

    return false;
}

//去除收到的数据包的包头,只留下数据部分
QByteArray NetworkManager::removeCmdPktHeader(QByteArray response,CmdPacketHeader *header)
{
    QByteArray headerBytes = response.left(12);
    memcpy(header, headerBytes.constData(), sizeof(CmdPacketHeader));
    response = response.mid(12);

    return response;
}


void NetworkManager::parseWorkmodeSetResponse(QByteArray response)
{
    emit WorkmodeSetResponse(response);

}


void NetworkManager::parsePhyParamResponse(QByteArray response, devPhysicsParameter *phyPara)
{
    //TODO:如果要对phyPara进行什么改动在这个函数中进行，信号发出后不在改动了
    emit PhyParamResponse(response, phyPara);

}
