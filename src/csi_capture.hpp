#include <Packet.h>
#include <PcapLiveDeviceList.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <vector>

#include "csi_reader_func.hpp"

#ifndef CSI_CAPTURE
#define CSI_CAPTURE

#define N_ROTATE 1024

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

  // グラフのためのメンバ
  // bool flag_graph;        // グラフ出力するのか
  // FILE *gnuplot;          // gnuplot標準入力
  // void draw_graph();      // グラフプロット
  // std::string graph_type; // グラフにプロットするもの
  // int n_file = 0; // ファイル番号

  // ファイル保存のためのメンバ
  std::filesystem::path dir_output; // 保存の指定をしていればこのディレクトリ
  std::filesystem::path path_csi_value;
  std::filesystem::path path_csi_seq;
  int save_mode;
  int label = 0;
  int n_file = 0;

  /*
   * CSIヘッダ情報書き出しのファイルストリーム
   */
  std::ofstream ofs_csi_seq;

  /*
   * CSIデータ書き出しのファイルストリーム
   */
  std::ofstream ofs_csi_value;

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
              bool new_header, std::string wlan_std, std::string output_dir);

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
   * 一時保存したCSIデータの書き出し
   */
  void write_temp_csi(int save_mode, int label = 0, bool continuous = true);

  /*
   * 一時保存したヘッダのMACアドレスを出力
   */
  std::string get_temp_mac_add(); 

  /*
   * ターゲットMACアドレス（末尾4桁）を出力
   */
  std::string get_target_mac_add() { return this->target_mac; };

  /*
   * パケットキャプチャコールバック関数
   * 静的メンバ関数
   */
  static void on_packet_arrives(pcpp::RawPacket *raw_packet,
                                pcpp::PcapLiveDevice *dev, void *cookie);

  /*
   * CSIデータの保存モードの出力
   */
  int csi_save_mode() { return this->save_mode; }

  /*
   * CSIデータ保存時のラベルの出力
   */
  int csi_label() { return this->label; }

  /*
   * CSIデータの保存モードを設定
   */
  void set_save_mode(int mode, int label = 0);

  /*
   * csi_valueのパス
   */
  const char *csi_value_path() { return this->path_csi_value.c_str(); }
};

} // namespace csirdr

#endif /* end of include guard */
