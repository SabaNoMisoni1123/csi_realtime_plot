#include <Packet.h>
#include <PcapFileDevice.h>
#include <chrono>
#include <cmdline.h>
#include <csi_reader_func.hpp>
#include <filesystem>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <thread>

#include "csi_capture.hpp"

int main(int argc, char *argv[]) {
  std::filesystem::path root_dir =
      std::filesystem::current_path().parent_path();
  std::filesystem::path test_dir = root_dir / "test";
  std::filesystem::path pcap_path = test_dir / "test_raspi1.pcap";

  csirdr::Csi_capture cap(1, 1, true);

  pcpp::IFileReaderDevice *reader =
      pcpp::IFileReaderDevice::getReader(pcap_path.string());
  pcpp::RawPacket raw_packet;

  reader->open();

  while (reader->getNextPacket(raw_packet)) {
    pcpp::Packet packet(&raw_packet);
    cap.load_packet(packet);
    if (cap.is_full_temp_csi()) {
      cap.write_temp_csi();
    }

    // 擬似的な待ち時間
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  reader->close();
}
