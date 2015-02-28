/*************************
 *EEL5934 ADV.SYSTEM.PROG
 *Juncheng Gu 5191-0572
 *Assignment2 Multi-thread
 *Feb 1, 2015
 *************************/
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<pthread.h>
#include<signal.h>
#include<semaphore.h>

#define COMMAND_LINE_LENGTH 1024
#define COMMAND_MIN_LENGTH 4
#define COMMAND_MAX_LENGTH 11
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

struct instr{
    int in;
    int out;
    int action;
    char bits[8];
};

struct para{
    FILE *fd;
    int sizeBuff;
    int sizeBlock;
    int blockFile;

};
struct dWrite{
    char *block;
    int out;
};

struct list_t{
    int in;
    int out;
    pthread_mutex_t mutex;
    sem_t* empty;
    sem_t* full;
    int *list;
    struct instr* instrlist;
    struct dWrite* dw;
};
/********************************** funcition definition ****************************/
long copyFile(FILE *in, FILE* out);
int getLine(FILE *fd, char *str, int max);
int getArguments(char *, char *[]);
int toInt(char *str);
int blockRead(char*, int in, int sizeBlock, FILE*);
int blockWrite(char*, int out, int sizeBlock, FILE*);
int commitAction(struct cmd*, int);
int updateAction(struct cmd* des, struct instr* src);

/* actions */
int revert2(char a[], int n, int b[]);
int revert(struct cmd*);
int zero2(char a[], int n, int b[]);
int zero(struct cmd*);
int convertArgu(struct cmd* ac, int len, char *arguments[], int, int, int);
int convertArgu2(struct instr* ac, int len, char *arguments[], int, int, int);

void printfcmd(struct cmd a);
void printfinstr(struct instr a);
void printfbitmap(int *map, int len);
/**************************** end of function definition *****************************/
/**************************** global variable *******************************/
pthread_t reader, writer, processing; 
struct instr *instrBuff;
struct cmd *buffer;
struct list_t m_r, r_p, p_w;
int *buff_out;
/*************************** end of global variable ************************/
/****************************** worker ************************************/
void *reader_handler(void *t){
    int use_num = 0; //current empty buffer index;
    int in = 0, i = 0;
    int *buff_in; // mapping in-buffer[]
    FILE* fd_r = ((struct para*)t)->fd;
    int sizeBuff = ((struct para*)t)->sizeBuff;
    int sizeBlock = ((struct para*)t)->sizeBlock;
    int blockFile = ((struct para*)t)->blockFile;
    struct instr* tmp;

    buff_in = (int*)malloc((blockFile+1)*sizeof(int)); //mapping in---buffer index
    tmp = (struct instr*)malloc(sizeof(struct instr));
    for (i=1; i <= blockFile; i++){
        buff_in[i] = -1;  
    }
    while (1){
        printf(" Reader: want to enter...\n");
        sem_wait(m_r.empty);
        pthread_mutex_lock(&(m_r.mutex));
        printf(" Reader: just enter...\n");
        (*tmp).action = instrBuff[m_r.out].action;
        if ((*tmp).action != 9){
            (*tmp).in = instrBuff[m_r.out].in;
            (*tmp).out = instrBuff[m_r.out].out;
            for (i=0; i<8; i++){
                (*tmp).bits[i] = instrBuff[m_r.out].bits[i];
            }
        }
        printfinstr(instrBuff[m_r.out]);
        m_r.out = (m_r.out + 1) % sizeBuff;
        sem_post(m_r.full);
        pthread_mutex_unlock(&(m_r.mutex));
        in = (*tmp).in;

        if ((*tmp).action == 9){
            printf("Reader: main thread wants me to terminate\n");
            sem_wait(r_p.full);
            pthread_mutex_lock(&(r_p.mutex));
            r_p.list[r_p.in] = sizeBuff + 9;
            r_p.in = (r_p.in +1) % sizeBuff; 
            sem_post(r_p.empty);
            pthread_mutex_unlock(&(r_p.mutex));
            break;
        }

        if (buff_in[in] >= 0){ // if buff_in[in] in buffer
            sem_wait(r_p.full);
            pthread_mutex_lock(&(r_p.mutex));
            r_p.list[r_p.in] = buff_in[in];
            r_p.instrlist[r_p.in].in = (*tmp).in;
            r_p.instrlist[r_p.in].out = (*tmp).out;
            r_p.instrlist[r_p.in].action = (*tmp).action;
            for (i = 0; i < 8; i++){
                r_p.instrlist[r_p.in].bits[i] = (*tmp).bits[i];
            }
            r_p.in = (r_p.in +1) % sizeBuff; 
            printf("  Reader: just left...\n");
            sem_post(r_p.empty);
            pthread_mutex_unlock(&(r_p.mutex));
        }else if(buff_in[in] < 0){ // if buff_in[in] out of buffer
            sem_wait(r_p.full);
            pthread_mutex_lock(&(r_p.mutex));
            blockRead(buffer[use_num].block, in, sizeBlock, fd_r);
            r_p.instrlist[r_p.in].in = (*tmp).in;
            r_p.instrlist[r_p.in].out = (*tmp).out;
            r_p.instrlist[r_p.in].action = (*tmp).action;
            for (i = 0; i < 8; i++){
                r_p.instrlist[r_p.in].bits[i] = (*tmp).bits[i];
            }
            buff_in[in] = use_num;
            r_p.list[r_p.in] = use_num;
            r_p.in = (r_p.in +1) % sizeBuff; 
            printf("  Reader: just left...\n");
            sem_post(r_p.empty);
            pthread_mutex_unlock(&(r_p.mutex));
            use_num += 1;
        }
    } 

    fclose(fd_r);
    printf("Reader... exit!");
    pthread_exit(NULL);
}


