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

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include <Packet.h>
#include <PayloadLayer.h>
#include <PcapLiveDeviceList.h>
#include <SystemUtils.h>
#include <UdpLayer.h>

#include "csi_capture.hpp"
#include "csi_reader_func.hpp"
#include "csi_realtime_graph.hpp"

namespace csirdr {
Csi_plot::Csi_plot(std::string target_mac, int nrx, int ntx, bool new_header,
                   std::string wlan_std, int skip)
    : Csi_capture("wlan0", target_mac, nrx, ntx, new_header, wlan_std) {
  this->skip = skip;
  this->gnuplot = popen("gnuplot", "w");
}

Csi_plot::~Csi_plot() { pclose(this->gnuplot); }

void Csi_plot::set_graph_opt(int top, int n_sub, std::string graph_type) {
  // gnuplot plot option setting
  fprintf(this->gnuplot, "set style line 1 lw 5 lc \'blue\'\n");
  fprintf(this->gnuplot, "set datafile separator \'\t\'\n");
  fprintf(this->gnuplot, "set terminal x11 size 1280, 960\n");
  fprintf(this->gnuplot, "set nokey\n");
  fprintf(this->gnuplot, "set xlabel font\"*,20\"\n");
  fprintf(this->gnuplot, "set ylabel font\"*,20\"\n");
  fprintf(this->gnuplot, "set tics font\"*,20\"\n");
  fprintf(this->gnuplot, "set xlabel offset 0,0\n");
  fprintf(this->gnuplot, "set ylabel offset -2,0\n");
  fprintf(this->gnuplot, "set lmargin 12\n");
  fprintf(this->gnuplot, "set xlabel \"Subcarrier index\"\n");

  if (graph_type == "abs") {
    fprintf(this->gnuplot, "set ylabel \"CSI amplitude\"\n");
    fprintf(this->gnuplot, "set xrange [0:%d]\n", n_sub);
    fprintf(this->gnuplot, "set yrange [0:%d]\n", top);
    this->graph_type = "amplitude";
  } else if (graph_type == "arg") {
    fprintf(this->gnuplot, "set ylabel \"CSI phase\"\n");
    fprintf(this->gnuplot, "set xrange [0:%d]\n", n_sub);
    fprintf(this->gnuplot, "set yrange [-pi:pi]\n");
    this->graph_type = "phase";
  }
  fflush(this->gnuplot);
}

bool Csi_plot::is_skip() {
  if (this->skip == 0) {
    return false;
  }

  this->graph_counter = (this->graph_counter + 1) % (this->skip + 1);
  return !(this->graph_counter == 1);
}

void Csi_plot::csi_app() {
  // スキップ処理
  if (this->is_skip()) {
    this->clear_temp_csi();
    return;
  }

  // MACアドレスが異なれば終了
  if (!this->is_target_mac()) {
    this->clear_temp_csi();
    return;
  }

  // CSIの数が足りてなければ終了
  if (!this->is_full_temp_csi()) {
    return;
  }

  // ビーコンフレームなら終了
  if (this->is_from_beacon()) {
    this->clear_temp_csi();
    return;
  }

  // グラフに図示するデータを取得
  std::vector<float> data = this->get_temp_csi_series(this->get_graph_type());

  // gnuplotで処理
  fprintf(this->gnuplot, "plot \'-\' ls 1 with lines\n");
  for (int i = 0; i < (int)data.size(); i++) {
    fprintf(this->gnuplot, "%d\t%f\n", i, data[i]);
  }
  fprintf(this->gnuplot, "e\n");
  fflush(this->gnuplot);

  this->clear_temp_csi();
}

void Csi_plot::on_packet_arrives(pcpp::RawPacket *raw_packet,
                                 pcpp::PcapLiveDevice *dev, void *cookie){
  pcpp::Packet parsed_packet(raw_packet);
  if (!parsed_packet.isPacketOfType(pcpp::UDP))
    return;

  Csi_plot *cap = (Csi_plot *)cookie;

  // デコードしてクラスのメンバ変数にCSIを一時保存
  cap->load_packet(parsed_packet);

  // MACアドレスの表示
  std::cout << "\rtarget MAC address: " << cap->get_target_mac_add()
            << ", this CSI MAC address: " << cap->get_temp_mac_add();

  // アプリケーション
  cap->csi_app();
}

} // namespace csirdr
