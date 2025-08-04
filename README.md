# OS_MicOSoft

Minimalist operating system developed as a personal project.

---

## Main Features

- Custom bootloader compatible with various BIOS types
- Integrated FAT file system for managing files and directories
- Basic read/write support on storage devices
- Compatible with both FAT12 (floppy) and FAT32 (disk) file systems
- Modular architecture with stage1 and stage2 bootloaders
- Cross-platform build system using SCons

---

## Project Structure

### Bootloader
- `src/bootloader/stage1/`  
  First stage bootloader that fits in the MBR (446 bytes)

- `src/bootloader/stage2/`  
  Second stage bootloader with extended functionality

### Kernel
- `src/kernel/`  
  Core kernel components including:
  - Memory management
  - Device drivers
  - File system implementation
  - Interrupt handling

### Build System
- `SConstruct`  
  Main build configuration file
- `image/`  
  Disk image creation scripts
- `scripts/`  
  Build and development helper scripts

---

## Prerequisites

### Required Software
- Linux/WSL2
- Python 3.12 or newer
- SCons build system
- NASM assembler
- QEMU for emulation
- Bochs for debugging

### Installation on Ubuntu/WSL2
```bash
# Install basic dependencies
sudo apt update
sudo apt install python3 python3-pip nasm qemu-system-x86 qemu-utils

# Install SCons
pip install scons

# Install Bochs (optional, for debugging)
sudo apt install bochs bochs-sdl bochsbios vgabios
```

---

## Building

```bash
# Build the entire project
scons

# Build with specific configuration
scons config=debug arch=i686 imageType=disk
```

## Running

```bash
# Run in QEMU
scons run

# Debug with Bochs
scons bochs
```

## Build Configuration Options

- `config`: Build configuration (`debug` or `release`)
- `arch`: Target architecture (`i686`)
- `imageType`: Image type (`floppy` or `disk`)
- `imageSize`: Size of disk image (e.g., `250m`)
- `imageFS`: File system type (`fat12` or `fat32`)

---

## Development

### Directory Structure
```
.
├── src/
│   ├── bootloader/
│   │   ├── stage1/
│   │   └── stage2/
│   └── kernel/
├── image/
├── scripts/
└── toolchain/
```

### Toolchain
The project uses a custom cross-compiler toolchain for i686-elf target. The toolchain is automatically built when needed.

```bash
# Build toolchain manually
scons toolchain
```