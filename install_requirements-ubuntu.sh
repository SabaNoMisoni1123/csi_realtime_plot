sudo apt install libpcap-dev gnuplot

git clone https://github.com/seladb/PcapPlusPlus.git
cd PcapPlusPlus
./configure-linux.sh --default
make
sudo make install
