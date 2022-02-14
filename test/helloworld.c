#include <stdio.h>

int main (int argc, char **argv)
{
   if(argc>=3)
    printf("Hello,%s!\n",argv[1]);
   else
    printf("Hello world \n");
    return 0;
}
