;boot.s 主引导程序
;第一个扇区是通过BIOS读取到内存0x7c00，然后跳转到0x7c00位置执行代码
;
%include "boot.inc"
section mbr vstart=0x7c00         ;通过jmp 0x0000:0x7c00 跳转到MBR，此时段寄存器cs=0
   mov ax,cs      
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov fs,ax
   mov sp,0x7c00
   mov ax,0xb800                 ;文本显示区域
   mov gs,ax

;-------------------------------------------
;INT 0x10   功能号:0x06	   功能描述:上卷窗口
;-------------------------------------------
;输入：
;AH 功能号= 0x06
;AL = 上卷的行数(如果为0,表示全部)
;BH = 背景颜色和前景颜色，0x07表示白字黑底
;(CL,CH) = 窗口左上角的(X,Y)位置
;(DL,DH) = 窗口右下角的(X,Y)位置
;无返回值：
   mov  ax, 0x600
   mov  bx, 0x700
   mov  cx, 0x0                 ; 左上角: (0, 0)
   mov  dx, 0x184f	           ; 右下角: (80,25),
			                       ; VGA文本模式中,一行只能容纳80个字符,共25行。
			                       ; 下标从0开始,所以0x18=24,0x4f=79
   int  0x10                    ; int 0x10

;---------------------------------------------
;INT 0x10   功能号:0x03   功能描述:获取光标位置
;---------------------------------------------
;输入：
;AH 功能号= 0x03
;AL = 
;BX = 待获取光标的页号
;输出: 
;ch=光标开始行,cl=光标结束行
;dh=光标所在行号,dl=光标所在列号

   mov ah, 0x03	
   mov bx, 0x0000
   int 0x10		

;---------------------------------------------
;INT 0x10   功能号:0x13   功能描述:显示字符串
;---------------------------------------------
;AH=0x13功能号
;AL=写模式
;BH=页码，BL=颜色
;CX=字符串长度
;DH=行，DL=列
;ES:BP=字符串偏移量 

   mov ax, message                ;es:bp指向字符串开头地址
   mov bp, ax
   mov cx, 0x10		             ;字符串长度，不包括结束符0的字符个数
   mov ax, 0x1301	                ;使用0x13号中断，AL=0x01，写字符串，字符跟随光标移动
   mov bx, 0x000a		             ; bh存储要显示的页号,此处是第0页,bl字符属性，0x02表示黑底绿字
   int 0x10		
   
   
                                  ;设置参数(开始扇区eax，写入内存地址bx，读出的扇区数1) 
   mov eax,LOADER_START_SECTOR    ;起始扇区LBA地址，第1扇区
   mov bx,LOADER_BASE_ADDR        ;写入的地址
   mov cx,4                       ;待读入的扇区数，setup包含4个扇区

   call rd_disk_m_16              ;读取程序的起始部分，1个扇区
   jmp LOADER_BASE_ADDR

;---------------------------------------------
;read disk in 16 bit mode 
;读取硬盘n个扇区
;eax=LBA扇区号
;bx=将数据写入的内存地址，0x900
;cx=读入的扇区数
;---------------------------------------------
rd_disk_m_16:
   mov esi,eax                    ;备份数据
   mov di,cx                        
   mov dx,0x1f2                   ;设置需要读取的扇区数
   mov al,cl
   out dx,al

   mov eax,esi                    ;恢复eax

   mov dx,0x1f3                   ;写入LBA low
   out dx,al

   mov cl,8                       ;写入LBA mid
   shr eax,cl
   mov dx,0x1f4
   out dx,al

   shr eax,cl                     ;写入LBA high
   mov dx,0x1f5
   out dx,al

   shr eax,cl                     ;设置LBA 24~27位
   and al,0x0f
   or al,0xe0                     ;31-28设置为1110表示LBA模式
   mov dx,0x1f6
   out dx,al

   mov dx,0x1f7                   ;0x17f端口写入读命令，0x20
   mov al,0x20
   out dx,al

.not_ready:                       ;检查硬盘状态
   nop
   in al,dx
   and al,0x88                    ;第4位1表示硬盘控制器准备好数据传输
                                  ;第7位1表示硬盘忙
   cmp al,0x08
   jnz .not_ready                 ;硬盘未就绪一直等待


   mov ax,di                      ;从0x1f0端口读数据
   mov dx,256                     ;每次2B(1W)512B读256次，di个扇区
   mul dx                         ;被乘数隐含在al或者ax中，if 16 bit result high 16 bits in dx，low 16 bits in ax
   mov cx,ax                      ;由于扇区数=1，每扇区读256次，所以只用ax即可
   mov dx,0x1f0                   ;需要回复dx的值，不然之后读数据会出问题
.continue_read:
   in ax,dx
   mov [bx],ax                   ;每次读入两个字节，地址+2，此处加载的程序不能超过64KB
   add bx,2
   loop .continue_read
   ret


message db "Welcome To LeOS!"
times 510-($-$$) db 0          ;扇区最后两个字节0xaa55，除代码和数据之外的内容用0填充
db 0x55,0xaa                   ;386小端模式

;第二扇区的数据读出来放在0x0000~0xffff这样跳转执行为什么还会执行待0x900位置，在call rd_rd_disk_m_16之前给了地址参数bx=Loa