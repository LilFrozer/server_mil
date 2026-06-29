#ifndef INITIALIZER_H
#define INITIALIZER_H

#include "definesMil.h"
#include "dspapiviod.h"
#include "openapiviod.h"
#include "farbos/ts.h"
#include <unordered_map>
#include "lib/libdspapi.h"
#include <fstream>

#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QThread>
#include <memory>
#include "groupData.h"
#include "zstd.h"

class Server : public QObject
{
    Q_OBJECT
public:
    void startSVthread();
    static Server &instance();
    void doStart();
    void sendData(uint8_t idData);
    static QByteArray compressData(const char* data, int size);
private:
    explicit Server(QObject *parent = nullptr);
    ~Server() = default;
public slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
private slots:
    void processData();
protected:
    std::unique_ptr<QTcpServer> server_tcp_;
    std::unique_ptr<QUdpSocket> server_udp_;
    QTcpSocket *client_tcp_;
    GROUP_DATA::client_data client_data_;
    std::unique_ptr<QTimer> process_data_timer_;

    QVector<int> sin1{};
};

class Executor : public QObject
{
    Q_OBJECT
public:
    void startExthread();
    static Executor &instance();
    void doStart();
    void init_fpga_regs();
    void loadTS(const std::string &fullpath);
    far_data &get_far_data() {
        return static_cast<OpenApiViod*>(viodSender_.get())->get_far_data();
    }
public slots:
    void loadCss();
    void loadStart();
    void loadStop();
    void loadNewKu(int ki, int ns);
private:
    std::unique_ptr<fpga_regs::fpga_mem_summator> fpga_regs_;

    ApiMode mode_{ApiMode::OpenApi};
    std::unique_ptr<ViodSender> viodSender_;

    TS::signal tsTable_[Constants::MAX_KNS]{};

    explicit Executor(QObject *parent = nullptr, ApiMode mode = ApiMode::OpenApi);
    ~Executor() {}
};

#endif // INITIALIZER_H
