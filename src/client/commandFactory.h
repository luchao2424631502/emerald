#ifndef _COMMANDFACTORY_H_
#define _COMMANDFACTORY_H_

#include "command.h"
#define COMMAND_BEGIN void CommandFactory::addCommand() {
#define COMMAND_END }
#define COMMAND_ADD(cmdName,cmdClass) {\
  ICommand *pObj = new cmdClass();\
  std::string str = cmdName;\
  _cmdMap.insert(COMMAND_MAP::value_type(str,pObj));\
  }\

class CommandFactory 
{
  typedef std::map<std::string,ICommand*> COMMAND_MAP;
  public:
    CommandFactory();
    ~CommandFactory(){}
    //在command.cpp中用上方定义的宏来实现
    void addCommand();
    //生成对应的命令就是返回base class的指针,具体指向的是派生类
    ICommand *getCommandProcesser(const char *pcmd);
  private:
    COMMAND_MAP _cmdMap;
};

#endif
