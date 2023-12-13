#include<iostream>
#include<cstring>

using namespace std;

const string ok_200_title = "HTTP/1.1 200 OK\r\n";
const string error_400_title = "HTTP/1.1 400 Bad Request";
const string error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const string error_403_title = "HTTP/1.1 403 Forbidden\r\n";
const string error_403_form = "You do not have permission to get file from this server.\n";
const string error_404_title = "HTTP/1.1 404 Not Found\r\n";
const string error_404_form = "The requested file was not found on this server.\n";
const string error_500_title = "HTTP/1.1 500 Internal Error\r\n";
const string error_500_form = "There was an unusual problem serving the requested file.\n";
const string doc_root = "/var/www/html";

string get_file_type(string name)
{
    
    char* dot;
    dot = strrchr((char *)name.c_str(), '.');	//自右向左查找‘.’字符;如不存在返回NULL
    /*
     *charset=iso-8859-1	西欧的编码，说明网站采用的编码是英文；
     *charset=gb2312		说明网站采用的编码是简体中文；
     *charset=utf-8			代表世界通用的语言编码；
     *						可以用到中文、韩文、日文等世界上所有语言编码上
     *charset=euc-kr		说明网站采用的编码是韩文；
     *charset=big5			说明网站采用的编码是繁体中文；
     *
     *以下是依据传递进来的文件名，使用后缀判断是何种文件类型
     *将对应的文件类型按照http定义的关键字发送回去
     */
    if (dot == (char*)0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

string analysis_mess(string mess)
{
    string mess2;
    for(char c:mess)
    {
        mess2.push_back(c);    
        if(c=='\n')   break;
    }
    char reqType[16] = {0};
	char fileName[255] = {0};
	char protocal[16] = {0};
    sscanf(mess2.c_str(),"%[^ ] /%[^ ] %[^\r\n]",reqType,fileName,protocal);
    return string(fileName);
}



string send_httpmess(int stat,string fileName,int filelen)
{
    string mess;
    switch (stat)
    {
    case 100:
        /* code */
        break;
    case 200:
        mess=ok_200_title;
        break;
    case 404:
        mess=error_404_title;
        break;
    default:
        break;
    }
    mess+="Server: YYTWS/1.1\r\n";
    mess+="Content-Length: "+to_string(filelen)+"\r\n";
    mess+="Content-Type: "+get_file_type(fileName)+"\r\n";
    mess+="\r\n";
    return mess;
}