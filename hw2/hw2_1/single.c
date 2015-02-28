/*************************
 *EEL5934 ADV.SYSTEM.PROG
 *Juncheng Gu 5191-0572
 *Assignment2 single
 *Jan 26, 2015
 *************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>

#define COMMAND_LINE_LENGTH 1024
#define COMMAND_MIN_LENGTH 4
#define COMMAND_LENGTH 11
#define NUM_ARGS 5

struct cmd{
    int in;
    int out;
    int action; //1revert -1zero
    char bits[8];//1revert -1zero 0non-action
    char *block;
    int sizeblock;
    int sizebuff;
};

int copyFile(FILE *in, FILE* out);
int getLine(FILE *fd, char *str, int max);
int getArguments(char *, char *[]);
int toInt(char *str);
int blockRead(char*, int in, int sizeBlock, FILE*);
int blockWrite(char*, int out, int sizeBlock, FILE*);
int commitAction(struct cmd*, int);
int updateAction(struct cmd* des, struct cmd* src);

/* actions */
//int revert2(char a[], int n, int b[]);
//int revert(struct cmd*);
//int zero2(char a[], int n, int b[]);
//int zero(struct cmd*);
int convertArgu(struct cmd* ac, int len, char *arguments[], int, int, int);

void printfcmd(struct cmd a){
    int i =0;
    printf("  ###  in %d, out %d, action %d ", a.in, a.out, a.action);
    for (i = 0; i < 8; i++){
        printf(",%d", a.bits[i]);
    }
    printf(" , block %s,", a.block); 
    printf("\n");
}
void printfbitmap(int *map, int len){
    int i =0;
    printf("bitmap: ");
    for (i = 0; i < len; i++){
        printf("%d,", map[i]);
    }
    printf("\n");
}



