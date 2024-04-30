# OS_Lab
这是[**南京大学软件学院**](https://software.nju.edu.cn)操作系统课实验作业

## Lab1

### 实验内容

使用汇编语言 [`NASM`](https://www.nasm.us) 实现十进制数转换成其他进制的表示

**输入格式**： `十进制数` `转换类型`

**两者中间有一个空格**

**数据范围**： `十进制数` 的范围是 $[0, 10^{30}]$；`转换类型` 包含 `b` 、`o` 和 `h` ，分别代表二进制、八进制和十六进制。输入 `q` 结束程序。

**输出格式**： `转换结果`

**若输入无效则输出错误信息**

### 配置方法

1. 安装 `NASM` 编译器 （以 `Ubuntu` 系统，`x64` 架构为例）。

   ```shell
   apt-get install nasm
   ```

2. 编写以下脚本，并将 `lab1.asm` 与其放至同一文件夹下。

   ```shell
   name=$1
   nasm -g -f elf64 -F dwarf -o mid.o $name \
   && ld -o "$name.out" mid.o \
   && rm -f mid.o \
   && echo 'Compile success!!!' \
   && "./$name.out"
   ```

3. 运行脚本即可编译并运行该程序，指令格式如下。

   ```
   <脚本路径> lab1.asm
   ```

## Lab2

### 实验内容

用 `C` 和 `NASM` 编写一个 `FAT12` 文件系统的软盘镜像查看工具，读取该文件夹下名为 `a.img` 的文件并响应用户输入。

**输入格式**：`指令名称`  `指令选项`  `路径`

**三者中间有一个空格**

其中指令名称包括 `ls` 和 `cat`；指令选项为 `-l`，仅当指令名称为 `ls` 可用；指令名称为 `ls` 时路径必为某一目录，指令名称为 `cat` 时路径必为某一文件。

**输出格式**：指令名称为 `ls` 时输出目录及其子目录内容；指令名称为 `cat` 时输出文件内容。

### 配置方法

1. 安装 `NASM`、[`gcc`](https://gcc.gnu.org) 和 [`make`](https://www.gnu.org/software/make/)。

2. 在项目文件夹，运行以下指令。

   ```shell
   make compile
   make run
   ```
