# Introduction
This is an x86 OS built from scratch

# Tools
- Linux or WSL
- Make
- NASM (as assembler)
- QEMU (as virtualization software)
```
apt install make nasm qemu-system qemu-utils
```
# Testing
```
make
qemu-system-i386 -fda build/main_floppy.img
```

# Git Tools (im a noob)
```
git commit -m ""
git push -u origin main
```
