#include "core.h"
#include "ossPrimitiveFileOp.h"

ossPrimitiveFileOp::ossPrimitiveFileOp()
{
  _fileHandle = OSS_INVALID_HANDLE_FD_VALUE;
  _bIsStdout = false;
}

//判断文件是否合法==判断
bool ossPrimitiveFileOp::isValid()
{
  return (_fileHandle != OSS_INVALID_HANDLE_FD_VALUE);
}

//关闭文件
void ossPrimitiveFileOp::Close()
{
  //合法,并且不是stdout(是stdout,什么也不做)
  if (isValid() && (!_bIsStdout))
  {
    //close(fd);
    oss_close(_fileHandle);
    _fileHandle = OSS_INVALID_HANDLE_FD_VALUE;
  }
}

//打开文件, options的默认参数是 OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS
int ossPrimitiveFileOp::Open(const char *pFilePath,unsigned int options)
{
  int rc = 0;
  int mode = O_RDWR;

  if (options & OSS_PRIMITIVE_FILE_OP_READ_ONLY)
  {
    mode = O_RDONLY;
  }
  else if (options & OSS_PRIMITIVE_FILE_OP_WRITE_ONLY)
  {
    mode = O_WRONLY;
  }

  if (options & OSS_PRIMITIVE_FILE_OP_OPEN_EXISTING)
  {
  }
  else if (options & OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS)
  {
    mode |= O_CREAT;
  }

  //文件截断,从0开始写入
  if (options & OSS_PRIMITIVE_FILE_OP_OPEN_TRUNC)
  {
    mode |= O_TRUNC;
  }

  do {
    _fileHandle = oss_open(pFilePath,mode,0644);
    //由于中断而打开失败则再次尝试打开
  } while((-1 == _fileHandle) && (EINTR == errno));

  // _fileHandle <= -1,其他的错误在打开文件的过程中发生了
  if (_fileHandle <= OSS_INVALID_HANDLE_FD_VALUE)
  {
    rc = errno;
    goto exit;
  }

exit:
  return rc;
}

//设置输出文件的描述符
void ossPrimitiveFileOp::openStdout()
{
  //设置文件fd==1
  setFileHandle(STDOUT_FILENO);
  _bIsStdout = true;
}

//得到当前文件point的偏移
offsetType ossPrimitiveFileOp::getCurrentOffset() const 
{
  return oss_lseek(_fileHandle,0,SEEK_CUR);
}

//设置文件point的偏移量
void ossPrimitiveFileOp::seekToOffset(offsetType offset)
{
  if ((oss_off_t)-1 != offset)
  {
    oss_lseek(_fileHandle,offset,SEEK_SET);
  }
}

//设置文件point的偏移量到文件末尾
void ossPrimitiveFileOp::seekToEnd()
{
  oss_lseek(_fileHandle,0,SEEK_END);
}

// pBytesRead是返回读取到的字节
int ossPrimitiveFileOp::Read(const size_t size,void *const pBuf,int *const pBytesRead)
{
  int retval = 0;
  ssize_t bytesRead = 0;
  if (isValid())
  {
    do 
    {
      bytesRead = oss_read(_fileHandle,pBuf,size);
      //只处理read过程中的中断错误
    } while((bytesRead == -1) && (errno == EINTR));

    //其他的错误
    if (bytesRead == -1)
      goto err_read;
  }
  else {
    goto err_read;
  }

  if (pBytesRead)
  {
    *pBytesRead = bytesRead;
  }

done:
  return retval;
err_read:
  *pBytesRead = 0;
  retval = errno;
  goto done;
}

int ossPrimitiveFileOp::Write(const void *pBuf,size_t size)
{
  int rc = 0;
  size_t currentSize = 0;
  //size==0表示不知道要写多长,自己测出来
  if (size == 0)
  {
    size = strlen((char *)pBuf);
  }
  
  if (isValid())
  {
    do 
    {
      rc = oss_write(_fileHandle, &((char*)pBuf)[currentSize], size-currentSize);
      if (rc >= 0)
        currentSize += rc;
      //发生了中断或者有没写完
    } while(((-1 == rc) && (EINTR == errno)) || 
            ((-1 == rc) && (currentSize != size)));
    if (rc == -1)
    {
      rc = errno;
      goto done;
    }
    rc = 0;
  }
done:
  return rc;
}

//可变参数
int ossPrimitiveFileOp::fWrite(const char *format,...)
{
  int rc = 0;
  va_list ap;
  char buf[OSS_PRIMITIVE_FILE_OP_FWRITE_BUF_SIZE] = {0};

  //拿到填充好的buffer
  va_start(ap,format);
  vsnprintf(buf,sizeof(buf),format,ap);
  va_end(ap);

  rc = Write(buf);
  return rc;
}

void ossPrimitiveFileOp::setFileHandle(handleType handle)
{
  _fileHandle = handle;
}

int ossPrimitiveFileOp::getSize(offsetType *const pFileSize)
{
  int rc = 0;
  oss_struct_stat buf = {0};

  if (oss_fstat(_fileHandle,&buf) == -1)
  {
    rc = errno;
    goto err_exit;
  }

  *pFileSize = buf.st_size;
done:
  return rc;
err_exit:
  *pFileSize = 0;
  goto done;
}
