#include <stdio.h>

#include "logger.h"

void  Logger::error(const char* fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  add(ERROR, fmt, ap);
  va_end(ap);
}

void  Logger::info(const char* fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  add(INFO, fmt, ap);
  va_end(ap);
}

void  Logger::debug(const char* fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  add(DEBUG, fmt, ap);
  va_end(ap);
}

void  Logger::add(LEVEL lv, const char* fmt, va_list ap)
{
  if(m_level<lv) return;
  vfprintf(m_fp, fmt, ap);
}
