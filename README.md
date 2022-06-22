# リアルタイムCSI取得・プロットプログラム「nexlive」
Raspberry pi 4BにNexmon CSIを導入して，取得されるCSIをリアルタイムにプロットするプログラム．主に以下のパッケージ・プログラムを用いる．

- Nexmon CSI [seemoo-lab/nexmon_csi](https://github.com/seemoo-lab/nexmon_csi)
- pcapplusplus [seladb/PcapPlusPlus](https://github.com/seladb/PcapPlusPlus)
- libpcap
- gnuplot

## セットアップ

### Nexmon CSI
Raspberry pi 4BにLinuxのカーネルバージョンが5.4の[Raspi OS Lite](https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2020-08-24/)を導入する．導入には，[OS配布サイト](https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2020-08-24/)からzipファイルダウンロードし，[Etcher](https://www.balena.io/etcher/)などを用いる．


Nexmon CSIのセットアップには，コンパイル済みバイナリを配布しているgithubレポジトリ[nexmonster/nexmon_csi_bin](https://github.com/nexmonster/nexmon_csi_bin)を利用すると良い．以下のコマンドを実行することでNexmon CSIを利用できる状態になる．
```
sudo apt update
curl -fsSL https://raw.githubusercontent.com/nexmonster/nexmon_csi_bin/main/install.sh | sudo bash
```

### GUI化
[参考URL：「ラズベリーパイにGUIをインストールする」](https://walking-succession-falls.com/%E3%83%A9%E3%82%BA%E3%83%99%E3%83%AA%E3%83%BC%E3%83%91%E3%82%A4%E3%81%ABGUI%E3%82%92%E3%82%A4%E3%83%B3%E3%82%B9%E3%83%88%E3%83%BC%E3%83%AB%E3%81%99%E3%82%8B)
```
sudo apt install --no-install-recommends xserver-xorg
sudo apt install --no-install-recommends xinit
sudo apt install raspberrypi-ui-mods
sudo apt install --no-install-recommends raspberrypi-ui-mods lxsession
sudo apt install xfce4 xfce4-terminal
sudo apt install lightdm
startx
```

### nexliveのビルド
必要パッケージをインストール．
```
sudo apt install libpcap-dev gnuplot cmake

git clone https://github.com/seladb/PcapPlusPlus.git
cd PcapPlusPlus
./configure-linux.sh --default
make
sudo make install
```

CMAKEでビルド．
```
git clone https://github.com/SK-eee-ku/csi_realtime_plot.git
cd csi_realtime_plot
mkdir build && cd build
cmake ..
make -j
sudo make install
```

## 使い方
[Nexmon CSI](https://github.com/seemoo-lab/nexmon_csi)の導入でインストールされるコマンドを用いてRaspberry pi 4Bの無線インターフェイスをCSI取得のためのモニタモードにする．以下のコマンドをシェルスクリプトで実行する．必要があればsudoで実行する．
```
mcp -c 157/80 -C 1 -N 1 -m 00:11:22:33:44:55,aa:bb:aa:bb:aa:bb -b 0x88
# m+IBEQGIAgAAESIzRFWqu6q7qrsAAAAAAAAAAAAAAAAAAA==

ifconfig wlan0 up
nexutil -Iwlan0 -s500 -l34 -vm+IBEQGIAgAAESIzRFWqu6q7qrsAAAAAAAAAAAAAAAAAAA==
iw dev wlan0 interface add mon0 type monitor
ip link set mon0 up
```

nexliveコマンドを実行する．
```
sudo nexlive -t 60 -m 4e50 # 60秒間，MACアドレス末尾4e50の端末からの信号によるCSIをプロット
```

