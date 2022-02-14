#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>


#define TTY_NAME "/dev/ttyS4"    /* 串口名称 */
#define TTY_BAUDRATE 115200    /* 波特率设置 */

/* set_opt(fd,115200,8,'N',1) */
int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio,oldtio;
	
	if ( tcgetattr( fd,&oldtio) != 0) { 
		perror("SetupSerial 1");
		return -1;
	}
	
	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag |= CLOCAL | CREAD; 
	newtio.c_cflag &= ~CSIZE; 

	newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	newtio.c_oflag  &= ~OPOST;   /*Output*/

	switch( nBits )
	{
	case 7:
		newtio.c_cflag |= CS7;
	break;
	case 8:
		newtio.c_cflag |= CS8;
	break;
	}

	switch( nEvent )
	{
	case 'O':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
	break;
	case 'E': 
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
	break;
	case 'N': 
		newtio.c_cflag &= ~PARENB;
	break;
	}

	switch( nSpeed )
	{
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
	break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
	break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
	break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
	break;
	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
	break;
	}
	
	if( nStop == 1 )
		newtio.c_cflag &= ~CSTOPB;
	else if ( nStop == 2 )
		newtio.c_cflag |= CSTOPB;
	
	newtio.c_cc[VMIN]  = 1;  /* 读数据时的最小字节数: 没读到这些数据我就不返回! */
	newtio.c_cc[VTIME] = 1; /* 等待第1个数据的时间: 
	                         * 比如VMIN设为10表示至少读到10个数据才返回,
	                         * 但是没有数据总不能一直等吧? 可以设置VTIME(单位是10秒)
	                         * 假设VTIME=1，表示: 
	                         *    10秒内一个数据都没有的话就返回
	                         *    如果10秒内至少读到了1个字节，那就继续等待，完全读到VMIN个数据再返回
	                         */

	tcflush(fd,TCIFLUSH);
	
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("com set error");
		return -1;
	}
	//printf("set done!\n");
	return 0;
}

int open_port(char *com)
{
	int fd;
	//fd = open(com, O_RDWR|O_NOCTTY|O_NDELAY);
	fd = open(com, O_RDWR|O_NOCTTY);
    if (-1 == fd){
		return(-1);
    }
	
	  if(fcntl(fd, F_SETFL, 0)<0) /* 设置串口为阻塞状态*/
	  {
			printf("fcntl failed!\n");
			return -1;
	  }
  
	  return fd;
}




/*处理RK3568数据函数

简单协议形式：
ASCII码
Header+CMD+DATA+\r\n
Header _YJ   _RS  回复
CMD    p    0 off   1 on     //自动上电
       a    0 off   1 on     //定时唤醒
       w    0 off   1 on     //远程唤醒 
       d    0 off   1 on     //看门狗使能
       t    0  12s  1  15s  2 18s  3 21s  //喂狗时间
       u    1                //更新软件
       f    1                //强制更新软件
       i    1                //查询信息

\r\n
*/
int YJread_MCU_raw_data(int fd, char *buf)
{
	int i = 0;
	int iRet;
	char c;
	int start = 0;
	
	while (1)
	{
		iRet = read(fd, &c, 1);
		if (iRet == 1)
		{

			buf[i++] = c;
			if((buf[i-1]=='\n')&&(buf[i-2]=='\r'))
				return 0;

			//if (c == '\n')
			//	return 0;
		}
		else
		{
			return -1;
		}
	}
}
int YJread_MCU_rawupdata(int fd, char *buf)
{
	int i = 0;
	int iRet;
	char c;
	int start = 0;
	
	while (1)
	{
		iRet = read(fd, &c, 1);
		if (iRet == 1)
		{

			buf[i++] = c;
			if(i==64)
				return 2;

			//if (c == '\n')
			//	return 0;
		}
		else
		{
			return -1;
		}
	}
}

