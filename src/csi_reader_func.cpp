#include <Packet.h>
#include <PayloadLayer.h>
#include <PcapFileDevice.h>
#include <UdpLayer.h>
#include <bitset>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <vector>

#include "csi_reader_func.hpp"

namespace csirdr {

csi_header get_csi_header(uint8_t *payload, bool new_header) {
  /*
   * UDPデータからCSIのヘッダ情報を読み取る関数
   * UDPパケットのペイロードを引数に，
   * ヘッダの構造体を返却
   * ヘッダがバージョンによって変わっているので，それに対応する必要がある
   */
  csi_header header = {0, 0, 0};

  if (new_header) {
    // 新しいタイプのヘッダ
    for (int i = 2; i < 8; i++) {
      // MACアドレスはビックエンディアン
      header.tx_mac_add = (header.tx_mac_add << BITS_PER_BYTE) | payload[i];
    }
    for (int i = 8; i < 10; i++) {
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

std::vector<std::vector<int>> get_csi_from_packet_bcm4366c0(uint8_t *payload,
                                                            int data_len) {
  uint8_t *csi_data =
      payload +
      CSI_HEADER_OFFSET; //  UDPデータのうち，ヘッダを除いたCSIデータのポインタ

  int num_subcarrier = cal_number_of_subcarrier(data_len); // サブキャリア数

  uint32_t csi_data_unit = 0; // 4ByteのCSIデータを一時保存する

  // UDPペイロードから読み出したCSIデータの保存
  // 実数部，虚数部，指数部をサブキャリアの個数だけ格納する．
  std::vector<std::vector<int>> csi_data_extracted(0, std::vector<int>(0, 3));

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
    csi_data_extracted.push_back(extract_csi_bcm4366c0(csi_data_unit));
  }

  // 実部・虚部に対して，指数部を反映させて出力を完成させる部分
  // 関数化して，のちに変更しやすくする
  // パケット単位でのデコード結果の出力
  // ファイルへの書き出しは別の関数で実行
  // 書き出しはパケット単位にしておく
  return cal_csi_bcm4366c0_int(csi_data_extracted);
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

std::vector<std::vector<int>>
cal_csi_bcm4366c0_int(std::vector<std::vector<int>> extracted_csi) {
  // nexmon csiが提供しているプログラムをそのまま使うだけ
  // 正しい処理ではないので，doubleで返却するような
  // 正しい処理の関数を将来的には作る必要がある．

  // 最大ビット数を記録
  // ありえない小さな数で初期化しているだけ
  int maxbit = -MAX_EXPONENT_PART - 1;
  int e_zero = -BITS_OF_NUMERICS_PART - 1;
  int e;
  uint32_t vi, vq;

  // maxbitを算出
  for (int i = 0; i < (int)extracted_csi.size(); i++) {

    vi = extracted_csi[i][0] < 0 ? -extracted_csi[i][0] : extracted_csi[i][0];

    vq = extracted_csi[i][1] < 0 ? -extracted_csi[i][1] : extracted_csi[i][1];
    e = extracted_csi[i][2];

    uint32_t x = vi | vq;
    uint32_t m = 0xffff0000, b = 0x0000ffff;
    int s = 16;

    // よくわからん処理だけど，nexmon csiからコピペ
    while (s > 0) {
      if (x & m) {
        e += s;
        x >>= s;
      }
      s >>= 1;
      m = (m >> s) & b;
      b >>= s;
    }
    if (e > maxbit) {
      maxbit = e;
    }
  }

  // CSI計算
  int shft = 10 - maxbit; // 謎定数
  std::vector<std::vector<int>> csi(0, std::vector<int>(0, 2));

  for (int i = 0; i < (int)extracted_csi.size(); i++) {
    e = extracted_csi[i][2] + shft;
    int real_part = extracted_csi[i][0];
    int imag_part = extracted_csi[i][1];

    // シフトの際は符号を外す
    // シフト用の別の関数を用意しておく
    if (e < e_zero) {
      real_part = 0;
      imag_part = 0;
    } else {
      real_part = shft_element_bcm4366c0(real_part, e);
      imag_part = shft_element_bcm4366c0(imag_part, e);
    }

    csi.push_back({real_part, imag_part});
  }

  return csi;
}

int shft_element_bcm4366c0(int x, int e) {
  // シフト計算の前に符号を外して，あとからつける
  int sgn = x >= 0 ? 1 : -1;
  x = sgn * x;

  if (e >= 0) {
    x = (x << e);
  } else {
    e = -e;
    x = (x >> e);
  }

  return sgn * x;
}

csi_vec get_csi_from_packet_raspi(uint8_t *payload, int data_len) {
  uint8_t *csi_data =
      payload +
      CSI_HEADER_OFFSET; //  UDPデータのうち，ヘッダを除いたCSIデータのポインタ

  int num_subcarrier = cal_number_of_subcarrier(data_len); // サブキャリア数

  uint32_t csi_data_unit = 0; // 4ByteのCSIデータを一時保存する

  // UDPペイロードから読み出したCSIデータの保存
  // 実部，虚部をサブキャリアの個数だけ格納する．
  std::vector<std::vector<int>> csi_data_extracted(0, std::vector<int>(0, 2));

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
    csi_data_extracted.push_back({real, imag});

    // std::cout << std::hex << std::setw(8) << std::setfill('0') << csi_data_unit
    //           << '\t' << std::dec << std::setw(8) << std::setfill(' ') << real
    //           << '\t' << std::dec << std::setw(8) << std::setfill(' ') << imag
    //           << std::endl;
  }

  return csi_data_extracted;
}
} // namespace csirdr
