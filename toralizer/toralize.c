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

/*
Now in the update we need to

1) Turn the client into a library (Shared library) .so file
2) Turn main() into our own connect() 
3) Replace regular connect()
4) Need to grab IP and PORT from original connect() 



*/

#include "toralize.h"

// Req *request(const char *dstip, const int dstport){
//     Req *req;

//     req = malloc(reqsize);

//     req->vn = 4;
//     req->cd = 1; // since it is a connect request
//     req->dstport = htons(dstport);
//     req->dstip = inet_addr(dstip);
//     strncpy(req->userid,USERNAME,8);

//     return req;
// }

Req *request(struct sockaddr_in *sock2){
    Req *req;

    req = malloc(reqsize);

    req->vn = 4;
    req->cd = 1; // since it is a connect request
    /* no need to do htons and inet_addr anymore since the network command that will follow toralize will do it */
    req->dstport = sock2->sin_port;
    req->dstip = sock2->sin_addr.s_addr;
    strncpy(req->userid,USERNAME,8);

    return req;
}



// int main(int argc, char *argv[]){

int connect(int s2, const struct sockaddr *sock2, socklen_t addrlen){
      
        
    // char *host; // hostname we want to connect to
    // int port; // port number on host we want to connect to
    int s; // socket file descriptor

    struct sockaddr_in sock;
    // s & sock is ours s2 and sock2 is provided by the application toralizer
    Req *req;
    Res *res;

    char buf[ressize];
    char temp[512];

    int success; // this will be used as predicate: predicate is a function or an operation that returns true or false

    // function pointer, with the same signature as connect()
    // we will use this function pointer to store original unmodified version of connect
    int (*p)(int, const struct sockaddr *, socklen_t );

    // if(argc < 3){
    //     fprintf(stderr, "Usage: %s <host> <port>\n",argv[0]);
    //     // ./toralize 1.2.3.4 80 : argv[0] = ./toralize
        
    //     return -1;
    // }

    // host = argv[1];
    // port = atoi(argv[2]); // convert argv[2] which is a string to integer using atoi() as port number is an integer
    // to check if atoi requires any header file do
    // $man 3 atoi

    // LD_PRELOAD, which lets us intercept libc functions (like connect) and lets us use our version of connect() instead of libc
    // this line finds the next connect() function after we override, using RTLD_NEXT It loads the real libc connect()
    p = dlsym(RTLD_NEXT, "connect");

    // next step, connect to proxy server, because we are not going to directly connect to the host that user specified
    // we are going to connect to proxy whose IP and PORT is specified in toralize.h file

    // sock_stream for tcp connection
    // socket to connect to tor on 127.0.0.1:9050
    s = socket(AF_INET,SOCK_STREAM,0);
    if(s < 0) {
        perror("socket");
        return -1;
    }

    sock.sin_family = AF_INET;
    sock.sin_port = htons(PROXYPORT); /* 9050 */
    sock.sin_addr.s_addr = inet_addr(PROXY); /* 127.0.0.1 */

    // connect to the proxy server(127.0.0.1:9050) using original connect, original connect function pointer stored in var `p`
    // original connect() of libc also helps in dns resolution
    if(p(s, (struct sockaddr *)&sock, sizeof(sock)) != 0){
        perror("connect");
        
        return -1;
    }

    printf("Connected to the proxy server\n");
    // req = request(host, port); // creating request object
    req = request( (struct sockaddr_in *) sock2); // creating request object

    write(s, req, reqsize);
    // need a buffer to receive data

    memset(buf, 0, ressize); // clean the buffer before reading it

    // read the proxy's response
    if(read(s, buf, ressize) < 1){ // read returns number of bytes
        perror("read");
        free(req);
        close(s);

        return -1;
    }

    res = (Res *)buf;
    success = (res->cd == 90);

    if(!success){
        fprintf(stderr, "Unable to traverse the proxy, error code: %d\n", res->cd);

        close(s);
        free(req);

        return -1;
    }

    // printf("Successfully connected through the proxy to %s:%d\n", host, port);
    printf("Connected through the proxy.\n");

    
    // memset(temp,0,512);

    // snprintf(temp, 511, 
    //     "GET / HTTP/1.0\r\n"
    //     "Host: www.google.com\r\n"
    //     "\r\n");

    // write(s,temp,strlen(temp));

    // memset(temp, 0, 512);
    // read(s, temp, 511);

    // printf("'%s'\n",temp);

    // close(s);

    /* 
    everthing that is sent to s we pipe it to s2 and everything that is sent to s2 we pipe it to s, we can do it using dup2 syscall
    int dup2(int oldfd, int newfd);
    */
   /*
    Before dup2: 
    s2 = socket created by the original application
    e.g., curl, wget, etc.
    s = the socket we created
    this is or connection to the Tor proxy at 127.0.0.1:9050
    through this proxy, our request goes to the actual destination (e.g., 1.2.3.4:80)

    Before dup2:
    ------------

    App     Our code                 Tor Proxy        Real Destination
    ┌───┐   ┌────────────┐            ┌─────────┐      ┌───────────────┐
    │App│ → │ s2 (unused)│                              // nothing yet
    └───┘   └────────────┘

    ┌────────────┐                   → 127.0.0.1:9050 → Tor → 1.2.3.4:80
    │ s (ours)  │
    └────────────┘

    After dup2(s, s2):
    ------------------

    App     our code                 Tor Proxy        Real Destination
    ┌───┐   ┌────────────┐            ┌─────────┐      ┌───────────────┐
    │App│ → │ s2 → s     │──────────→ │127.0.0.1 │────→│1.2.3.4:80     │
    └───┘   └────────────┘            └─────────┘      └───────────────┘

    toralize app created s2 and planned to use it for a real connection
    Our code:
    Intercepts that connect() call
    Creates its own socket s, connects it to Tor
    Sends a SOCKS4 request through s to reach 1.2.3.4:80

    Then: dup2(s, s2);
    Overwrites s2 so it now points to s
    App is unaware — it just uses s2 normally, but data actually flows through Tor

    */

    ////////////////////////////////////////////

    /*

    Example :
    APP = the program we are  running with toralize
    bash : toralize curl 8.8.8.8 80

    curl calls connect(s2, 8.8.8.8:80);
    But we have overridden connect() using LD_PRELOAD. So instead of the real system connect(), our version runs
    
    Inside our overridden connect
    s = socket(AF_INET, SOCK_STREAM, 0);         // we create our own socket
    connect(s, 127.0.0.1:9050);                   // connect to the local Tor proxy

    we are now connected to Tor (SOCKS4 proxy at 127.0.0.1:9050)
    But Tor doesn't know yet where to go — so we send a SOCKS4 request:
    write(s, req, reqsize);
    read(s, &response, ...);                      // Wait for SOCKS4 proxy response

    dup2(s, s2);     // Replace the apps socket with OUR socket
    The app thinks it's connected directly to 8.8.8.8:80
    But in reality, its socket s2 has been hijacked and now routes traffic:
    APP (curl) → s2 (really s) → Tor proxy → 8.8.8.8:80



    curl creates s2, the original socket.
    toralize creates s, its own socket to connect to the Tor proxy (127.0.0.1:9050)
    dup2(s, s2);
    This system call replaces file descriptor `s2` with `s`

    From now on:
    s2 is s
    any data that curl sends to s2, actually goes to s — i.e., to Tor.
    any data Tor sends back via s, arrives to curl via s2.
   
    we just intercept the connection and plug in a new pipe.
    */
    dup2(s,s2);


    free(req);


    return 0;
}

/*
For the updates

the original signature of connect is : int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
we basically converted the main function into the connect function

POC :
1) do 
curl wtfismyip.com 

You will see your real IP

Now do

toralize curl wtfismyip.com

And it will show a different IP now

*/