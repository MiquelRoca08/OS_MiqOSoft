# Introduction
This is an x86 OS built from scratch

# Prerequisites
- Linux or WSL
- Make
- NASM (as assembler)
- QEMU (as virtualization software)
- Bochs (as debugger)
```
apt install make nasm qemu-system qemu-utils bochs bochs-sdl bochsbios vgabios
```
# Testing with QEMU
```
make
./run.sh
```

# Debugging with Bochs
```
make
./debug.sh
```

# Git Tools (im a noob)
```
git commit -m ""
git push -u origin main
```
