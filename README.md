# さんぱくん外出Linux用チューナーアプリケーション  

ベース recfsusb2n   for Linux OS 2017-01-22, Ver. 0.1.13  

対象デバイス  

さんぱくん外出  
VENDOR-ID   0x0511  
PRODUCT-ID  0x0045  

## 【改造箇所】  
１．コンパイル時のワーニング除去  
２．内蔵カードリーダーを使用できるように修正  
３．tssplitter_liteが正常に機能するように修正  
　(B25Decodeを行ってからtssplitter_liteを実行するようにした)  
４．地デジ視聴時は地デジのみ、BS/CS視聴時はBS/CSチューナーのみ開くように修正  
　(mirakurunではチャンネルを切り替える度にプロセスを再起動しているので  
　地デジ視聴時にBSチューナーを開けてスリープにしている運用を止めた）  
５．実行ファイル名をrecsanpakunに変更  
６．チャンネルファイル名称をbscs_ch.confに変更  
　(recdvbもこのファイルを使用するように修正)  
７．bscs_ch.confをフルパスで指定する環境変数 BSCSCHPATH を設定  
　BSCSCHPATH設定時はそのディレクトリパス配下にbscs_ch.confを置く  
　BSCSCHPATHが設定されていない時はrecsanpakunと同じディレクトリ(通常は/usr/local/bin)にbscs_ch.confを置く  
8. 上記6 7 を全て廃止 チャンネル情報はchannelconfから取得するように修正  
  

## 【ビルド方法】  
b25デコードをする場合、事前に https://github.com/AngieKawai-4649/libarib25 を導入する  
事前に https://github.com/AngieKawai-4649/channelconf を導入する（必須）  
取得  
git clone https://github.com/AngieKawai-4649/recsanpakun.git  
ビルド方法はMakefile中のコメントを参照  

## 【使用方法】  
recsanpakun [-v] [--b25] [--dev devfile] [--tsid n] [--sid hd,sd1,sd2,sd3,1seg,all,epg,epg1seg] channel recsec destfile  

-v : ログを標準出力する  
--b25 : B25解除する  B25解除を行わないでビルドした場合は指定不可  
--dev : デバイスを指定する  
　　　例： lsusb の結果 Bus 001 Device 003: ID 0511:0045 N'Able (DataBook) Technologies, Inc. の場合  
　　　　　ubuntuでは --dev /dev/bus/usb/001/003 と指定するが各環境のUSBデバイスパスを調べて指定すること  
　　　　　また、USBコネクタの接続する場所を変えるとデバイスパスが変わるので注意  
　　　　　無指定の場合は接続された未使用のデバイスを順に使用する  
--tsid : TSIDを指定　16進数の場合、頭に0x or 0X を付ける  
--sid : tssplitter_liteでTSを分割し特定のストリームを抽出する場合に指定、複数オプションはカンマで区切る  
　　B25解除を行わないでビルドした場合は指定不可  
　　　例： NHK BS1 サブチャンネルを録画する場合  
　　　　　--sid 102  
　　　　　注： mirakurunを導入している場合、mirakurunで指定したsidのストリームを抽出できるので  
　　　　　　tssplitter_lite機能を使用する必要はない  
　　　　　注:bscs_ch.confで設定するチャンネル名称(BS15_0,CS24等)はmirakurunのchannels.yml内のchannel:XXX と合わせなければならない  
rectime: 録画時間を秒単位で指定する  ( - 指定時は何時までも出力する)  
destfile: TS出力ファイル名を指定する ( - 指定時は標準出力）  

例: NHK東京 1segをデバイスを指定して30秒録画する  
　./recsanpakun -v --b25  --dev /dev/bus/usb/001/003 --sid 1seg 27 30 aaa.ts  
　NHK BS1を１分間録画する (チャンネルはbscs_ch.confを参照)  
　./recsanpakun -v --b25 BS15_0 60 bbb.ts  
　スーパードラマTVを１分間録画する  
　./recsanpakun -v --b25 --sid 310 CS14 60 ccc.ts  

## 【USBデバイスの固定】  
USB機器を追加、取り外しを行うとチューナーデバイスのUSBパスが変わってしまうので  
チューナーを接続しているUSBポートの物理的な場所に対してチューナーデバイス名称を付ける  

$ lsusb で各チューナーデバイスのUSBパスを調べる  
$ udevadm でUSBポートの物理的な場所を調べる  

例 $ udevadm info -n /dev/bus/usb/001/005 -q path  
　出力値例 /devices/pci0000:00/0000:00:08.1/0000:05:00.3/usb1/1-4/1-4.1 が物理的な場所である  
　rulesファイルでDEVPATHに指定する  
　SYMLINKで指定した名称で /devにデバイスのシンボリックリンクが作成されるのでmirakurun tuners.ymlでそのデバイス名称を使用する  

　ATTRを調べる例 $ udevadm info -a -p \`udevadm info -q path -n /dev/bus/usb/001/005\`  

## [ruleファイルの作成]  
/lib/udev/rules.d  and /etc/udev/rules.d 配下のファイルをチェックし、使用していない番号のルールファイルを/etc/udev/rules.dに作成する  
　例：93-tuner.rules  
　　ファイル保存後、ルールを反映  
　　$ sudo udevadm control --reload  
　　リブート後、デバイスを確認する  
　　$ ls /dev  

## [ruleファイルの内容例]  
\# SANPAKUN  
SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="0511", ATTRS{idProduct}=="0045",
 DEVPATH=="/devices/pci0000:00/0000:00:08.1/0000:05:00.3/usb1/1-4/1-4.3", MODE="0664", GROUP="video", SYMLINK+="SANPAKUN_1"  
SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="0511", ATTRS{idProduct}=="0045",
 DEVPATH=="/devices/pci0000:00/0000:00:08.1/0000:05:00.3/usb1/1-4/1-4.4", MODE="0664", GROUP="video", SYMLINK+="SANPAKUN_2"  

## [mirakurun tuners.ymlの内容例]  

\- name: SANPAKUN-ST1  
types:  
 \- GR  
 \- BS  
 \- CS  
  command: recsanpakun --b25 --dev /dev/SANPAKUN_1 <channel> - -  
  decoder:  
  isDisabled: false  

\- name: SANPAKUN-ST2  
  types:  
    \- GR  
    \- BS  
    \- CS  
  command: recsanpakun --b25 --dev /dev/SANPAKUN_2 <channel> - -  
  decoder:  
  isDisabled: false  

※地デジを使用しない場合は - GR を削除する  
