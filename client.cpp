
// CLIENT FILE 
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic> // flag for ending the program 
#include <fcntl.h> // nonblocking input for linux https://www.ibm.com/docs/en/zvm/7.3?topic=functions-fcntl

// 127.0.0.1 for loop around testing 

// compile like g++ -o client client.cpp 
// run like ./client IP Address 
// Type exit or ctrl+c terminate whenever 
// packet is only message as setting packet struct was something I was unable to work 


using namespace std;

#define PORT 50000 // port 50000 for entire program

atomic<bool> done(false); // atomic boolean used for checking when exit to terminate program https://stackoverflow.com/questions/64910350/pattern-for-thread-to-check-if-it-should-stop


/* packet header 
// struct Packet{
//      char payload[1024];
        char version[8];
        int time; 
        int ID; 
        int checksum;
}*/ 

// SEND MESSAGE FUNCTION 
void sendMessage(int sockfd) {
    char buffer[1024];
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    while (!done) {
        memset(buffer, 0, sizeof(buffer));
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            ssize_t sentBytes = send(sockfd, buffer, strlen(buffer), 0);
           
            // if it is exit in buffer, end program 
            if (sentBytes < 0){
                cout << "Error sending message to server" << endl;
            }
            if (strncmp(buffer, "exit", 4) == 0) {
                done = true;
                shutdown(sockfd, SHUT_WR); // Stop further sends
                break;
            }
            cout<<"Message sent\n"<<endl; // server acknowledgement I think 
        }
        usleep(10000);
    }
}

// RECEIVE MESSAGE FUNCTION 
void receiveMessages(int sockfd) {
    char buffer[1024]; // non blocking with buffer
    int valread; // number of bits read

    fcntl(sockfd, F_SETFL, O_NONBLOCK); // SET NON BLOCK USING FNCTL!!!!!!!! 
    // while loop atomic boolean is not done 
    while (!done) {
        memset(buffer, 0, sizeof(buffer));
        valread = read(sockfd, buffer, sizeof(buffer) - 1); // valread to read bits from socket https://stackoverflow.com/questions/64761140/read-from-a-socket-without-knowing-the-size-of-the-buffer
        if (valread > 0) {
            buffer[valread] = '\0';
            cout << "Server: " << buffer << endl;
        } 
        usleep(100000);
    }
}

int main(int argc, char *argv[]) { //input ip address 
    if (argc != 2) {
        cout << "Error: run program like ./client IP-Address-Value(Use 127.0.0.1 for self) " << endl;
        return -1;
    }
    const char *serverIP = argv[1]; // server ip is the input when running client code 

    // STEP 1 Create client socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // socket
    if (sockfd < 0) {
        cout << "Client socket error " << endl;
        close(sockfd); 
        return -1;
    }
    //cout << "Client socket created" << endl;
    // set server address
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    // convert and check ip address of server
    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0) {
        cout << "IP not working" << endl;
        close(sockfd);
        return -1;
    }
    cout << "Server IP " << serverIP << endl;

    //connect
    if (connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        cout << "Error connecting to server" << endl;
        close(sockfd);
        return -1;
    }
    cout << "Connected to server type 'exit' to disconnect" << endl;


    // use thread for sending and receiving message functions
    thread sendThread(sendMessage, sockfd); 
    thread receiveThread(receiveMessages, sockfd);

    sendThread.join();
    receiveThread.join();


    // close the sockets 
    cout<<"Terminating client"<<endl;
    close(sockfd);
    return 0;
}
