[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/RYoGTaJ4)
# Keossku Band!

> IF2130 - 2025 - Operating System from scratch, based on C lang and some asm stuff.
<br/>

<p align="center">
    <img src="https://github.com/user-attachments/assets/a2aef526-bd35-4267-845b-2a836dcb45a7">
</p>
    <h4 align="center">Us when trying to implement Doom (it's absolute doomsday)</h4>

<br/>
 <div align="center" id="contributor">
   <strong>
     <h3> Me And My Homies </h3>
     <table align="center">
       <tr align="center">
         <td>NIM</td>
         <td>Name</td>
         <td>GitHub</td>
       </tr>
       <tr align="center">
         <td>13523069</td>
         <td>Mochammad Fariz Rifqi Rizqulloh</td>
         <td><a href="https://github.com/Fariz36">@Fariz36</a></td>
       </tr>
       <tr align="center">
         <td>13523085</td>
         <td>Muhammad Jibril Ibrahim</td>
         <td><a href="https://github.com/BoredAngel">@BoredAngel</a></td>
       </tr>
       <tr align="center">
         <td>13523090</td>
         <td>Nayaka Ghana Subrata</td>
         <td><a href="https://github.com/Nayekah">@Nayekah</a></td>
       </tr>
       <tr align="center">
         <td>13523098</td>
         <td>Muhammad Adha Ridwan</td>
         <td><a href="https://github.com/adharidwan">@adharidwan</a></td>
       </tr>
     </table>
   </strong>
 </div>

<div align="center">
  <h3></h3>

  <p>
    <img src="https://github.com/user-attachments/assets/b0350d7b-9a2e-44c2-ad13-10648b35a207" alt="C" width="250"/>
  </p>

  <p>
     
   https://github.com/user-attachments/assets/165f78bd-33dc-4352-bfc3-972ac14b4544

  </p>  



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

<br/>

### Installing dependencies
#### On Windows:
1. VSCode
      ```bash
   winget install microsoft.visualstudiocode
   ```
2. Extension for VSCode
      ```bash
   code --install-extension ms-vscode.cpptools-extension-pack
   code --install-extension ms-vscode-remote.remote-wsl
   ```

#### On Linux:
1. All dependencies and Qemu
      ```bash
   sudo apt update
   sudo apt install -y nasm gcc qemu-system-x86 make genisoimage gdb
   ```
---

<a id="makefile"></a>
## Guide For Makefile

**Here are some of important commands**

1. runs the operating system, will do if the bin file, kernel, and iso is initiated

   ```bash
   make run
   ```
   
2. Building all (new), also runs the operating system

   ```bash
   make new
   ```

3. Build all (without running the os)
   
   ```bash
   make all
   ```

4. make the iso and kernel
   
   ```bash
   make build
   ```

5. cleaning the bin folder
   
   ```bash
   make clean
   ```

6. Initialize a disk

   ```bash
   make disk
   ```

7. Making inserter to insert a binary file

   ```bash
   make inserter
   ```

8. Initiate all
   
   ```bash
   make init
   ```

---

## How To Run
### **Makefile (Linux)**

1. Open a terminal
2. Clone the repository

   ```bash
   git clone https://github.com/labsister22/os-2025-keosskuband.git
   ```

3. Make os-2025-keosskuband as root directory
4. Run any make commands as you like (refer to makefile's [guide](#makefile)):

<br/>

### **VSCode debugger (Linux/Windows)**
1. Open VSCode
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

## ✨ Features
---



<p align="center">
    <img src="https://github.com/user-attachments/assets/ee8099f8-e009-4e5f-b51d-03d1e8717972">
</p>
    <h5 align="center">"I often compare open source to science. To where science took this whole notion of developing ideas in the open and improving on other peoples' ideas and making it into what science is today and the incredible advances that we have had. And I compare that to witchcraft and alchemy, where openness was something you didn't do."</h5>

<br/>
<br/>

### **Available Commands**

1. **help** 

   Show this help message

2. **clear** 

   Clear the screen

3. **ls** 

   List files in current directory

4. **cd** 

   Move to spesified directory

5. **mkdir** 

   Create a new directory

6. **rm**

   Delete a file

7. **cp** 

   Copy a file to the target location

8. **mv** 

   move a file to the target location

9. **find** 

   find filename location

10. **cat** 

    Print out the specified file content

11. **echo** 

    Echo the text

12. **ps** 

     List active process

13. **exec** 

     Run an executable file

14. **kill** 

     Kill an active process

15. **sleep**

    sleep current running process for the given amount time (ms)

16. **exit** 

    Exit the shell

### **Extra Features**

1. **apple**

   Behold, bad apple in our OS

2. **ikuyo**

   @Fariz36 waifu

3. **donut**

   Watch the classic 3d donut in action








<br/>
<br/>

<div align="center">
 Operating System • © 2025 • Keossku Band!
</div>
