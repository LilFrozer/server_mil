#include "executor.h"
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QByteArray>
#include <QDataStream>

QByteArray Server::compressData(const char* data, int size)
{
    size_t maxCompressed = ZSTD_compressBound(size);
    QByteArray compressed(maxCompressed, 0);
    size_t compressedSize = ZSTD_compress(compressed.data(), maxCompressed, data, size, 3);

    if (ZSTD_isError(compressedSize))
        return QByteArray();

    compressed.resize(compressedSize);
    return compressed;
}

Server& Server::instance()
{
    static Server object;
    return object;
}

void Server::startSVthread()
{
    QThread *T = new QThread;
    QObject::connect(T, &QThread::started, this, &Server::doStart);
    this->moveToThread(T);
    T->start(QThread::HighPriority);
}

void Server::doStart()
{
    if (server_tcp_->listen(QHostAddress(SERVER_ADDR), 1212))
        qDebug() << "Started listen tcp...";
    if (server_udp_->bind(QHostAddress(SERVER_ADDR), 1212))
        qDebug() << "Started listen udp...";
}

Server::Server(QObject *parent) :
    QObject{parent}
    , server_tcp_{std::make_unique<QTcpServer>(this)}
    , server_udp_{std::make_unique<QUdpSocket>(this)}
    , process_data_timer_{std::make_unique<QTimer>(this)}
{
    QObject::connect(server_tcp_.get(), &QTcpServer::newConnection, this, &Server::onConnected);
    QObject::connect(process_data_timer_.get(), &QTimer::timeout, this, &Server::processData);
}

void Server::onConnected()
{
    client_tcp_ = server_tcp_->nextPendingConnection();
    QObject::connect(client_tcp_, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    QObject::connect(client_tcp_, &QTcpSocket::disconnected, this, &Server::onDisconnected);

    QString ipString = client_tcp_->peerAddress().toString();
    if (ipString.startsWith("::ffff:"))
        ipString.remove(0, 7);
    client_data_.ip = ipString.toStdString();
    client_data_.port = client_tcp_->peerPort();

    QByteArray payload = GROUP_DATA::client_data::serilize(client_data_);
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_7);
    out << quint32(payload.size());   // размер тела
    block.append(payload);

    client_tcp_->write(block);
    client_tcp_->flush();

    qDebug() << "Connected!" << ipString << client_data_.port;
}

void Server::onDisconnected() {
    client_tcp_->deleteLater();

    qDebug() << "Disconnected!";
}

void Server::onReadyRead()
{
    QDataStream stream(client_tcp_);
    stream.setVersion(QDataStream::Qt_5_5);

    quint32 msgSize;
    stream >> msgSize;                              // читаем размер
    QByteArray payload = client_tcp_->read(msgSize); // читаем тело

    // Теперь у нас есть полное сообщение
    GROUP_DATA::command_data data = GROUP_DATA::command_data::deserialize(payload);

    if (data.cmd == "loadcss") {
        QMetaObject::invokeMethod
        (
            &Executor::instance(),
            "loadCss",
            Qt::QueuedConnection
        );
    } else if (data.cmd == "start") {
        QMetaObject::invokeMethod
        (
            &Executor::instance(),
            "loadStart",
            Qt::QueuedConnection
        );
        this->process_data_timer_->start(100);
    } else if (data.cmd == "stop") {
        QMetaObject::invokeMethod
        (
            &Executor::instance(),
            "loadStop",
            Qt::QueuedConnection
        );
        this->process_data_timer_->stop();
    } else if (data.cmd == "loadNewKu") {
        QMetaObject::invokeMethod
        (
            &Executor::instance(),
            "loadNewKu",
            Qt::QueuedConnection,
            Q_ARG(int, data.ki),
            Q_ARG(int, data.ns)
        );
    }
}

void Server::processData() {
//    qDebug() << "processData!";
    this->sendData(0);
    this->sendData(1);

    sin1.reserve(Executor::instance().get_far_data().ki_kd);
    for (size_t i{};i<Executor::instance().get_far_data().ki_kd;++i)
        sin1.push_back(Executor::instance().get_far_data().channel1[i]);

    this->sendData(2);

    sin1.clear();
}

