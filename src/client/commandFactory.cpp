#include "commandFactory.h"

CommandFactory::CommandFactory()
{
  addCommand(); //添加命令
}

//用字符串返回具体的derived class对象
ICommand *CommandFactory::getCommandProcesser(const char *pcmd)
{
  ICommand *pProcessor = nullptr;
  do {
    COMMAND_MAP::iterator iter;
    iter = _cmdMap.find(pcmd); //由传入的string作为key找到对应的命令对象
    //找到了
    if (iter != _cmdMap.end())
    {
      pProcessor = iter->second;
    }
  }while(0);
  return pProcessor;
}

