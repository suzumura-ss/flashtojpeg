#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "jpegenc.h"

#define ERR(...)  m_msg->error("jpegenc: "__VA_ARGS__)
#define DBG(...)  m_msg->debug("jpegenc: "__VA_ARGS__)


const char* JpegEncoderException::get_text()
{
  if(m_txt) return m_txt;
  return strerror(m_errno);
}


void  JpegEncoder::set_size(size_t width, size_t height)
{
  DBG("set_size(%d, %d)\n", width, height);

  m_width = width;
  m_height = height;
}


void  JpegEncoder::set_quality(int quality)
{
  DBG("set_quality(%d)\n", quality);

  m_quality = quality;
}


void  JpegEncoder::open(const char* file_name)
{
  DBG("open(%s)\n", file_name);

  if(m_fp) throw JpegEncoderException("open: m_fp!=NULL");
  m_fp = strcmp(file_name, "--")? fopen(file_name, "w"): stdout;
  if(!m_fp) throw JpegEncoderException(errno);
}


void  JpegEncoder::encode(const unsigned char* bgra32)
{
  DBG("encode(%p)\n", bgra32);

  unsigned char rgb24[m_width*3];

  start_compress();
  for(size_t it=0; it<m_height; it++) {
    convert_bgra32_to_rgb24(bgra32, rgb24);
    encode_single_scanline(rgb24);
    bgra32 += m_cinfo.image_width*4;
  }
  finish_compress();
}


void  JpegEncoder::close()
{
  DBG("close()\n");

  if(m_fp && m_fp!=stdout) fclose(m_fp);
  m_fp = NULL;
}


void  JpegEncoder::start_compress()
{
  DBG("start_compress()\n");

  m_cinfo.image_width = m_width;
  m_cinfo.image_height = m_height;
  m_cinfo.input_components = 3;
  m_cinfo.in_color_space = JCS_RGB;

  DBG("- jpeg_set_defaults()\n");
  jpeg_set_defaults(&m_cinfo);

  DBG("- jpeg_set_quality()\n");
  jpeg_set_quality(&m_cinfo, m_quality, TRUE);

  DBG("- jpeg_stdio_dest()\n");
  jpeg_stdio_dest(&m_cinfo, m_fp? m_fp: stdout);

  DBG("- jpeg_start_compress()\n");
  jpeg_start_compress(&m_cinfo, TRUE);
}


void  JpegEncoder::convert_bgra32_to_rgb24(const unsigned char* bgra32, unsigned char* rgb24)
{
  DBG("convert_bgra32_o_rgb24(%p, %p)\n", bgra32, rgb24);

  for(size_t it=0; it<m_width; it++) {
    rgb24[it*3+0] = bgra32[it*4+2]; // R
    rgb24[it*3+1] = bgra32[it*4+1]; // G
    rgb24[it*3+2] = bgra32[it*4+0]; // B
  }
}


void  JpegEncoder::encode_single_scanline(unsigned char* rgb24)
{
  DBG("encode_single_scanline(%p)\n", rgb24);

  JSAMPROW    jsr[] = { rgb24 };
  JSAMPARRAY  jsa = jsr;
  jpeg_write_scanlines(&m_cinfo, jsa, 1);
}


void  JpegEncoder::finish_compress()
{
  DBG("finish_compress()\n");

  jpeg_finish_compress(&m_cinfo);
}