void Server::sendData(uint8_t idData) {
    if (idData == 2 && Executor::instance().get_far_data().ki_kd == 0) return;

    static std::vector<int> txBuf_;
    auto makeHeaderFunc = [&](quint32 total, quint32 num, quint32 cnt, quint32 cur, bool first, bool last)
    {
        TransferProtocol::PacketHeader h;
        h.m_int_serverHash_ = Constants::SERVER_HASH;
        h.m_int_totalDataSize_ = total;
        h.m_int_totalPackets_   = cnt;
        h.m_int_packetNumber_   = num;
        h.m_int_currentPacketSize_ = cur;
        h.m_flags_.m_boolean_isFirst_  = first ? 1u : 0u;
        h.m_flags_.m_boolean_isLast_   = last  ? 1u : 0u;
        h.m_flags_.m_reserved_  = 0;
        return h;
    };

    const void* src = nullptr;
    quint32 size_DATA = 0;
    txBuf_.clear();
    auto needBuf = [&](size_t nInts){
        txBuf_.resize(nInts);
        return txBuf_.data();
    };

    switch(idData)
    {
    case 0:
    {
        src = Executor::instance().get_far_data().status_css;
        size_DATA = Constants::AMOUNT_STATUSES_CSS * sizeof(unsigned);
        break;
    }
    case 1:
    {
        int* d = needBuf(Constants::AMOUNT_SUM * Constants::AMOUNT_STATUSES_SUM);
        int* p = d;
        for (size_t i=0; i<Constants::AMOUNT_SUM; ++i)
            for (size_t j=0; j<Constants::AMOUNT_STATUSES_SUM; ++j)
                *p++ = Executor::instance().get_far_data().status_sum[i][j];
        src = d;
        size_DATA = (quint32)((p - d) * sizeof(int));
        break;
    }
    case 2:
    {
        const quint32 n = 12u;
        if (n == 0)
        {
            src = nullptr;
            size_DATA = 0;
            break;
        }
        // все одинаковой длины
        const quint32 len = static_cast<quint32>(sin1.size());
        int* d = needBuf(size_t(n) * len);
        int* p = d;
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        p = std::copy(sin1.begin(), sin1.end(), p);
        src = d; size_DATA = (quint32)((p - d) * sizeof(int));
        break;
    }
    default: break;
    }

    const quint32 MAX_PAY = Constants::MAX_SIZE_PACKET_DATA;
    const quint32 total = size_DATA;
    const void *data = src;

    QByteArray allData(static_cast<const char*>(data), total);
    QByteArray compressedAll = compressData(allData.constData(), allData.size());

    bool useCompression = !compressedAll.isEmpty() && compressedAll.size() < static_cast<int>(total);
    QByteArray& dataToSend = useCompression ? compressedAll : allData;
    quint32 sendTotalSize = useCompression ? static_cast<quint32>(dataToSend.size()) : total;

    const quint32 cnt = (sendTotalSize + MAX_PAY - 1) / MAX_PAY;

    for (quint32 idx{}; idx<cnt; ++idx) {
        const bool last = (idx + 1 == cnt);
        const quint32 chunk = last ? (sendTotalSize - MAX_PAY * idx) : MAX_PAY;

        TransferProtocol::PacketHeader H = makeHeaderFunc(total, idx + 1, cnt,
                                     sizeof(TransferProtocol::PacketHeader)+sizeof(int8_t)+chunk,
                                     idx==0, last);
        H.m_flags_.m_boolean_isCompressed = useCompression ? 1 : 0;

        QByteArray payload;
        payload.resize(int(sizeof(TransferProtocol::PacketHeader) + sizeof(int8_t) + chunk));
        char* w = payload.data();
        memcpy(w, &H, sizeof(H));
        w += sizeof(H);
        memcpy(w, &idData, sizeof(idData));
        w += sizeof(idData);
        memcpy(w, dataToSend.constData() + MAX_PAY*idx, chunk);

        qint64 bytesSent = server_udp_->writeDatagram(payload,
                                    QHostAddress("190.0.3.102"), 2999);
        if (bytesSent == -1)
            qDebug() << "Error_udp:" << server_udp_->errorString();
    }
}

