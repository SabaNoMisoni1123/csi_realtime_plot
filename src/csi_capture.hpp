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
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <vector>

#include <Packet.h>
#include <PcapLiveDeviceList.h>

#include "csi_reader_func.hpp"

#ifndef CSI_CAPTURE
#define CSI_CAPTURE

namespace csirdr {
class Csi_capture {
protected:
  pcpp::PcapLiveDevice *dev; // アンテナデバイス

  bool new_header;      // ヘッダのバージョン
  std::string wlan_std; // 標準規格

  // パスやデバイス名など
  std::string device;     // CSI取得のデバイス
  std::string interface;  // インターフェイス名
  std::string target_mac; // 対象機器のMACアドレスの末尾4ケタ

  /*
   * CSIの行列サイズ
   */
  int n_tx;           // 送信アンテナ
  int n_rx;           // 受信アンテナ
  int n_csi_elements; // CSI行列の要素数

  /*
   * 取得CSIの一時保存
   */
  std::vector<csirdr::csi_vec> temp_csi;

  /*
   * 取得ヘッダの一時保存
   */
  csirdr::csi_header temp_header;

public:
  /*
   * コンストラクタ
   * ファイルパスの保存と，MACアドレスの保存を実行
   */
  Csi_capture();
  Csi_capture(std::string interface, std::string target_mac, int nrx, int ntx,
              bool new_header, std::string wlan_std);

  ~Csi_capture(); // ディストラクタ

  /*
   * パケットキャプチャ関数
   * キャプチャ，デコード，出力は一緒に実行
   */
  void capture_packet(uint32_t time = 10);

  /*
   * parsed packetからCSIを算出する関数
   */
  void load_packet(pcpp::Packet parsed_packet);

  /*
   * アプリケーションを提供する関数
   * 純粋仮想関数なので，継承したら必ずオーバーライド
   */
  virtual void csi_app() = 0;

  /*
   * 一時保存したCSIの要素数が最大なのか確認
   */
  bool is_full_temp_csi();

  /*
   * 一時保存したCSIのMACアドレスがターゲットと一致するのか判定
   */
  bool is_target_mac();

  /*
   * 一時保存CSIがビーコンフレームによるものか判定
   */
  bool is_from_beacon();

  /*
   * 一時保存したCSIデータのクリア
   */
  void clear_temp_csi();

  /*
   * 一時保存したCSIデータの出力
   */
  std::vector<csirdr::csi_vec> get_temp_csi();

  /*
   * 一時保存したCSIデータを1次元ベクトルとして出力
   */
  std::vector<float> get_temp_csi_series(std::string mode);

  /*
   * 一時保存したヘッダのMACアドレスを出力
   */
  std::string get_temp_mac_add();

  /*
   * ターゲットMACアドレス（末尾4桁）を出力
   */
  std::string get_target_mac_add() { return this->target_mac; };

  /*
   * CSIが格納されたパケットが取得されたときに呼び出される関数
   * アプリケーション関数はここで呼び出す
   * 継承したら，同じ名前の関数で書き直す
   */
  static void on_packet_arrives(pcpp::RawPacket *raw_packet,
                                pcpp::PcapLiveDevice *dev, void *cookie);

  /*
   * 一時保存CSIの要素数
   */
  inline int get_n_elements() {
    if (this->temp_csi.size() == 0) {
      return 0;
    } else {
      return this->temp_csi.size() * this->temp_csi[0].size();
    }
  }
};

} // namespace csirdr

#endif /* end of include guard */
