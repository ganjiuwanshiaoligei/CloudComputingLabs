#ifndef _COORDINATE_H_
#define _COORDINATE_H_
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <map>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include "Global.h"
using namespace std;
#define SIZE 1000
class Coordinate{
    protected:
        int participantNUm;
        string filename;
        string ip;
        int port;
        int coordinateFd;
        vector <pair<string,int> >participant;
        map<pair<string,int>,int> participantSocket;
        void setnonblockingmode(int fd);
        map<int,int> count;
    public:
        friend class ClientProc;
        Coordinate(string name);
        void createKVserver();
        void run();
        friend void* heartProc(void* arg);
        
};

#endif