int YJparse_MCU_raw_data(char *buf)
{
    int i;
	char tmp [10];
	char *ch1;
    char ch2[100]="_YJi";
	char *ch;

	//char *power, char *alarm, char *wake, char *dog, char *time;
		
    //printf("YJ raw data: %s\n", buf);



	if(strstr(buf,"_RSi"))
		{
		   /*
			printf("jsp  power Loss    control------>: %c\n", ch[4]);
			printf("jsp  alam  on      control------>: %c\n", ch[5]);
			printf("jsp  wake on Lan   control------>: %c\n", ch[6]);
			printf("jsp  watchdog      control------>: %c\n", ch[7]);
			printf("jsp  watchdog Time control------>: %c\n", ch[8]);
			*/
		    ch=strstr(buf,"_RSi");
			printf("\n");
			printf("\n");
            printf("YJ  McuSoft Version: %s\n", ch+9);
            printf("---------YJ  MCU Set Information-----------\n");
			printf("\n");
			printf("YJ  power Loss    control------>: %c\n", ch[4]);
			printf("YJ  alam  on      control------>: %c\n", ch[5]);
			printf("YJ  wake on Lan   control------>: %c\n", ch[6]);
			printf("YJ  watchdog      control------>: %c\n", ch[7]);
			printf("YJ  watchdog Time control------>: %c\n", ch[8]);
            printf("\n");
			printf("\n");
			/*
	        power[0]=ch[4];
			alarm[0]=ch[5];
			wake[0]=ch[6];
			dog[0]=ch[7];
			time[0]=ch[8];
		    printf("YJ  power Loss    control------>: %s\n", power);
			printf("YJ  alam  on      control------>: %s\n", alarm);
			printf("YJ  wake on Lan   control------>: %s\n", wake);
			printf("YJ  watchdog      control------>: %s\n", dog);
			printf("YJ  watchdog Time control------>: %s\n", time);
			*/

		}
	    else
	    	{
			   if(strstr(buf,"_RSs"))
			   	{
			   	  ch=strstr(buf,"_RSs");
                  printf("YJ  Baseboard-product-name: %s\n", ch+4);
			   	}
				else
				{
                   	if(strstr(buf,"_RSu"))
					{
                      ch=strstr(buf,"_RSu");
					  //printf("_RSu------>: %c\n", ch[4]);
					  if(ch[4]=='1')
					   {
                        // printf("START \n");
						return 2;
					   }  
					  else
					  {
                       // printf("FAILESSS \n");
					   return 3;
					  }
					    
                     // printf("YJ _RSu Baseboard-product-name: %s\n", ch+4);

					}
				    else
					{
						if(strstr(buf,"_RSf"))
						{
							ch=strstr(buf,"_RSf");
							printf("YJ _RSf Baseboard-product-name: %s\n", ch+4);
							return 2;
						}

					}
				}
	    	}
		/*if((ch=strstr(buf,(char*)("_YJs")))!=NULL)
		{
		
			//printf("jsp  Baseboard-product-name: %s\n", ch[4]);
			for(i=0;i<5;i++)
				{
				  ch1[i]=ch[4+i];

				}
			printf("YJ  Baseboard-product-name: %s\n", ch1);

		}*/
	return 0;

}

