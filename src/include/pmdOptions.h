#ifndef _PMD_OPTIONS_H
#define _PMD_OPTIONS_H

#include "core.h"
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

#define PMD_OPTION_HELP     "help"
#define PMD_OPTION_DBPATH   "dbpath"
#define PMD_OPTION_SVCNAME  "svcname"
#define PMD_OPTION_MAXPOOL  "maxpool"
#define PMD_OPTION_LOGPATH  "logpath"
#define PMD_OPTION_CONFPATH "confpath"

/*
 * 为了 boost::program_options 服务
 * */
#define PMD_ADD_PARAM_OPTIONS_BEGIN(desc) desc.add_options()
#define PMD_ADD_PARAM_OPTIONS_END ;

#define PMD_COMMANDS_STRING(a,b) (std::string(a) + std::string(b)).c_str()
/*
 * options_description.add_desc()后使用,实际上是给一个代理类的重载函数作参数
 * options_description_easy_init::operator()() 
 * */
#define PMD_COMMANDS_OPTIONS  \
  (PMD_COMMANDS_STRING(PMD_OPTION_HELP,",h"),"help")\
  (PMD_COMMANDS_STRING(PMD_OPTION_DBPATH,",d"),boost::program_options::value<std::string>(),"detabase file full path")\
  (PMD_COMMANDS_STRING(PMD_OPTION_SVCNAME,",s"),boost::program_options::value<std::string>(),"local srevice name")\
  (PMD_COMMANDS_STRING(PMD_OPTION_MAXPOOL,",m"),boost::program_options::value<unsigned int>(),"max pool agent")\
  (PMD_COMMANDS_STRING(PMD_OPTION_LOGPATH,",l"),boost::program_options::value<std::string>(),"diagnostic log file full path")\
  (PMD_COMMANDS_STRING(PMD_OPTION_CONFPATH,",c"),boost::program_options::value<std::string>(),"configuration file full path")

#define CONFFILENAME  "edb.conf"
#define LOGFILENAME   "diag.log"
#define DBFILENAME    "edb.data"
#define SVCNAME       "48127"
#define NUMPOOL       20

class pmdOptions
{
  public:
    pmdOptions();
    ~pmdOptions();
  public:
    //从程序启动命令行初始化
    int readCmd(int argc,char **argv,
                boost::program_options::options_description &desc,
                boost::program_options::variables_map &vm);

    //通过map提取初值
    int importVM(const boost::program_options::variables_map &vm,
                bool isDefault = true); 

    //从配置文件路径来初始化
    int readConfigureFile(const char *path,
                boost::program_options::options_description &desc,
                boost::program_options::variables_map &vm);

    int init(int argc,char **argv);
  public:
    inline char *getDBPath() { return _dbPath;}
    inline char *getLogPath() { return _logPath; }
    inline char *getConfPath() { return _confPath; }
    inline char *getServiceName() { return _svcName; }
    inline int getMaxPool() { return _maxPool; }

  private:
    char _dbPath[OSS_MAX_PATHSIZE + 1]; //数据库路径
    char _logPath[OSS_MAX_PATHSIZE + 1];
    char _confPath[OSS_MAX_PATHSIZE + 1];
    char _svcName[NI_MAXSERV + 1];
    int _maxPool;
};
#endif
