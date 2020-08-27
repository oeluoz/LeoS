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
    
    times 12 dq 0                                               ;预留30个描述符空位，预留60个描述符，显示信息会出现问题
    
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
   mov [total_mem_bytes], edx   ;0x983
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

;创建页目录及页表并初始化页内存位图
    call setup_page
    sgdt [gdt_ptr]         ;将描述符表地址及偏移写入内存gdt_ptr

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
    mov byte [gs:160],'V'
    jmp $


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
    mov [PAGE_DIR_TABLE_POS+4092],eax
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
; message db "Loader!"

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
;    所以每次只能写入一个扇区，导致执行到第2扇区的非法代码