//new updata
int YJ_updata(char cmd, char data,char *filebin)
{
   
	FILE *file;
	unsigned char *buffer;
	unsigned char Sendbuffer[65];
	int fileLen;
	int i,j,SendCount,updataPack;
    char szSnd[512];
	int fd;
	int iRet,NewRet;
	char c;
    char buf[1000];
	file = fopen(filebin, "rb+");
	if (!file)
	{
		printf("can't open file ");
		return 0;
	}
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	SendCount=fileLen/64;
	printf("fileLen  data =%d\n",fileLen);
	printf("file SendCount=  %d\n",SendCount);
	fseek(file, 0, SEEK_SET);
	buffer=(unsigned char *)malloc(fileLen+1);
	if (!buffer)
	{
		printf("Memory error!");
		fclose(file);
		return 0;
	}
	fread(buffer, 1, fileLen+1, file);
	fclose(file);
	
	memcpy(&szSnd,(char*)("_YJ"),3);
	szSnd[3]=cmd;
	szSnd[4]=buffer[5];
	//memcpy(&szSnd[4],&buffer[5],5);
	szSnd[5]='1';
	szSnd[6]='1';
	//szSnd[9]='1';
	//szSnd[10]='1';
	
	//printf("YJ_send_Data:%s\n",szSnd);
	
	fd = open_port(TTY_NAME);
	if (fd < 0)
	{
		printf("open err!\n");
		return -1;
	}
	iRet = set_opt(fd, TTY_BAUDRATE,8, 'N', 1);
	if (iRet)
	{
		printf("set port err!\n");
		return -1;
	}

    iRet = write(fd, szSnd, 7);
	iRet = YJread_MCU_raw_data(fd, buf);		
	/* parse line */
	if (iRet == 0)
	{
		iRet = YJparse_MCU_raw_data(buf);
        close(fd);
		sleep(3);


		fd = open_port(TTY_NAME);
		if (fd < 0)
		{
			printf("open err!\n");
			return -1;
		}
		NewRet = set_opt(fd, TTY_BAUDRATE,8, 'N', 1);
		if (NewRet)
		{
			printf("set port err!\n");
			return -1;
		}
        //printf("YJparse_MCU_raw_data %d \n",iRet);
		if(iRet==2)  //升级
		{
          iRet=0;
		  printf("Update mcu firmware Start \n");
          updataPack=0;
          while(updataPack<SendCount)
		  {
			iRet = write(fd, &buffer[updataPack*64], 64);
			iRet=YJread_MCU_rawupdata(fd, buf);
			printf("YJread_MCU_rawupdata %d \n",iRet);
			if(iRet==2)
			{
              updataPack++;
			  //sleep(1);
			  usleep(20000);
			  printf("YJ_send_DataPack :%d\n",updataPack);
			}
		  }
		  printf("Update mcu firmware Success \n");
		  /*	for(i = 0;i <64 ;i++)
			{
				printf("%02x ",buffer[(updataPack-1)*64+i]);
			}
		  */
		}
		if(iRet==3)
		{
          printf("Update mcu firmware Fail version Error \n ");
		}
	}
    free(buffer);
	return 0;
}
int YJ_updata_test(char cmd, char data,char *filebin)
{
   
	FILE *file;
	unsigned char *buffer;
	unsigned char Sendbuffer[65];
	int fileLen;
	int i,j,SendCount,updataPack;
    char szSnd[512];
	int fd;
	int iRet,NewRet;
	char c;
    char buf[1000];
	file = fopen(filebin, "rb+");
	if (!file)
	{
		printf("can't open file ");
		return 0;
	}
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	SendCount=fileLen/64;
	printf("fileLen  data =%d\n",fileLen);
	printf("file SendCount=  %d\n",SendCount);
	fseek(file, 0, SEEK_SET);
	buffer=(unsigned char *)malloc(fileLen+1);
	if (!buffer)
	{
		printf("Memory error!");
		fclose(file);
		return 0;
	}
	fread(buffer, 1, fileLen+1, file);
	fclose(file);
	
	fd = open_port(TTY_NAME);
	if (fd < 0)
	{
		printf("open err!\n");
		return -1;
	}
	NewRet = set_opt(fd, TTY_BAUDRATE,8, 'N', 1);
	if (NewRet)
	{
		printf("set port err!\n");
		return -1;
	}

		iRet=0;
		printf("Update mcu firmware Start \n");
		updataPack=0;
		while(updataPack<SendCount)
		{
			iRet = write(fd, &buffer[updataPack*64], 64);
			iRet=YJread_MCU_rawupdata(fd, buf);
			printf("YJread_MCU_rawupdata %d \n",iRet);
			if(iRet==2)
			{
			updataPack++;
			//sleep(1);
			usleep(20000);
			printf("YJ_send_DataPack :%d\n",updataPack);
			}
		}
	     printf("Update mcu firmware Success \n");
		for(i = 0;i <64 ;i++)
		{
			printf("%02x ",buffer[(updataPack-1)*64+i]);
		}
       free(buffer);
	   return 0;
}
/*
 * ./serial_send_recv <dev>
 */
