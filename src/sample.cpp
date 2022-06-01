#include <Packet.h>
#include <PcapFileDevice.h>
#include <chrono>
#include <cmdline.h>
#include <csi_reader_func.hpp>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <thread>

#include "csi_capture.hpp"

int main(int argc, char *argv[]) {
  std::filesystem::path home_dir = std::getenv("HOME");
  std::filesystem::path root_dir =
      std::filesystem::current_path().parent_path();
  std::filesystem::path test_dir = root_dir / "test";
  std::filesystem::path pcap_path = test_dir / "test_raspi1.pcap";
  std::filesystem::path temp_data_dir = home_dir / "temp";

  // CSI capture instance
  csirdr::Csi_capture cap("wlan0", 1, 1, true, true, temp_data_dir.string(),
                          true);
}
