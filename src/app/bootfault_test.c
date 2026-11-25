/* Explicit lab build only. No automatic injection or recovery at startup. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <console.h>

static const unsigned offsets[] = {32800U*512,24576U*512,41088U*512,41344U*512};
static const char *names[] = {"uboot", "uboot-b", "env", "env-b"};
static unsigned char data[4096] __attribute__((aligned(64)));
static unsigned char original[512] __attribute__((aligned(64)));
static uint32_t word(const unsigned char *p) { uint32_t n; memcpy(&n,p,4); return n; }
static int io(int fd, unsigned off, void *p, unsigned n, int wr)
{
    if (lseek(fd,off,SEEK_SET)<0) return -1;
    return (wr ? write(fd,p,n) : read(fd,p,n)) == (int)n ? 0 : -1;
}
static int valid(int fd,int id)
{
    uint32_t expected, length, sum=0, crc=~0U; unsigned off,i,j;
    if (io(fd,offsets[id],data,4096,0)) return 0;
    if (id<2) {
        if(word(data+16)!=0x89119800U) return 0;
        expected=word(data+20);length=word(data+36);
        if(length<4096 || length>4U*1024*1024 || (length&511)) return 0;
    } else { expected=word(data);length=4096; }
    for(off=0;off<length;off+=4096) {
        unsigned n=length-off; if(n>4096)n=4096;
        if(off && io(fd,offsets[id]+off,data,n,0))return 0;
        if(id<2) for(i=0;i<n;i+=4) sum+=(!off && i==20)?0x5f0a6c39U:word(data+i);
        else for(i=5;i<n;i++) {crc^=data[i];for(j=0;j<8;j++)crc=(crc>>1)^((crc&1)?0xedb88320U:0);}
    }
    return (id<2?sum:~crc)==expected;
}
static int bootfault(int argc,char **argv)
{
    int fd,id=-1,b,i,result=-1,peer,sum_fault,fault; char path[96];
    if(argc<2) {printf("bootfault status | inject/restore uboot/uboot-b/env/env-b\n");return -1;}
    fd=open("/dev/emmc",O_RDWR);if(fd<0){printf("[BF] open failed\n");return -1;}
    if(!strcmp(argv[1],"status")) {
        for(i=0;i<4;i++)printf("[BF] %s valid=%d offset=%u\n",names[i],valid(fd,i),offsets[i]);
        result=0;goto out;
    }
    if(argc!=3)goto out;
    for(i=0;i<4;i++)if(!strcmp(argv[2],names[i]))id=i;
    if(id<0)goto out;
    snprintf(path,sizeof(path),"/userdata/.bootfault-%s.bin",names[id]);
    peer=!strcmp(argv[1],"inject-peer") || !strcmp(argv[1],"inject-peer-sum");
    sum_fault=!strcmp(argv[1],"inject-sum") || !strcmp(argv[1],"inject-peer-sum");
    fault=id<2?(sum_fault?20:16):0;
    if(!strcmp(argv[1],"inject") || peer || sum_fault) {
        if((peer && id!=1) || (sum_fault && id>1))goto out;
        for(i=0;i<4;i++)if(!(peer && i==0) && !valid(fd,i)){printf("[BF] refusing: invalid %s\n",names[i]);goto out;}
        if(peer) {
            /* Only allow a second fault after proving the first matches its
             * durable, original header backup and differs by exactly our bit. */
            if(valid(fd,0))goto out;
            b=open("/userdata/.bootfault-uboot.bin",O_RDONLY);if(b<0)goto out;
            i=read(b,original,512);close(b);if(i!=512)goto out;
            if(io(fd,offsets[0],data,512,0))goto out;
            original[fault]^=1;
            if(memcmp(data,original,512))goto out;
        }
        if(io(fd,offsets[id],original,512,0))goto out;
        b=open(path,O_RDONLY);if(b>=0){close(b);printf("[BF] backup exists; restore first\n");goto out;}
        b=open(path,O_WRONLY|O_CREAT|O_EXCL,0600);if(b<0)goto out;
        i=write(b,original,512); if(i!=512 || fsync(b)){close(b);goto out;}close(b);
        b=open(path,O_RDONLY);if(b<0)goto out;i=read(b,data,512);close(b);
        if(i!=512 || memcmp(data,original,512))goto out;
        memcpy(data,original,512);data[fault]^=1;
        /* Raw SDMMC writes complete synchronously; this node has no fsync. */
        if(io(fd,offsets[id],data,512,1))goto out;
        if(io(fd,offsets[id],data,512,0))goto out;
        original[fault]^=1;
        if(memcmp(data,original,512))goto out;
        printf("[BF] INJECTED %s readback=match backup=%s\n",names[id],path);result=0;
    } else if(!strcmp(argv[1],"restore")) {
        b=open(path,O_RDONLY);if(b<0)goto out;i=read(b,original,512);close(b);if(i!=512)goto out;
        if(io(fd,offsets[id],original,512,1)||io(fd,offsets[id],data,512,0)||memcmp(data,original,512)||!valid(fd,id))goto out;
        if(unlink(path))goto out;
        printf("[BF] RESTORED %s readback=match valid=1\n",names[id]);result=0;
    }
out:
    close(fd);printf("[BF] result=%d\n",result);return result;
}
FINSH_FUNCTION_EXPORT_CMD(bootfault, bootfault, lab boot header fault test);