void Executor::init_fpga_regs()
{
    auto *regs = this->fpga_regs_.get();

    regs->dir_tx[0].dir_tx = 0;
    regs->dir_tx[1].dir_tx = 5;
    regs->dir_tx[2].dir_tx = 0;     // 6вход в BOS
    regs->dir_tx[3].dir_tx = 0;     // 1вход в BOS
    regs->dir_tx[4].dir_tx = 2;     // 2вход в BOS
    regs->dir_tx[5].dir_tx = 3;     // 3вход в BOS
    regs->dir_tx[6].dir_tx = 4;     // 4вход в BOS

    regs->cntr_sum.ctrl_sum = 0x2;
    regs->mask[0].mask = 0x1f;
    regs->mask[1].mask = 0x1f;
    regs->mask[2].mask = 0x1f;

    // !!!с какого чида, линпорта, процессора, ячейки!!!

    constexpr u16 procDestination = 1;  // на какой процессор все пакеты будет приходить
    constexpr u16 cDestionation = 4;    // на какую ячейку все пакеты будут приходить

    regs->dir_rx_az = {0, 3, procDestination, cDestionation};
    regs->dir_rx_css = {5, 3, procDestination, cDestionation};
    regs->dir_rx[0] = {1, 3, procDestination, cDestionation};
    regs->dir_rx[1] = {1, 3, procDestination, cDestionation};
    regs->dir_rx[2] = {2, 3, procDestination, cDestionation};
    regs->dir_rx[3] = {3, 3, procDestination, cDestionation};
    regs->dir_rx[4] = {4, 3, procDestination, cDestionation};

    // Данные вспомогательного канала
    regs->dir_rx_sum1[0] = {11, 3, procDestination, cDestionation};
    regs->dir_rx_sum1[1] = {12, 3, procDestination, cDestionation};
    regs->dir_rx_sum1[2] = {13, 3, procDestination, cDestionation};
    regs->dir_rx_sum1[3] = {14, 3, procDestination, cDestionation};
    regs->dir_rx_sum1[4] = {15, 3, procDestination, cDestionation};

    // Данные основного канала
    regs->dir_rx_sum3[0] = {6, 3, procDestination, cDestionation};
    regs->dir_rx_sum3[1] = {7, 3, procDestination, cDestionation};
    regs->dir_rx_sum3[2] = {8, 3, procDestination, cDestionation};
    regs->dir_rx_sum3[3] = {9, 3, procDestination, cDestionation};
    regs->dir_rx_sum3[4] = {10, 3, procDestination, cDestionation};

    // ---
    regs->dir_rx_sum2[0] = {0, 3, 3, 4};
    regs->dir_rx_sum2[1] = {12, 3, 3, 4};
    regs->dir_rx_sum2[2] = {13, 3, 3, 4};
    regs->dir_rx_sum2[3] = {14, 3, 3, 4};
    regs->dir_rx_sum2[4] = {15, 3, 3, 4};

    regs->mask_devices = 0x7e;

    unii_init("192.0.1.101", "192.0.1.101", true);
    BUS_VME_DESCRIPTOR = unii_open((char*)"/devices/hla");

    unii_write_reg(BUS_VME_DESCRIPTOR, 0x0, static_cast<u32>(0));
    unii_write_reg(BUS_VME_DESCRIPTOR, 0x0, static_cast<u32>(1));

    unii_write_reg(BUS_VME_DESCRIPTOR, 0x4000, static_cast<u32>(0));
    unii_write_reg(BUS_VME_DESCRIPTOR, 0x4000, static_cast<u32>(1));

    QThread::msleep(200);

    unii_write_reg(BUS_VME_DESCRIPTOR, fpga_regs::FPGAREG_FARBOS_CNTR, static_cast<u32>(0));
    unii_write_reg(BUS_VME_DESCRIPTOR, fpga_regs::FPGAREG_FARBOS_CNTR, static_cast<u32>(1));

    QThread::msleep(200);

    auto *ptr_ = reinterpret_cast<u32*>(this->fpga_regs_.get()) + fpga_regs::SHIFT_DIR_TX;
    for (u32 i{};i<fpga_regs::amount_regs_summator-fpga_regs::SHIFT_DIR_TX;++i) {
        int err = unii_write_reg(BUS_VME_DESCRIPTOR, fpga_regs::FPGAREG_FARBOS_DIR_TX+i, ptr_[i]);
        if (err == -1)
            throw std::runtime_error("error unii_write");
    }
}

