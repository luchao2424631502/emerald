#include "pmdOptions.h" 
#include "pd.h"

pmdOptions::pmdOptions()
{
  memset(_dbPath, 0, sizeof(_dbPath));
  memset(_logPath, 0, sizeof(_logPath));
  memset(_confPath, 0, sizeof(_confPath));
  memset(_svcName, 0, sizeof(_svcName));
  _maxPool = NUMPOOL;
}

pmdOptions::~pmdOptions()
{

}

//从命令行读取(按照desc的规则)读取到vm(std::map)中
int pmdOptions::readCmd(int argc,char **argv,
                    boost::program_options::options_description &desc,
                    boost::program_options::variables_map &vm)
{
  int rc = EDB_OK;
  try 
  {
    //把所有命令行的参数(按照desc的规则)解析到vm(std::map) 中
    boost::program_options::store(boost::program_options::command_line_parser(argc,argv).options(desc).allow_unregistered().run(),
                                  vm);
    boost::program_options::notify(vm);
  }
  catch (boost::program_options::unknown_option &e)
  {
    std::cerr << "Unknown arguments: " << e.get_option_name() << std::endl;
  }
  catch (boost::program_options::invalid_option_value &e)
  {
    std::cerr << "Invalid arguments: " << e.get_option_name() << std::endl; 
    rc = EDB_INVALIDARG;
    goto error;
  }
  catch (boost::program_options::error &e)
  {
    std::cerr << "Error: "<< e.what() << std::endl;
    rc = EDB_INVALIDARG;
    goto error;
  }

done:
  return rc;
error:
  goto done;
}

//从配置文件中根据(desc)的规则,生成vm(std::map)
int pmdOptions::readConfigureFile(const char *path,
                                  boost::program_options::options_description &desc,
                                  boost::program_options::variables_map &vm)
{
  int rc = EDB_OK;
  char conf[OSS_MAX_PATHSIZE + 1]={0};
  strncpy(conf,path,OSS_MAX_PATHSIZE);

  try 
  {
    boost::program_options::store(boost::program_options::parse_config_file<char>(conf,desc,true),vm);
    boost::program_options::notify(vm);
  }
  catch (boost::program_options::reading_file)
  {
    std::cerr << "Failed to open config file: " << (std::string)conf << std::endl << "Using default settings" << std::endl;
    rc = EDB_IO;
    goto error;
  }
  catch (boost::program_options::unknown_option &e)
  {
    std::cerr << "Unknown config element: " << e.get_option_name() << std::endl;
    rc = EDB_INVALIDARG;
    goto error;
  }
  catch (boost::program_options::invalid_option_value &e)
  {
    std::cerr << (std::string) "Invalid config element: " << e.get_option_name() << std::endl;
    rc = EDB_INVALIDARG;
    goto error;
  }
  catch (boost::program_options::error &e)
  {
    std::cerr << e.what() << std::endl;
    rc = EDB_INVALIDARG;
    goto error;
  }

done:
  return rc;
error:
  goto done;
}

//从vm(std::map)中提取配置项
int pmdOptions::importVM(const boost::program_options::variables_map &vm,bool isDefault)
{
  int rc = EDB_OK;
  const char *p = nullptr;

  //conf file path
  if (vm.count(PMD_OPTION_CONFPATH))
  {
    p = vm[PMD_OPTION_CONFPATH].as<std::string>().c_str();
    strncpy(_confPath,p,OSS_MAX_PATHSIZE);
  }
  else if (isDefault)
  {
    strcpy(_confPath,"./" CONFFILENAME);
  }

  //log file path
  if (vm.count(PMD_OPTION_LOGPATH))
  {
    p = vm[PMD_OPTION_LOGPATH].as<std::string>().c_str();
    strncpy(_logPath,p,OSS_MAX_PATHSIZE);
  }
  else if (isDefault)
  {
    strcpy(_logPath,"./" LOGFILENAME);
  }

  //db file path
  if (vm.count(PMD_OPTION_DBPATH))
  {
    p = vm[PMD_OPTION_DBPATH].as<std::string>().c_str();
    strncpy(_dbPath,p,OSS_MAX_PATHSIZE);
  }
  else if (isDefault)
  {
    strcpy(_dbPath,"./" DBFILENAME);
  }

  //svcname
  if (vm.count(PMD_OPTION_SVCNAME))
  {
    p = vm[PMD_OPTION_SVCNAME].as<std::string>().c_str();
    strncpy(_svcName,p,NI_MAXSERV);
  }
  else if (isDefault)
  {
    strcpy(_svcName,SVCNAME);
  }

  //maxpool
  if (vm.count(PMD_OPTION_MAXPOOL))
  {
    _maxPool = vm[PMD_OPTION_MAXPOOL].as<unsigned int>();
  }
  else if (isDefault)
  {
    _maxPool = NUMPOOL;
  }
  return rc;
}

int pmdOptions::init(int argc,char **argv)
{
  int rc = EDB_OK;
  boost::program_options::options_description all("Command options");
  //添加options_descriptions规则
  all.add_options()PMD_COMMANDS_OPTIONS;

  boost::program_options::variables_map vm;
  boost::program_options::variables_map vm2;

  //1.解析命令行参数存放到vm
  readCmd(argc,argv,all,vm);
  if (rc)
  {
    PD_LOG(PDERROR,"Failed to read cmd, rc = %d",rc);
    goto error;
  }

  //check if we have help options(有help命令则其他的不需要管)
  if (vm.count(PMD_OPTION_HELP))
  {
    std::cout << all << std::endl;
    rc = EDB_PMD_HELP_ONLY;
    goto done;
  }

  //检查是否有conf path(配置文件选项)
  if (vm.count(PMD_OPTION_CONFPATH))
  {
    //从配置文件中读取的配置放到 vm2 中
    rc = readConfigureFile(vm[PMD_OPTION_CONFPATH].as<std::string>().c_str(),all,vm2);
    if (rc)
    {
      PD_LOG(PDERROR,"Unexpected error when reading conf file, rc = %d",rc);
      goto error;
    }
  }

  //2.根据命令行指出的conf file,解析conf file里面的配置选项到pmdOptions类中
  rc = importVM(vm2); //没有的选项 isDefault=true 就用默认值
  if (rc)
  {
    PD_LOG(PDERROR,"Failed to import from vm2, rc = %d",rc);
    goto error;
  }

  //命令行的选项优先级高,
  rc = importVM(vm);
  if (rc)
  {
    PD_LOG(PDERROR,"Failed to import from vm. rc = %d",rc);
    goto error;
  }
done:
  return rc;
error:
  goto done;
}
