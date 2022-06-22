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
#include <filesystem>
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <vector>

#include "csi_reader_func.hpp"

#ifndef CSI_READER
#define CSI_READER

namespace csirdr {
class Csi_reader {

public:
  std::filesystem::path pcap_path;  // pcapファイルのパス
  std::filesystem::path output_dir; // 出力ディレクトリ
  std::string device;               // CSI取得デバイス

  /*
   * CSIの行列サイズの保存
   * CSIのMACアドレス読み込み時に記録しておく
   */
  int n_tx;           // 送信アンテナ
  int n_rx;           // 受信アンテナ
  int n_csi_elements; // CSI行列の要素数

  /*
   * コンストラクタ
   * ファイルパスの保存と，MACアドレスの保存を実行
   */
  Csi_reader(std::filesystem::path pcap_path, std::filesystem::path output_dir,
             std::string device, bool new_header, int ntx = 4, int nrx = 4,
             std::string wlan_std = "ac");
  ~Csi_reader(); // ディストラクタ

  /*
   * デコード実行関数
   * 初期指定，またはリストアップしたMACアドレスに従い複数のファイル出力を行う
   */
  void decode();

private:
  bool new_header;
  std::string wlan_std;
};
} // namespace csirdr

#endif /* end of include guard */
