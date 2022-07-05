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

#include <bitset>
#include <complex>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include <Packet.h>
#include <PayloadLayer.h>
#include <PcapFileDevice.h>
#include <UdpLayer.h>

#include "csi_reader_func.hpp"

namespace csirdr {

std::vector<int> zero_sub_20_ac = {0, 1, 2, 3, 32, 61, 62, 63, 11, 25, 39, 53};
std::vector<int> zero_sub_40_ac = {0,   1,   2,   3,   4,  5,  63, 64, 65, 123,
                                   124, 125, 126, 127, 11, 39, 53, 75, 89, 117};
std::vector<int> zero_sub_80_ac = {0,   1,   2,   3,   4,   5,   127, 128,
                                   129, 251, 252, 253, 254, 255, 25,  53,
                                   89,  117, 139, 167, 203, 231};
std::vector<int> zero_sub_20_ax = {0, 1, 2, 3, 11, 25, 39, 53, 32, 61, 62, 63};
std::vector<int> zero_sub_40_ax = {0,   1,   2,   3,   4,  5,  63, 64, 65, 123,
                                   124, 125, 126, 127, 11, 39, 53, 75, 89, 117};
std::vector<int> zero_sub_80_ax = {0,   1,   2,   3,   25,  53,  88,  117, 127,
                                   128, 129, 139, 168, 203, 231, 253, 254, 255};

csi_header get_csi_header(uint8_t *payload, bool new_header) {
  /*
   * UDPデータからCSIのヘッダ情報を読み取る関数
   * UDPパケットのペイロードを引数に，
   * ヘッダの構造体を返却
   * ヘッダがバージョンによって変わっているので，それに対応する必要がある
   */
  csi_header header;

  if (new_header) {
    // 新しいタイプのヘッダ
    for (int i = 4; i < 10; i++) {
      // MACアドレスはビックエンディアン
      header.tx_mac_add = (header.tx_mac_add << BITS_PER_BYTE) | payload[i];
    }
    for (int i = 10; i < 12; i++) {
      // シーケンス番号はリトルエンディアン？
      // 配布プログラムの結果と比較した結果
      header.seq_num = (header.seq_num << BITS_PER_BYTE) | payload[17 - i];
    }
    for (int i = 12; i < 14; i++) {
      // コア・ストリーム番号はビッグエンディアン
      // 新しいヘッダでも変わってなさそう
      header.core_stream_num =
          (header.core_stream_num << BITS_PER_BYTE) | payload[i];
    }
  } else {
    // 古いタイプのヘッダ
    for (int i = 4; i < 10; i++) {
      // MACアドレスはビックエンディアン
      header.tx_mac_add = (header.tx_mac_add << BITS_PER_BYTE) | payload[i];
    }
    for (int i = 10; i < 12; i++) {
      // シーケンス番号はリトルエンディアン？
      // 配布プログラムの結果と比較した結果
      header.seq_num = (header.seq_num << BITS_PER_BYTE) | payload[21 - i];
    }
    for (int i = 12; i < 14; i++) {
      // コア・ストリーム番号はビッグエンディアン
      header.core_stream_num =
          (header.core_stream_num << BITS_PER_BYTE) | payload[i];
    }
  }

  return header;
}

int cal_number_of_subcarrier(int data_len) {
  if ((data_len - 18) / 4 >= 256) {
    return 256;
  } else if ((data_len - CSI_HEADER_OFFSET) / 4 >= 128) {
    return 128;
  } else {
    return 64;
  }
}

csi_vec get_csi_from_packet_bcm4366c0(uint8_t *payload, int data_len,
                                      std::string wlan_std,
                                      bool rm_guard_pilot) {
  uint8_t *csi_data =
      payload +
      CSI_HEADER_OFFSET; //  UDPデータのうち，ヘッダを除いたCSIデータのポインタ

  int num_subcarrier = cal_number_of_subcarrier(data_len); // サブキャリア数

  uint32_t csi_data_unit = 0; // 4ByteのCSIデータを一時保存する

  // UDPペイロードから読み出したCSIデータの保存
  // 実数部，虚数部，指数部をサブキャリアの個数だけ格納する．
  std::vector<std::vector<int>> csi_data_extracted(num_subcarrier,
                                                   std::vector<int>(3, 0));

  // バイナリから，実部・虚部・指数部を取り出す部分
  // サブキャリア数で繰り返す
  for (int sub = 0; sub < num_subcarrier; sub++) {

    // CSI1要素が記録されている4バイトを読み出し
    // リトルエンディアンなのかビッグエンディアンなのか？
    for (int i = 0; i < BYTE_OF_CSI_DATA_UNIT; i++) {
      // リトルエンディアン
      csi_data_unit = (csi_data_unit << BITS_PER_BYTE) |
                      csi_data[(3 - i) + sub * BYTE_OF_CSI_DATA_UNIT];
    }

    // 専用の抽出関数に投げて出力を得る．
    // サブキャリア全体のデータを格納する
    csi_data_extracted[sub] = extract_csi_bcm4366c0(csi_data_unit);
  }

  // 実部・虚部に対して，指数部を反映させて出力を完成させる部分
  // 関数化して，のちに変更しやすくする
  // パケット単位でのデコード結果の出力
  // ファイルへの書き出しは別の関数で実行
  // 書き出しはパケット単位にしておく
  if (rm_guard_pilot) {
    return post_process_csi(cal_csi_bcm4366c0(csi_data_extracted), wlan_std);
  } else {
    return cal_csi_bcm4366c0(csi_data_extracted);
  }
}

std::vector<int> extract_csi_bcm4366c0(uint32_t csi_data_unit) {
  // 2つの読み出し方の説があるので両方に対応させる．

  // ビットマスク
  uint32_t mask_exponent_part = (1 << BITS_OF_EXPONENT_PART) - 1;
  uint32_t mask_numerics_part = (1 << BITS_OF_NUMERICS_PART) - 1;
  // シフト幅
  int shft_for_real_part = BITS_OF_EXPONENT_PART + BITS_OF_NUMERICS_PART + 1;
  int shft_for_imag_part = BITS_OF_EXPONENT_PART;
  int shft_for_sigh_real =
      BITS_OF_EXPONENT_PART + 2 * BITS_OF_NUMERICS_PART + 1;
  int shft_for_sigh_imag = BITS_OF_EXPONENT_PART + BITS_OF_NUMERICS_PART;

  // 数値の読み出し
  int real_part =
      (int)(csi_data_unit >> shft_for_real_part) & mask_numerics_part;
  int imag_part =
      (int)(csi_data_unit >> shft_for_imag_part) & mask_numerics_part;
  int exp_part = (int)csi_data_unit & mask_exponent_part;

  // 符号の処理
  // 実数部と虚数部の符号処理が怪しい
  if ((csi_data_unit >> shft_for_sigh_real) & 1U) {
    // 実数部
    real_part = -real_part;
  }
  if ((csi_data_unit >> shft_for_sigh_imag) & 1U) {
    // 虚数部
    imag_part = -imag_part;
  }
  if (exp_part > MAX_EXPONENT_PART) {
    // 指数部
    // 6ビット符号付整数の最大値は31(= MAX_EXPONENT_PART)
    // これを超える場合は64を減算することで2の補数となる
    exp_part = exp_part - (MAX_EXPONENT_PART + 1) * 2;
  }

  std::vector<int> ret = {real_part, imag_part, exp_part};
  return ret;
}

csi_vec cal_csi_bcm4366c0(std::vector<std::vector<int>> extracted_csi) {
  int num_subcarrier = (int)extracted_csi.size();
  csi_vec csi(num_subcarrier, std::complex<float>(0));

  for (int sub = 0; sub < num_subcarrier; sub++) {
    float real_part =
        float_element_bcm4366c0(extracted_csi[sub][0], extracted_csi[sub][2]);
    float imag_part =
        float_element_bcm4366c0(extracted_csi[sub][1], extracted_csi[sub][2]);

    csi[sub] = std::complex<float>(real_part, imag_part);
  }

  return csi;
}

float float_element_bcm4366c0(int x, int e) { return x * pow(2.0, e); }

csi_vec get_csi_from_packet_raspi(uint8_t *payload, int data_len,
                                  std::string wlan_std, bool rm_guard_pilot) {
  uint8_t *csi_data =
      payload +
      CSI_HEADER_OFFSET; //  UDPデータのうち，ヘッダを除いたCSIデータのポインタ

  int num_subcarrier = cal_number_of_subcarrier(data_len); // サブキャリア数

  uint32_t csi_data_unit = 0; // 4ByteのCSIデータを一時保存する

  // UDPペイロードから読み出したCSIデータの保存
  // 実部，虚部をサブキャリアの個数だけ格納する．
  csi_vec csi_data_extracted(num_subcarrier);

  // バイナリから，実部・虚部を取り出す部分
  // サブキャリア数で繰り返す
  for (int sub = 0; sub < num_subcarrier; sub++) {

    // CSI1要素が記録されている4バイトを読み出し
    // リトルエンディアンなのかビッグエンディアンなのか？
    for (int i = 0; i < BYTE_OF_CSI_DATA_UNIT; i++) {
      // リトルエンディアン
      csi_data_unit = (csi_data_unit << BITS_PER_BYTE) |
                      csi_data[(3 - i) + sub * BYTE_OF_CSI_DATA_UNIT];
    }

    // 上位16bitが実部，下位16bitが虚部
    // その両方が16ビット整数
    // todo: 正負の確認を実験データから行う
    int16_t real = (int16_t)((csi_data_unit >> 16) & 0x0000FFFF);
    int16_t imag = (int16_t)(csi_data_unit & 0x0000FFFF);
    csi_data_extracted[sub] = std::complex<float>(real, imag);
  }

  if (rm_guard_pilot) {
    return post_process_csi(csi_data_extracted, wlan_std);
  } else {
    return csi_data_extracted;
  }
}

csi_vec post_process_csi(csi_vec vec, std::string wlan_std) {
  // サブキャリア系列の前後半の入れ替え
  int n_sub = (int)vec.size();
  int n_sub_half = n_sub / 2;
  int idx_vec;
  csi_vec ret_vec(n_sub, std::complex<float>(0));

  for (int idx_ret_vec = 0; idx_ret_vec < n_sub; idx_ret_vec++) {
    // 周波数系列の前半後半を入れ替える
    idx_vec = (idx_ret_vec + n_sub_half) % n_sub;
    ret_vec[idx_ret_vec] = vec[idx_vec];
  }

  // ガードバンドとパイロットサブキャリアでの
  // CSIの要素を削除
  std::vector<int> zero_sub;
  if (n_sub == 64 and wlan_std == "ac") {
    zero_sub = zero_sub_20_ac;
  } else if (n_sub == 128 and wlan_std == "ac") {
    zero_sub = zero_sub_40_ac;
  } else if (n_sub == 256 and wlan_std == "ac") {
    zero_sub = zero_sub_80_ac;
  } else if (n_sub == 64 and wlan_std == "ax") {
    zero_sub = zero_sub_80_ax;
  } else if (n_sub == 128 and wlan_std == "ax") {
    zero_sub = zero_sub_80_ax;
  } else if (n_sub == 256 and wlan_std == "ax") {
    zero_sub = zero_sub_80_ax;
  }

  for (int i = 0; i < (int)zero_sub.size(); i++) {
    ret_vec[zero_sub[i]] = std::complex<float>(0., 0.);
  }
  return ret_vec;
}

void write_csi(std::ofstream &ofs, std::vector<csi_vec> csi, int n_tx, int n_rx,
               int mode, int label) {
  int n_sub = (int)csi[0].size();
  int e_idx;

  // 1行分の一時保存（いい方法が思いつかなかった）
  std::stringstream temp_ss;
  std::string temp_str;

  // データセット作成モード（mode==3）
  if (mode == 3) {
    temp_ss << label << ',';
  }

  for (int sub = 0; sub < n_sub; sub++) {
    for (int e = 0; e < n_tx * n_rx; e++) {
      e_idx = (e % n_rx) * n_rx + (e / n_tx); // 要素番号計算
      if (mode == 0) {
        temp_ss << std::noshowpos << "(" << csi[e_idx][sub].real()
                << std::showpos << csi[e_idx][sub].imag() << "j"
                << "),";
      } else if (mode == 1) {
        temp_ss << csi[e_idx][sub].real() << ',' << csi[e_idx][sub].imag()
                << std::endl;
      } else if (mode == 2) {
        temp_ss << std::abs(csi[e_idx][sub]) << ',' << std::arg(csi[e_idx][sub])
                << std::endl;
      } else if (mode == 3) {
        temp_ss << std::abs(csi[e_idx][sub]) << ',';
      } else {
        temp_ss << csi[e_idx][sub].real() << ',' << csi[e_idx][sub].imag()
                << ',';
      }
    }
  }

  // カンマ区切りで，最後のカンマを消す
  // よりよい方法があれば変更する．
  temp_str = temp_ss.str();
  temp_str.pop_back();

  ofs << temp_str << std::endl;
}
} // namespace csirdr