void *processing_handler(void *t){
    int index, out, i, j; 
    int sizeBlock = ((struct para*)t)->sizeBlock;
    int sizeBuff= ((struct para*)t)->sizeBuff;
    int blockFile= ((struct para*)t)->blockFile;
    while (1){ 
        printf("   Processing: want to enter...\n");
        sem_wait(r_p.empty);
        pthread_mutex_lock(&(r_p.mutex));
        printf("  Processing: just enter...\n");
        index = r_p.list[r_p.out];
        if (index != (sizeBuff + 9)){
            buffer[index].out = r_p.instrlist[r_p.out].out;
            buffer[index].in = r_p.instrlist[r_p.out].in;
            buffer[index].action = r_p.instrlist[r_p.out].action;
            for (i = 0; i < 8; i++){
                buffer[index].bits[i] = r_p.instrlist[r_p.out].bits[i];
            }
        }
        r_p.out = (r_p.out + 1) % sizeBuff;
        printf("  Processing: want to left...\n");
        sem_post(r_p.full);
        pthread_mutex_unlock(&(r_p.mutex));
        printf("  Processing: just left...\n");
        if (index == (sizeBuff + 9)){
            printf("Processing: reader let me to terminate\n");
            sem_wait(p_w.full);
            pthread_mutex_lock(&(p_w.mutex));
            p_w.list[p_w.in] = sizeBuff + 9;
            sem_post(p_w.empty);
            pthread_mutex_unlock(&(p_w.mutex));
            break;
        }
        
        commitAction(buffer+index, sizeBlock);
        out = buffer[index].out;
        sem_wait(p_w.full);
        pthread_mutex_lock(&(p_w.mutex));
        if (buff_out[out] > 0){
            p_w.dw[buff_out[out]].out = -1;
        }
        buff_out[out] = p_w.in;
        p_w.dw[p_w.in].out = out;
        for (i = 0; i < sizeBlock; i++){
            p_w.dw[p_w.in].block[i] = buffer[index].block[i];
        }
        p_w.list[p_w.in] = index; 
        p_w.in = (p_w.in + 1) % sizeBuff;
        printf(" Processing: just left...\n");
        sem_post(p_w.empty);
        pthread_mutex_unlock(&(p_w.mutex));
    }
    printf("Processing... exit!");
    pthread_exit(NULL);
}

void *writer_handler(void *t){
    int index;
    FILE* fd_w = ((struct para*)t)->fd;
    int sizeBuff = ((struct para*)t)->sizeBuff;
    int sizeBlock = ((struct para*)t)->sizeBlock;

    while (1){
        printf("   Writer: just enter...\n");
        sem_wait(p_w.empty);
        pthread_mutex_lock(&(p_w.mutex));
        index = p_w.list[p_w.out];
        if (index != (sizeBuff + 9)){
            if (p_w.dw[p_w.out].out != -1){
                blockWrite(p_w.dw[p_w.out].block, p_w.dw[p_w.out].out, sizeBlock, fd_w);
                printf(" Writer: get index %d\n", index);
                buff_out[p_w.dw[p_w.out].out] = -1;
                p_w.dw[p_w.out].out = -1;
            }
        }else{
            printf(" Writer: get index %d\n", index);
            printf("Writer: processing let me to terminate\n");
            pthread_mutex_unlock(&(p_w.mutex));
            break;
        }
        p_w.out = (p_w.out + 1) % sizeBuff;
        printf("   Writer: just left...\n");
        sem_post(p_w.full);
        pthread_mutex_unlock(&(p_w.mutex));
    }

    fclose(fd_w);
    printf("Writer... exit!");
    pthread_exit(NULL);
}
/****************************** end of worker *****************************/

