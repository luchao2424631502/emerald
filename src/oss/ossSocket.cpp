#include "ossSocket.h"

//创建listen socket (只给出端口)
_ossSocket::_ossSocket(unsigned int port,int timeout)
{
  _init = false;
  _fd = 0;
  _timeout = timeout;
  memset(&_sockAddress,0,sizeof(struct sockaddr_in));
  memset(&_peerAddress,0,sizeof(struct sockaddr_in));
  _peerAddressLen = sizeof(_peerAddress);

  _sockAddress.sin_family = AF_INET;
  _sockAddress.sin_addr.s_addr = htonl(INADDR_ANY); //地址是本机网卡IP inaddr_any
  _sockAddress.sin_port = htons(port);

  _addressLen = sizeof(_sockAddress);
}

//创建a socket (什么地址都没填写的socket)
_ossSocket::_ossSocket()
{
  _init = false;
  _fd = 0;
  _timeout = 0;

  memset(&_sockAddress,0,sizeof(struct sockaddr_in));
  memset(&_peerAddress,0,sizeof(struct sockaddr_in));
  _peerAddressLen = sizeof(_peerAddress);
  _addressLen = sizeof(_sockAddress);
}

//创建一个connect socket,低 
_ossSocket::_ossSocket(const char *pHostName,unsigned int port,int timeout)
{
  struct hostent *hp;

  _init = false;
  _timeout = timeout;
  _fd = 0;

  memset(&_sockAddress,0,sizeof(struct sockaddr_in));
  memset(&_peerAddress,0,sizeof(struct sockaddr_in));
  _peerAddressLen = sizeof(_peerAddress);

  _sockAddress.sin_family = AF_INET;
  //根据hostname拿到ip (或者hostname是ip的字符串)
  if((hp = gethostbyname(pHostName)))
    _sockAddress.sin_addr.s_addr = *((int *)hp->h_addr_list[0]);
  else 
    _sockAddress.sin_addr.s_addr = inet_addr(pHostName);
  _sockAddress.sin_port = htons(port);
  _addressLen = sizeof(_sockAddress);
}

//从已经创建的socket中创建一个socket
_ossSocket::_ossSocket(int *sock,int timeout)
{
  int rc = EDB_OK;
  _fd = *sock;
  _init = true;
  _timeout = timeout;
  _addressLen = sizeof(_sockAddress);

  memset(&_peerAddress,0,sizeof(struct sockaddr_in));
  _peerAddressLen = sizeof(_peerAddress);

  //返回当前绑定套接字的信息
  rc = getsockname(_fd,(sockaddr*)&_sockAddress,&_addressLen);
  if(rc)
  {
    //SOCK_GETLASTERROR = errno
    printf("Failed to get sock name, error = %d",SOCKET_GETLASTERROR);
    _init = false;//则初始化socket失败
  }
  else 
  {
    //返回客户端的sockaddr信息
    rc = getpeername(_fd,(sockaddr*)&_peerAddress,&_peerAddressLen);
    if(rc)
    {
      printf("Failed to get peer name, error = %d",SOCKET_GETLASTERROR);
    }
  }
}

//做一个新socket,首先要初始化
int ossSocket::initSocket()
{
  int rc = EDB_OK;
  if(_init)
  {
    goto done;
  }
  memset(&_peerAddress,0,sizeof(struct sockaddr_in));
  _peerAddressLen = sizeof(_peerAddress);

  _fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if(_fd == -1)
  {
    printf("Failed to initialize socket, error = %d",SOCKET_GETLASTERROR);
    rc = EDB_NETWORK;
    goto error;
  }
  _init = true;

  //设置超时时间
  setTimeout(_timeout);
done: 
  return rc;
error:
  goto done;
}

//设置tcp断开连接的方式
int ossSocket::setSocketLi(int lOnOff,int linger)
{
  int rc = EDB_OK;
  struct linger _linger;
  _linger.l_onoff = lOnOff;
  _linger.l_linger = linger;
  rc = setsockopt(_fd,SOL_SOCKET,SO_LINGER,
                  (const char *)&_linger,sizeof(_linger));
  return rc;
}

//设置当前socket的本机ip和端口
void ossSocket::setAddress(const char *pHostName,unsigned int port)
{
  struct hostent *hp;
  memset(&_sockAddress,0,sizeof(struct sockaddr_in));
  memset(&_peerAddress,0,sizeof(struct sockaddr_in));
  _peerAddressLen = sizeof(_sockAddress);
  _sockAddress.sin_family = AF_INET;
  if((hp = gethostbyname(pHostName)))
    _sockAddress.sin_addr.s_addr = *((int *)hp->h_addr_list[0]);
  else 
    _sockAddress.sin_addr.s_addr = inet_addr(pHostName);

  _sockAddress.sin_port = htons(port);;
  _addressLen = sizeof(_sockAddress);
}

