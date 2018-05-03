#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <json-c/json.h>
#include <time.h>     //C语言的头文件
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>  
#include <net/if.h>  
#include <net/if_arp.h>  
#include <arpa/inet.h>  
#include <sys/ioctl.h>  
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define GATEWAY_PRAGMA_FILE_PATH    "/usr/gatewaypragma.cfg"

int main(int argc, char*argv[])
{
    system("rm /usr/gatewaypragma.cfg");
    system("sync");
    system("/etc/init.d/lora restart");
    return   0;
}

