#include "client.h"
#include "./ui_client.h"

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
    , plot_{std::make_unique<QCustomPlot>(this)}
    , socket_{std::make_unique<QTcpSocket>(this)}
    , udp_socket_{std::make_unique<QUdpSocket>(this)}
{
    ui->setupUi(this);

    plot_->addGraph();
    ui->graphicLayout->addWidget(plot_.get());

    wdgts_status_css_ = {ui->wdgt_status_css1,
                        ui->wdgt_status_css2,
                        ui->wdgt_status_css3,
                        ui->wdgt_status_css4,
                        ui->wdgt_status_css5,
                        ui->wdgt_status_css6,
                        ui->wdgt_status_css7,
                        ui->wdgt_status_css8,
                        ui->wdgt_status_css9,
                        ui->wdgt_status_css10,
                        ui->wdgt_status_css11,
                        ui->wdgt_status_css12,
                        ui->wdgt_status_css13,
                        ui->wdgt_status_css14,
                        ui->wdgt_status_css15,
                        ui->wdgt_status_css16};

    wdgts_status_sum1_ = {ui->wdgt_status_sum11,
                          ui->wdgt_status_sum12,
                          ui->wdgt_status_sum13,
                          ui->wdgt_status_sum14,
                          ui->wdgt_status_sum15,
                          ui->wdgt_status_sum16,
                          ui->wdgt_status_sum17,
                          ui->wdgt_status_sum18,
                          ui->wdgt_status_sum19,
                          ui->wdgt_status_sum110,
                          ui->wdgt_status_sum111,
                          ui->wdgt_status_sum112,
                          ui->wdgt_status_sum113,
                          ui->wdgt_status_sum114,
                          ui->wdgt_status_sum115,
                          ui->wdgt_status_sum116,
                          ui->wdgt_status_sum117,
                          ui->wdgt_status_sum118,
                          ui->wdgt_status_sum119,
                          ui->wdgt_status_sum120,
                          ui->wdgt_status_sum121,
                          ui->wdgt_status_sum122,
                          ui->wdgt_status_sum123,
                          ui->wdgt_status_sum124,
                          ui->wdgt_status_sum125,
                          ui->wdgt_status_sum126,
                          ui->wdgt_status_sum127,
                          ui->wdgt_status_sum128,
                          ui->wdgt_status_sum129,
                          ui->wdgt_status_sum130,
                          ui->wdgt_status_sum131,
                          ui->wdgt_status_sum132};

    wdgts_status_sum2_ = {ui->wdgt_status_sum21,
                          ui->wdgt_status_sum22,
                          ui->wdgt_status_sum23,
                          ui->wdgt_status_sum24,
                          ui->wdgt_status_sum25,
                          ui->wdgt_status_sum26,
                          ui->wdgt_status_sum27,
                          ui->wdgt_status_sum28,
                          ui->wdgt_status_sum29,
                          ui->wdgt_status_sum210,
                          ui->wdgt_status_sum211,
                          ui->wdgt_status_sum212,
                          ui->wdgt_status_sum213,
                          ui->wdgt_status_sum214,
                          ui->wdgt_status_sum215,
                          ui->wdgt_status_sum216,
                          ui->wdgt_status_sum217,
                          ui->wdgt_status_sum218,
                          ui->wdgt_status_sum219,
                          ui->wdgt_status_sum220,
                          ui->wdgt_status_sum221,
                          ui->wdgt_status_sum222,
                          ui->wdgt_status_sum223,
                          ui->wdgt_status_sum224,
                          ui->wdgt_status_sum225,
                          ui->wdgt_status_sum226,
                          ui->wdgt_status_sum227,
                          ui->wdgt_status_sum228,
                          ui->wdgt_status_sum229,
                          ui->wdgt_status_sum230,
                          ui->wdgt_status_sum231,
                          ui->wdgt_status_sum232};

    wdgts_status_sum3_ = {ui->wdgt_status_sum31,
                          ui->wdgt_status_sum32,
                          ui->wdgt_status_sum33,
                          ui->wdgt_status_sum34,
                          ui->wdgt_status_sum35,
                          ui->wdgt_status_sum36,
                          ui->wdgt_status_sum37,
                          ui->wdgt_status_sum38,
                          ui->wdgt_status_sum39,
                          ui->wdgt_status_sum310,
                          ui->wdgt_status_sum311,
                          ui->wdgt_status_sum312,
                          ui->wdgt_status_sum313,
                          ui->wdgt_status_sum314,
                          ui->wdgt_status_sum315,
                          ui->wdgt_status_sum316,
                          ui->wdgt_status_sum317,
                          ui->wdgt_status_sum318,
                          ui->wdgt_status_sum319,
                          ui->wdgt_status_sum320,
                          ui->wdgt_status_sum321,
                          ui->wdgt_status_sum322,
                          ui->wdgt_status_sum323,
                          ui->wdgt_status_sum324,
                          ui->wdgt_status_sum325,
                          ui->wdgt_status_sum326,
                          ui->wdgt_status_sum327,
                          ui->wdgt_status_sum328,
                          ui->wdgt_status_sum329,
                          ui->wdgt_status_sum330,
                          ui->wdgt_status_sum331,
                          ui->wdgt_status_sum332};

    QObject::connect(ui->wdgt_connect, &QPushButton::clicked, this, & Client::onConnect);
    QObject::connect(ui->wdgt_load_css, &QPushButton::clicked, this, & Client::onLoadCss);
    QObject::connect(ui->wdgt_start, &QPushButton::clicked, this, & Client::onStart);
    QObject::connect(ui->wdgt_stop, &QPushButton::clicked, this, & Client::onStop);
    QObject::connect(ui->wdgt_send_ku, &QPushButton::clicked, this, & Client::onSendKu);

    QObject::connect(socket_.get(), &QTcpSocket::connected, this, &Client::onConnected);
    QObject::connect(socket_.get(), &QTcpSocket::disconnected, this, &Client::onDisconnected);
    QObject::connect(socket_.get(), &QTcpSocket::readyRead, this, &Client::onReadyRead);

    udp_socket_ = std::make_unique<QUdpSocket>(this);
    QObject::connect(udp_socket_.get(), &QUdpSocket::readyRead, this, &Client::onReadyReadUdp);
    if (udp_socket_->bind(QHostAddress("190.0.3.102"), 2999)) {
        qDebug() << "udp`s opened";
    }
}

