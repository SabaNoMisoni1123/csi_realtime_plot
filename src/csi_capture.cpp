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
Csi_capture::Csi_capture() {
  this->interface = "wlan0";
  this->target_mac = "";
  this->n_rx = 1;
  this->n_tx = 1;
  this->new_header = true;
  this->wlan_std = "ac";
  this->dir_output = std::filesystem::absolute("out");
}
Csi_capture::Csi_capture(std::string interface, std::string target_mac, int nrx,
                         int ntx, bool new_header, std::string wlan_std,
                         std::string output_dir) {
  // インターフェイス
  this->interface = interface;

  // ヘッダバージョン
  this->new_header = new_header;

  // 標準規格
  this->wlan_std = wlan_std;

  // 対象機器のMACアドレスの末尾4ケタ
  this->target_mac = target_mac;

  // アンテナ本数
  this->n_rx = nrx;
  this->n_tx = ntx;
  this->n_csi_elements = nrx * ntx;

  // 出力設定
  this->dir_output = std::filesystem::absolute(output_dir);
  this->save_mode = 0;

  // 保存パスの作成
  if (!std::filesystem::exists(this->dir_output)) {
    std::filesystem::create_directory(this->dir_output);
  }
  this->path_csi_value = this->dir_output / "csi_value.csv";
  this->path_csi_seq = this->dir_output / "csi_seq.csv";

  // 設定の出力
  std::cout << "=========================================" << std::endl;
  std::cout << "Header version: " << (new_header ? "new" : "old") << std::endl
            << "interface: " << this->interface << std::endl
            << "wlan standard: " << this->wlan_std << std::endl
            << "n_tx: " << this->n_tx << std::endl
            << "n_rx: " << this->n_rx << std::endl
            << "n_csi_elements: " << this->n_csi_elements << std::endl
            << "out dir: " << this->dir_output.string() << std::endl
            << "target mac: " << this->target_mac << std::endl
            << std::endl;
  std::cout << "=========================================" << std::endl;

  // インターフェイス
  this->dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(
      this->interface);
  if (this->dev == NULL) {
    std::cerr << "Cannot find interface: " << this->interface << std::endl;
  }
  std::cout << "=========================================" << std::endl;
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
  std::cout << "=========================================" << std::endl;
}

Csi_capture::~Csi_capture() { ; }

void Csi_capture::capture_packet(uint32_t time_sec) {
  // デバイスのオープン
  if (!this->dev->open()) {
    std::cerr << "Cannot open devie: " << this->interface << std::endl;
  }

  // キャプチャ開始
  this->dev->pcpp::PcapLiveDevice::startCapture(this->on_packet_arrives, this);

  // 測定時間のsleep
  // この時間の処理は，キャプチャー時のコールバック関数で実装する
  pcpp::multiPlatformSleep(time_sec);

  // キャプチャ終了
  this->dev->pcpp::PcapLiveDevice::stopCapture();

  // デバイスのクローズ
  this->dev->close();
}

void Csi_capture::on_packet_arrives(pcpp::RawPacket *raw_packet,
                                    pcpp::PcapLiveDevice *dev, void *cookie) {
  pcpp::Packet parsed_packet(raw_packet);
  if (!parsed_packet.isPacketOfType(pcpp::UDP))
    return;

  Csi_capture *cap = (Csi_capture *)cookie;

  // デコードしてクラスのメンバ変数にCSIを一時保存
  cap->load_packet(parsed_packet);

  // MACアドレスの表示
  std::cout << "\rtarget MAC address: " << cap->get_target_mac_add()
            << ", this CSI MAC address: " << cap->get_temp_mac_add();

  // CSIの数が足りてなければ終了
  if (!cap->is_full_temp_csi()) {
    return;
  }

  // MACアドレスが異なれば終了
  if (!cap->is_target_mac()) {
    cap->clear_temp_csi();
    return;
  }

  // デコードしてクラスのメンバ変数にCSIを一時保存
  if (cap->is_full_temp_csi()) {
    cap->write_temp_csi(cap->csi_save_mode(), cap->csi_label());
    cap->clear_temp_csi();
  }
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
      csirdr::get_csi_from_packet_raspi(payload, data_len, this->wlan_std));
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

std::vector<float> Csi_capture::get_temp_csi_series(std::string mode) {
  int n_sub = (int)this->temp_csi[0].size();
  int n_csi_elements = (int)this->temp_csi.size();
  int n_rx = this->n_rx;
  int n_tx = this->n_tx;
  int e_idx;
  std::vector<float> csi_series(n_sub * n_csi_elements, 0.0);

  for (int sub = 0; sub < n_sub; sub++) {
    for (int e = 0; e < n_csi_elements; e++) {
      e_idx = (e % n_rx) * n_rx + (e / n_tx); // 要素番号計算
      if (mode == "amplitude") {
        csi_series[e + n_csi_elements * sub] =
            std::abs(this->temp_csi[e_idx][sub]);
      } else if (mode == "phase") {
        csi_series[e + n_csi_elements * sub] =
            std::arg(this->temp_csi[e_idx][sub]);
      }
    }
  }

  return csi_series;
}

void Csi_capture::write_temp_csi(int save_mode, int label, bool continuous) {
  if (continuous) {
    this->ofs_csi_value.open(path_csi_value.string(), std::ios::app);
  } else {
    std::stringstream basename_value_ss;
    basename_value_ss << "csi_value_" << std::setw(4) << std::setfill('0')
                      << this->n_file << ".csv";
    this->n_file = (this->n_file + 1) % N_ROTATE;
    this->path_csi_value = this->dir_output / basename_value_ss.str();
    this->ofs_csi_value.open(path_csi_value.string());
  }
  this->ofs_csi_seq.open(path_csi_seq.string(), std::ios::app);

  // 書き込み処理
  csirdr::write_csi(this->ofs_csi_value, this->temp_csi, this->n_tx, this->n_rx,
                    save_mode, label);

  this->ofs_csi_seq << std::hex << std::setw(4) << std::setfill('0')
                    << (this->temp_header.tx_mac_add & 0x0000FFFF) << ","
                    << std::dec << this->temp_header.seq_num / 16 << ","
                    << this->temp_header.seq_num % 16 << std::endl;

  // ファイルストリームのクローズ
  this->ofs_csi_value.close();
  this->ofs_csi_seq.close();
}

void Csi_capture::set_save_mode(int mode, int label) {
  this->save_mode = mode;
  this->label = label;
  if (mode == 3) {
    this->path_csi_value = this->dir_output / "dataset.csv";
  }
}

std::string Csi_capture::get_temp_mac_add() {
  std::stringstream ss;
  ss << std::hex << std::setw(4) << std::setfill('0')
     << (this->temp_header.tx_mac_add & 0x0000FFFF);

  return ss.str();
}

bool Csi_capture::is_target_mac() {
  if (this->target_mac == "") {
    return true;
  }
  return this->get_temp_mac_add() == this->target_mac;
}

bool Csi_capture::is_from_beacon() {
  if (this->temp_csi.size() == 0) {
    return false;
  }

  int cnt = 0;
  int n_sub = this->temp_csi[0].size();
  int th = 150;

  for (int i = 0; i < n_sub; i++) {
    if (std::abs(std::complex<float>(temp_csi[0][i].real(),
                                     temp_csi[0][i].imag())) < th) {
      cnt++;
    }
  }

  return cnt > (n_sub / 2);
}
} // namespace csirdr
