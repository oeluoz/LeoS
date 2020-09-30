;setup.s
;引导扇区之后的四个扇区，进入保护模式

%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR
    jmp loader_start

    ;4个段描述符
    GDT_BASE: dd 0x00000000 
              dd 0x00000000
    CODE_DESC: dd 0x0000FFFF 
               dd DESC_CODE_HIGH4                               ;代码段描述符
    DATA_STACK_DESC: dd 0x0000FFFF 
                     dd DESC_DATA_HIGH4                         ;栈段描述符，数据段的选择子之前type错误，造成了反复启动的问题，
    VIDEO_DESC: dd 0x80000007                                    ;段寄存器在加载选择子的时候会对type进行检查，所以type错误导致一直重启
               dd DESC_VIDEO_HIGH4                             ;段界限=(0xbffff-0xb8000)/4k=0x7

    ;GDT界限
    GDT_SIZE equ $-GDT_BASE
    GDT_LIMIT equ GDT_SIZE-1                                    ;GDT界限
    
    times 12 dq 0                                               
    
    ;内存容量存放位置
    total_mem_bytes dd 0                                        ;(4+12)*8=0x80 0x900+0x80=0x980 之前还有jmp指令，占用3B所以在0x983位置

    ;选择子，第0个段描述符是保留的
    SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0
    SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
    SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

    ;GDT指针，存放在GDTR
    gdt_ptr dw GDT_LIMIT 
            dd GDT_BASE

    ;total_mem_bytes + gdt_pr + ards_buf + ards_nr = 4+6+244+2=256B
    ards_buf times 144 db 0
    ards_nr dw 0                ;用来记录ARDS结构体数量
loader_start:
    xor ebx, ebx		      ;第一次调用时，ebx值要为0
    mov edx, 0x534d4150	      ;edx只赋值一次，循环体中不会改变
    mov di, ards_buf	      ;ards结构缓冲区
.e820_mem_get_loop:	      ;循环获取每个ARDS内存范围描述结构
    mov eax, 0x0000e820	      ;执行int 0x15后,eax值变为0x534d4150,所以每次执行int前都要更新为子功能号。
    mov ecx, 20		      ;ARDS地址范围描述符结构大小是20字节
    int 0x15
; jc .error   ;若cf位为1则有错误发生，尝试0xe801子功能
    add di, cx		      ;使di增加20字节指向缓冲区中新的ARDS结构位置
    inc word [ards_nr]	      ;记录ARDS数量
    cmp ebx, 0		      ;若ebx为0且cf不为1,这说明ards全部返回，当前已是最后一个
    jnz .e820_mem_get_loop

    ;在所有ards结构中，找出(base_add_low + length_low)的最大值，即内存的容量。
    mov cx, [ards_nr]	      ;遍历每一个ARDS结构体,循环次数是ARDS的数量
    mov ebx, ards_buf 
    xor edx, edx		      ;edx为最大的内存容量,在此先清0
.find_max_mem_area:	      ;无须判断type是否为1,最大的内存块一定是可被使用
    mov eax, [ebx]	      ;base_add_low
    add eax, [ebx+8]	      ;length_low
    add ebx, 20		      ;指向缓冲区中下一个ARDS结构
    cmp edx, eax		      ;冒泡排序，找出最大,edx寄存器始终是最大的内存容量
    jge .next_ards
    mov edx, eax		      ;edx为总内存大小
.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok
.mem_get_ok:
    mov [total_mem_bytes], edx   ;内存总量存放在0x983
    
;开启A20地址线
    in al,0x92
    or al,0000_0010B
    out 0x92,al
;加载GDT
    lgdt [gdt_ptr]
;开启CR0 PE
    mov eax,cr0
    or eax,0x00000001
    
    mov cr0,eax
;刷新流水线
    jmp dword SELECTOR_CODE:p_mode_start                ;dword指明偏移地址是32bit

; 开始在实模式下执行
[bits 32]
p_mode_start:
    mov ax,0
    mov ax,SELECTOR_DATA                
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,LOADER_BASE_ADDR                            
    mov ax,SELECTOR_VIDEO
    mov gs,ax
    mov byte [gs:160],'p'
    mov byte [0xb8000],'A'
    

