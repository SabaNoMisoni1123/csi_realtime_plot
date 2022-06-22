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
  ps.add<std::string>("temp-dir", 'd', "temp dir", false, "");
  ps.add<std::string>("macadd", 'm', "target MAC address", false, "");
  ps.add<int>("num-sub", 'n', "Number of subcarrier for graph plot", false,
              256);
  ps.add<int>("height", 'h', "Max value of graph's y axis", false, 3000);
  ps.add<std::string>("wlan-std", 's', "wlan standard [\'ac\', \'ax\']",
                      false, "ac");
  ps.parse_check(argc, argv);

  // 一時保存ディレクトリ
  std::filesystem::path temp_data_dir;
  if (ps.exist("temp-dir")) {
    temp_data_dir = std::filesystem::absolute(ps.get<std::string>("temp-dir"));
  } else {
    temp_data_dir = std::filesystem::absolute("temp");
  }

  // 対象MACアドレス
  std::string target_mac = ps.get<std::string>("macadd");
  std::transform(target_mac.begin(), target_mac.end(), target_mac.begin(),
                 tolower);

  csirdr::Csi_plot cap(target_mac, 1, 1, true, ps.get<std::string>("wlan-std"),
                       temp_data_dir.string());

  cap.run_graph(ps.get<int>("time"), ps.get<int>("height"),
                ps.get<int>("num-sub"), "abs");
  std::cout << "\n\n\nDONE" << std::endl;

  return 0;
}
