#!/bin/sh

echo "===== Build RC700 emulator for Linux"
make rc700

#echo "===== Build RC700 emulator for Windows"
#wine cmd /C winbuild.cmd

echo "===== Make rc700-src.zip"
rm -f /mnt/www/rc702/rc700-src.zip
zip /mnt/www/rc702/rc700-src.zip *.c *.h *.ico *.rc *.m Makefile* *.rom rccpm22.imd autoload.imd COPYING

echo "===== Make rc700.zip"
rm -f /mnt/www/rc702/rc700.zip
zip /mnt/www/rc702/rc700.zip rc700 rccpm22.imd

#echo "===== Make rc700-win.zip"
#rm -f /mnt/www/rc702/rc700-win.zip
#zip /mnt/www/rc702/rc700-win.zip rc700.exe rccpm22.imd SDL.dll

#echo "===== Make rc700-win-setup.exe"
#wine ../innosetup/ISCC rc700.iss /o.
#osslsigncode sign -pkcs12 ../certum.pfx -pass hemlig \
#    -n "RC700 Emulator" \
#    -i "http://www.jbox.dk/rc702/emulator.shtm" \
#    -in rc700-win-setup.exe \
#    -out /mnt/www/rc702/rc700-win-setup.exe
#rm *.exe *.obj *.res

