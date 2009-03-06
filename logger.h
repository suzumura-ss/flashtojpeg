#ifndef __INCLUDE_LOGGER_H__
#define __INCLUDE_LOGGER_H__

#include <stdarg.h>

class Logger
{
public:
  enum LEVEL {
    ERROR, INFO, DEBUG
  };

  Logger() {
    m_fp = stderr;
    m_level = ERROR;
  };
  virtual ~Logger() {};

  void set_level(LEVEL lv) {
    m_level = lv;
  };

  void error(const char* fmt, ...);
  void info(const char* fmt, ...);
  void debug(const char* fmt, ...);

protected:
  void add(LEVEL lv, const char* fmt, va_list ap);

private:
  FILE* m_fp;
  LEVEL m_level;
};

#endif //__INCLUDE_LOGGER_H__