int main(int argc, char *argv[]){
    char commandLine[COMMAND_LINE_LENGTH+1];    
    char *arguments[COMMAND_LENGTH+1];
    int i, j, failed;
    int sizeBuff = 0;
    int sizeBlock = 0;
    int sizeFile = 0;
    int blockFile = 0;
    int lenArgu = 0;
    char intBuff[10];
    FILE *fd[3];   //0=in, 1=out, 2=instr
    struct para r_data, p_data, w_data;
    void *status1, *status2, *status3;
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
        if (fd[0] == NULL || fd[1] == NULL || fd[2] == NULL){
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
    printf("sizeFile is %d, blockFile is %d\n", sizeFile, blockFile);
    buffer = (struct cmd*)malloc(sizeBuff * sizeof(struct cmd));
    instrBuff = (struct instr*)malloc(sizeBuff * sizeof(struct instr));
    buff_out = (int*)malloc((blockFile+1)*sizeof(int)); //[0]x, [1] ~ [blockFile], mapping out---buffer index
    m_r.list = (int*)malloc(sizeBuff*sizeof(int)); 
    r_p.list = (int*)malloc(sizeBuff*sizeof(int)); 
    r_p.instrlist = (struct instr*)malloc(sizeBuff*sizeof(struct instr)); 
    p_w.list = (int*)malloc(sizeBuff*sizeof(int)); 
    p_w.dw = (struct dWrite*)malloc(sizeBuff*sizeof(struct dWrite)); 
    for (i=0; i < sizeBuff; i++){
        buffer[i].block = (char*)malloc(sizeof(char)*sizeBlock);
        buffer[i].in = -1;
        buffer[i].out = -1;
        m_r.list[i] = -1;
        r_p.list[i] = -1;
        r_p.instrlist[i].in = -1;
        r_p.instrlist[i].out = -1;
        r_p.instrlist[i].action = -1;
        p_w.list[i] = -1;
        p_w.dw[i].block = (char*)malloc(sizeof(char)*sizeBlock);
        p_w.dw[i].out = -1;
    }
    for (i=1; i <= blockFile; i++){
        buff_out[i] = -1;  
    }
    m_r.full = sem_open("m_r_f", O_CREAT, 0644, sizeBuff);
    m_r.empty = sem_open("m_r_e", O_CREAT, 0644, 0);
    r_p.full = sem_open("r_p_f", O_CREAT, 0644, sizeBuff);
    r_p.empty = sem_open("r_p_e", O_CREAT, 0644, 0);
    p_w.full = sem_open("p_w_f", O_CREAT, 0644, sizeBuff);
    p_w.empty = sem_open("p_w_e", O_CREAT, 0644, 0);
    pthread_mutex_init(&(m_r.mutex), NULL);
    pthread_mutex_init(&(r_p.mutex), NULL);
    pthread_mutex_init(&(p_w.mutex), NULL);
    m_r.in = 0;
    m_r.out = 0;
    r_p.in = 0;
    r_p.out = 0;
    p_w.in = 0;
    p_w.out = 0;
    r_data.fd = fd[0];
    r_data.sizeBuff = sizeBuff;
    r_data.sizeBlock = sizeBlock;
    r_data.blockFile = blockFile;
    p_data.blockFile = blockFile;
    p_data.sizeBuff = sizeBuff;
    p_data.sizeBlock = sizeBlock;
    w_data.fd = fd[1];
    w_data.sizeBuff = sizeBuff;
    w_data.sizeBlock = sizeBlock;
    failed = pthread_create(&reader, NULL, reader_handler, &r_data);
    if (failed) {
        printf("thread_create failed!\n");
        return -1;
    }
    failed = pthread_create(&processing, NULL, processing_handler, &p_data);
    if (failed) {
        printf("thread_create failed!\n");
        return -1;
    }
    failed = pthread_create(&writer, NULL, writer_handler, &w_data);
    if (failed) {
        printf("thread_create failed!\n");
        return -1;
    }
    printf("Instr %s, Input %s, Output %s, Block %d, Buffer %d \n", argv[1], argv[2], argv[3], sizeBlock, sizeBuff); 

    //action perform
    while (getLine(fd[2], commandLine, COMMAND_LINE_LENGTH+1) != -1){
        /*analysis action*/
        lenArgu = getArguments(commandLine, arguments);
        printf("Main: just enter.......\n");
        sem_wait(m_r.full);
        pthread_mutex_lock(&(m_r.mutex));
        if (lenArgu < 0){
            instrBuff[m_r.in].action = 9;
            m_r.in = (m_r.in + 1) % sizeBuff;
            sem_post(m_r.empty);
            pthread_mutex_unlock(&(m_r.mutex));
            printf("Error Arugments!\n");
            fclose(fd[2]);
            pthread_join(reader, &status1);
            pthread_join(processing, &status2);
            pthread_join(writer, &status3);  
            pthread_exit(NULL);
        }
        if (strcmp(arguments[0], "revert") == 0){
            //revert
            instrBuff[m_r.in].action = 1;
            if (convertArgu2(instrBuff+m_r.in, lenArgu, arguments, sizeBlock, sizeBuff, blockFile) < 0){
                printf("Error occured in arguments convert\n");
                instrBuff[m_r.in].action = 9;
                m_r.in = (m_r.in + 1) % sizeBuff;
                sem_post(m_r.empty);
                pthread_mutex_unlock(&(m_r.mutex));
                fclose(fd[2]);
                pthread_join(reader, &status1);
                pthread_join(processing, &status2);
                pthread_join(writer, &status3);  
                pthread_exit(NULL);
            }
            printfinstr(instrBuff[m_r.in]);             
        }else if (strcmp(arguments[0], "zero") == 0){
            //zero
            instrBuff[m_r.in].action = -1;
            if (convertArgu2(instrBuff+m_r.in, lenArgu, arguments, sizeBlock, sizeBuff, blockFile) < 0){
                printf("Error occured in arguments convert\n");
                instrBuff[m_r.in].action = 9;
                m_r.in = (m_r.in + 1) % sizeBuff;
                sem_post(m_r.empty);
                pthread_mutex_unlock(&(m_r.mutex));
                fclose(fd[2]);
                pthread_join(reader, &status1);
                pthread_join(processing, &status2);
                pthread_join(writer, &status3);  
                pthread_exit(NULL);
            }
            printfinstr(instrBuff[m_r.in]);
        }else{
            printf("Unknow action %s\n", arguments[0]);
            instrBuff[m_r.in].action = 9;
            m_r.in = (m_r.in + 1) % sizeBuff;
            sem_post(m_r.empty);
            pthread_mutex_unlock(&(m_r.mutex));
            fclose(fd[2]);
            pthread_join(reader, &status1);
            pthread_join(processing, &status2);
            pthread_join(writer, &status3);  
            pthread_exit(NULL);
        }
        m_r.in = (m_r.in + 1) % sizeBuff;
        printf("Main: just left.......\n");
        sem_post(m_r.empty);
        pthread_mutex_unlock(&(m_r.mutex));
    }
    sem_wait(m_r.full);
    pthread_mutex_lock(&(m_r.mutex));
    instrBuff[m_r.in].action = 9;
    m_r.in = (m_r.in + 1) % sizeBuff;
    sem_post(m_r.empty);
    pthread_mutex_unlock(&(m_r.mutex));
   
    fclose(fd[2]);
    pthread_join(reader, &status1);
    pthread_join(processing, &status2);
    pthread_join(writer, &status3);  
    pthread_exit(NULL);
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
    if (i < COMMAND_MIN_LENGTH || i > COMMAND_MAX_LENGTH){
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


long copyFile(FILE* in, FILE* out){
    long num = 0;
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

int convertArgu2(struct instr* ac, int len, char *arguments[], int block, int buff, int numblock){
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
    return 1;
}

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

int updateAction(struct cmd* des, struct instr* src){
    int i = 0;    
    (*des).action = (*src).action;
    (*des).in = (*src).in;
    (*des).out = (*src).out;
    for (i=0; i<8; i++){
        (*des).bits[i] = (*src).bits[i]; 
    }
    printf("    in updateAction----\n");
    printfinstr((*src));
    printf("    after updateAction---\n");
    return 1;
}

void printfcmd(struct cmd a){
    int i =0;
    printf("  ###  in %d, out %d, action %d ", a.in, a.out, a.action);
    for (i = 0; i < 8; i++){
        printf(",%d", a.bits[i]);
    }
    printf(" , block %s,", a.block); 
    printf("\n");
}
void printfinstr(struct instr a){
    int i =0;
    printf("   instr###  in %d, out %d, action %d ", a.in, a.out, a.action);
    for (i = 0; i < 8; i++){
        printf(",%d", a.bits[i]);
    }
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