int ossSocket::bind_listen()
{
  int rc = EDB_OK;
  int tmp = 1;
  //设置当前ip+端口可以复用
  rc = setsockopt(_fd,SOL_SOCKET,SO_REUSEADDR,(char *)&tmp,sizeof(int));
  if(rc)
  {
    printf("Failed to setsockopt to SO_REUSEADDR, rc = %d",SOCKET_GETLASTERROR);
  }
  //设置返回时间: a!=0 b>0,30ms内发送完则正确退出,否则socket强制退出
  rc = setSocketLi(1,30);
  if(rc)
  {
    printf("Failed to setsockopt SO_LINTER,rc = %d",SOCKET_GETLASTERROR);
  }

  //将 ip和端口绑定到 socket上
  rc = ::bind(_fd,(struct sockaddr*)&_sockAddress,_addressLen);
  if(rc)
  {
    printf("Failed to bind socket, rc = %d",SOCKET_GETLASTERROR);
  }

  rc = listen(_fd,SOMAXCONN);
  if(rc)
  {
    printf("Failed to listen socket, rc = %d",SOCKET_GETLASTERROR);
    rc = EDB_NETWORK;
    goto error;
  }
done:
  return rc;
error:
  //调用自己的成员close()
  close();
  goto done;
}

int ossSocket::send(const char *pMsg,int len,int timeout,int flags) 
{
  int rc = EDB_OK;
  int maxFD = _fd;
  struct timeval maxSelectTime;
  fd_set fds;

  maxSelectTime.tv_sec = timeout/1000000;
  maxSelectTime.tv_usec = timeout%1000000;
  //发送长度是0,直接返回
  if(len == 0)
    return EDB_OK;

  //循环等待套接字收到信息
  while(true)
  {
    //清空fd
    FD_ZERO(&fds);
    FD_SET(_fd,&fds);
    //select监听是否描述符可以发送信息
    rc = select(maxFD + 1, NULL,&fds,NULL,
                timeout>=0?&maxSelectTime:NULL);
    //返回0表示超时
    if(rc == 0)
    {
      rc = EDB_TIMEOUT;
      goto done;
    }
    //<0表示有错误
    else if (rc<0)
    {
      rc = SOCKET_GETLASTERROR;
      //
      if(EINTR == rc)
        continue;//恢复while循环,结束有中断发生的错误
      //否则是其他的错误
      printf("Failed to select from socket, rc = %d",rc);
      rc = EDB_NETWORK;
      goto error;
    }

    //是不是 fd已经准备好
    if (FD_ISSET(_fd,&fds))
      break;
  }

  while (len > 0)
  {
    rc = ::send(_fd,pMsg,len,MSG_NOSIGNAL | flags);
    if (rc == -1)
    {
      printf("Failed to send,rc = %d",SOCKET_GETLASTERROR);
      rc = EDB_NETWORK;
      goto error;
    }
    len -= rc;
    pMsg += rc;//每次加发送过得距离
  }

  rc = EDB_OK;
done:
  return rc;
error:
  goto done;
}

//class没有生命,判断当前连接是否存在
bool ossSocket::isConnected()
{
  int rc = EDB_OK;
  rc = ::send(_fd,"",0,MSG_NOSIGNAL);
  if(rc < 0)
    return false;
  return true;
}

#define MAX_RECV_RETRIES  5
int ossSocket::recv(char *pMsg,int len,int timeout,int flags)
{
  int rc = EDB_OK;
  int retries = 0;
  int maxFD = _fd;

  struct timeval maxSelectTime;
  fd_set fds;

  //什么都不需要接受
  if (len == 0)
    return EDB_OK;
  maxSelectTime.tv_sec = timeout / 1000000;
  maxSelectTime.tv_usec = timeout % 1000000;
  //拿到准备就绪的描述符
  while(true)
  {
    FD_ZERO(&fds);
    FD_SET(_fd,&fds);
    rc = select(maxFD+1,&fds,NULL,NULL,timeout>=0?&maxSelectTime:NULL);
    //超时了,没有一个fd准备好,
    if (rc == 0)
    {
      rc = EDB_TIMEOUT;
      goto done;
    }
    //select出错了
    if (rc < 0)
    {
      rc = SOCKET_GETLASTERROR;
      //是中断的错误,则继续等待描述符
      if (rc == EINTR)
      {
        continue;
      }
      //别的错误
      printf("Failed to select fron socket, rc = %d",rc);
      goto error;
    }

    //判断是否是_fd准备好了(fds里面也就_fd)
    if(FD_ISSET(_fd,&fds))
      break;
  }

  //接受消息
  while(len > 0)
  {
    rc = ::recv(_fd,pMsg,len,MSG_NOSIGNAL | flags);
    //接受了
    if (rc > 0)
    {
      //MSG_PEEK在recv后不会删除tcp_buffer中的内容,所以一般都是用MSG_PEEK做测试,
      //所以有这个标志,并且接受一次后就 退出
      if(flags & MSG_PEEK)
      {
        goto done;
      }
      len -= rc;
      pMsg += rc;
    }
    //一般是对方shutdown连接了
    else if (rc == 0)
    {
      printf("Peer unexpected shutdown");
      rc = EDB_NETWORK_CLOSE;
      goto error;
    }
    //发生了错误
    else {
      rc = SOCKET_GETLASTERROR;
      //阻塞情况下发生这种错误就要重试,但是我们这里直接退出
      if ((rc == EAGAIN || rc == EWOULDBLOCK) &&
          _timeout > 0)
      {
        printf("Recv() timeout: rc = %d",rc);
        rc = EDB_NETWORK;
        goto error;
      }
      //由中断引起的错误,
      if ((EINTR == rc) && (retries < MAX_RECV_RETRIES))
      {
        retries++;
        continue; //重试
      }

      //其他错误
      printf("Recv() Failed: rc = %d",rc);
      rc = EDB_NETWORK;
      goto error;
    }
  }
  rc = EDB_OK;
done:
  return rc;
error:
  goto done;
}

