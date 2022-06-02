#include <Packet.h>
#include <PayloadLayer.h>
#include <PcapLiveDeviceList.h>
#include <SystemUtils.h>
#include <UdpLayer.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "csi_capture.hpp"
#include "csi_reader_func.hpp"

namespace csirdr {
Csi_capture::Csi_capture(std::string interface, std::string target_mac, int nrx,
                         int ntx, bool new_header, bool realtime_flag,
                         std::string output_dir, bool graph_flag) {
  // インターフェイス
  this->interface = interface;

  // ヘッダバージョン
  this->new_header = new_header;

  // 対象機器のMACアドレスの末尾4ケタ
  this->target_mac = target_mac;

  // アンテナ本数
  this->n_rx = nrx;
  this->n_tx = ntx;
  this->n_csi_elements = nrx * ntx;

  // 出力設定
  this->flag_realtime = realtime_flag;
  this->output_dir = std::filesystem::absolute(output_dir);

  // 保存パスの作成
  if (!std::filesystem::exists(this->output_dir)) {
    std::filesystem::create_directory(this->output_dir);
  }

  // グラフ出力
  this->flag_graph = graph_flag;
  if (graph_flag) {
    this->init_gnuplot();
  }

  // 設定の出力
  std::cout << "=========================================" << std::endl;
  std::cout << "Header version: " << (new_header ? "new" : "old") << std::endl
            << "interface: " << this->interface << std::endl
            << "n_tx: " << this->n_tx << std::endl
            << "n_rx: " << this->n_rx << std::endl
            << "n_csi_elements: " << this->n_csi_elements << std::endl
            << "out dir: " << this->output_dir.string() << std::endl
            << "mode: " << (realtime_flag ? "realtime" : "no realtime")
            << std::endl
            << "graph: " << (graph_flag ? "draw" : "no draw") << std::endl;
  std::cout << "=========================================" << std::endl;
}

Csi_capture::~Csi_capture() {
  if (this->flag_graph) {
    pclose(this->gnuplot);
  }
}

void Csi_capture::capture_packet(uint32_t time_sec) {
  // インターフェイスのオープン
  pcpp::PcapLiveDevice *dev =
      pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(
          this->interface);
  if (dev == NULL) {
    std::cerr << "Cannot find interface: " << this->interface << std::endl;
  }

  // インターフェイスの詳細情報
  std::cout << "Interface info:" << std::endl
            << "   Interface name:        " << dev->getName()
            << std::endl // get interface name
            << "   Interface description: " << dev->getDesc()
            << std::endl // get interface description
            << "   MAC address:           " << dev->getMacAddress()
            << std::endl // get interface MAC address
            << "   Default gateway:       " << dev->getDefaultGateway()
            << std::endl // get default gateway
            << "   Interface MTU:         " << dev->getMtu()
            << std::endl; // get interface MTU

  if (!dev->open()) {
    std::cerr << "Cannot open devie: " << this->interface << std::endl;
  }

  // キャプチャ開始
  dev->pcpp::PcapLiveDevice::startCapture(on_packet_arrives, this);

  // 測定時間sleep
  pcpp::multiPlatformSleep(time_sec);

  // キャプチャ終了
  dev->pcpp::PcapLiveDevice::stopCapture();
}

void Csi_capture::load_packet(pcpp::Packet parsed_packet) {
  pcpp::UdpLayer *udp_layer = parsed_packet.getLayerOfType<pcpp::UdpLayer>();
  uint8_t *payload = udp_layer->getLayerPayload();
  int data_len = udp_layer->getDataLen();

  // ヘッダーの保存
  this->temp_header = csirdr::get_csi_header(payload, this->new_header);

  // CSIをデコードして保存
  // Csi_captureはraspi専用
  this->temp_csi.push_back(
      csirdr::get_csi_from_packet_raspi(payload, data_len));
}

bool Csi_capture::is_full_temp_csi() {
  return ((int)this->temp_csi.size()) == this->n_csi_elements;
}

void Csi_capture::clear_temp_csi() {
  // 一時保存されているCSIデータの削除
  // 消去の仕方が悪いかもしれない
  this->temp_csi.clear();
}

std::vector<csirdr::csi_vec> Csi_capture::get_temp_csi() {
  // あえてゲッタを作るのは，ここでなんらかの処理を入れるかもしれないから
  return this->temp_csi;
}

void Csi_capture::write_temp_csi() {
  // ファイル名，ファイルストリーム
  std::filesystem::path csi_value_path = this->output_dir / "csi_value.csv";

  // ファイルストリームのオープンは初期化するのか追記するのかで分ける
  if (this->flag_realtime) {
    this->ofs_csi_value.open(csi_value_path.string());
  } else {
    this->ofs_csi_value.open(csi_value_path.string(), std::ios::app);
  }

  // 書き込み処理
  bool ret = this->write_csi_func();

  // clear temp_csi
  this->clear_temp_csi();

  // ファイルストリームのクローズ
  this->ofs_csi_value.close();

  // draw graph
  if (this->is_graph() and ret) {
    this->draw_graph();
  }
}

bool Csi_capture::write_csi_func() {
  // 書き込み関数は紀平さんのプログラムを参考にする
  // 順番はかなり大事
  // どうも前半と後半を入れ替える必要がある（20MHzではの話）
  int n_sub = (int)this->temp_csi[0].size();
  int e_idx;

  // 空データはスキップ
  if (n_sub == 0) {
    return false;
  }

  // 対象MACアドレスの確認
  std::stringstream macadd_tail_ss;
  macadd_tail_ss << std::hex << std::setw(4) << std::setfill('0')
                 << (this->temp_header.tx_mac_add & 0x0000FFFF);

  if (this->target_mac != "" and this->target_mac != macadd_tail_ss.str()) {
    return false;
  }

  for (int sub = 0; sub < n_sub; sub++) {
    for (int e = 0; e < this->n_csi_elements; e++) {
      e_idx = (e % this->n_rx) * this->n_rx + (e / this->n_tx); // 要素番号計算
      if (this->flag_realtime) {
        this->ofs_csi_value << this->temp_csi[e_idx][sub][0] << ','
                            << this->temp_csi[e_idx][sub][1] << std::endl;
      } else {
        this->ofs_csi_value << std::showpos << "("
                            << this->temp_csi[e_idx][sub][0]
                            << this->temp_csi[e_idx][sub][1] << "j"
                            << ")" << std::endl;
      }
    }
  }
  return true;
}

void Csi_capture::init_gnuplot() {
  // gnuplot shell open
  this->gnuplot = popen("gnuplot", "w");
  // gnuplot plot option setting
  fprintf(this->gnuplot, "set datafile separator \',\'\n");
  fprintf(this->gnuplot, "set terminal x11\n");
  fprintf(this->gnuplot, "set xrange [0:63]\n");
  fprintf(this->gnuplot, "set yrange [0:3000]\n");
  fprintf(this->gnuplot, "set style line 1 lw 5 lc \'blue\'\n");
  fprintf(this->gnuplot, "set nokey\n");
  fprintf(this->gnuplot, "set xlabel font\"*,20\"\n");
  fprintf(this->gnuplot, "set ylabel font\"*,20\"\n");
  fprintf(this->gnuplot, "set tics font\"*,20\"\n");
  fprintf(this->gnuplot, "set xlabel offset 0,0\n");
  fprintf(this->gnuplot, "set ylabel offset -2,0\n");
  fprintf(this->gnuplot, "set lmargin 12\n");
  fprintf(this->gnuplot, "set xlabel \"Subcarrier index\"\n");
  fprintf(this->gnuplot, "set ylabel \"CSI amplitude\"\n");
  fflush(this->gnuplot);
}

void Csi_capture::draw_graph() {
  std::filesystem::path temp_csi_path = this->output_dir / "csi_value.csv";
  std::stringstream gnuplot_cmd_ss;
  gnuplot_cmd_ss << "plot \"" << temp_csi_path.string()
                 << "\" using ($0):(sqrt($1**2 + $2**2)) ls 1 with lines"
                 << std::endl;
  fprintf(gnuplot, gnuplot_cmd_ss.str().c_str());
  fflush(gnuplot);
}

void on_packet_arrives(pcpp::RawPacket *raw_packet, pcpp::PcapLiveDevice *dev,
                       void *cookie) {
  Csi_capture *cap = (Csi_capture *)cookie;

  pcpp::Packet parsed_packet(raw_packet);
  if (!parsed_packet.isPacketOfType(pcpp::UDP))
    return;

  // デコードしてクラスのメンバ変数にCSIを一時保存
  cap->load_packet(parsed_packet);

  // デコードしてクラスのメンバ変数にCSIを一時保存
  if (cap->is_full_temp_csi()) {
    cap->write_temp_csi();
  }
}

} // namespace csirdr
