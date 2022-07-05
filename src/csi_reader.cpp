/*
  Copyright (c) 2022, Sota Kondo
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of the <organization> nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <Packet.h>
#include <PayloadLayer.h>
#include <PcapFileDevice.h>
#include <UdpLayer.h>

#include "csi_reader.hpp"
#include "csi_reader_func.hpp"

namespace csirdr {

Csi_reader::Csi_reader(std::filesystem::path pcap_path,
                       std::filesystem::path output_dir, std::string device,
                       bool new_header, int ntx, int nrx,
                       std::string wlan_std) {
  // パスの保存
  this->pcap_path = pcap_path;
  this->output_dir = output_dir;

  // デバイスの設定
  this->device = device;

  // ヘッダのバージョン
  this->new_header = new_header;

  // 標準規格のバージョン
  this->wlan_std = wlan_std;

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
            << "wlan standard: " << this->wlan_std << std::endl
            << "n_tx: " << this->n_tx << std::endl
            << "n_rx: " << this->n_rx << std::endl
            << "n_csi_elements: " << this->n_csi_elements << std::endl
            << "pcap file name: " << this->pcap_path.string() << std::endl
            << "out dir: " << this->output_dir.string() << std::endl;
  std::cout << "=========================================" << std::endl;
}

Csi_reader::~Csi_reader() { ; }

void Csi_reader::decode(bool rm_guard_pilot) {
  // 保存用テキストファイルの作成
  // - CSIデータ
  // - シーケンス番号などの雑多データ
  std::filesystem::path csi_value_path = this->output_dir / "csi_value.csv";
  std::filesystem::path csi_seq_path = this->output_dir / "csi_seq.csv";

  // 出力ファイル名
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
        csirdr::write_csi(fs_csi_value, temp_csi, this->n_tx, this->n_rx);
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
      temp_csi.push_back(csirdr::get_csi_from_packet_bcm4366c0(
          payload, data_len, this->wlan_std, rm_guard_pilot));
    } else if (this->device == "raspi") {
      temp_csi.push_back(csirdr::get_csi_from_packet_raspi(
          payload, data_len, this->wlan_std, rm_guard_pilot));
    }
  }

  reader->close();

  // 出力ファイルのクローズ
  fs_csi_seq.close();
  fs_csi_value.close();
}
} // namespace csirdr
