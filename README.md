# LeoS

LeoS是在看[《操作系统真象还原》](https://book.douban.com/search/郑钢)过程中代码实践，在elephantOS的基础上进行修改了部分代码

## 文件目录



## 环境搭建

### windows平台

- 下载并安装[Bochs 2.6.11](https://sourceforge.net/projects/bochs/files/bochs/2.6.11/)，修改`LeoS/boot/run.sh`，将`/mnt/c/'Program Files'/Bochs-2.6.11/bochs.exe`替换成你的bochs安装目录
- 到Microsoft Store安装Ubuntu 18.04LTS
- 在Ubuntu中通过`sudo apt-get install nasm`安装nasm，通过`sudo apt-get install gcc`
- 在Ubuntu `cd LeoS目录/boot`，然后`./run.sh`启动LeoS

## 参考书籍

- [《操作系统真象还原》郑钢](https://book.douban.com/subject/26745156/)
- [《汇编语言》 王爽](https://book.douban.com/subject/25726019/)
- [《Linux内核完全剖析》 赵炯](https://book.douban.com/subject/3229243//)
- [《x86汇编语言》](https://book.douban.com/subject/20492528/)

## 致谢
非常感谢[nightmare-man](https://github.com/nightmare-man/os_kernel/commits?author=nightmare-man)在学习操作系统过程中的指导
