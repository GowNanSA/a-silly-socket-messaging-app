// SERVER FILE 

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic>
#include <fcntl.h> // nonblocking input for linux https://www.ibm.com/docs/en/zvm/7.3?topic=functions-fcntl

// compile like g++ -o server server.cpp 
// run like ./server
// Type ctrl+c to terminate because server will continue listening for client after client disconnected 

using namespace std;
#define PORT 50000

atomic<bool> done(false); //https://stackoverflow.com/questions/64910350/pattern-for-thread-to-check-if-it-should-stop
int clientSocket;
int serverSocket; 

void *serverReceive(void *arg) {
    clientSocket = *((int *)arg);
    char buffer[1024];
    int valread;
    fcntl(clientSocket, F_SETFL, O_NONBLOCK);
    while (!done) {
        memset(buffer, 0, sizeof(buffer));
        valread = read(clientSocket, buffer, sizeof(buffer) - 1); // number of bits read from socket 
        if (valread > 0) {
            buffer[valread] = '\0';
            cout << "Client: " << buffer << endl;
        } 
        else if (valread == 0 ) {
            cout << "Client disconnected" << endl;
            break;
        }
        usleep(10000);
    }
    close(clientSocket);
    pthread_exit(NULL);
}

void *serverSend(void *arg) {
    char buffer[1024];
    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK); // set to nonblocking input 
   
    // while the server is on 
    while (!done) {
        memset(buffer, 0, sizeof(buffer));
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            ssize_t sentBytes = send(clientSocket, buffer, strlen(buffer), 0); // buffer
            if (sentBytes < 0) {
                cout << "Error sending message to client" << endl;
            } else {
                cout << "Message sent\n" << endl; // server acknowledgement 
            }
        }
    }
    pthread_exit(NULL);
}

int main() {
    // STEP 1 Create Server Scoekt 
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cout << "Server socket error"  << endl;
        return -1;
    }
    cout << "Server socket created" << endl;

    // SERVER ADDRESS 
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(PORT);

    // STEP 2 BIND SOCKET
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        cout << "Error binding socket" << endl;
        close(serverSocket);
        return -1;
    }
    cout << "Server binding success to port " << PORT << endl;

    // LISTEN AND ACCEPT CLIENT!!!
    if (listen(serverSocket, 5) < 0) {
        cout << "Error listening"  << endl;
        close(serverSocket);
        return -1;
    }
    cout << "Server listening on port, type 'ctrl+c' to terminate server." << endl;


    // Use threads to send and receive 
    pthread_t inputThread;
    if (pthread_create(&inputThread, NULL, serverSend, NULL) != 0) { //make input thread 
        cout << "Thread error" << endl;
        close(serverSocket);
        return -1;
    }

    // LISTEN 
    while (!done) {
        struct sockaddr_in clientAddress;
        socklen_t addrLen = sizeof(clientAddress);

        // accept when client runs 
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addrLen);

        // display client ip
        cout << "Client connected: " << inet_ntoa(clientAddress.sin_addr) << " " << ntohs(clientAddress.sin_port)  << endl;
        pthread_t thread;
        if (pthread_create(&thread, NULL, serverReceive, (void *)&clientSocket) != 0) {
            cout << "Failed to create thread for client" << endl;
            close(clientSocket);
        } 
        else {
            pthread_detach(thread);
        }
    }

    // close the sockets 
    close(serverSocket);
    return 0;
}
