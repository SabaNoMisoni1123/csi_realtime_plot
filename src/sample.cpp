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
  csirdr::Csi_capture cap("wlan0", 1, 1, true, true, temp_data_dir.string());

  pcpp::IFileReaderDevice *reader =
      pcpp::IFileReaderDevice::getReader(pcap_path.string());
  pcpp::RawPacket raw_packet;

  reader->open();

  // gnuplot shell open
  FILE *gnuplot = popen("gnuplot", "w");

  // gnuplot plot option setting
  fprintf(gnuplot, "set datafile separator \',\'\n");
  // fprintf(gnuplot, "set terminal x11\n");
  fprintf(gnuplot, "set terminal qt\n");
  fprintf(gnuplot, "set xrange [0:63]\n");
  fprintf(gnuplot, "set yrange [0:3000]\n");
  fprintf(gnuplot, "set style line 1 lw 3 lc 1\n");
  fprintf(gnuplot, "set nokey\n");
  fprintf(gnuplot, "set xlabel font\"*,20\"\n");
  fprintf(gnuplot, "set ylabel font\"*,20\"\n");
  fprintf(gnuplot, "set tics font\"*,20\"\n");
  fprintf(gnuplot, "set xlabel offset 0,0\n");
  fprintf(gnuplot, "set ylabel offset -2,0\n");
  fprintf(gnuplot, "set lmargin 12\n");
  fprintf(gnuplot, "set xlabel \"Subcarrier index\"\n");
  fprintf(gnuplot, "set ylabel \"CSI amplitude\"\n");
  fflush(gnuplot);

  // create gnuplot command
  std::filesystem::path temp_csi_path = temp_data_dir / "csi_value.csv";
  std::stringstream gnuplot_cmd_ss;
  gnuplot_cmd_ss << "plot \"" << temp_csi_path.string()
                 << "\" using ($0):(sqrt($1**2 + $2**2)) ls 1 with lines"
                 << std::endl;

  while (reader->getNextPacket(raw_packet)) {
    pcpp::Packet packet(&raw_packet);
    cap.load_packet(packet);
    if (cap.is_full_temp_csi()) {
      cap.write_temp_csi();
    }
    // send command to gnuplot
    // fprintf(gnuplot, "clear\n");
    fprintf(gnuplot, gnuplot_cmd_ss.str().c_str());
    fflush(gnuplot);

    // 擬似的な待ち時間
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  reader->close();
}
