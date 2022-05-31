#include <cmdline.h>
#include <csi_reader.hpp>
#include <csi_reader_func.hpp>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  // コマンドライン引数
  cmdline::parser ps;
  ps.add<std::string>("file", 'f', "pcap file path", true);
  ps.add<std::string>("outdir", 'o', "output directory", true);
  ps.add<std::string>("device", 'd',
                      "csi capture device ([\'asus\', \'raspi\'])", false,
                      "asus");
  ps.add("new-header", '\0', "new header");
  ps.add<int>("nss", 'N', "number with spatial streams to capture", false, 4);
  ps.add<int>("core", 'C', "number with cores where to active capture", false,
              4);
  ps.parse_check(argc, argv);

  // 相対パスの処理
  std::filesystem::path pcap_path =
      std::filesystem::absolute(ps.get<std::string>("file"));
  std::filesystem::path outdir =
      std::filesystem::absolute(ps.get<std::string>("outdir"));

  // ファイルの存在確認
  if (!std::filesystem::exists(pcap_path)) {
    std::cout << "No such file " << pcap_path.string() << " ." << std::endl;
    return 1;
  }

  csirdr::Csi_reader cr(pcap_path, outdir, ps.get<std::string>("device"),
                        ps.exist("new-header"), ps.get<int>("nss"),
                        ps.get<int>("core"));
  cr.decode();

  // 終了
  std::cout << "DONE" << std::endl;

  return 0;
}
