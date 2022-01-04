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

//打开文件
int ossPrimitiveFileOp::Open(const char *pFilePath,int options = OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS)
{
  int rc = 0;
  int mode = O_RDWR;

  //
  if (options & OSS_PRIMITIVE_FILE_OP_READ_ONLY)
  {
    mode = O_RDONLY;
  }
  else if (options & )
}
