/*toralize.h*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// man 2 socket : why 2 ? because it is a system call

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define PROXY        "127.0.0.1" 
#define PROXYPORT    9050
#define reqsize      sizeof(struct proxy_request)
#define ressize      sizeof(struct proxy_response)
// 7 bytes username, last byte is null because that is the request format, thats why we allocated 8 bytes such that 7 bytes for username and 1 NULL byte
#define USERNAME     "toraliz"

// typedef to create our own variable type, unsigned char is 8 bits, and we are now calling it int8
typedef unsigned char int8;
typedef unsigned short int int16; 
typedef unsigned int int32;


/*
Request fields

              +----+----+----+----+----+----+----+----+----+----+....+----+
		      | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
		      +----+----+----+----+----+----+----+----+----+----+....+----+
# of bytes:	   1     1      2              4           variable        1
*/

struct proxy_request {
    int8 vn;
    int8 cd;
    int16 dstport;
    int32 dstip;
    unsigned char userid[8];
};

typedef struct proxy_request Req;

/*
Response fields

                +----+----+----+----+----+----+----+----+
		        | VN | CD | DSTPORT |      DSTIP        |
		        +----+----+----+----+----+----+----+----+
 # of bytes:	  1    1      2              4
*/

struct proxy_response {
    int8 vn;
    int8 cd;
    int16 _; // not important field thats why not named, from the rfc
    int32 __; // not important field thats why not named, from the rfc
};

typedef struct proxy_response Res;


// signatures
Req *request(const char *, const int); // const keyword means we cannot change these variables inside the function
int main(int,char**);