Client::~Client()
{
    delete ui;
}

void Client::onConnect() {
    socket_->connectToHost(QHostAddress("190.0.3.34"), 1212);
}

void Client::onConnected() {
    qDebug() << "Подключено к серверу!";
}

void Client::onDisconnected() {
    qDebug() << "Отключено от сервера!";
//    udp_socket_->close();
//    udp_socket_.reset();
}

void Client::onReadyRead() {
    QDataStream stream(socket_.get());
    stream.setVersion(QDataStream::Qt_5_5);

    quint32 msgSize;
    stream >> msgSize;                          // читаем размер
    QByteArray payload = socket_->read(msgSize); // читаем тело

    // Теперь у нас есть полное сообщение
    GROUP_DATA::client_data data = GROUP_DATA::client_data::deserialize(payload);
    qDebug() << "Получены данные: IP =" << QString::fromStdString(data.ip)
             << "порт =" << data.port;
}

void Client::onReadyReadUdp() {
    static QHostAddress sAddr;
    static quint16 sPort = 0;

    while (udp_socket_ && udp_socket_->hasPendingDatagrams())
    {
        const qint64 pendingSize = udp_socket_->pendingDatagramSize();
        if (pendingSize <= 0)
        {
            udp_socket_->readDatagram(nullptr, 0, &sAddr, &sPort);
            continue;
        }

        // !!!резервируем если необходимо и затем resize для чтения!!!
        if (this->recvBuffer_.capacity() < pendingSize)
            this->recvBuffer_.reserve(static_cast<int>(pendingSize));
        this->recvBuffer_.resize(static_cast<int>(pendingSize));

        qint64 got = udp_socket_->readDatagram(this->recvBuffer_.data(), this->recvBuffer_.size(), &sAddr, &sPort);
        if (got <= 0)
            continue;

        const qint64 minHeaderSize = static_cast<qint64>(sizeof(TransferProtocol::PacketHeader) + sizeof(int8_t));
        if (got < minHeaderSize)
            continue;

        TransferProtocol::Packet packet = readPacketFast(this->recvBuffer_);

        switch (packet.m_int_dataType_)
        {
        case 0x0: {
            static QByteArray assembledStatusCssData;
            this->status_css_.clear();
            assembledStatusCssData.append(packet.m_data_);
            QByteArray finalData;
            if (packet.m_header_.m_flags_.m_boolean_isCompressed)
                finalData = decompressData(assembledStatusCssData);
            else
                finalData = assembledStatusCssData;
            const int* intArray = reinterpret_cast<const int*>(finalData.constData());
            const quint32 dataSizeInInts = finalData.size() / sizeof(int);
            this->status_css_.resize(static_cast<int>(dataSizeInInts));
            memcpy(this->status_css_.data(), intArray, static_cast<size_t>(dataSizeInInts) << 2);

            for (u32 i{};i<16;++i)
                wdgts_status_css_[i]->setText("0x" + QString().asprintf("%.8x", status_css_[i]));

            assembledStatusCssData.clear();
            break;
        }
        case 0x1: {
            static QByteArray assembledStatusSumData;
            this->status_sum_.clear();
            assembledStatusSumData.append(packet.m_data_);
            QByteArray finalData;
            if (packet.m_header_.m_flags_.m_boolean_isCompressed)
                finalData = decompressData(assembledStatusSumData);
            else
                finalData = assembledStatusSumData;
            const int* intArray = reinterpret_cast<const int*>(finalData.constData());
            const quint32 dataSizeInInts = finalData.size() / sizeof(int);
            this->status_sum_.resize(static_cast<int>(dataSizeInInts));
            memcpy(this->status_sum_.data(), intArray, static_cast<size_t>(dataSizeInInts) << 2);

            for (u32 i{};i<32;++i)
                wdgts_status_sum1_[i]->setText("0x" + QString().asprintf("%.8x", status_sum_[i]));

            for (u32 i{};i<32;++i)
                wdgts_status_sum2_[i]->setText("0x" + QString().asprintf("%.8x", status_sum_[i+32]));

            for (u32 i{};i<32;++i)
                wdgts_status_sum3_[i]->setText("0x" + QString().asprintf("%.8x", status_sum_[i+32+32]));

            assembledStatusSumData.clear();
            break;
        }
        case 0x2: {
            static QByteArray assembledImpulseData;

            QVector<int> listSignalData;
            listSignalData.reserve(packet.m_header_.m_int_totalDataSize_/sizeof(int));

            const bool isFirst = packet.m_header_.m_flags_.m_boolean_isFirst_;
            const bool isLast = packet.m_header_.m_flags_.m_boolean_isLast_;
            if ((!isFirst && isLast) || (isFirst && isLast))
            {
                assembledImpulseData.append(packet.m_data_);
                QByteArray finalData;
                if (packet.m_header_.m_flags_.m_boolean_isCompressed)
                    finalData = decompressData(assembledImpulseData);
                else
                    finalData = assembledImpulseData;

                const int* intArray = reinterpret_cast<const int*>(finalData.constData());
                const quint32 dataSizeInInts = finalData.size() / sizeof(int);
                listSignalData.resize(static_cast<int>(dataSizeInInts));
                memcpy(listSignalData.data(), intArray, finalData.size());

                QVector<double> SIN1, COS1, AMP1, X;
                SIN1.reserve(listSignalData.size() / 12);
                AMP1.reserve(listSignalData.size() / 12);
                X.reserve(listSignalData.size() / 12);

                for(int i{};i<listSignalData.size() / 12;++i) {
                   SIN1.push_back(static_cast<short>(listSignalData[i] & 0xFFFF));
                   COS1.push_back(static_cast<short>((listSignalData[i] >> 16) & 0xFFFF));
                   AMP1.push_back(static_cast<float>(sqrt(10.0)*sqrt(SIN1[i] * SIN1[i]/10.0 + COS1[i] * COS1[i]/10.0)));
                   X.push_back(i);
               }

                plot_->graph(0)->setData(X, AMP1);

                plot_->xAxis->rescale();
                plot_->yAxis->rescale();

                plot_->replot();

                assembledImpulseData.clear();
            }
            if ((!isFirst && !isLast) || (isFirst && !isLast))
            {
                assembledImpulseData.append(packet.m_data_);
            }

            break;
        }
        }
    }
}

