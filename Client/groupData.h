
#pragma once

#include <string>
#include <cstdint>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

using u32 = unsigned;
using u16 = uint16_t;
using u8 = uint8_t;

namespace GROUP_DATA {

struct client_data {
    std::string ip{};
    u16 port{};
    static QByteArray serilize(const client_data &s) {
        QByteArray C;
        QDataStream out(&C, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_5);
        out << QString::fromStdString(s.ip)
            << s.port;
        return C;
    }
    static client_data deserialize(const QByteArray &s) {
        client_data C;
        QDataStream in(s);
        in.setVersion(QDataStream::Qt_5_5);
        QString str;
        in >> str;
        C.ip = str.toStdString();
        in >> C.port;
        return C;
    }
};

struct command_data {
    std::string cmd{""};
    int ki{0};
    int ns{0};
    static QByteArray serilize(const command_data &s) {
        QByteArray C;
        QDataStream out(&C, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_5);
        out << QString::fromStdString(s.cmd)
            << s.ki
            << s.ns;
        return C;
    }
    static command_data deserialize(const QByteArray &s) {
        command_data C;
        QDataStream in(s);
        in.setVersion(QDataStream::Qt_5_5);
        QString str;
        in >> str;
        C.cmd = str.toStdString();
        in >> C.ki >> C.ns;
        return C;
    }
};

}

namespace TransferProtocol
{

struct PacketFlags
{
    u8 m_boolean_isFirst_ : 1;
    u8 m_boolean_isLast_ : 1;
    u8 m_boolean_isCompressed : 1;
    u8 m_reserved_ : 5;
};

struct PacketHeader
{
    u32 m_int_serverHash_{};
    u32 m_int_totalDataSize_{};
    u16 m_int_totalPackets_{};
    u16 m_int_packetNumber_{};
    u16 m_int_currentPacketSize_{};
    PacketFlags m_flags_{};
};

struct Packet
{
    PacketHeader m_header_{};
    int8_t m_int_dataType_{};
    QByteArray m_data_{};
};

}
