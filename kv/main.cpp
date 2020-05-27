#include <getopt.h>
#include "Coordinate.h"
#include "Global.h"
#include "Participant.h"
pthread_mutex_t  cheartLock,pheartLock;
pthread_cond_t pheartcond;

int main(int argc,char *argv[]){
	if(argc!=3){
        printf("usage : %s ip port\n",argv[0]);
        exit(1);
    }
    int sock=socket(PF_INET,SOCK_STREAM,0);

    int opt=1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(void*)&opt,sizeof(opt));

    struct sockaddr_in ser;
    ser.sin_family=AF_INET;
    ser.sin_port=htons(atoi(argv[2]));
    ser.sin_addr.s_addr=inet_addr(argv[1]);

    if(connect(sock,(struct sockaddr*)&ser,sizeof(ser))==-1){
        printf("client connect error\n");
        exit(1);
    }
    while(1){
        string message;
        cin>>message;
        if(message=="q"||message=="Q") break;
        send(sock,message.c_str(),message.length(),0);
        sleep(4);
        char buf[4096];
        int len=recv(sock,buf,4095,0);
        buf[len]='\0';
        cout<<buf<<endl;
    }
    close(sock);
    if(argc<3){
        printf("Usage : %s  --config_path <configuration file>\n",argv[0]);
        exit(1);
        return 1;
    }
    int opt;
    const char *oprstring="c:";
    struct option longopt[]={
        {"config_path",1,NULL,'c'},
        {NULL,0,NULL,0}
    };
    string filename;
    opt=getopt_long(argc,argv,oprstring,longopt,NULL);
    if(opt!=-1&&opt=='c'){
        filename=optarg;
    }
   ifstream filefd;
   filefd.open(filename);
   if(!filefd.is_open()){
       printf("file %s does not exist\n",filename.c_str());
       return 1;
   }
   string mode="";
   string buf;
   while(getline(filefd,buf)){
       int pos=0;
       if(buf[0]=='!') continue;
       if((pos=buf.find("mode"))!=-1){
           mode=buf.substr(pos+5);
           break;
       }
   }
   if(mode.back()=='\r'||mode.back()=='\n')mode.pop_back();
   filefd.close();
   pthread_mutex_init(&cheartLock,NULL);
    pthread_mutex_init(&pheartLock,NULL);
    pthread_cond_init(&pheartcond,NULL);
   if(mode=="coordinator"){
       //cout<<"coordinator start"<<endl;
       Coordinate c(filename);
       c.createKVserver();
       c.run();
   }
   else {
       //cout<<"participant start"<<endl;
       Participant p(filename);
       p.pRunning();
   }

   pthread_mutex_destroy(&cheartLock);
   pthread_mutex_destroy(&pheartLock);
   pthread_cond_destroy(&pheartcond);

}
