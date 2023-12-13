#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>

using namespace std;

// static const char* request = "GET http://192.168.58.128:8899/main.cpp HTTP/1.1\r\nConnection: keep-alive\r\n\r\nxxxxxxxxxxxx";


int setnonblocking(int &fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void start_conn(int num, const char *ip, int port,int *cfds)
{
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    for (int i = 0; i < num; ++i)
    {
        //usleep(10);
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        ret=connect(sockfd,(sockaddr *)&address,(socklen_t)sizeof(address));
        if (ret < 0)
        {
            cout<<sockfd<<endl;
            i--;
            continue;
        }
        if(i%1000==999)     cout<<i<<endl;
        cfds[i]=sockfd;
        setnonblocking(cfds[i]);
    }
}

int main(int argc, char *argv[])
{
    sockaddr_in client,server;
    socklen_t size=sizeof(client);
    string request = (string)"GET /"+(string)argv[4]+(string)" HTTP/1.1\r\nConnection: keep-alive\r\n\r\nxxxxxxxxxxxx";
    int clients_num=atoi(argv[3]);
    int cfds[clients_num];
    start_conn(clients_num,argv[1],atoi(argv[2]),cfds);
    int write_failnum=0,read_failnum=0;
    for(int i=0;i<clients_num;i++)
    {
        int n=write(cfds[i],request.c_str(),request.size());
        //cout<<n<<endl;
        if(n<=0)    write_failnum++;
        //cout<<cfds[i]<<endl;
    }
    sleep(1);
    char buf[20480]={0x00};
    string mess;
    for(int i=0;i<clients_num;i++)
    {
        //sleep(100);
        mess.clear();
        int n,total=0;
        while (1)
        {
            //usleep(10);
            bzero(buf,sizeof(buf));
            n=read(cfds[i],buf,sizeof(buf));
            //cout<<n<<endl;
            if(n<=0)    
            {
                break;
            }     
            total+=n;
            mess+=string(buf);
        }
        cout<<total<<endl;
        //具体的大小是文件大小+http响应头大小
        if(total<3682)    
        {
            read_failnum++;
            cout<<total<<endl;
        }
        // getsockname(cfds[i],(sockaddr *)&server,&size);
        //getsockname(cfds[i],(sockaddr *)&client,&size);
        //cout<<htons(client.sin_port)<<":";
        //cout<<mess.size()<<endl;
        close(cfds[i]);
    }
    cout<<"write fail percent:"<<(1.0)*write_failnum/clients_num*100<<"%\n";
    cout<<"read fail percent:"<<(1.0)*read_failnum/clients_num*100<<"%\n";
    return 0;
}
