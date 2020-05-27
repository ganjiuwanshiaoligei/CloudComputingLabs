#include "Coordinate.h"
#include "Cthreadpool.h"

void* heartProc(void* arg){
    Coordinate* c=(Coordinate*) arg;
    while(1){    
        //Coordinate
        pthread_mutex_lock(&cheartLock);
        for(int i=0;i<(int)(c->participant).size();i++){
            
            int socket=(c->participantSocket)[(c->participant[i])];
            write(socket,"&&&&",4);
            char buf[10];
            int length=recv(socket,buf,9,MSG_DONTWAIT);
            if(length>0){
                buf[length]='\0';

                c->count[socket]=0;
            }
            else{
                c->count[socket]++;
                if(c->count[socket]==5){
                    cout<<"participant ("<<c->participant[i].first<<":"<<c->participant[i].second<<") died"<<endl;
                    close(socket);
                    c->count.erase(socket);
                    c->participantSocket.erase(c->participant[i]);
                    c->participant.erase(c->participant.begin()+i);
                    c->participantNUm--;
                    i--;
                }
            }
            
        }
        pthread_mutex_unlock(&cheartLock);
        sleep(4);
    }
    return NULL;
}

void Coordinate::setnonblockingmode(int fd){
    int flag =  fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,flag|O_NONBLOCK);
}

Coordinate::Coordinate(string name):filename(name){
    ifstream in;
    in.open(filename);
    string buf;
    while(getline(in,buf)){
        char c=buf[0];
        switch(c){
            case '!':
            case 'm':{
                break;
            }
            case 'c':{
                string ip_and_port=buf.substr(17);
                int pos=ip_and_port.find(":");
                ip=ip_and_port.substr(0,pos);
                port=atoi(((ip_and_port.substr(pos+1))).c_str());
                break;
            }
            case 'p':{
                string ip_and_port=buf.substr(17);
                int pos=ip_and_port.find(":");
                string tmpip=ip_and_port.substr(0,pos);
                int tmpport=atoi(((ip_and_port.substr(pos+1))).c_str());
                participant.push_back(make_pair(tmpip,tmpport));
                break;
            }
            default: break;
        }
        
    }
    in.close();
    participantNUm=participant.size();
    // cout<<"participants are : "<<endl;
    // for(int i=0;i<(int)participant.size();i++){
    //     cout<<participant[i].first<<" "<<participant[i].second<<endl;
    // }
    // cout<<"coordinate is :"<<endl;
    // cout<<ip<<" "<<port<<endl;
    // in.close();
}

void Coordinate::createKVserver(){
    struct sockaddr_in server;
    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr=inet_addr(ip.c_str());
    coordinateFd=socket(PF_INET,SOCK_STREAM,0);

    int option=1;
    socklen_t optlen=sizeof(option);
    setsockopt(coordinateFd,SOL_SOCKET,SO_REUSEADDR,(void*)&option,optlen);
    cout<<"coordinator : "<<ip<<" "<<port<<endl;
    if(bind(coordinateFd,(struct sockaddr*)&server,sizeof(server))==-1){
        printf("bind error\n");
        exit(1);
    }

    for(int i=0;i<(int)participant.size();i++){
        int fd=socket(PF_INET,SOCK_STREAM,0);
        struct sockaddr_in tmp;
        tmp.sin_family=AF_INET;
        tmp.sin_port=htons(participant[i].second);
        tmp.sin_addr.s_addr=inet_addr(participant[i].first.c_str());
        //cout<<participant[i].first<<" "<<participant[i].second<<endl;
       int ret=connect(fd,(struct sockaddr*)&tmp,sizeof(tmp));
        if(ret==-1){
            printf("connect error\n");
            participant.erase(participant.begin()+i);
            i--;
        }
        else{
             participantSocket[participant[i]]=fd;
             count[fd]=0;
        }
        //sleep(2);
    }

}

void Coordinate::run(){
    pthread_t heartThread;
    while(pthread_create(&heartThread,NULL,heartProc,(void*)this)!=0);
    pthread_detach(heartThread);

    if( listen(coordinateFd,1000)==-1){
       printf("listen error\n");
       exit(1);
    }
    setnonblockingmode(coordinateFd);
    int epfd=epoll_create(SIZE);
    struct epoll_event eve;
    eve.data.fd=coordinateFd;
    eve.events=EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,coordinateFd,&eve);

  
    Cthreadpool cthread(20,cMode,(void*)this);
    cthread.start();

    struct epoll_event *ep_events;
    ep_events=new epoll_event[SIZE];

    while(1){
         int event_cnt=epoll_wait(epfd,ep_events,SIZE,-1);
        if(event_cnt==-1){
            printf("epoll_wait() error\n");
            exit(1);
        }
        for(int i=0;i<event_cnt;i++){

            if(ep_events[i].data.fd==coordinateFd){
                struct sockaddr_in clnt_addr;
                socklen_t adr_sz=sizeof(clnt_addr);
                int clntsocket=accept(coordinateFd,(struct sockaddr*)&clnt_addr,&adr_sz);
                if(clntsocket!=-1){
        
                    setnonblockingmode(clntsocket);
                    eve.events=EPOLLIN|EPOLLET;
                    eve.data.fd=clntsocket;
                    
                    if(epoll_ctl(epfd,EPOLL_CTL_ADD,clntsocket,&eve)==-1){
                        
                    }
                    cthread.init_fd(clntsocket);
                   
                   
                }
            }
            else{
                if(participantNUm==0){
                    write(ep_events[i].data.fd,"-ERROR\r\n",15);
                }
                else cthread.queue_append(ep_events[i].data.fd);
            }
        }
    }

}