void Executor::loadTS(const std::string &fullpath)
{
    QFile file(QString::fromStdString(fullpath));
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        throw std::runtime_error("error open TS file");

    QTextStream in(&file);
    int count = 0, headerCount = 0;
    while(!in.atEnd()) {
        QString line = in.readLine().trimmed();
        // Пропускаем пустые строки
        if(line.isEmpty() || headerCount != 4) {
            ++headerCount;
            continue;
        }
        // Проверяем, что строка начинается с 0x (hex число)
        if(line.startsWith("0x", Qt::CaseInsensitive)) {
            bool ok;
            u32 num = line.toUInt(&ok, 16); // Пробуем преобразовать
            if(ok && num != 0) {
                TS::file_signal *ptr = reinterpret_cast<TS::file_signal*>(&num);
                tsTable_[count].kns = count;
                tsTable_[count].dzi = ptr->dzi;
                tsTable_[count].vvi = ptr->vvi*8;
                count++;
            }
        }
    }
    file.close();

    qDebug() << "Количество сигналов в таблице TS1.dat =" << count;
}

void Executor::loadStart() {
    std::vector<qword> config;
    config.reserve(4);
    for (size_t i{};i<4;++i)
        config.push_back(qword{0,0,0,0});

    config[0].word_0 = 1;

    int DMID = 4;
    int DLID = 1;
    int CHID = 3;
    viodSender_->sendViodMsg(config, 0, 0, 0, DMID, 1, DLID, CHID);
}

void Executor::loadStop() {
    std::vector<qword> config;
    config.reserve(4);
    for (size_t i{};i<4;++i)
        config.push_back(qword{0,0,0,0});

    config[0].word_0 = 0;

    int DMID = 4;
    int DLID = 1;
    int CHID = 3;
    viodSender_->sendViodMsg(config, 0, 0, 0, DMID, 1, DLID, CHID);
}

Executor::Executor(QObject *parent, ApiMode mode) : QObject{parent}, fpga_regs_{std::make_unique<fpga_regs::fpga_mem_summator>()}
{
    mode_ = mode;
    switch(mode_)
    {
    case ApiMode::DspApi: break;
    case ApiMode::OpenApi: viodSender_ = std::make_unique<OpenApiViod>(this); break;
    default: throw std::runtime_error("bad...");
    }
}

