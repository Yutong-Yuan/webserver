#pragma once

#include<iostream>

using namespace std;

//工作路径
const string work_path="/home/yyt/c_file/c_file1/project/webserver/webserver_yyt";
//打印日志路径
const string log_path="./work_log";
//最大可监听事件数量
const int max_event_num=65566;
//访问端口
const int port=8888;
//线程池线程数量
const int threads_num=6;
//工作队列最大容量
const int workqueue_num=50000;
//写缓冲区阻塞最大容忍时间
int64_t wait_time=5e6;
//读取请求信息的缓冲区大小
const int readbuf_size=2048;

//访问不存在页面时服务器的响应
const string nonfile_path="nonfile";

//日志文件描述符号，用于主线程和子线程共享
int log_pathfd;

//用于检测函数返回值
// int ret;