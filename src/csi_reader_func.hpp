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

#include <complex>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <vector>

#ifndef CSI_READER_FUNC
#define CSI_READER_FUNC

#define BITS_PER_BYTE 8
#define CSI_HEADER_OFFSET 18
#define BYTE_OF_CSI_DATA_UNIT 4
#define BITS_OF_EXPONENT_PART 6
#define BITS_OF_NUMERICS_PART 11
#define MAX_EXPONENT_PART 31

namespace csirdr {

typedef struct {
  uint64_t tx_mac_add;
  uint16_t seq_num;
  uint16_t core_stream_num;
} csi_header;

/*
 * 1パケットからデコードされたCSIの型
 * (サブキャリア数，2)
 */
typedef std::vector<std::complex<float>> csi_vec;

/*
 * UDPのペイロードからCSI情報のヘッダ部分を読み取る関数
 * input: uint8_t* payload (= udp_layer->getLayerPayload())
 * return: header (csireader::csi_header)
 */
csi_header get_csi_header(uint8_t *payload, bool new_header = false);

/*
 * UDPのペイロードのデータ長（バイト）からCSIのサブキャリア数を計算する関数
 * input: int data_len (= udp_layer->getDataLen())
 * return: number of subcarrier (int)
 */
int cal_number_of_subcarrier(int data_len);

/*
 * bcm4366c0専用のUDPのペイロードからCSIを出力する関数
 * パケット単位のデコードを実現する
 * input: uint8_t *payload
 *        int data_len (= udp_layer->getDataLen())
 * return: csi_vec
 */
csi_vec get_csi_from_packet_bcm4366c0(uint8_t *payload, int data_len,
                                      std::string wlan_std,
                                      bool rm_guard_pilot = true);

/*
 * bcm4366c0専用のCSIデータ抽出関数
 * 4バイトのCSI１要素のデータから，実数部，虚数部，指数部を出力
 * input: uint_32 csi_data_unit
 * return: csi_element_vec std::vector<int_16>
 */
std::vector<int> extract_csi_bcm4366c0(uint32_t csi_data_unit);

/*
 * bcm4366c0専用のCSI計算関数
 * 実数部，虚数部，指数部の出力結果からCSIを計算
 * 浮動小数で記録されているはずだが，整数出力になっている
 * input: extracted_csi (std::vector<std::vector<int_fast16_t>>)
 * return: csi (std::vector<std::vector<int>>)
 */
csi_vec cal_csi_bcm4366c0(std::vector<std::vector<int>> extracted_csi);

/*
 * bcm4366c0専用のCSI計算関数の補助
 * 実数部・虚数部に対して指数部を反映させる関数
 * xをeだけシフトさせる
 * input: x(int), e(int)
 * return: x_shft(int)
 */
inline float float_element_bcm4366c0(int x, int e);

/*k
 * raspi専用のUDPのペイロードからCSIを出力する関数
 * パケット単位のデコードを実現する
 * input: uint8_t *payload
 *        int data_len (= udp_layer->getDataLen())
 * return: csi_vec
 */
csi_vec get_csi_from_packet_raspi(uint8_t *payload, int data_len,
                                  std::string wlan_std,
                                  bool rm_guard_pilot = true);

/*
 * サブキャリア系列のCSIデータの処理をする関数
 * 1. サブキャリア系列を前後半で入れ替える
 * 2. ガードバンドの値を0にする
 * input: csi_vec
 * return: csi_vec
 */
csi_vec post_process_csi(csi_vec vec, std::string wlan_std);

/*
 * 完全なCSIのサブキャリア系列をofstreamに出力
 * CSV形式で出力
 * いろいろな出力形式をここで実装
 * mode 0: numpy複素数に対応（サンプル数，サブキャリア数x要素数）
 * mode 1: 実部と虚部で2列（サンプル数xサブキャリア数x要素数x2，2）
 * mode 2: 大きさと偏角で2列（サンプル数xサブキャリア数x要素数x2，2）
 * mode 3: 行の先頭にラベル（サンプル数，サブキャリア数x要素数 + 1）
 */
void write_csi(std::ofstream &ofs, std::vector<csi_vec> csi, int n_tx, int n_rx,
               int mode = 0, int label = 0);
} // namespace csirdr

#endif /* end of include guard */
