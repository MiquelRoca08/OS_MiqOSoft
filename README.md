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
- Keyboard drivers for most of alphanumerical characters

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
  - Hardware abstraction layer

### Build System
- `SConstruct`  
  Main build configuration file
- `build_scripts/`  
  Python build configuration and utility scripts
- `image/`  
  Disk image creation and management
- `scripts/`  
  Development and debugging helper scripts

### Tools
- `tools/`  
  Custom development and build tools

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

# Run with debug output
./scripts/debug.sh
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

## Development Tools

### Scripts
- `scripts/bochs.sh`: Launch Bochs debugger
- `scripts/debug.sh`: Run with debug output
- `scripts/run.sh`: Quick QEMU launch
- `scripts/setup_toolchain.sh`: Set up development environment

### Build Scripts
- `build_scripts/config.py`: Build configuration
- `build_scripts/generate_isrs.sh`: Generate interrupt handlers
- `build_scripts/utility.py`: Build utility functions

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
│       ├── arch/i686/
│       ├── drivers/
│       ├── hal
│       └── util
├── tools/
├── build_scripts/
├── image/
├── scripts/
├── tools/
└── build/
```

### Toolchain
The project uses a custom cross-compiler toolchain for i686-elf target. The toolchain is automatically built when needed.

```bash
# Build toolchain manually
scons toolchain
```

### Debug Builds
Debug builds include additional symbols and logging:

```bash
# Build with debug configuration
sudo scons config=debug

# Run with debug output
./scripts/debug.sh

```
