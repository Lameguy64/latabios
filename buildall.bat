@echo off

nasm -f bin -DSENSIBLE -o lata_s.bin latabios.asm
romfixup lata_s.bin

nasm -f bin -DSENSIBLE -DBOOTLOADER -o lata_sb.bin latabios.asm
romfixup lata_sb.bin

nasm -f bin -o latabios.bin latabios.asm
romfixup latabios.bin

nasm -f bin -DBOOTLOADER -o lata_b.bin latabios.asm
romfixup lata_b.bin

kzip /y labio100.zip license readme.txt latabios.bin lata_b.bin lata_s.bin lata_sb.bin
