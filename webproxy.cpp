#include <iostream>
#include <map>
#include <string>
#include <string.h>
#include <fstream>
#include <thread>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <mutex>



#define BUFSIZE 1028


std::map<std::string,std::string> cache;
std::map<std::string,std::string> blacklist;

std::mutex m;


void request_cleanup(int conn_fd) {
    if (close(conn_fd) < 0) {
        std::cout << "error closing connection socket" << std::endl;
        exit(1);
    }
    
    std::cout << std::endl;
}

void connection_handler(int conn_fd){
    for (;;) {
        //std::cout << "--------------------START----------------------" << std::endl;
        char receivedMessage[BUFSIZE];
        std::string error;
        std::string requestError;
        bzero(receivedMessage, sizeof(receivedMessage));
        int read_size = read(conn_fd, receivedMessage, BUFSIZE); //read in request from client
        std::cout << read_size << std::endl;
        if(read_size <= 0) { //error check the response
            if (errno == EWOULDBLOCK) { //this comes if file is too large or if there is a request timeout
                std::cout << "Closing connection..." << std::endl;
            }
            else { //internal server error
                std::string error = "<html><body text='red'>500 Internal Server Error</body></html>";
                std::string requestError = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text html\r\nContent-Length: " + std::to_string(error.length()) + "\r\n\r\n" + error;
                write(conn_fd, requestError.c_str(), requestError.length()*sizeof(char));
                //std::cout << "--------------------END----------------------" << std::endl;
                
            }
            close(conn_fd); //close connection
            return;
        }
        std::string fullBody = receivedMessage; //parse reqeust for GET line
        std::stringstream ss(receivedMessage);
        
        //request
        std::string httpRequest;
        getline(ss, httpRequest, '\n');
        std::cout << fullBody << std:: endl;
        std:: string method, version, uri;
        std::stringstream ss2(httpRequest);
        ss2 >> method;
        ss2 >> uri;
        ss2 >> version;
        
        
        //port
        int found = uri.find_last_of(":");
        std::string port = uri.substr(found+1);
        std::cout << "PORT IS: " << port << std::endl;
        int portNum = 0;
        if (port[0] == '/') {
            portNum = 80;
        }
        else {
            portNum = std::stoi(port);
        }
        std::cout << port << std::endl;
        
        //path
        std::string path;
        path = uri.substr(uri.find("/") +1, found);
        
        
        //host
        std::string hostLine;
        std::string hostName;
        std::string placeHolder2 = "";
        ss >> placeHolder2;
        ss >> hostName;
        std::cout << "HOSTNAME IS: " << hostName << std::endl;
        
        
        //get IP address
        const char* hostNameC = hostName.c_str();
        struct hostent *lh = gethostbyname(hostNameC);
        struct in_addr **address_list = (struct in_addr **)lh->h_addr_list;
        std::cout << "Address is: " << address_list[0]->s_addr << std::endl;

        //std::cout << address_list[0] << std::endl;
        
        
        if (!lh) {
            error = "<html><body text='red'>404 Not Found Reason URL does not exist: " + uri + "<body></html>"; //if not send back 404...terrible
            std::string notFound = version + " 404 Not Found\r\nContent-Type: text/html\r\nContent-Length:  " + std::to_string(error.length()) + "\r\n\r\n" + error;
            write(conn_fd, notFound.c_str(), notFound.length()*sizeof(char));
            close(conn_fd);
            return;
        }
        
        bool keep = false;
        std::string placeHolder = "";
        std::string connectionLine;
        
        getline(ss, placeHolder, '\n');
        getline(ss, connectionLine, '\n'); //parse header for connection line to see if it has a keep-alive
        if (connectionLine == "Connection: Keep-alive\r" ) {
            keep = true;
        }
        
        if(keep) { //if it does have a keep alive, then we want to create a timer set at 10 seconds
            struct timeval tv; //construct timeval with 10 seconds
            tv.tv_sec = 10;
            setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv)); //timeout occurs at 10 seconds
        }
        
        if((method != "GET") || (version != "HTTP/1.0" && version != "HTTP/1.1")){ //error check on request
            error = "<html><body>400 Bad Request</body></html>"; //send back appropriate headers
            requestError = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(error.length()) + "\r\n\r\n" + error;
            write(conn_fd, requestError.c_str(), requestError.length()*sizeof(char));
            close(conn_fd);
            return;
        }
        
        
        bool cached = false;
        
        if (cache.find(uri) == cache.end()) {
            std::cout << "Not found in cache" << std::endl;
        }
        else {
            
        }
        
        struct sockaddr_in address;
        int sock = 0;
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Socket creation error \n");
            return;
        }
        
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(portNum);
        struct in_addr addr;
        addr.s_addr = address_list[0]->s_addr;
        char *s = inet_ntoa(addr);
        std::cout << "IP addr is " << s << std::endl;
        
        
        if(inet_pton(AF_INET, s, &serv_addr.sin_addr)<=0)
        {
            printf("\nInvalid address/ Address not supported \n");
            //return;
        }
        
        std::cout << "Trying to connect to " << uri << std::endl;
        std::cout << std::endl;

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("\nConnection Failed \n");
            return;
        }
        std::cout << "Established connection with " << uri << std::endl;
        std::cout << std::endl;
        
        
        char serverResponse[BUFSIZE];
        int valread = 0;
        write(sock, receivedMessage, sizeof(receivedMessage));
        std::cout << "THE CLIENT SAYS" << std::endl;
        std::cout << "-------------------------------------" << std::endl;
        std::cout << receivedMessage << std::endl;
        std::cout << "-------------------------------------" << std::endl;

        
        int bytesRead;
        std::ofstream content;
        content.open("temp.txt");
        
        while(read(sock, serverResponse, BUFSIZE) > 0) {
            //std::cout << "THE SERVER SAYS" << std::endl;
            //std::cout << "-------------------------------------" << std::endl;
            //std::cout << serverResponse << std::endl;
            //std::cout << "-------------------------------------" << std::endl;
            std::string responseCacheVer(serverResponse, sizeof(serverResponse));
            m.lock();
            cache[uri].append(responseCacheVer);
            m.unlock();
            content << serverResponse;
            bzero(serverResponse, BUFSIZE);
        }
        
        write(conn_fd, cache[uri].c_str(), sizeof(cache[uri].c_str()));
        

        
        //for(auto it = cache.cbegin(); it != cache.cend(); ++it)
        //{
        //    std::cout << it->first << " " << it->second << std::endl;
        //}

        std::cout << "Done with " << uri << std::endl;
    }
}

int main(int argc, char*argv[]) {
    // arg parsing and error checkin
    
    int port;
    try {
        port = std::stoi(argv[1]);
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << '\n';
        return 1;
    }
    if(port <= 1024) {
        std::cout << "port must be greater than 1024" << std::endl;
        return 1;
    }
    // setup
    struct sockaddr_in servaddr;
    int sock_fd, conn_fd;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);
    
    bind(sock_fd,(struct sockaddr *) &servaddr, sizeof(servaddr));
    listen(sock_fd, 128);
    
    std::cout << "Started proxy with:" << std::endl << "host - localhost" << std::endl << "port - "<< port << std::endl;
    while(1) {
        conn_fd = accept(sock_fd,(struct sockaddr*) NULL, NULL);
        std::thread threader(connection_handler, conn_fd);
        threader.join();
    }
    close(sock_fd);
}
