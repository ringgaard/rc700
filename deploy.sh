#!/bin/sh

make rc700
rm -f /mnt/www/rc702/rc700-src.zip /mnt/www/rc702/rc700.zip
zip /mnt/www/rc702/rc700-src.zip *.c *.h Makefile eprom-roa-375.bin rccpm22.imd
zip /mnt/www/rc702/rc700.zip rc700 rccpm22.imd

