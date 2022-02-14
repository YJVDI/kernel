#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>


#define VERSION   "V1.11"

typedef unsigned int DWORD;
typedef unsigned char BYTE;


int main(int argc, char* argv[]) {
	int  i, j,SendCount;
	unsigned char *buffer;
    unsigned char Sendbuffer[65];

	int  iResult = 1;
	FILE *fpPack = NULL;
	FILE *fpSrc = NULL;
	BYTE szBuf[409600];
	DWORD dwChkSum = 0;
	int  iPackFileSize = 0;
	int  iSrcFileSize = 0;
	int  printf_info;
	char szPackFile[40];	
	int packup_file_size = 0;
	int src_file_offset = 0;
	int byte_alignment_missing = 0;
	time_t timestamp;
	struct tm *tblock;
	BYTE intBuf[4];
	BYTE fill_byte = 0xFF;
    
	char szSnd[10];
    int Len,version=0;

	printf("\r\n=========================================================");
	printf("\r\n=    	        (C) COPYRIGHT 2021 Yj                   =");
	printf("\r\n=        Bin File Packup Tools (Version 1.1.0) 	        =");
	printf("\r\n=                    		       By Embedded Team =");
	printf("\r\n=========================================================");
	printf("\r\n\r\n");

    printf("Argv Number :%d\n",argc);

	if(access(argv[1], F_OK) != 0) {
		printf("There is no %s!\n", argv[1]);
		goto _PACK_END;
	}
	if(argc!=3)
    {
		printf("There is input  eg: YJ_VDI V10 !\n");
		goto _PACK_END;
	}
	strcpy(szPackFile, "packFile/");
	strcat(szPackFile, argv[2]);
	strcat(szPackFile, argv[1]);

   
    Len=snprintf(szSnd, sizeof(szSnd)/sizeof(szSnd[0]), "%s", argv[2]);
	printf("snprintf szSnd:%s\n",szSnd);
    printf("snprintf Len:%d\n",Len);

    
    if(szSnd[1] >= '0' && szSnd[1] <= '9')
      version=(szSnd[1]-0x30)*10;
	else
	 {
       printf("There is input version Number eg: V10!\n");
	   goto _PACK_END;
	 }
    if(szSnd[2] >= '0' && szSnd[2] <= '9')
       version=(szSnd[2]-0x30)+version;
	else
	 {
		 printf("There is input version Number eg: V10 \n");
		 goto _PACK_END;
	 } 

     printf("There is bin version Number :%d\n",version);


	if(NULL == (fpPack = fopen(szPackFile, "wb+"))) {
		perror("Create xxx.bin failed:");
		goto _PACK_END;
	}
	
	if(4 != fwrite((void*)&dwChkSum, 1, 4, fpPack))
	{
		perror("Write file num failed1:");
		goto _PACK_END;
	}

	if(4 != fwrite((void*)&dwChkSum, 1, 4, fpPack))
	{
		perror("Write file num failed2:");
		goto _PACK_END;
	}
	
	//fpSrc  打开源文件
	if (NULL == (fpSrc = fopen(argv[1], "rb+"))) {
		perror("Open src file failed:");
		goto _PACK_END;
	}
	
	// 读取源文件长度
	fseek(fpSrc, 0, SEEK_END);
	iSrcFileSize = ftell(fpSrc);
	if((iSrcFileSize-48)%56==0)
	{
	  SendCount=(iSrcFileSize-48)/56+1;
	}
	else
	{
	  SendCount=(iSrcFileSize-48)/56+2;  
	}
	printf(".bin Src file size:%d\r\n", iSrcFileSize);
	printf(".bin Src SendCount:%d\r\n", SendCount);
	
    printf("\r\n\r\n");

	fseek(fpSrc, 0, SEEK_SET);
	buffer=(unsigned char *)malloc(iSrcFileSize+1);
	if (!buffer)
	{
		printf("Memory error!\r\n");
		fclose(fpSrc);
		//exit(1);
	}
	fread(buffer, 1, iSrcFileSize+1, fpSrc);
	fclose(fpSrc);
	/*//打印bin文件
	for(i = 0;i < iSrcFileSize ;i++)
	{
		printf("%02x ",buffer[i]);
		if(i%16==15)printf("\n");
		if(i%4==3)printf("  ");
	}*/
	for(j=0;j<SendCount;j++)
	{
		memset(Sendbuffer,0,64);
		if(j==0)
		{
			Sendbuffer[0]=0xA0;
			Sendbuffer[4]=0x03;
            Sendbuffer[5]=version;
			//memcpy(&Sendbuffer[5],VERSION,5);
			Sendbuffer[12]=iSrcFileSize&0xff;
			Sendbuffer[13]=(iSrcFileSize>>8)&0xff;
			memcpy(&Sendbuffer[16],&buffer[0],48);
		
		}
		else
		{
			Sendbuffer[0]=0xA4;
			memcpy(&Sendbuffer[8],&buffer[48+(j-1)*56],56);
		}
		/*//发送数据/打印数据
		for(i = 0;i < 64 ;i++)
		{
			printf("%02x ",Sendbuffer[i]);
		}*/
		
		fseek(fpPack, 64*j, SEEK_SET);
		if(64 != fwrite((void*)Sendbuffer, 1, 64, fpPack)) 
		{
			perror("Write src file relative offset failed!");
			goto _PACK_END;	
		}
		//printf("\r\n");
	}
	printf("Make Bin Success\n");
	//fclose(fpPack);
	printf("\n");
	free(buffer);

_PACK_END:
        if (fpPack != NULL) {
                fclose(fpPack);
                fpPack = NULL;
        }

	return iResult;
}

