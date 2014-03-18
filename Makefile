#
# Makefile for RC700 simulator
#

all: rc700

clean:
	rm rc700 bootrom

SRCFILES=sim1.c sim2.c sim3.c sim4.c sim5.c sim6.c sim7.c disas.c iosim.c simglb.c simctl.c \
         rc700.c rom.c pio.c sio.c ctc.c dma.c crt.c fdc.c disk.c fifo.c

HDRFILES=sim.h simglb.h disk.h bootrom

rc700: $(SRCFILES) $(HDRFILES) rcterm-curses.c
	$(CC) -o $@ $(SRCFILES) -O3 rcterm-curses.c -lncurses

rom2struct: rom2struct.c
	$(CC) -o $@ rom2struct.c

bootrom: rom2struct eprom-roa-375.bin
	./rom2struct eprom-roa-375.bin > bootrom

ana2imd: ana2imd.c disk.c disk.h sim.h
	$(CC) -o $@ ana2imd.c disk.c

imd2bin: imd2bin.c disk.c disk.h
	$(CC) -o $@ imd2bin.c disk.c

cpmdisk: cpmdisk.c disk.c disk.h
	$(CC) -o $@ cpmdisk.c disk.c

comaldisk: cpmdisk.c disk.c disk.h
	$(CC) -o $@ cpmdisk.c disk.c -DCOMAL
	
rxtext: rtext.c
	$(CC) -o $@ rctext.c

