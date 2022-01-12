#ifndef PD_H_
#define PD_H_

#include <string>

#define PD_LOG_STRINGMAX  4096
/*## 消除逗号(如果可变参数是空,消除可变参数前面的,)*/
#define PD_LOG(level,fmt,...)\
  do {\
    if (_curPDLevel >= level)\
    {\
      pdLog(level,__func__,__FILE__,__LINE__,fmt,##__VA_ARGS__);\
    }\
  } while(false)\

#define PD_CHECK(cond,retCode,gotoLabel,level,fmt,...)  \
    do {\
      if (!(cond))\
      {\
        rc = (retCode);\
        PD_LOG((level),fmt,##__VA_ARGS__);\
        goto gotoLabel;\
      }\
    } while(false)\

#define PD_RC_CHECK(rc,level,fmt,...)\
    do {\
      PD_CHECK((EDB_OK==rc),rc,error,level,fmt,##__VA_ARGS__);\
    } while(false)\

#define EDB_VALIDATE_GOTOERROR(cond,ret,str)    \
    {\
      if (!(cond)) \
      {\
        pdLog(PDERROR,__func__,__FILE__,__LINE__,str); \
        rc = ret;\
        goto error;\
      }\
    }

#ifdef _DEBUG
#define EDB_ASSERT(cond,str) \
    {\
      if (!(cond)) \
      {\
        pdassert(str,__func__,__FILE__,__LINE__);\
      }\
    }

#define EDB_CHECK(cond,str) \
    {\
      if (!(cond)) \
      {\
        pdcheck(str,__func__,__FILE__,__LINE__);\
      }\
    }
#else

#define EDB_ASSERT(cond,str) {if(cond){}}
#define EDB_CHECK(cond,str) {if(cond){}}

#endif

enum PDLEVEL 
{
  PDSEVERE = 0,
  PDERROR,
  PDEVENT,
  PDWARNING,
  PDINFO,
  PDDEBUG
};

//估计是定义在pd.cc中的全局变量
extern PDLEVEL _curPDLevel;

const char *getPDLevelDesp(PDLEVEL level);

//默认的诊断级别
#define PD_DFT_DIAGLEVEL PDWARNING

//pdLog(level,__func__,__FILE__,__LINE__,fmt,##__VA_ARGS__);
void pdLog(PDLEVEL level,const char *func,const char *file,unsigned int line,const char *fmt, ...);
void pdLog(PDLEVEL level,const char *func,const char *file,unsigned int line,std::string message);

#endif
