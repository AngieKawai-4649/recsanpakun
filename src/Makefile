
#【ビルド方法】
#1.STARBUCKSを使用
#    1.1 libarib25をセットアップする
#    1.2 libstarbucksをセットアップする
#    1.3 recsanpakun/srcディレクトリで
#        make STARBUCKS=1
#        sudo make install
#
#2.内蔵カードリーダーを使用
#	2.1 ktv_cardreader libktvcardreader.so をbuild,installする
#    2.2 recsanpakun/srcディレクトリで
#        make BUILT-IN=1
#        sudo make install
#
#3.外付けカードリーダーを使用
#    3.1 libarib25をセットアップする
#    3.2 recsanpakun/srcディレクトリで
#        make EXTERNAL=1
#        sudo make install
#
#4.B25解除を行わない場合
#    B25解除を当プログラムで行わず、mirakurun arib-b25-stream-test等で行う場合
#    recsanpakun/srcに移動し
#    make
#    sudo make install
#    注 B25解除を当プログラムで行わない場合、tssplitter_liteは使用できません
#
#B25処理を変更する為にビルドし直す場合はフルコンパイルする
#例：内蔵カードリーダーからSTARBUCKSに変更する場合
#make BUILT-IN=1 clean 
#make STARBUCKS=1
#
#それぞれのbuildによりリンクするカードリーダーライブラリが異なるので確認する
#ldd recsanpakun
#
#STARBUCKS: libarib25.so.0直下にlibstarbucks.so
#BUILT-IN : libarib25.so.0直下にlibktvcardreader.so
#EXTERNAL : libarib25.so.0直下にlibpcsclite.so.1

CHANNELCNF = `pkg-config libchannelcnf --cflags`
LIBS = `pkg-config libchannelcnf --libs`
ifdef STARBUCKS
STD_B25 = -DSTD_B25
LIBS    += -larib25 -lstarbucks
else
ifdef BUILT-IN
STD_B25 = -DSTD_B25
DB_CARDREADER = -DDB_CARDREADER
PCSC = `pkg-config libpcsclite --cflags`
LIBS    += -larib25 -lktvcardreader
B25_PATH = ./arib25
else
ifdef EXTERNAL
STD_B25 = -DSTD_B25
LIBS    += -larib25 `pkg-config libpcsclite --libs`
endif
endif
endif

CXX = g++
CC	= gcc
CFLAGS	= -O2 -Wall -pthread -D_LARGEFILE_SOURCE $(DB_CARDREADER) $(CHANNELCNF) $(PCSC)
CXXFLAGS = -O2 -Wall -pthread -D_LARGEFILE_SOURCE $(DB_CARDREADER)  $(CHANNELCNF)$(PCSC)
OBJS	= main.o utils.o message.o osdepend.o usbdevfile.o usbops.o tsthread.o em287x.o em287x_usb.o tc90522.o mxl136.o tda20142.o tssplitter_lite.o

LIBS	+= -lpthread
TARGET	= recsanpakun
PREFIX = /usr/local

ifeq ($(shell getconf LONG_BIT),32)
CFLAGS	+= -D_FILE_OFFSET_BITS=64
endif

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

uninstall:
	rm -vf $(PREFIX)/bin/$(TARGET)

install: uninstall
	install -m 755 $(TARGET) $(PREFIX)/bin

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
ifdef STARBUCKS
	@echo "\n"B25 DECODE USING STARBUCKS"\n"
else
ifdef BUILT-IN
	@echo "\n"B25 DECODE USING BUILT-IN CARDREADER"\n"
else
ifdef EXTERNAL
	@echo "\n"B25 DECODE USING EXTERNAL CARDREADER"\n"
endif
endif
endif
ifndef STD_B25
	@echo "\n"B25 DECODE OFF
	@echo "STARBUCKS           -> make STARBUCKS=1"
	@echo "BUILT-IN CARDREADER -> make BUILT-IN=1"
	@echo "EXTERNAL CARDREADER -> make EXTERNAL=1""\n"
endif

depend:
	$(CC) -MM $(OBJS:.o=.c) > Makefile.dep
ifdef BUILT-IN
	$(CC) -MM $(B25_OBJS_EXTC:.o=.c) >> Makefile.dep
	$(CC) -MM $(B25_OBJS_EXTP:.o=.cpp) >> Makefile.dep
endif

-include Makefile.dep

utils.o :
	$(CC) $(CFLAGS) $(STD_B25) -c -o $@ $(@:.o=.c)
