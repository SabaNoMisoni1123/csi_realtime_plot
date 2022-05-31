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

#include "csi_reader.hpp"
#include "csi_reader_func.hpp"

namespace csirdr {

Csi_reader::Csi_reader(std::filesystem::path pcap_path,
                       std::filesystem::path output_dir, std::string device,
                       bool new_header, int ntx, int nrx) {
  // パスの保存
  this->pcap_path = pcap_path;
  this->output_dir = output_dir;

  // デバイスの設定
  this->device = device;

  // ヘッダのバージョン
  this->new_header = new_header;

  // 保存パスの作成
  if (!std::filesystem::exists(this->output_dir)) {
    std::filesystem::create_directory(this->output_dir);
  }

  // アンテナ本数
  this->n_rx = nrx;
  this->n_tx = ntx;

  // 行列要素数
  this->n_csi_elements = this->n_rx * this->n_tx;

  // 設定の出力
  std::cout << "=========================================" << std::endl;
  std::cout << "Header version: " << (new_header ? "new" : "old") << std::endl
            << "n_tx: " << this->n_tx << std::endl
            << "n_rx: " << this->n_rx << std::endl
            << "n_csi_elements: " << this->n_csi_elements << std::endl
            << "pcap file name: " << this->pcap_path.string() << std::endl
            << "out dir: " << this->output_dir.string() << std::endl;
  std::cout << "=========================================" << std::endl;
}

Csi_reader::~Csi_reader() { ; }

void Csi_reader::decode() {
  // 保存用テキストファイルの作成
  // - CSIデータ
  // - シーケンス番号などの雑多データ
  std::filesystem::path csi_value_path = this->output_dir / "csi_value.csv";
  std::filesystem::path csi_seq_path = this->output_dir / "csi_seq.csv";

  // 出力ファイルストリーム
  std::ofstream fs_csi_value;
  std::ofstream fs_csi_seq;

  // 出力ファイルのオープン
  fs_csi_value.open(csi_value_path.string());
  fs_csi_seq.open(csi_seq_path.string());
  fs_csi_seq << "macadd"
             << ","
             << "seq"
             << ","
             << "subseq"
             << ","
             << "timestamp" << std::endl;

  // デコードの実行・出力
  pcpp::IFileReaderDevice *reader =
      pcpp::IFileReaderDevice::getReader(this->pcap_path);
  pcpp::RawPacket raw_packet;
  reader->open();

  std::stringstream temp_seq;            // 出力データの一時保存
  std::vector<csirdr::csi_vec> temp_csi; // 出力データの一時保存
  uint32_t target_mac_add = 0xFFFFFFFF;  // APのMACアドレス

  while (reader->getNextPacket(raw_packet)) {
    // フレーム解析
    pcpp::Packet packet(&raw_packet);
    pcpp::UdpLayer *udp_layer = packet.getLayerOfType<pcpp::UdpLayer>();
    uint8_t *payload = udp_layer->getLayerPayload();
    int data_len = udp_layer->getDataLen();
    csirdr::csi_header header = csirdr::get_csi_header(payload);

    // 送受信アンテナが0,0の場合は，temp_*を書き込むか消去するか
    // 書き込んだ後はtarget_mac_addの更新
    if (header.core_stream_num == 0) {
      // 書き込むか，消去するかの分岐
      if ((target_mac_add != 0xFFFFFFFF) and
          ((int)temp_csi.size() == this->n_csi_elements)) {
        // 書き込み処理
        fs_csi_seq << temp_seq.str() << std::endl;
        this->write_csi_data(fs_csi_value, temp_csi);
      }

      // temp_*のクリア
      temp_csi.clear();
      temp_seq.str("");
      temp_seq.clear(std::stringstream::goodbit);

      // target_mac_addの更新
      target_mac_add = header.tx_mac_add;

      // シーケンス番号などのデータはこのタイミングで取得
      temp_seq << std::hex << std::setw(4) << std::setfill('0')
               << (target_mac_add & 0x0000FFFF) << "," << std::dec
               << header.seq_num / 16 << "," << header.seq_num % 16 << ","
               << raw_packet.getPacketTimeStamp().tv_sec << "."
               << raw_packet.getPacketTimeStamp().tv_nsec;
    }

    // CSIデータの読み込み
    // asusやraspiの分岐はここで行う
    if (this->device == "asus") {
      temp_csi.push_back(
          csirdr::get_csi_from_packet_bcm4366c0(payload, data_len));
    } else if (this->device == "raspi") {
      temp_csi.push_back(csirdr::get_csi_from_packet_raspi(payload, data_len));
    }
  }

  reader->close();

  // 出力ファイルのクローズ
  fs_csi_seq.close();
  fs_csi_value.close();
}

void Csi_reader::write_csi_data(std::ofstream &ofs,
                                std::vector<csirdr::csi_vec> csi) {
  // 書き込み関数は紀平さんのプログラムを参考にする
  // 順番はかなり大事
  // どうも前半と後半を入れ替える必要がある（20MHzではの話）
  int n_sub = (int)csi[0].size();
  int n_sub_half = n_sub / 2;
  int sub_idx, e_idx;

  for (int sub = 0; sub < n_sub; sub++) {
    sub_idx = (sub + n_sub_half) % n_sub; // 前後半入れ替えの処理
    for (int e = 0; e < this->n_csi_elements; e++) {
      e_idx = (e % this->n_rx) * this->n_rx + (e / this->n_tx); // 要素番号計算
      ofs << std::showpos << "(" << csi[e_idx][sub_idx][0]
          << csi[e_idx][sub_idx][1] << "j"
          << ")" << std::endl;
    }
  }
}

} // namespace csirdr
