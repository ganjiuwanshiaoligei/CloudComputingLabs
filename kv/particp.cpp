#include "Participant.h"
#include <fstream>
#include <fcntl.h>
void Participant::setnonblockingmode1(int fd){
    int flag =  fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,flag|O_NONBLOCK);
}

void* heartprocess(void* arg){
    //cout<<"create"<<endl;
    Participant* p=(Participant*) arg;
    int counts=0;//计数
    while(1){    
        //cout<<"heart beat"<<endl;
        pthread_mutex_lock(&pheartLock);
        while(p->is_c_close){
            pthread_cond_wait(&pheartcond,&pheartLock);
        }
        //cout<<"here"<<endl;
        char buf[10];
        int length=recv(p->sock_to_cor,buf,9,MSG_DONTWAIT);
        if(length>0){
            counts=0;
        }
        else{
            counts++;
            if(counts==5){
                cout<<"coordinate died"<<endl;
                counts=0;
                close(p->sock_to_cor);
                p->is_c_close=true;
            }
        }
        
        write(p->sock_to_cor,"&&&&",4);
        
        pthread_mutex_unlock(&pheartLock);
        sleep(4);
    }
    return NULL;
}

Participant::Participant(string names):filename(names){
  
    is_c_close=true;
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
                cip=ip_and_port.substr(0,pos);
                cport=atoi(((ip_and_port.substr(pos+1))).c_str());
                break;
            }
            case 'p':{
                
                string ip_and_port=buf.substr(17);
                int pos=ip_and_port.find(":");
                myip=ip_and_port.substr(0,pos);
                myport=atoi(((ip_and_port.substr(pos+1))).c_str());
                
                break;
            }
            default: break;
        }
        
    }
    in.close();
}

void Participant::pRunning(){
    mysocket=socket(PF_INET,SOCK_STREAM,0);
   
    struct sockaddr_in ser_addr,clnt_addr;

    int option=1;
    socklen_t optlen=sizeof(option);
    setsockopt(mysocket,SOL_SOCKET,SO_REUSEADDR,(void*)&option,optlen);

    socklen_t clnt_sz=sizeof(clnt_addr);

    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(myport);
    ser_addr.sin_addr.s_addr=inet_addr(myip.c_str());
    cout<<"participant : "<< myip<<" "<<myport<<endl;
    if(bind(mysocket,(struct sockaddr*)&ser_addr,sizeof(ser_addr))==-1){
        printf("participant bind error\n");
        exit(-1);
    }
    listen(mysocket,200);
    
        
        int ret=accept(mysocket,(struct sockaddr*)&clnt_addr,&clnt_sz);
        if(ret==-1){
            printf("participant accept error!\n");
            exit(-1);
        }
        
        //char *tmpip=inet_ntoa(clnt_addr.sin_addr);
        //cout<<tmpip<<" "<<ntohs(clnt_addr.sin_port)<<endl;
        cout<<"coordinate connect successfully"<<endl;
           
        is_c_close=false;
        sock_to_cor=ret;
            
        
    
    pthread_t heartThread;
    while(pthread_create(&heartThread,NULL,heartprocess,(void*)this)!=0);
    pthread_detach(heartThread);

    Cthreadpool Ppool(20,pMode,this);
    Ppool.start();

    while(1){
        int fd=accept(mysocket,(struct sockaddr*)&clnt_addr,&clnt_sz);
        //cout<<fd<< " success"<<endl;
        
        sleep(1);
        char buf[6];
        int len=recv(fd,buf,5,MSG_DONTWAIT|MSG_PEEK);
        buf[len]='\0';
        if(len==4&&strcmp(buf,"&&&&")==0){
            cout<<"Coordinate restart"<<endl;
            is_c_close=false;
            sock_to_cor=fd;
            pthread_cond_signal(&pheartcond);
        }
        else{
            setnonblockingmode1(fd);
            Ppool.queue_append(fd);
        }
    }
}
