#ifndef DEFINESMIL_H
#define DEFINESMIL_H

// !!!IMITATOR!!!
#define IMIT

#ifdef IMIT
    // some info: 043 -> ячейка, где лежит ОС(kpda2021),
    // як -> ячейка управления(техно. канал + канал данных)
    #define O43_ADDR "192.168.58.100"
    #define YAK_ADDR "192.168.58.100"
    #define SERVER_ADDR "192.168.58.100"
#else
    #define O43_ADDR "192.0.0.100"
    #define YAK_ADDR "192.0.0.115"
    #define SERVER_ADDR "190.0.3.34"
#endif

#include <thread>
#include <atomic>
#include <vector>
#include <QObject>
#include <QDebug>

using u32 = unsigned;
using u16 = uint16_t;
using u8 = uint8_t;

namespace Constants
{
const u16 VIOD_MAX_PAYLOAD_WORDS = 89;
const u16 CNT_SUMMATOR_LINES = 20;
const u16 MAX_KNS = 255;
const u32 SERVER_HASH = 0x228;
const u32 MAX_SIZE_PACKET_DATA = 1400;
const u32 AMOUNT_STATUSES_CSS = 16;
const u32 AMOUNT_SUM = 3;
const u32 AMOUNT_STATUSES_SUM = 32;
}
#define PATH_HARD "/po/complex/tech_new/furke/hard.json"

struct qword
{
    u32 word_0;
    u32 word_1;
    u32 word_2;
    u32 word_3;
};

namespace fpga_regs
{
const u16 SHIFT_DIR_TX = 0x8;
const u16 FPGAREG_FARBOS = 0x450;  // addr "Контроллер ФАР-БОС"
const u16 FPGAREG_FARBOS_CNTR = 0x7 + FPGAREG_FARBOS;
const u16 FPGAREG_FARBOS_DIR_TX = SHIFT_DIR_TX + FPGAREG_FARBOS;
struct summ_reg_dir_rx
{
    u32 ch_id : 4;
    u32 dlid : 2;
    u32 dpid : 2;
    u32 dmid : 4;
    u32 : 20;
};
struct summ_reg_dir_tx
{
    u32 dir_tx :4;
    u32 : 28;
};
struct summ_reg_ctrl_sum
{
    u32 ctrl_sum : 2;
    u32 : 30;
};
struct summ_reg_mask
{
    u32 mask : 5;
    u32 : 27;
};
struct fpga_mem_summator
{
    u32 status[7]{};
    u32 cntr{};
    summ_reg_dir_tx dir_tx[7]{};
    summ_reg_ctrl_sum cntr_sum{};
    summ_reg_mask mask[3]{};
    summ_reg_dir_rx dir_rx_az{};
    summ_reg_dir_rx dir_rx_css{};
    summ_reg_dir_rx dir_rx[5]{};
    summ_reg_dir_rx dir_rx_sum1[5]{};
    summ_reg_dir_rx dir_rx_sum2[5]{};
    summ_reg_dir_rx dir_rx_sum3[5]{};
    u32 mask_devices{};
};
const u32 amount_regs_summator = sizeof(fpga_mem_summator) / sizeof(u32);
}

static int BUS_VME_DESCRIPTOR = 0;

enum class ApiMode : u8
{
    DspApi = 0x0,
    OpenApi = 0x1
};

class ViodSender : public QObject
{
    Q_OBJECT
protected:
    std::thread m_thread;
    std::atomic<bool> m_running{true};
public:
    explicit ViodSender(QObject *parent = nullptr) {}
    virtual ~ViodSender() {}
    virtual void sendViodMsg(std::vector<qword> &data,
                            const u16 smid,
                            const u16 spid,
                            const u16 slid,
                            const u16 dmid,
                            const u16 dpid,
                            const u16 dlid,
                            const u16 chid) = 0;
    virtual void rcvLoop() = 0;
};

#endif // DEFINESMIL_H