int ossSocket::recvNF(char *pMsg,int len,int timeout)
{
  int rc = EDB_OK;
  int retries = 0;
  int maxFD = _fd;

  struct timeval maxSelectTime;
  fd_set fds;

  //什么都不需要接受
  if (len == 0)
    return EDB_OK;
  maxSelectTime.tv_sec = timeout / 1000000;
  maxSelectTime.tv_usec = timeout % 1000000;
  //拿到准备就绪的描述符
  while(true)
  {
    FD_ZERO(&fds);
    FD_SET(_fd,&fds);
    rc = select(maxFD+1,&fds,NULL,NULL,timeout>=0?&maxSelectTime:NULL);
    //超时了,没有一个fd准备好,
    if (rc == 0)//利用gdb断点调试找出来的错误,配置g++ warning信息,所以warning一定不能忽视
    {
      rc = EDB_TIMEOUT;
      goto done;
    }
    //select出错了
    if (rc < 0)
    {
      rc = SOCKET_GETLASTERROR;
      //是中断的错误,则继续等待描述符
      if (rc == EINTR)
      {
        continue;
      }
      //别的错误
      printf("Failed to select fron socket, rc = %d",rc);
      goto error;
    }

    //判断是否是_fd准备好了(fds里面也就_fd)
    if(FD_ISSET(_fd,&fds))
      break;
  }

  //和上面函数一样,只是此函数只接受1字节,判断是否接收到消息
  rc = ::recv(_fd,pMsg,len,MSG_NOSIGNAL);
  //接收成功
  if (rc > 0)
  {
    len = rc;
  }
  //0表示peer shutdown了(end-of-file)
  else if (rc == 0)
  {
    printf("Peer unexpected shuwdown");
    rc = EDB_NETWORK_CLOSE;
    goto error;
  }
  //错误发生
  else 
  {
    rc = SOCKET_GETLASTERROR;
    //由超时时间,并且是again/wouldblock错误应该重新来等待,但是我们这里直接返回错误了
    if((EAGAIN == rc || EWOULDBLOCK == rc) && _timeout>0)
    {
      printf("Recv() timeout: rc = %d",rc);
      rc = EDB_NETWORK;
      goto error;
    }
    //发生了中断
    if ((EINTR == rc) && (retries < MAX_RECV_RETRIES))
    {
      retries++;
    }
    //其他错误
    printf("Recv() Failed: rc = %d",rc);
    rc = EDB_NETWORK;
    goto error;
  }
  rc = EDB_OK;
done:
  return rc;
error:
  goto done;
}

//填好peer和自己的ip+端口后,发起连接请求
int ossSocket::connect()
{
  int rc = EDB_OK;
  // ???和自己连接???
  rc = ::connect(_fd,(struct sockaddr *)&_sockAddress,_addressLen);
  if (rc)
  {
    printf("Failed to connect,rc = %d",SOCKET_GETLASTERROR);
    rc = EDB_NETWORK;
    goto error;
  }
  //得到本地的地址
  rc = getsockname(_fd,(sockaddr*)&_sockAddress,&_addressLen);
  if (rc)
  {
    printf("Failed to get local address, rc = %d",rc);
    rc = EDB_NETWORK;
    goto error;
  }

  //得到对方的地址信息
  rc = getpeername(_fd,(sockaddr *)&_peerAddress,&_peerAddressLen);
  if (rc)
  {
    printf("Failed to get peer address, rc = %d",rc);
    rc = EDB_NETWORK;
    goto error;
  }
done:
  return rc;
error:
  goto done;
}

