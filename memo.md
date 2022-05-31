# メモ

## 実装の方針

- リアルタイムパケットキャプチャにはコールバック関数が相性がいい
- rawpacketを引数に動作する機械学習モデルのクラスを書いたらOK
- 学習フェーズは別で作成するほうがいい
- 学習フェーズと運用委フェーズの2つのプログラムでOK

```
static void onPacketArrives(pcpp::RawPacket* raw_packet, pcpp::PcapLiveDevice* dev, void* cookie){
  Myclass* ist = (Myclass *) cookie;
  pcpp::Packet packet = parsedPacket(raw_packet);
  ist->func(packet);
}
```

## リアルタイムの図示
リアルタイム図示はテキストなどで出力したCSIデータを外部ツール（gnuplot）でプロットする形式にする．機械学習アプリケーションの方もそっちのほうがいいのか?