;加载内核到0x7000
    mov eax,KERNEL_START_SECTOR         ;内核所在扇区号
    mov ebx,KERNEL_BASE_ADDR            ;内核加载的目标地址
    mov ecx,200                         ;读出kernel扇区数
    call rd_disk_m_32                   ;保护模式下读取硬盘

;创建页目录及页表并初始化页内存位图
    call setup_page
    sgdt [gdt_ptr]         ;将描述符表地址及偏移写入内存gdt_ptr
;修改显存的段描述符的段基址，内核在3GB空间以上，打印功能在内核中实现，显存的段基址需要改为3GB以上才可以
    mov ebx,[gdt_ptr+2]    ;gdt base
    or dword [ebx+0x18+4],0xc0000000         ;0x18=3*8，段描述符的高4B的最高位是段基址的32~24位
    add dword [gdt_ptr+2],0xc0000000         ;将gdt的基址+0xc000000使其成为内核所在的高地址
    
    add esp,0xc0000000                       ;将栈指针用射到内核地址

   
;将页目录地址赋给cr3
    mov eax,PAGE_DIR_TABLE_POS
    mov cr3,eax
;打开cr0的pg位
    mov eax,cr0
    or eax,0x80000000
    mov cr0,eax
;开启分页后，用gdt新的地址加载
    lgdt [gdt_ptr]
    jmp SELECTOR_CODE:enter_kernel          ;保护模式下重新刷新流水线                                ;3
enter_kernel:
   
    call kernel_init
    mov byte [gs:164],'T'
    mov esp,0xc009f000
    jmp KERNEL_ENTRY_POINT
    
; 设置页表
setup_page:
    mov ecx,4096                        ;将PDT所在的内存空间清零
    mov esi,0
.clear_page_loop:
    mov byte [PAGE_DIR_TABLE_POS+esi],0
    inc esi
    loop .clear_page_loop
.create_pde:                            ;创建页目录项PDE
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x1000                      ;第一个页表
    mov ebx,eax                         ;ebx为第一个页表的基址
    or eax,PG_US_U|PG_RW_W|PG_P
    mov [PAGE_DIR_TABLE_POS],eax        ;第一个页目录项
    mov [PAGE_DIR_TABLE_POS+0xc00],eax
    ;页目录项0xc00以上的目录项用于内核空间：页表的0xc0000000
    ;-0xffffffff共计1G空间属于内核
    ;0x0-0xffffffff共计3G空间属于用户进程
    ;这里将页目录项0和0xc00都存为了第一个页表的地址
    ;将来指向0x0~0xfffff，只是1MB的空间，其余3MB的空间没有分配

    ;两处指向同一个页表的原因：加载内核之前，程序一直运行的是loader，它本身的代码在1MB之内
    ;必须保证之前段机制下的线性地址和分页后的虚拟地址对应的物理地址一致，用PDE0来保证loader在分页机制下运行正确
    ;放置PDE768的原因是操作系统内核放置在低端1MB空间之内，但是操作虚拟地址空间是在高1GB空间，保证3GB以上的虚拟地址
    ;对应低端的1MB内核空间
    sub eax,0x1000
    mov [PAGE_DIR_TABLE_POS+4092],eax           ;最后一个页表项指向的4K页表是它自己。自己有1024个页表项(目录项)
;创建第 0个页表的PTE：用来映射1MB内存空间
    mov ecx,256               ;1MB低端内存的PTE
    mov esi,0
    mov edx,PG_US_U|PG_RW_W|PG_P
.create_pte:
    mov [ebx+esi*4],edx       ;ebx是第一个页表的基址
    add edx,0x1000            ;PDE存放的都是页表基址，每个页表4K
    inc esi
    loop .create_pte          ;以上是1MB内存空间的特殊页表，页表目录PDE0和PDE768都指向了这个1MB的内存空间
;创建内核其他页表的PDE 
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x2000            ;eax为第二个页表的位置
    or eax,PG_US_U|PG_RW_W|PG_P
    mov ebx,PAGE_DIR_TABLE_POS
    mov ecx,254               ;范围为769~1022的所有目录项数量，最后一个PDE指向的是PDT自身，内核空间实际为1GB-4MB
    mov esi,769               ;PDE的起始位置,之所以将页目录项定下来的目的是保证每个用户进程共享所有的内核空间
