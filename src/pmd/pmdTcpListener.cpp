#include "core.h"
#include "ossSocket.h"

#define PMD_TCPLISTENER_RETRY     5
#define OSS_MAX_SERVICENAME     NI_MAXSERV

int pmdTcpListenerEntryPoint()
{
  int rc = EDB_OK;

  int port = 48127;
  ossSocket sock(port);
  rc = sock.initSocket();
  if (rc)
  {
    printf("Failed to initialize socket, rc = %d",rc);
    goto error;
  }
  rc = sock.bind_listen();
  if (rc)
  {
    printf("Failed to bind and listen socket, rc = %d",rc);
    goto error;
  }

  //接受请求
  while (true)
  {
    int s;
    rc = sock.accept(&s,NULL,NULL);
    //超时了继续监听
    if (rc == EDB_TIMEOUT)
    {
      rc = EDB_OK;
      continue;
    }
    char buffer[1024];
    int size;
    //由accept接收到的连接,来新创建一个socket
    ossSocket sock1(&s);
    sock1.disableNagle();
    //根据协议内容,前4个字节表示数据包的长度
    do{
      rc = sock1.recv((char *)&size,4);
      if (rc && rc!=EDB_TIMEOUT)
      {
        printf("Failed to receive size, rc = %d",rc);
        goto error;
      }
      //超时了,就一直尝试去接收
    }while(rc == EDB_TIMEOUT);

    do {
      //将剩下接收的存放到buffer[]中
      rc = sock1.recv(&buffer[0],size-sizeof(int));
      if (rc && rc!=EDB_TIMEOUT)
      {
        printf("Failed to receive buffer, rc = %d",rc);
        goto error;
      }
    }while(rc == EDB_TIMEOUT);
    printf("%s\n",buffer);
    sock1.close();
  }

done:
  return rc;
error:
  switch(rc)
  {
    case EDB_SYS:
      printf("System error occured");
      break;
    default:
      printf("Internal error");
  }
  goto done;
}


