#include <EthLayer.h>
#include <Packet.h>
#include <PayloadLayer.h>
#include <PcapFileDevice.h>
#include <UdpLayer.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "csi_capture.hpp"
#include "csi_reader_func.hpp"

namespace csirdr {
Csi_capture::Csi_capture(int nrx, int ntx, bool new_header, std::string device,
                         bool realtime_flag, std::string output_dir) {
  // ヘッダバージョン
  this->new_header = new_header;

  // デバイス
  this->device = device;

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

  // 設定の出力
  std::cout << "=========================================" << std::endl;
  std::cout << "Header version: " << (new_header ? "new" : "old") << std::endl
            << "n_tx: " << this->n_tx << std::endl
            << "n_rx: " << this->n_rx << std::endl
            << "n_csi_elements: " << this->n_csi_elements << std::endl
            << "out dir: " << this->output_dir.string() << std::endl;
  std::cout << "=========================================" << std::endl;
}

Csi_capture::~Csi_capture() { ; }

void Csi_capture::load_packet(pcpp::Packet parsed_packet) {
  pcpp::UdpLayer *udp_layer = parsed_packet.getLayerOfType<pcpp::UdpLayer>();
  uint8_t *payload = udp_layer->getLayerPayload();
  int data_len = udp_layer->getDataLen();

  // CSIをデコードして保存
  if (this->device == "asus") {
    this->temp_csi.push_back(
        csirdr::get_csi_from_packet_bcm4366c0(payload, data_len));
  } else if (this->device == "raspi") {
    this->temp_csi.push_back(
        csirdr::get_csi_from_packet_raspi(payload, data_len));
  }
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
  std::ofstream fs_csi_value;

  // ファイルストリームのオープンは初期化するのか追記するのかで分ける
  if (this->flag_realtime) {
    fs_csi_value.open(csi_value_path.string());
  } else {
    fs_csi_value.open(csi_value_path.string(), std::ios::app);
  }

  // 書き込み処理
  this->write_csi_func(fs_csi_value);

  // clear temp_csi
  this->clear_temp_csi();

  // ファイルストリームのクローズ
  fs_csi_value.close();
}

void Csi_capture::write_csi_func(std::ofstream &ofs) {
  // 書き込み関数は紀平さんのプログラムを参考にする
  // 順番はかなり大事
  // どうも前半と後半を入れ替える必要がある（20MHzではの話）
  int n_sub = (int)this->temp_csi[0].size();
  int n_sub_half = n_sub / 2;
  int sub_idx, e_idx;

  for (int sub = 0; sub < n_sub; sub++) {
    sub_idx = (sub + n_sub_half) % n_sub; // 前後半入れ替えの処理
    for (int e = 0; e < this->n_csi_elements; e++) {
      e_idx = (e % this->n_rx) * this->n_rx + (e / this->n_tx); // 要素番号計算
      if (this->flag_realtime) {
        ofs << this->temp_csi[e_idx][sub_idx][0] << ','
            << this->temp_csi[e_idx][sub_idx][1] << std::endl;
      } else {
        ofs << std::showpos << "(" << this->temp_csi[e_idx][sub_idx][0]
            << this->temp_csi[e_idx][sub_idx][1] << "j"
            << ")" << std::endl;
      }
    }
  }
}

} // namespace csirdr
