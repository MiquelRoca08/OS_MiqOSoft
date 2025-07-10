#


# Tools
- Linux or WSL
- Make
- NASM (as assembler)
- quemu (as virtualization software)

apt install make nasm qemu-system qemu-utils


x86

# Progress
0. README.md
1. src/main.asm
2. ./build
3. Makefile

# Provar que funcioni

┌──(kali㉿LAPTOP-E7BS86DA)-[/mnt/c/Users/ASUS/OS]
└─$ make

┌──(kali㉿LAPTOP-E7BS86DA)-[/mnt/c/Users/ASUS/OS]
└─$ qemu-system-i386 -fda build/main_floppy.img

# Git Tools

git commit -m ""
git push -u origin main
