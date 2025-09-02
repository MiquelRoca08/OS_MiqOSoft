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

### Include Directory  
- `include/`  
  Centralized public header files (`*.h`) shared across the project, including kernel, bootloader, HAL, utilities, and drivers.  
  This replaces scattered headers previously located inside source folders, simplifying include paths and maintenance.

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
  - System call interface  
  - Virtual file system (VFS)  
  - Basic heap allocator  
  - Process management primitives

### Libraries
- `src/libs/core/`  
  Core library in C++ under development for future refactoring and modernization of OS components.  

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

### Shell Interface

The operating system provides an interactive shell environment with command-line interface. Features include:

#### Built-in Commands

- System Information:
  - `version`: Display OS version
  - `cpuinfo`: Show CPU information
  - `meminfo`: Display memory status
  - `uptime`: Show system uptime
  - `lspci`: List PCI devices

- File Operations: (Not fully implemented yet)
  - `ls`: List files and directories
  - `cat`: Display file contents
  - `mkdir`: Create directory
  - `rm`: Remove file or directory

- Process Management:
  - `ps`: List processes
  - `syscall_test`: Test system call functionality
  - `malloc_test`: Test memory allocation

- System Tools:
  - `clear`: Clear screen
  - `help`: Show available commands
  - `reboot`: Restart system
  - `history`: Show command history

#### Shell Features

- Command history navigation
- Basic line editing capabilities
- I/O redirection (planned)
- Error handling and status reporting
- Custom command registration support

#### Integration

The shell is tightly integrated with the kernel's:

- Virtual file system
- Memory management
- Process management
- System call interface

This provides a unified interface for system interaction and testing.

### System Calls

The operating system now provides a comprehensive system call interface through interrupt 0x80. This interface includes:

#### Process Management

- `sys_getpid()`: Get current process ID
- `sys_exit(code)`: Exit process with code
- `sys_yield()`: Yield CPU to scheduler

#### Memory Management

- `sys_malloc(size)`: Allocate memory
- `sys_free(ptr)`: Free allocated memory

#### File Operations

- `sys_open(path, flags)`: Open a file
- `sys_close(fd)`: Close a file descriptor
- `sys_read(fd, buffer, count)`: Read from file
- `sys_write(fd, buffer, count)`: Write to file

#### Time and Scheduling

- `sys_time()`: Get system time
- `sys_sleep(ms)`: Sleep for milliseconds

All system calls are accessible from both kernel and user space through the same interface. The system provides error handling and validation for all operations.

---

## Development

### Directory Structure

```text
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

### Notes

Keyboard driver header (keyboard.h) has been moved to include/ to provide shared access for other kernel modules and future shell integration.

The include/ folder centralizes all public headers, simplifying include paths and project maintenance.

src/libs/core/ holds the beginnings of a C++ refactor that is ongoing and can coexist with the C codebase.
