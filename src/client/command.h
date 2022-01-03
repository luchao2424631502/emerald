#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "core.h"
#include <bson/src/util/json.h>
#include "ossSocket.h"

#define COMMAND_QUIT        "quit"
#define COMMAND_INSERT      "insert"
#define COMMAND_QUERY       "query"
#define COMMAND_DELETE      "delete"
#define COMMAND_HELP        "help"
#define COMMAND_CONNECT     "connect"
#define COMMAND_TEST        "test"
#define COMMAND_SNAPSHOT    "snapshot"

#define RECV_BUF_SIZE     4096
#define SEND_BUF_SIZE     4096

#define EDB_QUERY_INVALID_ARGUMENT  -101
#define EDB_INSERT_INVALID_ARGUMENT -102
#define EDB_DELETE_INVALID_ARGUMENT -103

#define EDB_INVALID_RECORD          -104
#define EDB_RECV_DATA_LENGTH_ERROR  -107

#define EDB_SOCK_INIT_FAILED        -113
#define EDB_SOCK_CONNECT_FAILED     -114
#define EDB_SOCK_NOT_CONNECT        -115
#define EDB_SOCK_REMOTE_CLOSED      -116
#define EDB_SOCK_SEND_FAILED        -117

#define EDB_MSG_BUILD_FAILED        -119

class ICommand 
{
  //函数指针
  typedef int (*OnMsgBuild)(char **ppBuffer,int *pBufferSize,bson::BSONObj &obj);
  public:
    //所有命令都要继承的虚函数
    virtual int execute(ossSocket &sock,std::vector<std::string> &argVec);
    int getError(int code);
  protected:
    //接收传入的数据包
    int recvReply(ossSocket &sock);
    //发送消息+执行传入的回调函数
    int sendOrder(ossSocket &sock,OnMsgBuild onMsgBuild);
    //发送消息
    int sendOrder(ossSocket &sock,int opCode);
  protected:
    virtual int handleReply() { return EDB_OK; };
  protected:
    char _recvBuf[RECV_BUF_SIZE];
    char _sendBuf[SEND_BUF_SIZE];
    std::string _jsonString;
};

//每个命令都是继承非纯接口ICommand来
class ConnectCommand:public ICommand 
{
  public:
    int execute(ossSocket &sock,std::vector<std::string> &argVec);
  private:
    std::string _address;
    int _port;
};

class QuitCommand:public ICommand 
{
  public:
    int execute(ossSocket &sock,std::vector<std::string> &argVec);
  protected:
    int handleReply();
};

class HelpCommand:public ICommand
{
  public:
    int execute(ossSocket &sock,std::vector<std::string> &argVec);
};


#endif
