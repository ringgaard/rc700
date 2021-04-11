#
# Makefile for RC700 emulator
#

all: rc700

SRCFILES=cpu0.c cpu1.c cpu2.c cpu3.c cpu4.c cpu5.c cpu6.c cpu7.c  \
         rom.c charrom.c charram.c pio.c sio.c ctc.c dma.c crt.c fdc.c wdc.c ftp.c disk.c fifo.c \
         monitor.c disasm.c

HDRFILES=cpu.h rc700.h disk.h

#AUTOLOAD=roa375
AUTOLOAD=rob358

clean:
	rm rc700 $(AUTOLOAD).c

rc700: $(SRCFILES) $(HDRFILES) rc700.c rcterm-sdl.c screen.c $(AUTOLOAD).c
	$(CC) -o $@ -O3 -Wno-unused-result $(SRCFILES) $(AUTOLOAD).c rc700.c rcterm-sdl.c screen.c -lSDL

rc700-sdl2: $(SRCFILES) $(HDRFILES) rc700.c rcterm-sdl2.c screen.c $(AUTOLOAD).c
	$(CC) -o $@ -O3 -Wno-unused-result $(SRCFILES) $(AUTOLOAD).c rc700.c rcterm-sdl2.c screen.c -lSDL2

rc700-curses: $(SRCFILES) $(HDRFILES) rc700.c rcterm-curses.c $(AUTOLOAD).c
	$(CC) -o $@ -O3 -Wno-unused-result $(SRCFILES) $(AUTOLOAD).c rc700.c rcterm-curses.c -lncursesw

rc700-vt100: $(SRCFILES) $(HDRFILES) rc700.c rcterm-vt100.c $(AUTOLOAD).c
	$(CC) -o $@ -O3 -Wno-unused-result $(SRCFILES) $(AUTOLOAD).c rc700.c rcterm-vt100.c

rc700d: $(SRCFILES) $(HDRFILES) rc700d.c websock.c sha1.c sha1.h $(AUTOLOAD).c
	$(CC) -o $@ -m32 -O3 -Wno-unused-result $(SRCFILES) $(AUTOLOAD).c rc700d.c websock.c sha1.c

$(AUTOLOAD).c: rom2struct $(AUTOLOAD).rom
	./rom2struct $(AUTOLOAD).rom $(AUTOLOAD) > $(AUTOLOAD).c

rom2struct: rom2struct.c
	$(CC) -o $@ rom2struct.c

ana2imd: ana2imd.c disk.c disk.h sim.h
	$(CC) -o $@ ana2imd.c disk.c

imd2bin: imd2bin.c disk.c disk.h
	$(CC) -o $@ imd2bin.c disk.c

imdinfo: imdinfo.c disk.c disk.h
	$(CC) -o $@ imdinfo.c disk.c

readbios: readbios.c disk.c disk.h
	$(CC) -o $@ readbios.c disk.c

rom2hex: rom2hex.c
	$(CC) -o $@ rom2hex.c

cpmdisk: cpmdisk.c disk.c disk.h
	$(CC) -o $@ cpmdisk.c disk.c

comaldisk: cpmdisk.c disk.c disk.h
	$(CC) -o $@ cpmdisk.c disk.c -DCOMAL

blankimd: blankimd.c disk.c disk.h
	$(CC) -o $@ blankimd.c disk.c

raw2imd: raw2imd.c disk.c disk.h
	$(CC) -o $@ raw2imd.c disk.c

imdsectofs: imdsectofs.c disk.c disk.h
	$(CC) -o $@ imdsectofs.c disk.c

rxtext: rtext.c
	$(CC) -o $@ rctext.c

char2raw: char2raw.c charrom.c charram.c
	$(CC) -o $@ char2raw.c charrom.c charram.c

phe358.c: rom2struct phe358.rom
	./rom2struct phe358.rom phe358 > phe358.c

rc700-memdisk: $(SRCFILES) $(HDRFILES) rc700.c rcterm-sdl.c phe358.c memdisk.c
	$(CC) -o $@ -O3 -Wno-unused-result -DHAS_MEMDISK -DPROM0=phe358 $(SRCFILES) phe358.c memdisk.c rc700.c rcterm-sdl.c -lSDL

SANOS=../sanos
INSTALL=$(SANOS)/linux/install
TCC=$(SANOS)/linux/tools/cc
TCCFLAGS=-B $(SANOS)/linux/install/usr
MKDFS=$(SANOS)/linux/tools/mkdfs

rc700-sanos.exe: $(SRCFILES) $(HDRFILES) rc700.c rcterm-sanos.c screen.c $(AUTOLOAD).c
	$(TCC) -o $@ $(TCCFLAGS) -I .. $(SRCFILES) $(AUTOLOAD).c rc700.c rcterm-sanos.c screen.c

rc700.flp: rc700-sanos.exe rc700.lst
	$(MKDFS) -d rc700.flp -b $(INSTALL)/boot/boot -l $(INSTALL)/boot/osldr.dll -k $(INSTALL)/boot/krnl.dll -K video=1024x768x16 -c 1440 -i -f -S . -F rc700.lst

