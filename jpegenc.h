#ifndef __INCLUDE__JPEGENC_H__
#define __INCLUDE__JPEGENC_H__

#include <jpeglib.h>
#include "logger.h"


class JpegEncoderException
{
public:
  JpegEncoderException() {
    m_errno = 0;
    m_txt = NULL;
  };

  JpegEncoderException(int err) {
    m_errno = err;
    m_txt = NULL;
  };

  JpegEncoderException(const char* txt) {
    m_errno = 0;
    m_txt = txt;
  };

  virtual ~JpegEncoderException() {};
  const char* get_text();

private:
  int m_errno;
  const char* m_txt;
};


class JpegEncoder
{
public:
  JpegEncoder() {
    m_msg = new Logger;
    m_fp = NULL;
    m_quality = 100;
    m_width = 0;
    m_height = 0;
    m_cinfo.err = jpeg_std_error(&m_err);
    jpeg_create_compress(&m_cinfo);
  };

  virtual ~JpegEncoder() {
    jpeg_destroy_compress(&m_cinfo);
  };

  void  set_size(size_t width, size_t height);
  void  set_quality(int quality);
  void  open(const char* file_name);
  void  encode(const unsigned char* bgra32);
  void  close();

  void  set_logger(Logger& logger) {
    delete m_msg;
    m_msg = &logger;
  };

protected:
  void  start_compress();
  void  convert_bgra32_to_rgb24(const unsigned char* bgra32, unsigned char* rgb24);
  void  encode_single_scanline(unsigned char* rgb24);
  void  finish_compress();

private:
  struct jpeg_compress_struct m_cinfo;
  struct jpeg_error_mgr       m_err;
  FILE* m_fp;
  int   m_quality;
  size_t  m_width, m_height;
  Logger* m_msg;
};

#endif //__INCLUDE__JPEGENC_H__
