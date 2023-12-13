#include<iostream>
#include<pthread.h>
#include<deque>
#include<sys/epoll.h>
#include<unistd.h>
#include<fcntl.h>
#include <strings.h>
#include<arpa/inet.h>
#include<sys/sendfile.h>
#include <chrono>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include"options.h"
#include"http.hpp"

using namespace std;


//用于互斥访问工作队列的锁
pthread_mutex_t work_queuemutex;
//写日志的锁
pthread_mutex_t log_mutex;

//将文件描述符设置为非阻塞
void setnonblock(int &fd)
{
    int flags=fcntl(fd,F_GETFL);
    flags|=O_NONBLOCK;
    int ret=fcntl(fd,F_SETFL,flags);
    assert(ret!=-1);
}

//重置fd上的事件
void resetoneshot(int epfd,int fd)
{
    epoll_event ev;
    ev.data.fd=fd;
    ev.events=EPOLLIN|EPOLLET|EPOLLONESHOT;
    int ret=epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
    assert(ret!=-1);
}

//移除fd上的事件
void delaf(int epfd,int fd)
{
    int ret=epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
    assert(ret!=-1);
}

class WorkData
{
public:
    //事件表
    int epfd;
    //发生事件的文件描述符
    int fd;
    //发生的事件
    int af;
};

//线程池类（非单例模式）
class ThreadsPool
{
public:
    ThreadsPool();
    pthread_t *threads;
    static deque < WorkData >  *work_queue;
    void add_work(WorkData thing);
    static void *work (void *args);
private:

};

deque < WorkData > * ThreadsPool::work_queue=new deque < WorkData > [workqueue_num];

ThreadsPool::ThreadsPool()
{
    threads=new pthread_t [threads_num];
    pthread_mutex_init(&work_queuemutex,NULL);
    pthread_mutex_init(&log_mutex,NULL);
    int ret;
    for(int i=0;i<threads_num;i++)
    {
        ret=pthread_create(&threads[i],NULL,this->work,NULL);
        assert(ret==0);
        ret=pthread_detach(threads[i]);
        assert(ret==0);
    }
}

//该函数还需要优化
void ThreadsPool::add_work(WorkData thing)
{
    int ret;
    pthread_mutex_lock(&work_queuemutex);
    //当请求过多时，直接丢弃
    if((*work_queue).size()>workqueue_num)
    {
        pthread_mutex_unlock(&work_queuemutex);
        int epfd=thing.epfd;
        int fd=thing.fd;
        int af=thing.af;
        //获得对方的IP信息以及端口号
        struct sockaddr_in client;
        socklen_t addr_size=sizeof(client);
        ret=getpeername(fd,(sockaddr *)&client,&addr_size);
        assert(ret!=-1);
        char client_ip[16]={0x00};
        inet_ntop(AF_INET,&client.sin_addr.s_addr,client_ip,(socklen_t)sizeof(client_ip));
        int client_port=ntohs(client.sin_port);
        delaf(epfd,fd);
        ret=close(fd);
        assert(ret!=-1);
        //被动退出写到日志上
        string exit_mess="exit2   client IP:"+string(client_ip)+":"+to_string(client_port)+"\n";
        pthread_mutex_lock(&log_mutex);
        ret=write(log_pathfd,exit_mess.c_str(),exit_mess.size());
        assert(ret!=-1);
        pthread_mutex_unlock(&log_mutex); 
        return;
    }
    (*work_queue).push_back(thing);
    pthread_mutex_unlock(&work_queuemutex);
}