void ossSocket::close()
{
  if(_init)
  {
    int i = 0;
    i = ::close(_fd);
    if(i<0)
    {
      i = -1;
    }
    _init = false;
  }
}

int ossSocket::accept(int *sock,struct sockaddr *addr,socklen_t *addrlen,int timeout)
{
  int rc = EDB_OK;
  int maxFD = _fd;
  struct timeval maxSelectTime;

  fd_set fds;
  maxSelectTime.tv_sec = timeout / 1000000;
  maxSelectTime.tv_usec = timeout % 1000000;
  while(true)
  {
    FD_ZERO(&fds);
    FD_SET(_fd,&fds);
    rc = select(maxFD + 1,&fds,NULL,NULL,timeout>=0?&maxSelectTime:NULL);

    //=0表示select等待fd ready超时
    if (rc == 0)
    {
      *sock = 0;
      rc = EDB_TIMEOUT;
      //accept失败
      goto done;
    }
    //<0出错了
    if (rc < 0)
    {
      //拿到错误
      rc = SOCKET_GETLASTERROR;
      //如果是被中断,则重新select
      if (EINTR == rc)
        continue;
      printf("Failed to select from socket, rc = %d",SOCKET_GETLASTERROR);
      rc = EDB_NETWORK;
      goto error;
    }

    //fd准备好了
    if (FD_ISSET(_fd,&fds))
    {
      break;
    }
  }

  rc = EDB_OK;
  *sock = ::accept(_fd,addr,addrlen);
  if (-1 == *sock)
  {
    printf("Failed to accept socket, rc = %d",SOCKET_GETLASTERROR);
    rc = EDB_NETWORK;
    goto error;
  }
done:
  return rc;
error:
  goto done;
}

//关闭TCP Nagle算法
int ossSocket::disableNagle()
{
  int rc = EDB_OK;
  int temp = 1;
  rc = setsockopt(_fd,IPPROTO_TCP,TCP_NODELAY,(char *)&temp,sizeof(int));
  if (rc)
  {
    printf("Failed to setsockopt, rc = %d",SOCKET_GETLASTERROR);
  }

  rc = setsockopt(_fd,SOL_SOCKET,SO_KEEPALIVE,(char *)&temp,sizeof(int));
  if (rc)
  {
    printf("Failed to setsockopt, rc = %d",SOCKET_GETLASTERROR);
  }
  return rc;
}

unsigned int ossSocket::_getPort(struct sockaddr_in *addr)
{
  return ntohs(addr->sin_port);
}

int ossSocket::_getAddress(struct sockaddr_in *addr,char *pAddress,unsigned int length)
{
  int rc = EDB_OK;
  length = length < NI_MAXHOST ? length : NI_MAXHOST;
  rc = getnameinfo((struct sockaddr *)addr,sizeof(sockaddr),pAddress,length,NULL,0,NI_NUMERICHOST);
  if (rc)
  {
    printf("Failed to getnameinfo, rc = %d",SOCKET_GETLASTERROR);
    rc = EDB_NETWORK;
    goto error;
  }
done:
  return rc;
error:
  goto done;
}

unsigned int ossSocket::getPeerPort()
{
  return _getPort(&_peerAddress);
}

unsigned int ossSocket::getLocalPort()
{
  return _getPort(&_sockAddress);
}

int ossSocket::getLocalAddress(char *pAddress,unsigned int length) 
{
  return _getAddress(&_sockAddress,pAddress,length);
}

int ossSocket::getPeerAddress(char *pAddress,unsigned int length)
{
  return _getAddress(&_peerAddress,pAddress,length);
}

int ossSocket::setTimeout(int seconds)
{
  int rc = EDB_OK;
  struct timeval tv;
  tv.tv_sec = seconds;
  tv.tv_usec = 0;

  //设置接收数据超时时间
  rc = setsockopt(_fd,SOL_SOCKET,SO_RCVTIMEO,(char *)&tv,sizeof(tv));
  if (rc)
  {
    printf("Failed to setsockopt, rc = %d",SOCKET_GETLASTERROR);
  }

  //设置发送数据的超时时间
  rc = setsockopt(_fd,SOL_SOCKET,SO_SNDTIMEO,(char *)&tv,sizeof(tv));
  if (rc)
  {
    printf("Failed to setsockopt, rc = %d",SOCKET_GETLASTERROR);
  }

  return rc;
}

int ossSocket::getHostName(char *pName,int nameLen)
{
  return gethostname(pName,nameLen);
}

//根据服务名得到端口
int ossSocket::getPort(const char *pServiceName,unsigned short &port)
{
  int rc = EDB_OK;
  struct servent *servinfo;
  servinfo = getservbyname(pServiceName,"tcp");
  if (!servinfo)
    port = atoi(pServiceName);
  else 
    port = (unsigned short)ntohs(servinfo->s_port);

  return rc;
}