TransferProtocol::Packet Client::readPacketFast(QByteArray &DATA)
{
    TransferProtocol::Packet packet;
    const char* raw = DATA.constData();

    // !!!Вытаскиваем заголовок и dataType!!!
    memcpy(&packet.m_header_, raw, sizeof(TransferProtocol::PacketHeader));
    memcpy(&packet.m_int_dataType_, raw + sizeof(TransferProtocol::PacketHeader), sizeof(int8_t));

    const int dataOffset = static_cast<int>(sizeof(TransferProtocol::PacketHeader) + sizeof(int8_t));
    int dataSize = 0;
    int currentPacketSize = static_cast<int>(packet.m_header_.m_int_currentPacketSize_);
    (currentPacketSize > dataOffset) ? dataSize = currentPacketSize - dataOffset : dataSize = 0;
    if (dataSize > 0)
        packet.m_data_ = QByteArray::fromRawData(raw + dataOffset, dataSize);
    else packet.m_data_.clear();

    return packet;
}


QByteArray Client::decompressData(const QByteArray& compressed)
{
    // Проверяем, сжаты ли данные (Zstd magic number: 0xFD2FB528)
    if (compressed.size() < 4) return compressed;

    const unsigned char* data = reinterpret_cast<const unsigned char*>(compressed.constData());
    if (data[0] == 0x28 && data[1] == 0xB5 && data[2] == 0x2F && data[3] == 0xFD)
    {
        // Это Zstd сжатые данные
        size_t decompressedSize = ZSTD_getFrameContentSize(
            compressed.constData(),
            compressed.size()
            );

        if (decompressedSize != ZSTD_CONTENTSIZE_ERROR && decompressedSize > 0) {
            QByteArray decompressed(static_cast<int>(decompressedSize), 0);
            size_t actualSize = ZSTD_decompress(
                decompressed.data(), decompressedSize,
                compressed.constData(), compressed.size()
                );

            if (!ZSTD_isError(actualSize) && actualSize > 0) {
                decompressed.resize(static_cast<int>(actualSize));
                return decompressed;
            }
        }
    }

    // Если не сжато или ошибка распаковки - возвращаем как есть
    return compressed;
}