void * ThreadsPool::work (void *args)
{
    while (1)
    {
        int ret;
        if((*work_queue).empty())  continue;
        //对于工作队列上锁，防止临界区资源危险
        pthread_mutex_lock(&work_queuemutex);
        if((*work_queue).empty())
        {
            pthread_mutex_unlock(&work_queuemutex);
            continue;
        }
        WorkData thing=(*work_queue).front();
        (*work_queue).pop_front();
        pthread_mutex_unlock(&work_queuemutex);
        //取出要操作的文件描述符以及发生的事件
        int epfd=thing.epfd;
        int fd=thing.fd;
        int af=thing.af;
        //获得对方的IP信息以及端口号
        struct sockaddr_in client;
        socklen_t addr_size=sizeof(client);
        ret=getpeername(fd,(sockaddr *)&client,&addr_size);
        // if((int)errno==107)
        // {
        //     perror("");
        //     cout<<"yyt:";
        //     cout<<fd<<endl;
        // }
        assert(ret!=-1);
        char client_ip[16]={0x00};
        inet_ntop(AF_INET,&client.sin_addr.s_addr,client_ip,(socklen_t)sizeof(client_ip));
        int client_port=ntohs(client.sin_port);

        //收到的浏览器客户端发来的申请报头
        string httpmess;
        //申请的文件
        string fileName;
        //回复的http报头
        string resp_mess;

        //设置客户端退出标志
        int exit_flags=0;
        if(af==(int)EPOLLIN)
        {
            char buf[readbuf_size]={0x00};
            int n;
            setnonblock(fd);
            while(1)
            {
                bzero(buf,sizeof(buf));
                n=read(fd,buf,sizeof(buf)-1);
                if(n>0)
                {
                    //一直读取http报头
                    httpmess+=string(buf);
                }
                else if(n==-1)
                {
                    //浏览器客户端到底请求了什么数据（什么文件）
                    fileName=analysis_mess(httpmess);
                    string req_mess ="req     client IP:"+string(client_ip)+":"+to_string(client_port)+"   "+fileName+"\n";
                    pthread_mutex_lock(&log_mutex);
                    ret=write(log_pathfd,req_mess.c_str(),req_mess.size());
                    assert(ret!=-1);
                    pthread_mutex_unlock(&log_mutex);
                    resetoneshot(epfd,fd);
                    break;
                }
                else if(n==0)
                {
                    //浏览器客户端主动退出
                    string exit_mess="exit1   client IP:"+string(client_ip)+":"+to_string(client_port)+"\n";
                    pthread_mutex_lock(&log_mutex);
                    ret=write(log_pathfd,exit_mess.c_str(),exit_mess.size());
                    assert(ret!=-1);
                    pthread_mutex_unlock(&log_mutex);
                    //将事件移除
                    delaf(epfd,fd);
                    ret=close(fd);
                    assert(ret!=-1);
                    exit_flags=1;
                    break;
                }
            }
            if(exit_flags)      continue;
            struct stat st;  
            //判断请求的文件状态
            ret=lstat(fileName.c_str(),&st);
            if(ret!=-1)
            {
                //如果文件存在，发送http报头与该文件内容
                int file_fd=open(fileName.c_str(),O_RDONLY);
                string resp_mess=send_httpmess(200,fileName,st.st_size);
                write(fd,resp_mess.c_str(),resp_mess.size());
                int send_size,send_total=0;
                off_t send_pos=0;
                //等待时间最多是100ms
                int wait_flags=0;
                chrono::_V2::system_clock::time_point currentTime1,currentTime2;
                int64_t currentTimeUs1,currentTimeUs2;
                //若请求的文件过大，则需要循环发送
                while(1)
                {
                    if(send_total==st.st_size)   break;
                    send_size=sendfile(fd,file_fd,&send_pos,st.st_size);
                    //判断写缓冲区是否会一直阻塞5s，如果会的话就不往里面写了，防止线程一直被阻塞在同一事件
                    if(errno==EAGAIN && send_size==-1)  
                    {
                        if(wait_flags==0)
                        {
                            currentTime1 = std::chrono::high_resolution_clock::now();
                            currentTimeUs1 = std::chrono::time_point_cast<std::chrono::microseconds>(currentTime1).time_since_epoch().count();
                            wait_flags=1;
                            continue;
                        }
                        else
                        {
                            currentTime2 = std::chrono::high_resolution_clock::now();
                            currentTimeUs2 = std::chrono::time_point_cast<std::chrono::microseconds>(currentTime2).time_since_epoch().count();
                            if(currentTimeUs2-currentTimeUs1>=wait_time)    break;
                            else    continue;
                        }
                    }
                    send_total+=send_size;
                    send_pos=send_total;
                    wait_flags=0;
                    if(errno==EAGAIN)   continue;
                }
            }
            else
            {
                //如果文件不存在，发送http报头与相应响应文件内容
                int file_fd=open(nonfile_path.c_str(),O_RDONLY);
                lstat(nonfile_path.c_str(),&st);
                string resp_mess=send_httpmess(404,nonfile_path,st.st_size);
                write(fd,resp_mess.c_str(),resp_mess.size());
                sendfile(fd,file_fd,NULL,st.st_size);
            }
        }
    }
}

