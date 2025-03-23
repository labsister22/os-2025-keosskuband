[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/RYoGTaJ4)
# IF2130 - 2025       Keossku Band!

## Mochammad Fariz Rifqi Rizqulloh (13523069)
---
<img src="https://media1.tenor.com/m/tAyTQWwFDN0AAAAd/bocchi-the-rock-kita.gif" width="600">

## Muhammad Jibril Ibrahim (13523085)
---
<img src="https://media1.tenor.com/m/0zfqxlPxYOYAAAAC/bocchi-the-rock-bocchi.gif" width="600">

## Nayaka Ghana Subrata (13253090)
---
<img src="https://i.pinimg.com/originals/a8/e5/9c/a8e59cd6a342cc3df98f793229f8bc91.gif" width="600">

## Muhammad Adha Ridwan (13523098)
---
<img src="https://64.media.tumblr.com/d8cb6d904a4434d00710efccd6b68cf1/c79002f73b0ed652-bf/s540x810/accbca65608ef8e4155d54b8341554d214d7bcf3.gif" width="600">

**Izin tampil**

---
## 💻 Language and Framework Used
![Assembly](https://img.shields.io/badge/assembly-%23525252.svg?style=for-the-badge&logo=assembly&logoColor=white) ![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white) ![gdb](https://img.shields.io/badge/GDB-%23A42E2B.svg?style=for-the-badge&logo=gnu&logoColor=white)

---

## 🔍 What's Inside?

### Milestone 0
- Kernel setup
- Global Descriptor Table (GDT) Setup
---

### Milestone 1
- Interrupter setup
- Keyboard driver
- Framebuffer setup
---

### Milestone 2
- Disk setup
- Filesystem setup (EXT2)

---
<div align="center">
This page is intentionally left blank
</div>

---

## 📦 Installation & Setup

### Requirements
- Git
- Any IDE (recommended: VSCode)
- WSL2 (optional)
- Ubuntu 20.04/22.04
- Netwide Assembler (https://www.nasm.us/) 
- GNU C Compiler (https://man7.org/linux/man-pages/man1/gcc.1.html) 
- GNU Linker (https://linux.die.net/man/1/ld)
- QEMU - System i386 (https://www.qemu.org/docs/master/system/target-i386.html) 
- GNU Make (https://www.gnu.org/software/make/) 
- genisoimage (https://linux.die.net/man/1/genisoimage) 
- GDB (https://man7.org/linux/man-pages/man1/gdb.1.html) 

### Installing dependencies
#### On Windows, do this
1. VSCode
      ```bash
   winget install microsoft.visualstudiocode
   ```
2. Extension for VSCode
      ```bash
   code --install-extension ms-vscode.cpptools-extension-pack
   code --install-extension ms-vscode-remote.remote-wsl
   ```

#### On Linux, do this
1. all dependencies and Qemu
      ```bash
   sudo apt update
   sudo apt install -y nasm gcc qemu-system-x86 make genisoimage gdb
   ```
---

## Guide For Makefile
1.
      ```bash
   make run
   ```
   Build iso and kernel to bin folder, also running the operating system
2.
      ```bash
   make build
   ```
   Build iso and kernel to bin folder
3.
      ```bash
   make all
   ```
   Build iso and kernel to bin folder
4.
      ```bash
   make clean
   ```
   to remove *.o and *.iso from the bin folder
5.
      ```bash
   make kernel
   ```
   Build kernel to bin folder
6.
      ```bash
   make iso
   ```
   Build iso to bin folder
7.
      ```bash
   make disk
   ```
   Build non-volatile memory to bin folder
---

## How To Run
### **1. Windows**
#### **- Makefile**
1. Open a terminal
2. Clone the repository
      ```bash
   git clone https://github.com/labsister22/os-2025-keosskuband.git
   ```
3. Make os-2025-keosskuband as root directory
4. Run the following command to start the application in Qemu:
   ```bash
   make run
   ```
5. If you want to develop it with non volatile memory, do:
   ```bash
   make disk
   make run
   ```
---

#### **- VSCode debugger**
1. Open a terminal
2. Clone the repository
      ```bash
   git clone https://github.com/labsister22/os-2025-keosskuband.git
   ```
3. Make os-2025-keosskuband as root directory
4. Press the following key:
   ```bash
   f5
   ```
---

## ✨ Note

### ???
  
---

## 📃 Miscellaneous

| Milestone    | Topic                                         | Progress  |
|--------------|-----------------------------------------------|--------|
| 0            | Kernel, GDT                                       | 100/100|
| 1            | Framebuffer, Interrupter, Keyboard                                       | 90/100|
| 2            |Disk, filesys (EXT2)                                      | 50/100|
