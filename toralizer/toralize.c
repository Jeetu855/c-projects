/*toralize.c*/

/*command line client for connecting to the tor private network
when we run a cli command for doing a network call : example 
$curl https://10.18.56.101                     
the following things happen
the connect() function is invoked and the program establishes a tcp connection to the remote server and then 
the communication begins 
but with this tool we will be able to we will be able to add toralize in front of any command that performs a netowrk call
$ toralize curl https://10.18.56.101
toralize will intercept any call to the network function and execute our function instead
the traffic will be redirected to a local proxy server which is a part of tor software. It will connect to the 
tor network and then to the destination server effectively masking our identity and helping us to stay private


Creating proxy server
Proxies use protocol : SOCKS, it have 2 versions v4 and v5

*/

/*
./toralize 1.2.3.4 80
this tool will connect to a predefined proxy server. We are going to put that predefined server ip in toralize.h

Q How does the SOCKS protocol work
Google protcol name and rfc : socks4 rfc
first make tcp connection to proxy server
then when we are conncected we need to send a network packet with the correct format that is accepted by the protocol
fields :

              +----+----+----+----+----+----+----+----+----+----+....+----+
		      | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
		      +----+----+----+----+----+----+----+----+----+----+....+----+
# of bytes:	    1    1      2              4           variable        1

- first : VN : 1 byte : version number , value should be 4
- second : CD : 1 bytes : socks command code : should be 1 for connect request 
there are 2 different operations we can do through socks4 : connect and bind
connect operations is when we connect outwards
bind is when we prepare for an inbound connection towards us
- third : DSTPORT : 2 bytes : destination port number, need to format the port number in network byte order 
- fourth : DSTIP : 4 bytes : destination IP
- fifth : USERID : variable size
- last : NULL : 1 byte : null byte

Response fields

                +----+----+----+----+----+----+----+----+
		        | VN | CD | DSTPORT |      DSTIP        |
		        +----+----+----+----+----+----+----+----+
 # of bytes:	   1    1      2              4

- VN : 1 byte
- CD : 1 byte : 90 indicates connection has been established, 91 indicates connection failed and many more codes
- DSTPORT :  2 byte
- DSTIP : 4 byte


*/

#include "toralize.h"

Req *request(const char *dstip, const int dstport) {
    Req *req;

    req = malloc(reqsize);

    req->vn = 4;
    req->cd = 1; // since it is a connect request
    req->dstport = htons(dstport);
    req->dstip = inet_addr(dstip);
    strncpy(req->userid,USERNAME,8);

    return req;
}

int main(int argc, char *argv[]) {

    char *host; // hostname we want to connect to
    int port; // port number on host we want to connect to
    int s; // socket file descriptor

    struct sockaddr_in sock;

    Req *req;
    Res *res;

    char buf[ressize];
    char temp[512];

    int success; // this will be used as predicate: predicate is a function or an operation that returns true or false

    if(argc < 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n",argv[0]);
        // ./toralize 1.2.3.4 80 : argv[0] = ./toralize
        
        return -1;
    }

    host = argv[1];
    port = atoi(argv[2]); // convert argv[2] which is a string to integer using atoi() as port number is an integer
    // to check if atoi requires any header file do
    // $man 3 atoi


    // next step, connect to proxy server, because we are not going to directly connect to the host that user specified
    // we are going to connect to proxy whose IP and PORT is specified in toralize.h file

    // sock_stream for tcp connection
    s = socket(AF_INET,SOCK_STREAM,0);

    if(s < 0) {
        perror("socket");
        return -1;
    }

    sock.sin_family = AF_INET;
    sock.sin_port = htons(PROXYPORT);
    sock.sin_addr.s_addr = inet_addr(PROXY);

    // connect to the proxy server
    if(connect(s, (struct sockaddr *)&sock, sizeof(sock)) != 0){
        perror("connect");
        
        return -1;
    }

    printf("Connected to the proxy server\n");
    req = request(host, port); // creating request object

    write(s, req, reqsize);
    // need a buffer to receive data

    memset(buf, 0, ressize); // clean the buffer before reading it

    if(read(s, buf, ressize) < 1) { // read returns number of bytes
        perror("read");
        free(req);
        close(s);

        return -1;
    }

    res = (Res *)buf;
    success = (res->cd == 90);

    if(!success) {
        fprintf(stderr, "Unable to traverse the proxy, error code: %d\n", res->cd);

        close(s);
        free(req);

        return -1;
    }

    printf("Successfully connected through the proxy to %s:%d\n", host, port);


    memset(temp,0,512);

    snprintf(temp, 511, 
        "GET / HTTP/1.0\r\n"
        "Host: www.google.com\r\n"
        "\r\n");

    write(s,temp,strlen(temp));

    memset(temp, 0, 512);
    read(s, temp, 511);

    printf("'%s'\n",temp);

    close(s);
    free(req);


    return 0;
}