#ifndef _OSSPRIMITIVEFILEOP_H_
#define _OSSPRIMITIVEFILEOP_H_

#include "core.h"

#define OSS_F_GETLK         F_GETLK64
#define OSS_F_SETLK         F_SETLK64
#define OSS_F_SETLKW        F_SETLKW64

#define oss_struct_statfs     struct statfs64
#define oss_statfs            statfs64
#define oss_fstatfs           fstatfs64
#define oss_struct_statvfs    struct statvfs64
#define oss_statvfs           statvfs64
#define oss_fstatvfs          fstatvfs64
#define oss_struct_stat       struct stat64
#define oss_struct_flock      struct flock64
#define oss_stat              stat64
#define oss_lstat             lstat64
#define oss_fstat             fstat64
#define oss_open              open64
#define oss_lseek             lseek64
#define oss_ftruncate         ftruncate64
#define oss_off_t             off64_t
#define oss_close             close
#define oss_access            access
#define oss_chmod             chmod
#define oss_read              read
#define oss_write             write

#define OSS_PRIMITIVE_FILE_OP_FWRITE_BUF_SIZE 2048
#define OSS_PRIMITIVE_FILE_OP_READ_ONLY     (((unsigned int)1) << 1)
#define OSS_PRIMITIVE_FILE_OP_WRITE_ONLY    (((unsigned int)1) << 2)
#define OSS_PRIMITIVE_FILE_OP_OPEN_EXISTING (((unsigned int)1) << 3)
#define OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS   (((unsigned int)1) << 4)
#define OSS_PRIMITIVE_FILE_OP_OPEN_TRUNC    (((unsigned int)1) << 5)

#define OSS_INVALID_HANDLE_FD_VALUE   (-1)

//typedef off64_t offsetType;
typedef oss_off_t offsetType;

class ossPrimitiveFileOp
{
  public:
    typedef int handleType;
  private:
    //当前文件的fd(Windows下是句柄)
    handleType _fileHandle;
    //判断fd是普通文件还是fd=2==stdout输出流文件
    bool _bIsStdout;   

  private:
    //拷贝构造函数 私有 == 禁止拷贝构造函数
    ossPrimitiveFileOp(const ossPrimitiveFileOp &) {}
    //拷贝赋值运算符 私有 == 禁止拷贝赋值
    const ossPrimitiveFileOp &operator=(const ossPrimitiveFileOp &); 
    
  protected:
    //由已知的文件句柄生成对象
    void setFileHandle(handleType handle);
  public:
    //默认的拷贝构造函数
    ossPrimitiveFileOp();
    int Open(const char *pFilePath,unsigned int options = OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS);
    //将stdout(屏幕)作为文件直接打开
    void openStdout();
    void Close();
    //判断文件是否合法
    bool isValid();
    //从文件中读取指定长度的数据到pBuf中
    int Read(const size_t size,void *const pBuf,int * const pBytesRead);
    //将pBuf的信息写入到文件中
    int Write(const void *pBuf,size_t len = 0);
    //fmt
    int fWrite(const char *fmt,...);

    //得到当前偏移
    offsetType getCurrentOffset() const;
    //定位偏移
    void seekToOffset(offsetType offset);
    //偏移到最后
    void seekToEnd();
    //得到文件大小
    int getSize(offsetType *const pFileSize);
    //返回文件句柄
    handleType getHandle() const 
    {
      return _fileHandle;
    }
};

#endif
