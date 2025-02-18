#include "cmdBuild.h"

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

