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
  ps.add<std::string>("device", 'd', "csi capture device [\'asus\', \'raspi\']",
                      false, "asus");
  ps.add<int>("nss", 'N', "number with spatial streams to capture", false, 4);
  ps.add<int>("core", 'C', "number with cores where to active capture", false,
              4);
  ps.add<std::string>("wlan-std", 's', "wlan standard [\'ac\', \'ax\']",
                      false, "ac");
  ps.add("new-header", '\0', "decode as new header version");
  ps.add("non-zero", '\0', "non-zero values in guard band and pilot subcarrier");
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
                        ps.get<int>("core"), ps.get<std::string>("wlan-std"));

  if (ps.exist("non-zero")) {
    cr.decode(false);
  } else {
    cr.decode();
  }

  // 終了
  std::cout << "\n\n\nDONE" << std::endl;

  return 0;
}
