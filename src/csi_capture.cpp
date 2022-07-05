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

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include <Packet.h>
#include <PayloadLayer.h>
#include <PcapLiveDeviceList.h>
#include <SystemUtils.h>
#include <UdpLayer.h>

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
}

Csi_capture::Csi_capture(std::string interface, std::string target_mac, int nrx,
                         int ntx, bool new_header, std::string wlan_std) {
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

  // インターフェイス
  this->dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(
      this->interface);
  if (this->dev == NULL) {
    std::cerr << "Cannot find interface: " << this->interface << std::endl;
  }

  // デバイスのオープン
  if (!this->dev->open()) {
    std::cerr << "Cannot open devie: " << this->interface << std::endl;
  }

  std::cout << "=========================================" << std::endl;
  std::cout << "Interface info:" << std::endl
            << "   Interface name:        " << this->dev->getName()
            << std::endl // get interface name
            << "   Interface description: " << this->dev->getDesc()
            << std::endl // get interface description
            << "   MAC address:           " << this->dev->getMacAddress()
            << std::endl // get interface MAC address
            << "   Default gateway:       " << this->dev->getDefaultGateway()
            << std::endl // get default gateway
            << "   Interface MTU:         " << this->dev->getMtu()
            << std::endl; // get interface MTU
  std::cout << "=========================================" << std::endl;
}

Csi_capture::~Csi_capture() {
  // デバイスのクローズ
  this->dev->close();
}

void Csi_capture::capture_packet(uint32_t time_sec) {
  // キャプチャ開始
  this->dev->pcpp::PcapLiveDevice::startCapture(this->on_packet_arrives, this);

  // 測定時間のsleep
  // この時間の処理は，キャプチャー時のコールバック関数で実装する
  pcpp::multiPlatformSleep(time_sec);

  // キャプチャ終了
  this->dev->pcpp::PcapLiveDevice::stopCapture();
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

  // アプリケーション
  cap->csi_app();
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
