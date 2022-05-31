#include <Packet.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <vector>

#include "csi_reader_func.hpp"

#ifndef CSI_CAPTURE
#define CSI_CAPTURE

namespace csirdr {
class Csi_capture {
public:
  std::filesystem::path output_dir; // 保存の指定をしていればこのディレクトリ
  std::string device; // CSI取得のデバイス

  /*
   * CSIの行列サイズの保存
   */
  int n_tx;           // 送信アンテナ
  int n_rx;           // 受信アンテナ
  int n_csi_elements; // CSI行列の要素数

  /*
   * コンストラクタ
   * ファイルパスの保存と，MACアドレスの保存を実行
   */
  Csi_capture(int nrx, int ntx, bool new_header, std::string device = "raspi",
              bool realtime_flag = true, std::string output_dir = "out");

  ~Csi_capture(); // ディストラクタ

  /*
   * parsed packetからCSIを算出する関数
   */
  void load_packet(pcpp::Packet parsed_packet);

  /*
   * 一時保存したCSIの要素数が最大なのか確認
   */
  bool is_full_temp_csi();

  /*
   * 一時保存したCSIデータのクリア
   */
  void clear_temp_csi();

  /*
   * 一時保存したCSIデータの出力
   */
  std::vector<csirdr::csi_vec> get_temp_csi();

  /*
   * 一時保存したCSIデータの書き出し
   */
  void write_temp_csi();

private:
  bool new_header;    // ヘッダのバージョン
  bool flag_realtime; // 累積させるのか，1つだけなのか

  /*
   * 取得CSIの一時保存
   */
  std::vector<csirdr::csi_vec> temp_csi;

  /*
   * 一時保存CSIの書き出し関数
   * 少し面倒な処理なので関数化
   */
  void write_csi_func(std::ofstream &ofs);
};
} // namespace csirdr

#endif /* end of include guard */
