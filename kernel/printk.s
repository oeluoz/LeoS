;字符串输出
TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0      ;在setup已经初始化video选择子，为了按安全

[bits 32]
section .data
buffer dq 0            ;定义8B存放数字到字符的结果
section .text
global print
global println
global printint
global set_cursor
printint:
    pushad
    mov ebp,esp
    mov eax,[ebp+4*9]
    mov edx,eax
    mov edi,7           ;指定在buffer中的初始偏移量
    mov ecx,8           ;32位数字，16进制数字位数为8
    mov ebx,buffer
.Hex4Bits:
    and edx,0x0000000F
    cmp edx,9           ;大于10
    jg .isA2F
    add edx,'0'
    jmp .store
.isA2F:
    sub edx,10
    add edx,'A'
.store:
    mov [ebx+edi],dl    ;ASCII 8bit表示
    dec edi
    shr eax,4           ;右移4bit处理新的4bit
    mov edx,eax
    loop .Hex4Bits
    mov edi,0
.each_print:
    mov cl ,[buffer+edi]
    push ecx
    call print
    add esp,4
    inc edi
    cmp edi,8
    jl .each_print
    popad
    ret
println:
    push ebx
    push ecx
    xor ecx,ecx
    mov ebx,[esp+12]   ;字符串首地址
.goon:
    mov cl,[ebx]
    cmp cl,0           ;'\0'作为字符串结尾
    jz .over
    push ecx           ;为print传递参数
    call print         
    add esp,4          ;回收参数
    inc ebx
    jmp .goon
.over:
    pop ecx
    pop ebx
    ret
print:
    pushad             ;push all double EAX->ECX->
                       ;EDX->EBX-> ESP-> EBP->ESI->EDI顺序入栈
    mov ax,SELECTOR_VIDEO
    mov gs,ax
    
    ;读取光标位置
    mov dx,0x03d4      ;0x03d4端口输入索引0x0e(获取光标索引)
    mov al,0x0e
    out dx,al
    mov dx,0x03d5      ;0x03d5输出光标位置高8bit到al
    in al,dx
    mov ah,al

    mov dx,0x03d4      ;光标低8bit
    mov al,0x0f
    out dx,al
    mov dx,0x03d5      
    in al,dx

    mov bx,ax
    mov ecx,[esp+36]    ;寄存器4*8+返回地址4=36B

    cmp cl,0xd          ;CR=0x0d LF=0x0a
    jz .isCR
    cmp cl,0xa
    jz .isLF

    cmp cl,0x8          ;判断是否回车
    jz .isBackspace
    jmp .print_other          ;输出其他能够显示的字符
.isBackspace:           ;backspace光标移动到显存的前一个位置，然后增加一个空格覆盖
    dec bx
    shl bx,1
    mov byte [gs:bx],0x20
    inc bx
    mov byte [gs:bx],0xa
    shr bx,1
    jmp .setCursor
.print_other:
    shl bx,1
    mov byte [gs:bx],cl
    inc bx
    mov byte [gs:bx],0x0a
    shr bx,1
    inc bx
    cmp bx,2000         ;判断是否还在屏幕范围之内，否则需要刷新屏幕
    jl .setCursor       ;如果超出屏幕范围进行滚屏
.isLF:                  ;\n

.isCR:                  ;\r回车和换行都处理成回车+换行
    xor dx,dx
    mov ax,bx
    mov si,80

    div si              ;计算行数，dx中保存计算的余数
    sub bx,dx           ;回到行首
.isCREnd:               ;单独的判断最后位置是否需要滚屏
    add bx,80           ;到下一行
    cmp bx,2000
.isLFEnd:
    jl .setCursor
.roll_screen:
    cld                 ;esi+方向
    mov ecx,960         ;2000-80=1920字符，共1920*2=3840B，每次4B
    mov esi,0xc00b80a0  ;第1行行首
    mov edi,0xc00b8000  ;第0行行首
    rep movsd
    
    mov ebx,3840        ;最后一行全部用空格填充
    mov ecx,80
.cls:
    mov word [gs:ebx],0x0a20
    add ebx,2
    loop .cls

    mov bx,1920         ;设置光标为最后一行行首
.setCursor:
    mov dx,0x03d4      
    mov al,0x0e
    out dx,al
    mov dx,0x03d5      ;0x03d5输出光标位置高8bit到al
    mov al,bh
    out dx,al

    mov dx,0x03d4      ;光标低8bit
    mov al,0x0f
    out dx,al
    mov dx,0x03d5      
    mov al,bl
    out dx,al
.printDone:
    popad
    ret
;设置光标位置
set_cursor:
   pushad
   mov bx, [esp+36] ;8个寄存器，1个函数返回值，以上为参数

   mov dx, 0x03d4			  ;索引寄存器
   mov al, 0x0e				  ;用于提供光标位置的高8位
   out dx, al
   mov dx, 0x03d5			  ;通过读写数据端口0x3d5来获得或设置光标位置 
   mov al, bh
   out dx, al


   mov dx, 0x03d4
   mov al, 0x0f
   out dx, al
   mov dx, 0x03d5 
   mov al, bl
   out dx, al
   popad
   ret