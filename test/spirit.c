#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>



#define TTY_NAME "/dev/ttyS4"    /* 串口名称 */
#define TTY_BAUDRATE 115200    /* 波特率设置 */
#define RCV_MAX_SIZE 100          /* 定义接收缓冲区最大长度 */



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
	newtio.c_cc[VTIME] = 0; /* 等待第1个数据的时间: 
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


static int open_serial(void)
{

    int fd = -1,iRet=-1;
	fd = open_port(TTY_NAME);
	if (fd < 0)
	{
		printf("open err!\n");
		return -1;
	}
	iRet = set_opt(fd, TTY_BAUDRATE, 8, 'N', 1);
	if (iRet)
	{
		printf("set port err!\n");
		return -1;
	}
     return  iRet;
}

static void rcv_and_print(int iFd)
{
    int   iLen;
    char szRcvBuf[RCV_MAX_SIZE];

    if (iFd >= 0)
    {
       
        szRcvBuf[0] = '\0';
        iLen = read(iFd, szRcvBuf, sizeof(szRcvBuf));
        while ((iLen > 0) && ('\r' != szRcvBuf[iLen - 1]) && ('\n' != szRcvBuf[iLen - 1]))  /* 收到 '\r' 或 '\n' 才算结束 */
        {
            iLen += read(iFd, szRcvBuf + iLen, sizeof(szRcvBuf) - iLen);
        }

        printf("Recv data (len %d): [%s].\n", iLen, szRcvBuf);
        /* TODO 请自行按格式打印 */


        close(iFd);
    }

    return;
}

static void snd_cmd(char *pcData, int iLen)
{
    int iFd = -1;
    int iRet;

    iFd = open_serial();
    if (iFd >= 0)
    {
        iRet = write(iFd, pcData, iLen);
        if (iRet < 0)
        {
            printf("Write data (len %d) error: %s\n", iLen, strerror(errno));
        }
        else
        {
            printf("Write data (len %d) success: %s\n", iLen, pcData);
        }
        rcv_and_print(iFd);
        //close(iFd);
    }

    return;
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
        "    -f          : Force update (ignore version)\n"
        "    -i          : Show information\n",
        pcProgName);
    return;
}

int main(int argc, char **argv)
{
    int c = 0;
    char szSnd[512];

    if ((1 == argc) || 
        ((2 == argc) && (!strcmp(argv[1], "?") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))))
    {
        print_usage(argv[0]);
        return 0;
    }

    szSnd[0] = '\0';
    while (-1 != (c = getopt(argc, argv, "p:a:w:d:t:u:fis")))
    {
        switch (c)
        {
            case 'p':
            {
                int i=snprintf(szSnd, sizeof(szSnd), "p:%s\n", atoi(optarg));
                snd_cmd(szSnd, strlen(szSnd));
                printf("string:\n%s\ncharacter count = %d\n", szSnd, i);
                break;
            }
            case 'w':
            {
                memcpy(szSnd,(char*)"_YJp",4);
                snprintf(&szSnd[4], 1, "u:%s\n", optarg);
				szSnd[4]='1';
                szSnd[5]='\r';
                szSnd[6]='\n';
                snd_cmd(szSnd, 7);
                //rcv_and_print();
                printf("string:\n%s\ncharacter count = %d\n", szSnd, 7);
                break;
            }
            case 'u':
            {

			     snprintf(szSnd, sizeof(szSnd), "u:%s\n", optarg);
				 printf("sss:\n%s\n", optarg);
				 int i=snprintf(szSnd, sizeof(szSnd), "p:%s\n", atoi(optarg));
                 snd_cmd(szSnd, strlen(szSnd));
                 printf("ssss:\n%s\n ttttt count = %d\n", szSnd, i);
				 //printf("string:\n%d\n", optarg);
                //snd_cmd(szSnd, strlen(szSnd));
                break;
            }

            case 'i':
            {
                snprintf(szSnd, sizeof(szSnd), "i:\n");
                snd_cmd(szSnd, strlen(szSnd));
                //rcv_and_print();
                break;
            }

            default:
            {
                printf("TBD: -%c\n", c);
                break;
            }
        }
    }

    return 0;
}