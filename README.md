# muti
Muti is a C asynchronous I/O project, it is try to simplify socket request/response process.
It consists of 5 packages: util, network, res, server and client. 
Although this project is wrote by C, but it is object-oriented program.

##util package
Currently there are three interfaces in this package: Link list(from linux kernel), Map and Thread pool.
Map implements is a Hash Linked Map;
Thread pool implements contains three type, single thread pool, multi-thread pool and zero thread pool.

##network package
Network interface have two implements, one for windows( IOCP ), one for Linux( epoll ).

##res package
res means resource, the concept is from REST. For a resource we have many actions act on it. The actions on server are in server handler package.
The actions on client are in client handler package.

##server package
Server package is a server request handle framework, which contains network service, request encoding, action excute and response decoding and so on.

##client package
Client package is client send request framework, which contains network connecting, request decoding, response receiving and response encoding and so on.

##test folder
test folder is a test framework, it contains all modules unit test codes.

