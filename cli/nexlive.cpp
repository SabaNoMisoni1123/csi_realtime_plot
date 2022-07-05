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
* Neither the name of the copyright holder nor the names of its contributors
  may be used to endorse or promote products derived from this software
  without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cmdline.h>
#include <filesystem>
#include <iostream>
#include <string>

#include <csi_capture.hpp>
#include <csi_reader_func.hpp>
#include <csi_realtime_graph.hpp>

int main(int argc, char *argv[]) {
  // コマンドライン引数
  cmdline::parser ps;
  ps.add<int>("time", 't', "time (second)", true);
  ps.add<std::string>("macadd", 'm', "target MAC address", false, "");
  ps.add<std::string>("data", '\0', "graph data [\'abs\', \'arg\']", false,
                      "abs");
  ps.add<int>("num-sub", 'n', "Number of subcarrier for graph plot", false,
              256);
  ps.add<int>("height", 'h', "Max value of graph's y axis", false, 3000);
  ps.add<int>("skip", '\0', "number of CSIs to skip", false, 0);
  ps.add<std::string>("wlan-std", 's', "wlan standard [\'ac\', \'ax\']", false,
                      "ac");
  ps.parse_check(argc, argv);

  // 対象MACアドレス
  std::string target_mac = ps.get<std::string>("macadd");
  std::transform(target_mac.begin(), target_mac.end(), target_mac.begin(),
                 tolower);

  csirdr::Csi_plot cap(target_mac, 1, 1, true, ps.get<std::string>("wlan-std"),
                       ps.get<int>("skip"));

  cap.set_graph_opt(ps.get<int>("height"), ps.get<int>("num-sub"),
                    ps.get<std::string>("data"));

  cap.capture_packet(ps.get<int>("time"));

  std::cout << "\n\n\nDONE" << std::endl;

  return 0;
}