void Client::onLoadCss() {
    if (!socket_)
        return;

    GROUP_DATA::command_data cmd;
    cmd.cmd = "loadcss";
    cmd.ki = ui->wdgt_KI->value();
    cmd.ns = ui->wdgt_NS->value();

    QByteArray payload = GROUP_DATA::command_data::serilize(cmd);
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_7);
    out << quint32(payload.size());
    block.append(payload);

    socket_->write(block);
    socket_->flush();
}

void Client::onStart() {
    if (!socket_)
        return;

    GROUP_DATA::command_data cmd;
    cmd.cmd = "start";
    cmd.ki = ui->wdgt_KI->value();
    cmd.ns = ui->wdgt_NS->value();

    QByteArray payload = GROUP_DATA::command_data::serilize(cmd);
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_7);
    out << quint32(payload.size());
    block.append(payload);

    socket_->write(block);
    socket_->flush();
}

void Client::onStop() {
    if (!socket_)
        return;

    GROUP_DATA::command_data cmd;
    cmd.cmd = "stop";
    cmd.ki = ui->wdgt_KI->value();
    cmd.ns = ui->wdgt_NS->value();

    QByteArray payload = GROUP_DATA::command_data::serilize(cmd);
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_7);
    out << quint32(payload.size());
    block.append(payload);

    socket_->write(block);
    socket_->flush();
}

void Client::onSendKu() {
    if (!socket_)
        return;

    GROUP_DATA::command_data cmd;
    cmd.cmd = "loadNewKu";
    cmd.ki = ui->wdgt_KI->value();
    cmd.ns = ui->wdgt_NS->value();

    QByteArray payload = GROUP_DATA::command_data::serilize(cmd);
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_7);
    out << quint32(payload.size());
    block.append(payload);

    socket_->write(block);
    socket_->flush();
}
