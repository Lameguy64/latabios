@echo off

nasm -f bin -DTARGET_XT -DBOOTLOADER -l lata_xti.lst -o lata_xti.bin latabios.asm
romfixup lata_xti.bin

nasm -f bin -DSINGLE_DRV -DBOOTLOADER -l latasens.lst -o latasens.bin latabios.asm
romfixup latasens.bin

nasm -f bin -DBOOTLOADER -l latabios.lst -o latabios.bin latabios.asm
romfixup latabios.bin

kzip /y labio101.zip license readme.txt latabios.bin latasens.bin lata_xti.bin
