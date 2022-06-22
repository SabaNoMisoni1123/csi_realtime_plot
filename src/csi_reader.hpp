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
