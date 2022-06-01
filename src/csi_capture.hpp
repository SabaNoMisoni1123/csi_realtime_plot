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

namespace csirdr {
class Csi_capture {
public:
  std::filesystem::path output_dir; // 保存の指定をしていればこのディレクトリ
  std::string device;    // CSI取得のデバイス
  std::string interface; // インターフェイス名

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
  Csi_capture(std::string interface = "wlan0", int nrx = 1, int ntx = 1,
              bool new_header = true, bool realtime_flag = true,
              std::string output_dir = "out", bool graph_flag = false);

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

  // グラフのためのメンバ
  bool flag_graph;     // グラフ出力するのか
  FILE *gnuplot;       // gnuplot標準入力
  void init_gnuplot(); // gnuplot 初期設定
  void draw_graph();   // グラフプロット

  /*
   * 取得CSIの一時保存
   */
  std::vector<csirdr::csi_vec> temp_csi;

  /*
   * 一時保存CSIの書き出し関数
   * 少し面倒な処理なので関数化
   */
  void write_csi_func();

  /*
   * CSI書き出しのファイルストリーム
   */
  std::ofstream ofs_csi_value;
};

/*
 * パケットキャプチャコールバック関数
 * あえてクラスのメンバ関数にはしない
 */
void on_packet_arrives(pcpp::RawPacket *raw_packet, pcpp::PcapLiveDevice *dev,
                       void *cookie);

} // namespace csirdr

#endif /* end of include guard */
