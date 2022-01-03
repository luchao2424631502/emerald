#include <iostream>
#include <sstream>
#include "core.h"
#include "edb.h"
#include "command.h"

const char SPACE      = ' ';
const char TAB        = '\t';
//反斜线
const char BACK_SLANT = '\\';
const char NEW_LINE   = '\n';

//什么时候退出
int gQuit = 0;

void Edb::start()
{
  std::cout << "Welcome to EmeraldDB Shell!" << std::endl;
  std::cout << "edb help for help,Ctrl+c or quit to exit" << std::endl;
  while(0 == gQuit)
  {
    //打印提示符
    prompt();
  }
}

//打印提示符,等待接收命令
void Edb::prompt()
{
  int ret = EDB_OK;
  //读取用户输入
  ret = readInput("edb",0);
  if (ret)//有问题就退出
  {
    return ;
  }

  //用户输入的字符串
  std::string textInput = _cmdBuffer;
  //分割字符串
  std::vector<std::string> textVec;
  split(textInput,SPACE,textVec);

  //用来统计第一个命令和后面跟着的参数
  int count = 0;
  std::string cmd = "";
  std::vector<std::string> optionVec;

  std::vector<std::string>::iterator iter = textVec.begin();
  ICommand *pCmd = nullptr;
  for(; iter!=textVec.end(); ++iter)
  {
    std::string str = *iter;
    //第一个命令
    if(count == 0)
    {
      cmd = str;
      count++;
    }
    //后面的参数
    else 
    {
      optionVec.push_back(str);
    }
  }
  //得到命令 对应的command派生类
  pCmd = _cmdFactory.getCommandProcesser(cmd.c_str());
  if(NULL != pCmd)
  {
    //virtual int execute(ossSocket &sock,std::vector<std::string> &argVec);
    pCmd->execute(_sock,optionVec);
  }
}

//打印shell提示符,并且读取用户输入
int Edb::readInput(const char *pPrompt,int numIndent)
{
  memset(_cmdBuffer,0,CMD_BUFFER_SIZE);
  for(int i=0; i<numIndent; i++)
  {
    std::cout << TAB;
  }
  std::cout << pPrompt << "> ";
  
  //读取一行输入
  readLine(_cmdBuffer,CMD_BUFFER_SIZE-1);
  int curBufLen = strlen(_cmdBuffer);
  // 有\表示继续读取输入(因为是一个命令里面的)
  while(_cmdBuffer[curBufLen-1] == BACK_SLANT && (CMD_BUFFER_SIZE-curBufLen) > 0)
  {
    for(int i=0; i<numIndent; i++)
      std::cout << TAB;
    std::cout << "> ";
    //把\覆盖掉
    readLine(&_cmdBuffer[curBufLen-1],CMD_BUFFER_SIZE-curBufLen);
  }

  curBufLen = strlen(_cmdBuffer);
  for(int i=0; i<curBufLen; i++)
  {
    if(_cmdBuffer[i] == TAB)
    {
      _cmdBuffer[i] = SPACE;
    }
  }
  return EDB_OK;
}

char *Edb::readLine(char *p,int length)
{
  int len = 0;
  int ch;
  //new_line == \n
  while((ch = getchar()) != NEW_LINE)
  {
    switch(ch)
    {
      //碰到\:
      case BACK_SLANT:
        break;
      default:
        p[len++] = ch;
    }
  }
  len = strlen(p);
  p[len] = 0;
  return p;
}

void Edb::split(const std::string &text,char delim,std::vector<std::string> &result)
{
  size_t strLen = text.length();
  size_t first = 0;
  size_t pos = 0;
  for(first = 0; first < strLen; first = pos+1)
  {
    pos = first;
    //delim作为分隔符,找每一个小段字符串
    while(text[pos] != delim && pos<strLen)
    {
      pos++;
    }
    std::string str = text.substr(first,pos-first);
    result.push_back(str);
  }
  return ;
}

int main(int argc,char **argv)
{
  Edb edb;
  edb.start();
  return 0;
}

