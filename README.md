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
- libguestfs-tools (for disk image creation)

### Installation on Ubuntu/WSL2
```bash
# Install basic dependencies
sudo apt update
sudo apt install python3 python3-pip nasm qemu-system-x86 qemu-utils libguestfs-tools

# Install SCons
pip install scons

# Install Bochs (optional, for debugging)
sudo apt install bochs bochs-sdl bochsbios vgabios

# Set required environment variables for libguestfs
echo 'export LIBGUESTFS_BACKEND=direct' >> ~/.bashrc
echo 'export LIBGUESTFS_BACKEND_SETTINGS=force_tcg' >> ~/.bashrc
source ~/.bashrc
```

---

## Building

```bash
# Build the entire project (sudo required for disk image creation)
sudo scons

# Build with specific configuration
sudo scons config=debug arch=i686 imageType=disk
```

## Running

```bash
# Run in QEMU (sudo required due to disk image access)
sudo scons run

# Debug with Bochs
sudo scons bochs
```

### Note on Sudo Requirements
The build process requires sudo privileges due to libguestfs requirements for disk image creation and mounting. This is necessary for:
- Creating the disk image
- Mounting the image to copy files
- Running the emulator with direct disk access

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
