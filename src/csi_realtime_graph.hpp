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