int YJ_send_recv(char *Recbuf, int Len)
{
	int fd;
	int iRet;
	char c;
    char buf[1000];

	/* 1. open */

	/* 2. setup 
	 * 115200,8N1
	 * RAW mode
	 * return data immediately
	 */

	/* 3. write and read */
	
/*	if (argc != 2)
	{
		printf("Usage: \n");
		printf("%s </dev/ttySAC1 or other>\n", argv[0]);
		return -1;
	}
*/
	fd = open_port(TTY_NAME);
	if (fd < 0)
	{
		printf("open err!\n");
		return -1;
	}
	iRet = set_opt(fd, TTY_BAUDRATE,8, 'N', 1);
	if (iRet)
	{
		printf("set port err!\n");
		return -1;
	}

    iRet = write(fd, Recbuf, Len);
	iRet = YJread_MCU_raw_data(fd, buf);		
	/* parse line */
	if (iRet == 0)
	{
		iRet = YJparse_MCU_raw_data(buf);
	}
	return 0;
}

int YJ_send_Protocol(char cmd, char data)
{
    char szSnd[512];

 
	if(data >= '0' && data <= '9')
	{
		memcpy(&szSnd,(char*)("_YJ"),3);
		szSnd[3]=cmd;
		szSnd[4]=data;
		//szSnd[5]='\r';
		//szSnd[6]='\n';
		szSnd[5]='1';
		szSnd[6]='1';
		
		//printf("YJ_send_Data:%s\n",szSnd);
		
		YJ_send_recv(szSnd,7);
	}
	else
	 printf("INPUT ERROR,Please InPut Number\n");
	return 0;
}

static void print_usage(char *pcProgName)
{
    printf("\n Usage: %s [optioins]\n"
        "    -p [0|1]    : Auto power on(1)/off(0)\n"
        "    -a [0|1]    : Alarm power on(1)/off(0)\n"
        "    -w [0|1]    : Wake on lan on(1)/off(0)\n"
        "    -d [0|1]    : Watchdog on(1)/off(0)\n"
        "    -t [0|1|2|3]: Watchdog time, 0(12s), 1(15s), 2(18s), 3(31s)\n"
        "    -u <file>   : Update mcu firmware to <file>\n"
        "    -f <file>   : Force update (ignore version)\n"
        "    -i          : Show information\n"
        "    -s          : Baseboard-product-name\n",
        pcProgName);
    return;
}

int main(int argc, char **argv)
{
    int c = 0,Len=0;
    char szSnd[2];

    if ((1 == argc) || 
        ((2 == argc) && (!strcmp(argv[1], "?") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))))
    {
        print_usage(argv[0]);
        return 0;
    }

    szSnd[0] = '\0';
    while (-1 != (c = getopt(argc, argv, "p:a:w:d:t:u:f:e:is")))
    {
        //Len=snprintf(szSnd,sizeof(szSnd),"%.*S",optarg);
        Len=snprintf(szSnd, sizeof(szSnd), "%s\n", optarg);
	    //printf("string:%c   \noptarg: %s\n  szSnd:%s\n  Len:%d\n", c,optarg,szSnd,Len);
		switch (c)
        {   
			
			case 'p':
            {
                YJ_send_Protocol(c,szSnd[0]);
                break;
            }
			case 'a':
            {
                YJ_send_Protocol(c,szSnd[0]);
                break;
            }
		    case 'w':
            {
                YJ_send_Protocol(c,szSnd[0]);
                break;
            }
			case 'd':
            {
                YJ_send_Protocol(c,szSnd[0]);
                break;
            }
            case 't':
            {
                YJ_send_Protocol(c,szSnd[0]);
                break;
            }
            case 'u':
            {
               // YJ_send_Protocol(c,'1');
				YJ_updata(c,'1',optarg);
                break;
            }

            case 'f':
            {
				//YJ_send_Protocol(c,'1');
				YJ_updata(c,'1',optarg);
                break;
            }

            case 'i':
            {
				YJ_send_Protocol(c,'1');
                break;
            }
			case 's':
            {
				YJ_send_Protocol(c,'1');
                break;
            }
			case 'e':
            {
				YJ_updata_test(c,'1',optarg);
                break;
            }
            default:
            {
                printf("INPUT ERROR \n");
                break;
            }
        }
    }

    return 0;
}

