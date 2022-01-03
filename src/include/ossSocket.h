
#ifndef OSSSOCKET_H__
#define OSSSOCKET_H__

#include "core.h"
#define  SOCKET_GETLASTERROR errno

//默认超时时间10ms
#define OSS_SOCKET_DFT_TIMEOUT  10000

//最大的 hostname
#define OSS_MAX_HOSTNAME  NI_MAXHOST
#define OSS_MAX_SERVICENAME NI_MAXSERV

class _ossSocket
{
  private:
    int _fd;
    socklen_t _addressLen;
    socklen_t _peerAddressLen;
    struct sockaddr_in _sockAddress;
    struct sockaddr_in _peerAddress;
    bool _init;
    int _timeout;

  //私有函数,子类可以用(如果在private里面,则子类不能用
  protected:
    unsigned int _getPort(struct sockaddr_in *addr);
    int _getAddress(struct sockaddr_in *addr,char *pAddress,unsigned int length);

  public:
    int setSocketLi(int lOnOff,int linger);
    void setAddress(const char *pHostName,unsigned int port);
    //创建监听socket
    _ossSocket();
    _ossSocket(unsigned int port,int timeout = 0);
    //创建创建连接socket
    _ossSocket(const char* pHostName,unsigned int port,int timeout = 0);
    //创建socket(从已经存在的socket)
    _ossSocket(int *sock,int timeout = 0);
    //释放资源就是释放socket
    ~_ossSocket()
    {
      close();
    }

    int initSocket();
    int bind_listen();
    bool isConnected();
    int send(const char *pMsg,int len,
              int timeout = OSS_SOCKET_DFT_TIMEOUT,
              int flag = 0);
    int recv(char *pMsg,int len,
              int timeout = OSS_SOCKET_DFT_TIMEOUT,
              int flag = 0);
    //收到消息就返回
    int recvNF(char *pMsg,int len,
              int timeout = OSS_SOCKET_DFT_TIMEOUT);
    int connect();
    void close();
    int accept(int *sock,struct sockaddr *addr,socklen_t *addrlen,int timeout = OSS_SOCKET_DFT_TIMEOUT);

    //不用Nagle算法 直接发送小包
    int disableNagle();
    //得到对方的端口
    unsigned int getPeerPort();
    int getPeerAddress(char *pAddress,unsigned int length);
    //得到本地的端口
    unsigned int getLocalPort();
    int getLocalAddress(char *pAddress,unsigned int length);
    
    int setTimeout(int seconds);
    //得到本地域名
    static int getHostName(char *pName,int namelen);
    //将服务名(/etc/services)转为端口号
    static int getPort(const char *pServiceName,unsigned short &port);
};

typedef class _ossSocket ossSocket;

#endif