int main(int argc, char *argv[]){
    char commandLine[COMMAND_LINE_LENGTH+1];    
    char *arguments[COMMAND_LENGTH+1];
    int i, j;
    int sizeBuff = 0;
    int sizeBlock = 0;
    int sizeFile = 0;
    int blockFile = 0;
    int lenArgu = 0;
    int use_num = 0; //current empty buffer item;
    int in = -1, out = -1;
    FILE *fd[3];   //0=in, 1=out, 2=instr
    struct cmd *buffer;
    int *buff_out, *buff_in; // mapping out-buffer[] in-buffer[]
    struct cmd *tmp;
    char test[4];
    /* prepare arguments */
    if (argc != NUM_ARGS + 1){
        printf("Error wrong arguments array!\n");
        return 0;
    }
    sizeBuff = toInt(argv[5]);
    sizeBlock = toInt(argv[4]);
    if (sizeBuff <= 0 || sizeBlock <= 0){
        printf("Wrong sizeBuff or sizeBlock !\n");
        return 0;
    }
    if ((argv[1] != NULL) && (argv[2] != NULL) && (argv[3] != NULL)){
        fd[0] = fopen(argv[2], "r"); //in
        fd[1] = fopen(argv[3], "w"); //out 
        fd[2] = fopen(argv[1], "r"); //instr
        if ( fd[0] == NULL || fd[1] == NULL || fd[2] == NULL){
            printf("Error occur in opening file!\n");
            return 0;
        }
    }else{
        printf("Wrong filename arguments!\n");
        return 0;
    }
    /*init*/
     /*copy in -> out*/
    sizeFile = copyFile(fd[0], fd[1]);
    blockFile = sizeFile / sizeBlock;
    if (sizeFile % sizeBlock != 0){
        blockFile += 1;
    }
    printf("sizeFile is %d bytes, blockFile is %d blocks\n", sizeFile, blockFile);
    buffer = (struct cmd*)malloc(sizeBuff * sizeof(struct cmd));
    buff_out = (int*)malloc((blockFile+1)*sizeof(int)); //[0]x, [1] ~ [blockFile], mapping out---buffer index
    buff_in = (int*)malloc((blockFile+1)*sizeof(int)); //mapping in---buffer index
    tmp = (struct cmd*)malloc(sizeof(struct cmd));
    (*tmp).block = (char*)malloc(sizeof(char)*sizeBlock);
    for (i=0; i < sizeBuff; i++){
        buffer[i].block = (char*)malloc(sizeof(char)*sizeBlock);
        buffer[i].in = -1;
        buffer[i].out = -1;
    }
    for (i=1; i <= blockFile; i++){
        buff_out[i] = -1;  
        buff_in[i] = -1;  
    }
    for (i=0; i < 8; i++){
        (*tmp).bits[i] = 0;
    }
    printf("Instr %s, Input %s, Output %s, Block %d, Buffer %d \n", argv[1], argv[2], argv[3], sizeBlock, sizeBuff); 

    //action perform
    while (getLine(fd[2], commandLine, COMMAND_LINE_LENGTH+1) != -1){
        /*analysis action*/
        lenArgu = getArguments(commandLine, arguments);
        if (lenArgu < 0){
            return 0;
        }
        if (strcmp(arguments[0], "revert") == 0){
            //revert
            (*tmp).action = 1;
            if (convertArgu(tmp, lenArgu, arguments, sizeBlock, sizeBuff, blockFile) < 0){
                printf("Error occured in arguments convert\n");
                return 0;
            }
            printfcmd(*tmp);             
        }else if (strcmp(arguments[0], "zero") == 0){
            //zero
            (*tmp).action = -1;
            if (convertArgu(tmp, lenArgu, arguments, sizeBlock, sizeBuff, blockFile) < 0){
                printf("Error occured in arguments convert\n");
                return 0;
            }
            printfcmd(*tmp);
        }else{
            printf("Unknow action %s\n", arguments[0]);
            return 0;
        }
        //depedency analysis, suppose there no data lose
        in = (*tmp).in;
        out = (*tmp).out;
        if (buff_in[in] >= 0){ // ni in buffer
            if (buff_out[out] >= 0 ){ //no in buffer
                if (buffer[buff_in[in]].out == -1 || buffer[buff_in[in]].out == out){ // don't filewrite buffer[in]
                    //no write to file, update block[in]
                    updateAction(buffer+buff_in[in], tmp);
                    commitAction(buffer+buff_in[in], sizeBlock);
                    buffer[buff_out[out]].out = -1;
                    buffer[buff_in[in]].out = out;
                    buff_out[out] = buff_in[in];
                }else{ //write buffer[in]
                    //file write
                    blockWrite(buffer[buff_in[in]].block, buffer[buff_in[in]].out, sizeBlock, fd[1]); 
                    buff_out[buffer[buff_in[in]].out] = -1;
                    buffer[buff_in[in]].out = out;
                    buffer[buff_out[out]].out = -1;
                    buff_out[out] = buff_in[in]; 
                    updateAction(buffer+buff_in[in], tmp);
                    commitAction(buffer+buff_in[in], sizeBlock);
                }
            }else{ //no out of buffer
                if (buffer[buff_in[in]].out == -1){ // don't filewrite buffer[in]
                    //no filewrite, update block[in]
                    updateAction(buffer+buff_in[in], tmp);
                    commitAction(buffer+buff_in[in], sizeBlock);
                    buffer[buff_in[in]].out = out;
                    buff_out[out] = buff_in[in];
                }else{ //first write buffer[in], then update
                    //file write
                    blockWrite(buffer[buff_in[in]].block, buffer[buff_in[in]].out, sizeBlock, fd[1]); 
                    buff_out[buffer[buff_in[in]].out] = -1;
                    buffer[buff_in[in]].out = out;
                    buff_out[out] = buff_in[in];
                    updateAction(buffer+buff_in[in], tmp);
                    commitAction(buffer+buff_in[in], sizeBlock);
                }
            }
        }else{  //ni out of buffer
            //import in to buffer
            blockRead(buffer[use_num].block, in, sizeBlock, fd[0]);
            buffer[use_num].in = in;
            buffer[use_num].out = out;
            buff_in[in] = use_num;
            if (buff_out[out] >= 0){ //no in buffer
                buffer[buff_out[out]].out = -1;
                buff_out[out] = use_num; 
            }else{ //no out of buffer
                buff_out[out] = use_num;
            }
            updateAction(buffer+use_num, tmp);
            commitAction(buffer+use_num, sizeBlock);
            use_num += 1;
        }
        printfcmd(buffer[buff_in[in]]);             
        printfbitmap(buff_in, blockFile);
        printfbitmap(buff_out, blockFile);
        printf("          use_num %d\n", use_num);
    }
    
    for(i = 0; i < use_num; i++){
        if (buffer[i].out >= 0){
            //write 
            blockWrite(buffer[i].block, buffer[i].out, sizeBlock, fd[1]);
        }
    }
   
    fclose(fd[0]);
    fclose(fd[1]);
    fclose(fd[2]);
    return 0;
}

/******************* function *****************/
int getArguments(char *str, char *arg[]){
    char delimeter[] = " ";
    char *temp = NULL;
    int i = 0;
    temp = strtok(str, delimeter);
    while (temp != NULL){
        arg[i++] = temp;
        printf("%s, ", temp);
        temp = strtok(NULL, delimeter);
    }
    if (i < COMMAND_MIN_LENGTH){
        printf("Command: %s wrong!\n", str);
        return -1;
    }
    arg[i] = NULL;
    printf("\n");
    return i;
}

int getLine(FILE *fd, char *str, int max){
    char *temp;
    if ((temp = fgets(str, max, fd)) != NULL){
        int len = strlen(str);
        str[len-1] = '\0';
        printf("cmdline %s, len %d    ", str, len-1);
        return len-1;
    }else
        return -1;
}

int toInt(char str[]){
    int res = 0;
    int i = 0;
    while (str[i] != '\0'){
        if (str[i] < 48 || str[i] > 57){
            printf("Wrong num %s !\n", str);
            return -1;
        }else{
            i++;
        }
    }
    res = atoi(str);
    return res;
}


