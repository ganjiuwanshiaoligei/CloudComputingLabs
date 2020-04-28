
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])  
{
    char ss[100]="python3 server.py ";
   
    strcat(ss,argv[2]);
    strcat(ss," ");
    strcat(ss,argv[4]);
    strcat(ss," ");
    strcat(ss,argv[6]);
    printf("%s",ss);
    system(ss);
    system("pause");
    return 0;
}  
//./a --ip 127.0.0.1 --port 9999 --t 5

