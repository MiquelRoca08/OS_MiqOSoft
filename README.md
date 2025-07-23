# OS_MicOSoft

Minimalist operating system developed as a personal project.

---

## Main Features

- Custom bootloader compatible with various BIOS types.  
- Integrated FAT file system for managing files and directories.  
- Basic read/write support on storage devices.  
- Compatible with 1.44 MB floppy disks formatted with FAT12.  
- Optimizations to ensure reliable booting across different environments.

---

## Project Structure

- `src/bootloader/boot.asm`  
  Bootloader code including FAT support and BIOS compatibility tweaks.

- `tools/fat/fat.c`  
  Basic FAT file system management:  
  - Reading and writing sectors  
  - File Allocation Table (FAT) handling  
  - Directory and file management  
  - Maintaining the file system structure

- `Makefile`  
  Configured to compile the bootloader and kernel, including creation of a FAT disk image.

---

# Prerequisites
- Linux or WSL
- `Make`
- `NASM` (as assembler)
- `qemu-system-x86` (as virtualization software)
- `bochs-x bochsbios vgabios` (as debugger)
- Open Watcom
```
apt install make nasm qemu-system qemu-utils bochs bochs-sdl bochsbios vgabios
```

---

# Testing with QEMU
```
make
./run.sh
```

---

# Debugging with Bochs
```
make
./debug.sh
```

---


# Git Tools (cuz im a noob)
```
git commit -m ""
git push -u origin main
```
