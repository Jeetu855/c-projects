it is a command client for connecting to the TOR privacy network
eg : curl https://google.com --> invokes the connect() function and program establishes a tcp connection the
remote server and then the communication begins

With this tool we will be able to put `toralize` before any command which does a network call
`toralize` will intercept any calls to the connect() function and execute our function instead
The traffic will be redirected to a local proxy serve instead which is a part of the TOR software
It will connect to the TOR server and then the destination server effectively masking our identity

The proxies use a protocol named SOCKS: it has 2 versions, v4 and v5

creating our own client for the socks proxy and use it to connect to the TOR network

#### How does SOCKS protocol work

on google : socks4 rfc
