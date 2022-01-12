#include "core.h"
#include "pd.h"
#include "ossLatch.h"
#include "ossPrimitiveFileOp.h"


static const char *PDLEVELSTRING[] = 
{
  "SEVERE",
  "ERROR",
  "EVENT",
  "WARNING",
  "INFO",
  "DEBUG"
}; 

//根据级别返回字符串
const char *getPDLevelDesp(PDLEVEL level)
{
  if ((unsigned int)level > (unsigned int)PDDEBUG)
  {
    return "Unknow Level";
  }
  return PDLEVELSTRING[(unsigned int)level];
}

static const char *PD_LOG_HEADER_FORMAT = "%04d-%02d-%02d-%02d.%02d.%02d.%06d\
                                           Level:%s" OSS_NEWLINE "PID:%-37dTID:%d" OSS_NEWLINE "Function:%-32sLine:%d"\
                                            OSS_NEWLINE "File:%s" OSS_NEWLINE "Message:" OSS_NEWLINE "%s" OSS_NEWLINE OSS_NEWLINE;

//默认的日志级别
PDLEVEL _curPDLevel = PD_DFT_DIAGLEVEL;
//日志存放的默认路径
char _pdDiagLogPath[OSS_MAX_PATHSIZE + 1] = {0};

//ossXLatch:封装好的mutex
ossXLatch _pdLogMutex;
//文件读写类
ossPrimitiveFileOp _pdLogFile;

// open log file
static int _pdLogFileReopen()
{
  int rc = EDB_OK;
  _pdLogFile.Close();
  rc = _pdLogFile.Open(_pdDiagLogPath);
  if (rc)
  {
    printf("Failed to open log file, errno = %d" OSS_NEWLINE,rc);
    goto error;
  }
  //文件指针移动到末尾
  _pdLogFile.seekToEnd();
done:
  return rc;
error:
  goto done;
}

// write log file
static int _pdLogFileWrite(const char *pData) 
{
  int rc = EDB_OK;
  size_t dataSize = strlen(pData);
  // _pdLogMutex.lock();
  _pdLogMutex.get();
  //log file对象没持有文件句柄,意思就是重新打开log file
  if (!_pdLogFile.isValid())
  {
    rc = _pdLogFileReopen();
    if (rc)
    {
      printf("Failed to open log file, errno = %d" OSS_NEWLINE,rc);
      goto error;
    }
  }
  rc = _pdLogFile.Write(pData,dataSize);
  if (rc)
  {
    printf("Failed to write into log file, errno = %d" OSS_NEWLINE,rc);
    goto error;
  }
done:
  _pdLogMutex.release();
  return rc;
error:
  goto done;
}

void pdLog(PDLEVEL level,const char *func,const char *file,
          unsigned int line,const char *fmt,...)
{
  int rc = EDB_OK;
  //不严重的级别就忽略
  if (level > _curPDLevel)
    return ;
  va_list ap;
  char userInfo[PD_LOG_STRINGMAX];
  char sysInfo[PD_LOG_STRINGMAX];

  struct tm otm;
  struct timeval tv;
  struct timezone tz;
  time_t tt;
  
  gettimeofday(&tv,&tz);
  tt = tv.tv_sec;
  localtime_r(&tt,&otm);

  //用户的日志信息
  va_start(ap,fmt);
  vsnprintf(userInfo,PD_LOG_STRINGMAX,fmt,ap);
  va_end(ap);

  //system的系统信息
  snprintf(sysInfo,PD_LOG_STRINGMAX,PD_LOG_HEADER_FORMAT,
      otm.tm_year + 1900,
      otm.tm_mon + 1,
      otm.tm_mday,
      otm.tm_hour,
      otm.tm_min,
      otm.tm_sec,
      tv.tv_usec,
      PDLEVELSTRING[level], //级别对应的字符串
      getpid(),
      syscall(SYS_gettid),
      func,
      line,
      file,
      userInfo
      );

  printf("%s" OSS_NEWLINE,sysInfo);
  //写入日志文件
  if (_pdDiagLogPath[0] != '0')
  {
    rc = _pdLogFileWrite(sysInfo);
    //写入失败
    if (rc)
    {
      printf("Failed to write into log file, errno = %d" OSS_NEWLINE, rc);
      printf("%s" OSS_NEWLINE,sysInfo);
    }
  }

  return ;
}

void pdLog(PDLEVEL level,const char *func,const char *file,
          unsigned int line,std::string message)
{

}
