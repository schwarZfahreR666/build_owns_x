#include "crt.h"

extern int main(long argc,char* argv[]);
void exit(long);

static void crt_fatal_error(const char* msg){
    // printf("fatal error: %s", msg);
    exit(1);
}

void crt_entry(void){
    long ret;
    long argc;
    char** argv;

    //参数处理
    char* ebp_reg = 0;
    //ebp_reg = %ebp
    asm("movq %%rbp,%0 \n":"=r"(ebp_reg));
    //从栈顶（ebp位置）向上+4 +8分别为上层函数的argc和argv,64bit系统需要调整为+8，+16
    argc = *(long*)(ebp_reg + 8);
    argv = (char**) (ebp_reg + 16);
    
    //初始化堆
    if(!crt_init_heap()) crt_fatal_error("heap initialize failed");

    //初始化IO
    if(!crt_init_io()) crt_fatal_error("IO initialize failed");
    
    //调用main函数
    ret = main(argc,argv);

    exit(ret);
}

void exit(long exit_code){
    //crt_call_exit_routine();

    asm("movq %0,%%rbx \n\t"
    "movq $1,%%rax \n\t"
    "int $0x80 \n\t"
    "hlt \n\t"::"m"(exit_code));

}