.create_kernel_pde:
    mov [ebx+esi*4],eax
    inc esi
    add eax,0x1000
    loop .create_kernel_pde
    ret  
;将kernel.bin中的segment拷贝到编译地址
kernel_init:
    xor eax,eax               ;
    xor ebx,ebx               ;记录程序头表地址
    xor ecx,ecx               ;记录程序头表中的program_header数量
    xor edx,edx               ;记录program_header 尺寸 即为 e_phentsize

    mov dx,[KERNEL_BASE_ADDR+42]    ;偏移42字节是e_phentsize，表示一个段头的大小
    mov ebx,[KERNEL_BASE_ADDR+28]    ;e_phoff 表示第一个program header 在文件中的偏移量
    add ebx,KERNEL_BASE_ADDR        ;指向程序头表的物理地址
    mov cx,[KERNEL_BASE_ADDR+44]    ;e_phnum program header数量(段的数量)
.each_segment:
    cmp byte [ebx+0],PT_NULL
    je .PT_NULL                     ;p_type等于PT_NULL说明program header未使用

    test dword [ebx+8],0x80000000   ;检测segment的type是不是load，且必须加载的目标地址要是0xc0000000以上的（因为其他虚拟地址暂时未分配！）
    jz .PT_NULL
    test dword [ebx+8],0x40000000
    jz .PT_NULL

    push dword [ebx+16]             ;program header 基址偏移16B是p_filesz
                                    ;memcpy(dst,src,size)第3个参数
    mov eax,[ebx+4]                 ;program header基址偏移4B是p_offset
    add eax,KERNEL_BASE_ADDR        ;加上被加载到的物理地址，即为改段的物理地址
    push eax                        ;第2个参数 src
    push dword [ebx+8]              ;第1个参数 dst
                                    ;基址偏移8B是p_vaddr，即目的地址
    call mem_cpy
    add esp,12                      ;参数出栈
.PT_NULL:
    add ebx,edx                     ;edx为program header大小，即为e_phentsize
    loop .each_segment
    ret
mem_cpy:
    cld                             ;esi+移动字节数方向
    push ebp
    mov ebp,esp
    push ecx

    mov edi,[ebp+8]                 ;dst
    mov esi,[ebp+12]                ;src
    mov ecx,[ebp+16]                ;size

    rep movsb
; 测试：将rep movsb用循环实现，发现edi是页表没有初始化的内存
;     push eax                       ;逐字节拷贝
; .move_loop:
;     mov byte al,[ds:esi]
;     mov byte [es:edi],al
;     inc esi
;     inc edi
;     loop .move_loop
;     pop eax
    pop ecx
    pop ebp
    ret

rd_disk_m_32:	   
; eax=LBA扇区号
; ebx=将数据写入的内存地址
; ecx=读入的扇区数
      mov esi,eax	   ; 备份eax
      mov di,cx		   ; 备份扇区数到di
;读写硬盘:
;第1步：设置要读取的扇区数
      mov dx,0x1f2
      mov al,cl
      out dx,al            ;读取的扇区数

      mov eax,esi	   ;恢复ax

;第2步：将LBA地址存入0x1f3 ~ 0x1f6

      ;LBA地址7~0位写入端口0x1f3
      mov dx,0x1f3                       
      out dx,al                          

      ;LBA地址15~8位写入端口0x1f4
      mov cl,8
      shr eax,cl
      mov dx,0x1f4
      out dx,al

      ;LBA地址23~16位写入端口0x1f5
      shr eax,cl
      mov dx,0x1f5
      out dx,al

      shr eax,cl
      and al,0x0f	   ;lba第24~27位
      or al,0xe0	   ; 设置7～4位为1110,表示lba模式
      mov dx,0x1f6
      out dx,al

;第3步：向0x1f7端口写入读命令，0x20 
      mov dx,0x1f7
      mov al,0x20                        
      out dx,al


