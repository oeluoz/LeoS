#include "shell.h"
int shell() {
    pid_t pid;
    char cmd[50];
    char ** args;

    /*获取当前路径*/
    char *file_path_getcwd;
    file_path_getcwd=(char *)malloc(80);
    getcwd(file_path_getcwd,80);
    
    /*设置终端输出文字颜色*/
    printf("\033[1;32mleo@LAPTOP-MVSJ24RV:");
    printf("\033[35m%s",file_path_getcwd);
    printf("\33[0m$");
    gets(cmd);
    
    args = getParameter(cmd); /*通过一个二级指针返回，不能确定二级指针的长度，二级指针的最后一个元素是NULL*/
    
    pid = fork();
    if(pid == 0) { /*命令执行子线程，必须要等待子线程执行完毕父线程才退出*/
        execvp(args[0], args);  //最后加一个NULL作为参数的终结符号，参数就是完整指令，不能去掉开始的命令
    }else if(pid > 0) {
        wait();
        execv("/home/leo/c/Desktop/临时文件/OSExperiments/shell/shell",NULL);
    }else {
        printf("创建线程失败！\n");
    }
    return 0;
}

char ** getParameter(char* args) {
    int i,j,k,m,count;
    count = 0;
    char *temp;
    char** tempargs = (char**) malloc(sizeof(char*)*10);
    for(i=j=0; ; j++) { 
        if(args[j] == ' ' || args[j] == '\0') {
            temp = (char *)malloc(sizeof(char)*10);
            for(k = 0; i<j ;) { /*获取一个参数*/
                temp[k++] = args[i++];
            }
            temp[k] = '\0';
            tempargs[count++] = temp;
            i = j + 1;
            if(args[j] == '\0') 
                break;
        }
    }
    char**parameter = (char**) malloc(sizeof(char*)*(count+1));
    for(int i=0; i<count; i++) {
        parameter[i] = tempargs[i];
    }
    tempargs[count]=NULL;
    return parameter;
}

int main() {
    shell();
    return 0;
}
