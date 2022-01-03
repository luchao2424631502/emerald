#include "command.h"
#include "core.h"
#include "commandFactory.h"

//用commandFactory中的宏来实现CommandFactory::addCommand()
//向 命令工厂中 添加不同命令的实现
COMMAND_BEGIN
COMMAND_ADD(COMMAND_CONNECT,ConnectCommand)
COMMAND_ADD(COMMAND_QUIT,QuitCommand)
COMMAND_ADD(COMMAND_HELP,HelpCommand)
COMMAND_END

extern int gQuit;

//base class的virtual函数
int ICommand::execute(ossSocket &sock,std::vector<std::string> &argVec)
{
  //任何命令执行返回对
  return EDB_OK;
}

//在返回错误码同时打印错误码
int ICommand::getError(int code)
{
  switch(code)
  {
    case EDB_OK:
      break;
    case EDB_IO:
      std::cout << "io error is occurred" << std::endl;
      break;
    case EDB_INVALIDARG:
      std::cout << "invalid argument" << std::endl;
      break;
    case EDB_PERM:
      std::cout << "edb_perm" << std::endl;
      break;
    case EDB_OOM:
      std::cout << "edb_oom" << std::endl;
      break;
    case EDB_SYS:
      std::cout << "system error is occurred" << std::endl;
      break;
    case EDB_QUIESCED:
      std::cout << "EDB_QUIESCED" << std::endl;
      break;
    case EDB_NETWORK_CLOSE:
      std::cout << "new work is closed" << std::endl;
      break;
    case EDB_HEADER_INVALID:
      std::cout << "record header is not right" << std::endl;
    case EDB_IXM_ID_EXIST:
      std::cout << "record key is exist" << std::endl;
      break;
    case EDB_IXM_ID_NOT_EXIST:
      std::cout << "record key is not exist" << std::endl;
      break;
    case EDB_NO_ID:
      std::cout << "_id is needed" << std::endl;
      break;
    case EDB_QUERY_INVALID_ARGUMENT:
      std::cout << "invalid query argument" << std::endl;
      break;
    case EDB_INSERT_INVALID_ARGUMENT:
      std::cout << "invalid insert argument" << std::endl;
      break;
    case EDB_DELETE_INVALID_ARGUMENT:
      std::cout << "invalid delete argument" << std::endl;
      break;
    case EDB_INVALID_RECORD:
      std::cout << "invalid record string" << std::endl;
      break;
    case EDB_SOCK_NOT_CONNECT:
      std::cout << "sock connection does not exist" << std::endl;
      break;
    case EDB_SOCK_REMOTE_CLOSED:
      std::cout << "remote sock connection is closed" << std::endl;
      break;
    case EDB_MSG_BUILD_FAILED:
      std::cout << "msg build failed" << std::endl;
      break;
    case EDB_SOCK_SEND_FAILED:
      std::cout << "sock send msg failed" << std::endl;
      break;
    case EDB_SOCK_INIT_FAILED:
      std::cout << "sock init failed" << std::endl;
      break;
    case EDB_SOCK_CONNECT_FAILED:
      std::cout <<"sock connect remote server failed" << std::endl;
      break;
    default:
      break;
  }
  return code;
}

int ICommand::recvReply(ossSocket &sock)
{
  //message data length
  int length = 0;
  int ret = EDB_OK;

  memset(_recvBuf,0,RECV_BUF_SIZE);
  //连接断开了
  if (!sock.isConnected())
  {
    return getError(EDB_SOCK_NOT_CONNECT);
  }
  
  while(1)
  {
    //消息协议是: 前4个字节是数据包的长度
    ret = sock.recv(_recvBuf,sizeof(int));
    if (EDB_TIMEOUT == ret)//返回的是自己定义的错误
    {
      //超时继续接收
      continue;
    }
    //网络错误
    else if (EDB_NETWORK_CLOSE == ret)
    {
      return getError(EDB_SOCK_REMOTE_CLOSED);
    }
    //正常返回
    else {
      break;
    }
  }

  //将4字节转化为数据包的长度
  length = *(int*)_recvBuf;
  //接收的字节超过缓冲区的大小(实际数据库应该做变长的)
  if (length > RECV_BUF_SIZE)
  {
    return getError(EDB_RECV_DATA_LENGTH_ERROR);
  }
  //接收剩下的数据:length-sizeof(int)
  while(true)
  {
    ret = sock.recv(&_recvBuf[sizeof(int)],length-sizeof(int));
    if (ret == EDB_TIMEOUT)
    {
      continue;
    }
    else if (ret == EDB_NETWORK_CLOSE)
    {
      return getError(EDB_SOCK_REMOTE_CLOSED);
    }
    //协议数据包接收成功
    else {
      break;
    }
  }
  return ret;
}

