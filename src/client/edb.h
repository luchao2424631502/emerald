#ifndef _EDB_H__
#define _EDB_H__

#include "core.h"
#include "ossSocket.h"
#include "commandFactory.h"

const int CMD_BUFFER_SIZE = 512;

class Edb
{
  public:
    Edb(){}
    ~Edb(){}
  public:
    void start();
  protected:
    void prompt();
  private:
    //从string中将命令分割
    void split(const std::string &text,char delim,std::vector<std::string> &result);
    //读取一行用户输入的内容
    char *readLine(char *p,int length);
    //读取用户所有的输入,存放到_cmdBuffer[]中
    //1.打印提示符的内容 2.打印几个缩进
    int readInput(const char *pPrompt,int numIndent);
  private:
    //用户client的socket
    ossSocket _sock;
    //通过命令工厂类获得具体的命令class
    CommandFactory _cmdFactory;
    //socket通信的shell缓冲区
    char _cmdBuffer[CMD_BUFFER_SIZE];
};

#endif