;第4步：检测硬盘状态
  .not_ready:		   ;测试0x1f7端口(status寄存器)的的BSY位
      ;同一端口,写时表示写入命令字,读时表示读入硬盘状态
      nop
      in al,dx
      and al,0x88	   ;第4位为1表示硬盘控制器已准备好数据传输,第7位为1表示硬盘忙
      cmp al,0x08
      jnz .not_ready	   ;若未准备好,继续等。

;第5步：从0x1f0端口读数据
      mov ax, di	   ;以下从硬盘端口读数据用insw指令更快捷,不过尽可能多的演示命令使用,
      mov dx, 256	   ;di为要读取的扇区数,一个扇区有512字节,每次读入一个字,共需di*512/2次,所以di*256
      mul dx
      mov cx, ax	   
      mov dx, 0x1f0
  .go_on_read:
      in ax,dx		
      mov [ebx], ax
      add ebx, 2
			  ; 由于在实模式下偏移地址为16位,所以用bx只会访问到0~FFFFh的偏移。
			  ; loader的栈指针为0x900,bx为指向的数据输出缓冲区,且为16位，
			  ; 超过0xffff后,bx部分会从0开始,所以当要读取的扇区数过大,待写入的地址超过bx的范围时，
			  ; 从硬盘上读出的数据会把0x0000~0xffff的覆盖，
			  ; 造成栈被破坏,所以ret返回时,返回地址被破坏了,已经不是之前正确的地址,
			  ; 故程序出会错,不知道会跑到哪里去。
			  ; 所以改为ebx代替bx指向缓冲区,这样生成的机器码前面会有0x66和0x67来反转。
			  ; 0X66用于反转默认的操作数大小! 0X67用于反转默认的寻址方式.
			  ; cpu处于16位模式时,会理所当然的认为操作数和寻址都是16位,处于32位模式时,
			  ; 也会认为要执行的指令是32位.
			  ; 当我们在其中任意模式下用了另外模式的寻址方式或操作数大小(姑且认为16位模式用16位字节操作数，
			  ; 32位模式下用32字节的操作数)时,编译器会在指令前帮我们加上0x66或0x67，
			  ; 临时改变当前cpu模式到另外的模式下.
			  ; 假设当前运行在16位模式,遇到0X66时,操作数大小变为32位.
			  ; 假设当前运行在32位模式,遇到0X66时,操作数大小变为16位.
			  ; 假设当前运行在16位模式,遇到0X67时,寻址方式变为32位寻址
			  ; 假设当前运行在32位模式,遇到0X67时,寻址方式变为16位寻址.
      loop .go_on_read
      ret

;log 第一次测试，没有显示Loader信息，初步判断问题出在boot.s没有将第二个扇区读出
;    第二次测试，没有正确显示字符串，初步判定是访问内存的问题，问题原因
;    setup测试，没有正常跳转，初步判定是跨越段了，但是感觉不应该 问题原因：开始预留60描述符，每个4B超过近跳转的范围
;但是更改代码之后jmp near跳转范围改变也没有任何作用，修改成50仍然可以运行，非常奇怪的bug，可能是存放字符串的问题
;    第四次测试，中断正常运行，但是0x980位置没有内存信息
;    第五次测试：未知原因读取内存导致错误
;    不知道是不是超过512字节就会出错，减少ards_buf的大小程序就可以正常运行
;    存放最终字节数是在0x983，原因是代码增加一条跳转指令，3B
;     I giorni Andante


;    if=setup.bin of=LeoS.img bs=512 count=3 seek=1 conv=notrunc编译参数中将count设置成了1，
;    所以每次只能写入一个扇区，导致执行到第2扇区的非法代码，以上奇怪错误均为这个错误导致
;    第六次测试：开启分页并且读取内核到内存，出现错误：在进行流水线刷新的时候指令不知道到哪去了，问题出现在mem_cpy执行之后跳转到一个
;    未知的内存位置
;    第七次测试：在移动之后系统就重启了，跳转到BIOS的起始位置执行，初步判定是移动内核映像导致系统重启
;    测试文件
;    0xc0001000
;    0x08048000 增加test 具体原理不明·
;    rep movsb不能完整执行代码，用循环可以执行完毕，未知原因 重新编译解决问题