int ICommand::sendOrder(ossSocket &sock,OnMsgBuild onMsgBuild)
{
  int ret = EDB_OK;
  bson::BSONObj bsonData;
  try {
    bsonData = bson::fromjson(_jsonString);
  }
  catch(std::exception &e)
  {
    return getError(EDB_INVALID_RECORD);
  }

  memset(_sendBuf,0,SEND_BUF_SIZE);//清空发送缓冲区
  int size = SEND_BUF_SIZE;
  char *pSendBuf = _sendBuf;
  //根据json->bson->构建sendBuf的内容
  ret = onMsgBuild(&pSendBuf,&size,bsonData);
  if(ret)
  {
    return getError(EDB_MSG_BUILD_FAILED);
  }

  //发送消息
  ret = sock.send(pSendBuf,*(int *)pSendBuf);
  if (ret)
  {
    return getError(EDB_SOCK_SEND_FAILED);
  }
  return ret;
}

//
int ICommand::sendOrder(ossSocket &sock,int opCode)
{
  int ret = EDB_OK;
  memset(_sendBuf,0,SEND_BUF_SIZE);
  char *pSendBuf = _sendBuf;
  const char *pStr = "hello world";
  //在buf的首4字节填充数据包的长度,4(int)+内容+\n
  *(int *)pSendBuf = strlen(pStr) + 1 + sizeof(int);
  //copy内容进入sendBuf
  memcpy(&pSendBuf[4],pStr,strlen(pStr)+1);

  ret = sock.send(pSendBuf,*(int *)pSendBuf);
  return ret;
}

//1.实现第一个命令:连接
int ConnectCommand::execute(ossSocket &sock,
                          std::vector<std::string> &argVec)
{
  int ret = EDB_OK;
  _address = argVec[0];
  _port = atoi(argVec[1].c_str()); //char *->int
  sock.close();
  //void setAddress(const char *pHostName,unsigned int port);
  sock.setAddress(_address.c_str(),_port);
  //由填入的信息初始化socket
  ret = sock.initSocket();
  if (ret)
  {
    return getError(EDB_SOCK_INIT_FAILED);
  }
  //发起连接请求
  ret = sock.connect();
  if (ret)
  {
    return getError(EDB_SOCK_CONNECT_FAILED);
  }

  //小包发送
  sock.disableNagle();
  return ret;
}

int QuitCommand::handleReply()
{
  int ret = EDB_OK;
  //gQuit = 1;//退出(全局变量标志)
  return  ret;
}

int QuitCommand::execute(ossSocket &sock,
                        std::vector<std::string> &argVec)
{
  int ret = EDB_OK;
  if (!sock.isConnected())
  {
    return getError(EDB_SOCK_NOT_CONNECT);
  }

  //发送测试内容 hello world
  ret = sendOrder(sock,0);
  //sock.close();
  ret = handleReply(); //结束edb进程(用gQuit变量)
  return ret;
}

int HelpCommand::execute(ossSocket &sock,
    std::vector<std::string> &argVec)
{
  int ret = EDB_OK;
  printf("List of classes of commands:\n\n");
  printf("%s [server] [port]-- connecting emeralddb server\n", COMMAND_CONNECT);
  printf("%s -- sending a insert command to emeralddb server\n", COMMAND_INSERT);
  printf("%s -- sending a query command to emeralddb server\n", COMMAND_QUERY);
  printf("%s -- sending a delete command to emeralddb server\n", COMMAND_DELETE);
  printf("%s [number]-- sending a test command to emeralddb server\n", COMMAND_TEST);
  printf("%s -- providing current number of record inserting\n", COMMAND_SNAPSHOT);
  printf("%s -- quitting command\n\n", COMMAND_QUIT);
  printf("Type \"help\" command for help\n");
  return ret;
}
