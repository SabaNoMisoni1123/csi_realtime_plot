#include <Packet.h>
#include <PcapLiveDeviceList.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <vector>

#include "csi_capture.hpp"
#include "csi_reader_func.hpp"

#ifndef CSI_REALTIME_PLOT
#define CSI_REALTIME_PLOT

namespace csirdr {
class Csi_plot : public Csi_capture {
private:
  // gnuplotのパイプ
  FILE *gnuplot;

  std::string graph_type;

public:
  /*
   * コンストラクタ
   * ファイルパスの保存と，MACアドレスの保存を実行
   */
  Csi_plot(std::string target_mac, int nrx = 1, int ntx = 1,
           bool new_header = true, std::string wlan_std = "11ac",
           std::string output_dir = "out");

  ~Csi_plot(); // ディストラクタ

  /*
   * 運用段階の関数
   */
  void run_graph(uint32_t time_sec, int top, int num_sub,
                 std::string graph_type);

  /*
   * 運用段階のコールバック関数
   */
  static void on_packet_arrives_gnuplot(pcpp::RawPacket *raw_packet,
                                        pcpp::PcapLiveDevice *dev,
                                        void *cookie);

  /*
   * グラフタイプの出力
   */
  std::string get_graph_type() { return this->graph_type; }
};
} // namespace csirdr

#endif /* end of include guard */