void Executor::loadCss()
{
//    this->init_fpga_regs();

    std::string PATH{PATH_HARD};

    // -> word(reset)
    std::vector<qword> r;
    r.push_back(qword{0, 0, 0, 0});
    proto_farbos::farbos_header *phead = reinterpret_cast<proto_farbos::farbos_header*>(&r[0]);
    phead->dk = 0;
    phead->tk = 1;
    phead->prmbl = 0xA5;
    phead->snp = 0;
    phead->py = 0;
    phead->marker = 0;
    unsigned long *ptr = reinterpret_cast<unsigned long*>(&r[0]);
    std::string bytes = proto_farbos::extract14BytesToHex(ptr, 0);
    std::vector<u8> vecBytes = proto_farbos::hexToBytes(bytes);
    u16 calculatedCRC = proto_farbos::Calc_CRC(vecBytes);
    phead->crc = calculatedCRC;
    viodSender_->sendViodMsg(r, 0, 0, 0, 0x8, 0, 0, 5);

    // !!!SLEEEEEEEP!!!
    QThread::msleep(200);

    std::string pathCss = "";
    std::vector<std::string> filesCss{};

    QJsonParseError parseError;
    QFile jsonFile(QString::fromStdString(PATH));
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
        throw std::runtime_error("Cannot open file");

    QByteArray jsonData = jsonFile.readAll();
    jsonFile.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError)
        throw std::runtime_error("JSON parse error");

    if (!doc.isObject())
        throw std::runtime_error("JSON is not an object");

    QJsonObject root = doc.object();
    if (root.contains("PATH") && root["PATH"].isString())
        pathCss = root["PATH"].toString().toStdString();

    if (root.contains("CSS_FILES") && root["CSS_FILES"].isArray())
    {
        QJsonArray filesArray = root["CSS_FILES"].toArray();
        for (const QJsonValue& value : filesArray)
            filesCss.push_back(value.toString().toStdString());
    }


    for (size_t i{};i<filesCss.size();++i) {
        std::string fullPath = pathCss + filesCss[i];
        std::ifstream file(fullPath, std::ios::binary);

        if (!file.is_open()) {
            qDebug() << "Can not open file" << QString::fromStdString(fullPath);
            break;
        }

        file.seekg(0, std::ios::end);
        int fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        qDebug() << "------" << "File ->" << QString::fromStdString(fullPath) << "open OK. size =" << fileSize / sizeof(int) << "------";

        std::vector<std::vector<u32>> packets;
        std::vector<u32> nums;
        int count = 0, payloadSize = 0;
        u32 value;


        std::string strHex;

        while(file >> strHex) {
            value = std::stoul(strHex, nullptr, 16);
            nums.push_back(value);
            count++;

            proto_farbos::farbos_header *ptr = reinterpret_cast<proto_farbos::farbos_header*>(&value);
            if(ptr->prmbl == 0xA5 && payloadSize == 0)
                payloadSize = ptr->dk;

            if (payloadSize > 0 && count == 4 + payloadSize) {
                packets.push_back(nums);
                nums.clear();
                payloadSize = 0;
                count = 0;
            }
        }

        if(!nums.empty()) {
            qDebug() << "!nums.empty";
            packets.push_back(nums);
        }

        file.close();

//        qDebug() << "count of 0xA5.... in file ->" << packets.size();

        auto func = [&](const std::vector<std::vector<u32>> &wordBlocks) -> std::vector<std::vector<qword>>
        {
            std::vector<std::vector<qword>> result;
            result.reserve(wordBlocks.size());

            for (const auto& block : wordBlocks) {
                std::vector<qword> qwBlock;

                size_t i = 0;
                for (;i+3<block.size();i+=4)
                    qwBlock.push_back({block[i], block[i+1], block[i+2], block[i+3]});

                if (i < block.size()) {
                    qword last{0,0,0,0};
                    last.word_0 = block[i];
                    if (i+1<block.size()) last.word_1 = block[i+1];
                    if (i+2<block.size()) last.word_2 = block[i+2];
                    if (i+3<block.size()) last.word_3 = block[i+3];
                    qwBlock.push_back(last);
                }

                result.push_back(std::move(qwBlock));
            }

            return result;
        };

        std::vector<std::vector<qword>> data = func(packets);
        for(size_t i{};i<data.size();++i) {
            std::vector<std::vector<qword>> A;
            std::vector<qword> tmp;
            tmp.reserve(Constants::VIOD_MAX_PAYLOAD_WORDS - 1);
            for (size_t j{};j<data[i].size();++j) {
                tmp.push_back(data[i][j]);
                if ((j+1) % (Constants::VIOD_MAX_PAYLOAD_WORDS - 1) == 0) {
                    A.push_back(tmp);
                    tmp.clear();
                    tmp.reserve(Constants::VIOD_MAX_PAYLOAD_WORDS - 1);
                }
                if (j == data[i].size() - 1)
                    A.push_back(tmp);
            }
            for(size_t j{};j<A.size();++j) {
                viodSender_->sendViodMsg(A[j], 0, 0, 0, 0x8, 0, 0, 5);
                QThread::msleep(10);
            }
        }
    }
}

