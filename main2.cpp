#include<iostream>
#include<vector>
#include<unistd.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<string.h>
#include<signal.h>
#include<dirent.h>
#include<pthread.h>
#include <assert.h>
#include<arpa/inet.h>

#include"threadspool2.hpp"
#include"options.h"

using namespace std;

//ss -tanp | grep <pid>     查看当前进程的所有tcp连接


int main()
{
    int ret;
    // ret=setsid();
    // assert(ret!=-1);
    // ret=chdir(work_path.c_str());
    // assert(ret!=-1);
    // umask(0000);
    // ret=close(STDIN_FILENO);
    // assert(ret!=-1);
    // ret=close(STDOUT_FILENO);
    // assert(ret!=-1);
    // ret=close(STDERR_FILENO);
    // assert(ret!=-1);

    //打开日志文件
    log_pathfd=open(log_path.c_str(),O_RDWR|O_CREAT|O_TRUNC,0664);
    assert(log_pathfd!=-1);

    //初始化线程池
    ThreadsPool *threadspool=ThreadsPool::get_instance();

    //socket_tcp通信预备三件套
    int lfd=socket(AF_INET,SOL_SOCKET,0);
    assert(lfd!=-1);
    int opt=1;
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,(socklen_t)sizeof(opt));
    struct sockaddr_in server;
    server.sin_addr.s_addr=htonl(INADDR_ANY);
    server.sin_port=htons(port);
    server.sin_family=AF_INET;
    ret=bind(lfd,(sockaddr*)&server,(socklen_t)sizeof(server));
    assert(ret!=-1);
    ret=listen(lfd,128);
    assert(ret!=-1);
    
    //建立监听事件表
    int epfd=epoll_create(66);
    struct epoll_event ev;
    ev.data.fd=lfd;
    ev.events=EPOLLIN;
    ret=epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);
    assert(ret!=-1);

    string start_mess="all is ok\n";
    write(log_pathfd,start_mess.c_str(),start_mess.size());

    while (1)
    {
        epoll_event events[max_event_num];
        int event_num=epoll_wait(epfd,events,max_event_num,-1);
        assert(event_num>=0);
        for(int i=0;i<event_num;i++)
        {
            //如果监听到lfd有事件发生，则将连接形成的cfd纳入到事件监听范围中
            if(events[i].data.fd==lfd)
            {
                struct sockaddr_in client;
                socklen_t size=sizeof(client);
                int cfd=accept(lfd,(sockaddr *)&client,&size);
                assert(cfd!=-1);
                char client_ip[16]={0x00};
                inet_ntop(AF_INET,&client.sin_addr.s_addr,client_ip,(socklen_t)sizeof(client_ip));
                int client_port=htons(client.sin_port);
                string join_mess="join    client IP:"+string(client_ip)+":"+to_string(client_port)+"   cfd:"+to_string(cfd)+"\n";
                pthread_mutex_lock(&log_mutex);
                ret=write(log_pathfd,join_mess.c_str(),join_mess.size());
                assert(ret!=-1);
                pthread_mutex_unlock(&log_mutex);
                struct epoll_event ev2;
                ev2.data.fd=cfd;
                ev2.events=EPOLLIN|EPOLLET|EPOLLONESHOT;
                //ev2.events=EPOLLIN|EPOLLET;
                ret=epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ev2);
                assert(ret!=-1);
            }
            //如果监听到cfd，则将任务插入到工作队列，由线程池执行
            else if(events[i].events==EPOLLIN)
            {
                WorkData workdata; 
                workdata.epfd=epfd;
                workdata.fd=events[i].data.fd;
                workdata.af=(int)EPOLLIN;
                threadspool->add_work(workdata);
            }
            else if(events[i].events==EPOLLOUT)
            {

            }
        }
    }
    return 0;
}