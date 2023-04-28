#ifndef __CRT_H__
#define __CRT_H__

#ifdef __cplusplus
extern "C" {
#endif

//内存管理
#ifndef NULL
#define NULL (0)
#endif


void free(void *);
void* malloc(unsigned long);
static long brk(void *);
long crt_init_heap(void);

//字符串处理
char* itoa(long n,char* str,long radix);
long strcmp(const char * src,const char * dst);
char *strcpy(char *dest,const char *src);
unsigned long strlen(const char * str);

//文件IO
typedef long FILE;

#define EOF (-1)
#define stdin ((FILE*)0)
#define stdout ((FILE*)1)
#define stderr ((FILE*)2)

//int 0x80中断不能显示64位的地址，所以不能用栈
long write(long fd,const void *buffer,unsigned long size);

long crt_init_io(void);
FILE* fopen(const char * filename,const char *mode);
long fread(void* buffer,long size,long count,FILE *strem);
long fwrite(const void* buffer,long size,long count,FILE *stream);
long fclose(FILE *fp);
long fseek(FILE* fp,long offset,long set);

//printf
long fputc(long c,FILE *stream);
long fputs(const char *str,FILE *stream);
long printf(const char *format,...);
long fprintf(FILE *stream,const char *format,...);

//internal
void do_global_ctors();
void crt_call_exit_routine();

//atexit
typedef void (*atexit_func_t)(void);
long atexit(atexit_func_t func);


#ifdef __cplusplus
}
#endif
#endif //__CRT_H__