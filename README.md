# OS_MiqOSoft

Minimalist operating system developed as a personal project.

---

## Main Features

- Custom bootloader compatible with various BIOS types  
- Integrated FAT file system for managing files and directories  
- Basic read/write support on storage devices  
- Compatible with both FAT12 (floppy) and FAT32 (disk) file systems  
- Modular architecture with stage1 and stage2 bootloaders  
- Cross-platform build system using SCons  
- Support for i686 architecture  
- Memory management and paging  
- Hardware interrupt handling  
- Custom build scripts and tools  
- Keyboard drivers for most alphanumerical characters  
- System call interface (int 0x80) with support for:  
  - Process management (getpid, exit, yield)  
  - Memory management (malloc, free)  
  - File operations (open, close, read, write)  
  - Time and scheduling (sleep, time)  
- Virtual file system with basic file operations  
- Simple heap memory allocator  
- Interactive shell environment with:  
  - Built-in commands for system management  
  - File system operations  
  - Memory and process inspection  
  - System call testing interface  
  - Command history and line editing  

---

## Project Structure
```
.
├── include/
├── src/
│   ├── bootloader/
│   │   ├── stage1/
│   │   └── stage2/
│   ├── kernel/
│   │   ├── arch/i686/
│   │   ├── drivers/
│   │   ├── hal/
│   │   └── util/
│   └── libs/
│       └── core/
├── tools/
├── build_scripts/
├── image/
├── scripts/
├── SConstruct
├── requirements.txt
└── TODO.md
```
---

## Prerequisites

- Linux / WSL2  
- Python 3.12 or higher  
- SCons  
- NASM assembler  
- QEMU for emulation  
- Bochs for debugging (optional)  
- `libguestfs-tools` (for disk image creation)  

**Installation on Ubuntu / WSL2:**

```bash
sudo apt update
sudo apt install python3 python3-pip nasm qemu-system-x86 qemu-utils libguestfs-tools
pip install scons
sudo apt install bochs bochs-sdl bochsbios vgabios     # Optional
echo 'export LIBGUESTFS_BACKEND=direct' >> ~/.bashrc
echo 'export LIBGUESTFS_BACKEND_SETTINGS=force_tcg' >> ~/.bashrc
source ~/.bashrc
```
---

## Toolchain
The project uses a custom cross-compiler toolchain for i686-elf target. The toolchain is automatically built when needed.
```
# Build toolchain manually
scons toolchain
```

## Building
```
sudo scons
# For debug build
sudo scons config=debug arch=i686 imageType=disk
```
## Running
```
sudo scons run         # Run in QEMU
sudo scons bochs       # Run in Bochs (debugging)
./scripts/debug.sh     # Detailed debug output
```

>Note on sudo permissions:
>Root privileges are required to create and mount disk images, and for accessing the disk device in some emulators, especially when using libguestfs.

## Demo

To see the OS in action, I’ve uploaded several demonstrations on my YouTube channel, where the main features can be observed running on real builds.
https://www.youtube.com/@miquelroca08

