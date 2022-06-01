#include <algorithm>
#include <cmdline.h>
#include <csi_capture.hpp>
#include <csi_reader_func.hpp>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  // コマンドライン引数
  cmdline::parser ps;
  ps.add<int>("time", 't', "time (second)", true);
  ps.add<std::string>("temp-dir", 'd', "temp dir", false, "");
  ps.add<std::string>("macadd", 'm', "target MAC address", false, "");
  ps.parse_check(argc, argv);

  // 一時保存ディレクトリ
  std::filesystem::path temp_data_dir;
  if (ps.exist("temp-dir")) {
    temp_data_dir = std::filesystem::absolute(ps.get<std::string>("temp-dir"));
  } else {
    std::filesystem::path home_dir = std::getenv("HOME");
    temp_data_dir = home_dir / "temp";
  }

  // 対象MACアドレス
  std::string target_mac = ps.get<std::string>("macadd");
  std::transform(target_mac.begin(), target_mac.end(), target_mac.begin(),
                 tolower);

  csirdr::Csi_capture cap("wlan0", target_mac, 1, 1, true, true,
                          temp_data_dir.string(), true);

  cap.capture_packet(ps.get<int>("time"));
  std::cout << "DONE" << std::endl;

  return 0;
}
