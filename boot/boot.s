;boot.s ����������
;��һ��������ͨ��BIOS��ȡ���ڴ�0x7c00��Ȼ����ת��0x7c00λ��ִ�д���
;
%include "boot.inc"
section mbr vstart=0x7c00         ;ͨ��jmp 0x0000:0x7c00 ��ת��MBR����ʱ�μĴ���cs=0
   mov ax,cs      
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov fs,ax
   mov sp,0x7c00
   mov ax,0xb800                 ;�ı���ʾ����
   mov gs,ax

;-------------------------------------------
;INT 0x10   ���ܺ�:0x06	   ��������:�Ͼ���
;-------------------------------------------
;���룺
;AH ���ܺ�= 0x06
;AL = �Ͼ������(���Ϊ0,��ʾȫ��)
;BH = ������ɫ��ǰ����ɫ��0x07��ʾ���ֺڵ�
;(CL,CH) = �������Ͻǵ�(X,Y)λ��
;(DL,DH) = �������½ǵ�(X,Y)λ��
;�޷���ֵ��
   mov  ax, 0x600
   mov  bx, 0x700
   mov  cx, 0x0                 ; ���Ͻ�: (0, 0)
   mov  dx, 0x184f	           ; ���½�: (80,25),
			                       ; VGA�ı�ģʽ��,һ��ֻ������80���ַ�,��25�С�
			                       ; �±��0��ʼ,����0x18=24,0x4f=79
   int  0x10                    ; int 0x10

;---------------------------------------------
;INT 0x10   ���ܺ�:0x03   ��������:��ȡ���λ��
;---------------------------------------------
;���룺
;AH ���ܺ�= 0x03
;AL = 
;BX = ����ȡ����ҳ��
;���: 
;ch=��꿪ʼ��,cl=��������
;dh=��������к�,dl=��������к�

   mov ah, 0x03	
   mov bx, 0x0000
   int 0x10		

;---------------------------------------------
;INT 0x10   ���ܺ�:0x13   ��������:��ʾ�ַ���
;---------------------------------------------
;AH=0x13���ܺ�
;AL=дģʽ
;BH=ҳ�룬BL=��ɫ
;CX=�ַ�������
;DH=�У�DL=��
;ES:BP=�ַ���ƫ���� 

   mov ax, message                ;es:bpָ���ַ�����ͷ��ַ
   mov bp, ax
   mov cx, 0x10		             ;�ַ������ȣ�������������0���ַ�����
   mov ax, 0x1301	                ;ʹ��0x13���жϣ�AL=0x01��д�ַ������ַ��������ƶ�
   mov bx, 0x000a		             ; bh�洢Ҫ��ʾ��ҳ��,�˴��ǵ�0ҳ,bl�ַ����ԣ�0x02��ʾ�ڵ�����
   int 0x10		
   
   
                                  ;���ò���(��ʼ����eax��д���ڴ��ַbx��������������1) 
   mov eax,LOADER_START_SECTOR    ;��ʼ����LBA��ַ����1����
   mov bx,LOADER_BASE_ADDR        ;д��ĵ�ַ
   mov cx,4                       ;���������������setup����4������

   call rd_disk_m_16              ;��ȡ�������ʼ���֣�1������
   jmp LOADER_BASE_ADDR

;---------------------------------------------
;read disk in 16 bit mode 
;��ȡӲ��n������
;eax=LBA������
;bx=������д����ڴ��ַ��0x900
;cx=�����������
;---------------------------------------------
rd_disk_m_16:
   mov esi,eax                    ;��������
   mov di,cx                        
   mov dx,0x1f2                   ;������Ҫ��ȡ��������
   mov al,cl
   out dx,al

   mov eax,esi                    ;�ָ�eax

   mov dx,0x1f3                   ;д��LBA low
   out dx,al

   mov cl,8                       ;д��LBA mid
   shr eax,cl
   mov dx,0x1f4
   out dx,al

   shr eax,cl                     ;д��LBA high
   mov dx,0x1f5
   out dx,al

   shr eax,cl                     ;����LBA 24~27λ
   and al,0x0f
   or al,0xe0                     ;31-28����Ϊ1110��ʾLBAģʽ
   mov dx,0x1f6
   out dx,al

   mov dx,0x1f7                   ;0x17f�˿�д������0x20
   mov al,0x20
   out dx,al

.not_ready:                       ;���Ӳ��״̬
   nop
   in al,dx
   and al,0x88                    ;��4λ1��ʾӲ�̿�����׼�������ݴ���
                                  ;��7λ1��ʾӲ��æ
   cmp al,0x08
   jnz .not_ready                 ;Ӳ��δ����һֱ�ȴ�


   mov ax,di                      ;��0x1f0�˿ڶ�����
   mov dx,256                     ;ÿ��2B(1W)512B��256�Σ�di������
   mul dx                         ;������������al����ax�У�if 16 bit result high 16 bits in dx��low 16 bits in ax
   mov cx,ax                      ;����������=1��ÿ������256�Σ�����ֻ��ax����
   mov dx,0x1f0                   ;��Ҫ�ظ�dx��ֵ����Ȼ֮������ݻ������
.continue_read:
   in ax,dx
   mov [bx],ax                   ;ÿ�ζ��������ֽڣ���ַ+2���˴����صĳ����ܳ���64KB
   add bx,2
   loop .continue_read
   ret


message db "Welcome To LeOS!"
times 510-($-$$) db 0          ;������������ֽ�0xaa55�������������֮���������0���
db 0x55,0xaa                   ;386С��ģʽ

;�ڶ����������ݶ���������0x0000~0xffff������תִ��Ϊʲô����ִ�д�0x900λ�ã���call rd_rd_disk_m_16֮ǰ���˵�ַ����bx=Loa