void Executor::loadNewKu(int ki, int ns)
{
    loadTS("/po/complex/tech_new/furke/css/TS_1.dat");

    std::string PATH{PATH_HARD};

    // !css!
    std::vector<qword> words1;
    for(size_t i{};i<4;++i)
        words1.push_back(qword{0, 0, 0, 0});

    proto_farbos::farbos_header *cssfbhdr = reinterpret_cast<proto_farbos::farbos_header*>(&words1[0]);
    cssfbhdr->dk = 12;
    cssfbhdr->tk = 0x9;
    cssfbhdr->prmbl = 0xA5;
    cssfbhdr->snp = 0;
    cssfbhdr->py = 0;
    cssfbhdr->marker = 0;
    unsigned long *ptr = reinterpret_cast<unsigned long*>(&words1[0]);
    std::string bytes = proto_farbos::extract14BytesToHex(ptr, 0);
    std::vector<u8> vecBytes = proto_farbos::hexToBytes(bytes);
    u16 calculatedCRC = proto_farbos::Calc_CRC(vecBytes);
    cssfbhdr->crc = calculatedCRC;

    proto_farbos::frame_upr_css *css = reinterpret_cast<proto_farbos::frame_upr_css*>(&words1[1]);
    int kns = ns;
    css->nkch = 2;
    css->pr_zi = 1;
    css->pr_ps = 1;
    css->pr_nlchm = 1;
    css->pr_rk2 = 1;
    css->ki = ki;
    css->zero = 0;
    css->pr_pk_dsgp = 0;
    css->kns = tsTable_[kns].kns;

    // !sum!
    std::vector<qword> words2;
    for(size_t i{};i<40;++i)
        words2.push_back(qword{0, 0, 0, 0});

    proto_farbos::farbos_header *sumfbhdr = reinterpret_cast<proto_farbos::farbos_header*>(&words2[0]);
    sumfbhdr->dk = 156;
    sumfbhdr->tk = 0x9;
    sumfbhdr->prmbl = 0xA5;
    sumfbhdr->snp = 0;
    sumfbhdr->py = 0;
    sumfbhdr->marker = 0;
    unsigned long *p = reinterpret_cast<unsigned long*>(&words2[0]);
    std::string b = proto_farbos::extract14BytesToHex(p, 0);
    std::vector<u8> v = proto_farbos::hexToBytes(b);
    uint16_t c = proto_farbos::Calc_CRC(v);
    sumfbhdr->crc = c;

    proto_farbos::frame_upr_sum *sum = reinterpret_cast<proto_farbos::frame_upr_sum*>(&words2[1]);
    sum->ump = 1;
    sum->dzi = tsTable_[kns].dzi;
    sum->vvi = tsTable_[kns].vvi;
    sum->kd = tsTable_[kns].vvi - tsTable_[kns].dzi - 8;
//    qDebug() << "dzi=" << m_tsTable[kns].dzi <<
//                "vvi=" << m_tsTable[kns].vvi <<
//                "kd=" << m_tsTable[kns].vvi - m_tsTable[kns].dzi - 8 <<
//                "kns=" << m_tsTable[kns].kns;
    sum->ki = ki;
    sum->kns = tsTable_[kns].kns;
    sum->pr_ps = 1;
    sum->pr_zi = 1;
    sum->nkch = 2;

    for (size_t i{};i<20;++i) {
        sum->sinCos[i].sincos[0] = 0x7FFF7FFF;
        sum->sinCos[i].sincos[1] = 0x7FFF7FFF;
        sum->sinCos[i].sincos[2] = 0x7FFF7FFF;
        sum->sinCos[i].sincos[3] = 0x7FFF7FFF;
        sum->sinCos[i].sincos[4] = 0x7FFF7FFF;
        sum->sinCos[i].sincos[5] = 0x7FFF7FFF;
    }

//    sum->sinCos[1] = 0x7FFF;
//    sum->sinCos[2] = 0x7FFF;
//    sum->sinCos[3] = 0x7FFF;
//    sum->sinCos[4] = 0x7FFF;
//    sum->sinCos[5] = 0x7FFF;

    int smid = 4;
    viodSender_->sendViodMsg(words1, 0, 0, 0, smid, 1, 2, 2);
    viodSender_->sendViodMsg(words2, 0, 0, 0, smid, 1, 1, 2);
}

Executor& Executor::instance()
{
    static Executor object(nullptr, ApiMode::OpenApi);
    return object;
}

void Executor::startExthread()
{
    QThread *T = new QThread;
    QObject::connect(T, &QThread::started, this, &Executor::doStart);
    this->moveToThread(T);
    T->start(QThread::HighPriority);
}

void Executor::doStart()
{
    this->init_fpga_regs();
}
