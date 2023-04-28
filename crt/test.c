#include "crt.h"


int main(long argc,char* argv[]){
    int i;
    FILE* fp;
    char** v = malloc(argc * sizeof(char*));
    for(int i=0;i<argc;i++){
        v[i] = malloc(strlen(argv[i]) + 1);
        strcpy(v[i],argv[i]);
        // printf("%s\n",v[i]);
        
    }

    fp = fopen("test.txt","w");

    for(i=0;i<argc;i++){
        long len = strlen(v[i]);
        char buf[16];
        itoa(len,buf,10);
        printf("%d ",len);
        printf("%s\n",buf);
        // fwrite(&len,1,sizeof(long),fp);
        fprintf(fp,"%s",buf);
        fwrite(" ",1,sizeof(char),fp);
        fwrite(v[i],1,len,fp);
        fwrite("\n",1,sizeof(char),fp);
    }
    fclose(fp);

    fp = fopen("test.txt","r");
    //read部分有些问题
    for(i = 0;i<argc;i++){
        long len;
        char* buf;
        fread(&len,1,sizeof(int),fp);
        printf("%d ",len);
        buf = malloc(len + 1);
        fread(buf,1,len,fp);
        buf[len] = '\0';
        // printf("%d %s\n",len,buf);
        free(buf);
        free(v[i]);
    }
    fclose(fp);
}