int copyFile(FILE* in, FILE* out){
    int num = 0;
    char tmp;
    tmp = fgetc(in);
    while (tmp != EOF){
        fputc(tmp, out);
        tmp = fgetc(in);
        num++;
    }
    return num;
}


int convertArgu(struct cmd* ac, int len, char *arguments[], int block, int buff, int numblock){
    int i = 0;
    int tmp;
    for (i =0; i < 8; i++){
        (*ac).bits[i] = 0;
    }
    if ((tmp = toInt(arguments[1])) < 0){
        printf("Error input file block number!\n");
        return -1;
    }else{
        if (tmp > numblock){
            printf("Wrong number of block index !\n");
            return -1;
        }
        (*ac).in = tmp;
    }
    if ((tmp = toInt(arguments[2])) < 0){
        printf("Error output file block number!\n");
        return -1;
    }else{
        if (tmp > numblock){
            printf("Wrong number of block index !\n");
            return -1;
        }
        (*ac).out = tmp;
    }

    for (i = 3; i < len; i++){
        tmp = toInt(arguments[i]);
        if (tmp < 0 || tmp > 7){
            printf("Error bit number!\n");
            return -1;
        }
        (*ac).bits[tmp] = (*ac).action; //1convert -1zero
    }
    (*ac).sizeblock = block;
    (*ac).sizebuff = buff;
    return 1;
}
 /**** for test
int revert2(char a[], int n, int b[]){
    int i =0, j = 0;
    char tmp;
    char con[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    for (i=0; i< n; i++){
        printf("old:0x%hhx, ", a[i]);
        tmp = a[i];
        for (j = 0; j < 8; j++){
            printf("%d[", b[j]);
            if (b[j] == 1){
                tmp = tmp ^ con[j];
                printf("%hhx],",tmp);
            }
        }
        a[i] = tmp;
        printf(" new:0x%hhx\n ", a[i]); 
    } 
}
int zero2(char a[], int n, int b[]){
    int i =0, j = 0;
    char tmp;
    char con[8] = {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};
    for (i=0; i< n; i++){
        printf("old:0x%hhx, ", a[i]);
        tmp = a[i];
        for (j = 0; j < 8; j++){
            printf("%d[", b[j]);
            if (b[j] == 1){
                tmp = tmp & con[j];
                printf("%hhx],",tmp);
            }
        }
        a[i] = tmp;
        printf(" new:0x%hhx\n ", a[i]); 
    } 
}

int revert(struct cmd* r){
    int i =0, j = 0;
    char tmp;
    char con[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    for (i=0; i< (*r).sizeblock; i++){
        tmp = (*r).block[i];
        printf("old:0x%hhx, ", tmp);
        for (j = 0; j < 8; j++){
            if ((*r).bits[j] == 1){
                tmp = tmp ^ con[j];
            }
        }
        (*r).block[i] = tmp;   
        printf(" new:0x%hhx\n ", (*r).block[i]);
    } 
    return 1;
}
int zero(struct cmd* r){
    int i =0, j = 0;
    char tmp;
    char con[8] = {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};
    for (i=0; i< (*r).sizeblock; i++){
        tmp = (*r).block[i];
        printf("old:0x%hhx, ", tmp);
        for (j = 0; j < 8; j++){
            if ((*r).bits[j] == 1){
                tmp = tmp & con[j];
            }
        }
        (*r).block[i] = tmp;   
        printf(" new:0x%hhx\n ", (*r).block[i]);
    } 
    return 1;
}
*/

int blockRead(char* buff, int in, int sizeBlock, FILE* stream){
    fseek(stream, (in-1)*sizeof(char)*sizeBlock, SEEK_SET);
    fread(buff, sizeBlock, 1, stream); 
    return 1;
}

int blockWrite(char* buff, int out, int sizeBlock, FILE* stream){
    fseek(stream, (out-1)*sizeof(char)*sizeBlock, SEEK_SET);
    fwrite(buff, sizeBlock, 1, stream); 
    return 1;
}

int commitAction(struct cmd* ac, int sizeBlock){
    int i = 0, j = 0, k;
    char tmp;
    char con_z[8] = {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};
    char con_r[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    printf("      before tmp: %s\n", (*ac).block);
    for (j = 0; j < sizeBlock; j++){
        tmp = (*ac).block[j];
        printf("        insider: %hhx ", (*ac).block[j]);
        for (i = 0; i < 8; i++){
            switch((*ac).bits[i]){
                case -1: tmp = tmp & con_z[i]; break;
                case 0:  break;
                case 1:  tmp = tmp ^ con_r[i]; break;
                default: return -1;
            }
        }
        (*ac).block[j] = tmp;
        printf("(a): %hhx ", (*ac).block[j]);
    }
    printf("      after tmp: %s\n", (*ac).block);
    return 1;
}

int updateAction(struct cmd* des, struct cmd* src){
    int i = 0;    
    (*des).action = (*src).action;
    (*des).in = (*src).in;
    (*des).out = (*src).out;
    for (i=0; i<8; i++){
        (*des).bits[i] = (*src).bits[i]; 
    }
    return 1;
}
