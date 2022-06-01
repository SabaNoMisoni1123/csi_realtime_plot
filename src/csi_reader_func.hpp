#include <iostream>
#include <stdlib.h>
#include <vector>

#ifndef CSI_READER_FUNC
#define CSI_READER_FUNC

#define BITS_PER_BYTE 8
#define CSI_HEADER_OFFSET 18
#define BYTE_OF_CSI_DATA_UNIT 4
#define BITS_OF_EXPONENT_PART 6
#define BITS_OF_NUMERICS_PART 11
#define MAX_EXPONENT_PART 31

namespace csirdr {

typedef struct {
  uint64_t tx_mac_add;
  uint16_t seq_num;
  uint16_t core_stream_num;
} csi_header;

/*
 * 1パケットからデコードされたCSIの型
 * (サブキャリア数，2)
 */
typedef std::vector<std::vector<int>> csi_vec;

/*
 * UDPのペイロードからCSI情報のヘッダ部分を読み取る関数
 * input: uint8_t* payload (= udp_layer->getLayerPayload())
 * return: header (csireader::csi_header)
 */
csi_header get_csi_header(uint8_t *payload, bool new_header = false);

/*
 * UDPのペイロードのデータ長（バイト）からCSIのサブキャリア数を計算する関数
 * input: int data_len (= udp_layer->getDataLen())
 * return: number of subcarrier (int)
 */
int cal_number_of_subcarrier(int data_len);

/*
 * bcm4366c0専用のUDPのペイロードからCSIを出力する関数
 * パケット単位のデコードを実現する
 * input: uint8_t *payload
 *        int data_len (= udp_layer->getDataLen())
 * return: csi_vec (std::vector<int>, size: (num_subcarrier, 2))
 */
csi_vec get_csi_from_packet_bcm4366c0(uint8_t *payload, int data_len);

/*
 * bcm4366c0専用のCSIデータ抽出関数
 * 4バイトのCSI１要素のデータから，実数部，虚数部，指数部を出力
 * input: uint_32 csi_data_unit
 * return: csi_element_vec std::vector<int_16>
 */
std::vector<int> extract_csi_bcm4366c0(uint32_t csi_data_unit);

/*
 * bcm4366c0専用のCSI計算関数
 * 実数部，虚数部，指数部の出力結果からCSIを計算
 * 浮動小数で記録されているはずだが，整数出力になっている
 * input: extracted_csi (std::vector<std::vector<int_fast16_t>>)
 * return: csi (std::vector<std::vector<int>>)
 */
csi_vec cal_csi_bcm4366c0_int(std::vector<std::vector<int>> extracted_csi);

/*
 * bcm4366c0専用のCSI計算関数の補助
 * 実数部・虚数部に対して指数部を反映させる関数
 * xをeだけシフトさせる
 * input: x(int), e(int)
 * return: x_shft(int)
 */
int shft_element_bcm4366c0(int x, int e);

/*
 * raspi専用のUDPのペイロードからCSIを出力する関数
 * パケット単位のデコードを実現する
 * input: uint8_t *payload
 *        int data_len (= udp_layer->getDataLen())
 * return: csi_vec (std::vector<int>, size: (num_subcarrier, 2))
 */
csi_vec get_csi_from_packet_raspi(uint8_t *payload, int data_len);

/*
 * bcm4366c0専用のCSIデータ抽出関数
 * 4バイトのCSI１要素のデータから，実数部，虚数部出力
 * input: uint_32 csi_data_unit
 * return: csi_element_vec std::vector<int_16>
 */
std::vector<int> extract_csi_raspi(uint32_t csi_data_unit);

/*
 * サブキャリア系列のCSIデータの処理をする関数
 * 1. サブキャリア系列を前後半で入れ替える
 * 2. ガードバンドの値を0にする
 * input: csi_vec
 * return: csi_vec
 */
csi_vec post_process_csi(csi_vec vec);

} // namespace csirdr

#endif /* end of include guard */
