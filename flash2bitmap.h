#ifndef __INCLUDE_FLASH2BITMAP_H__
#define __INCLUDE_FLASH2BITMAP_H__

#include <SDL/SDL.h>
#ifdef __cplusplus
#define __cplusplsu
#endif
#include <fad.h>
#include "logger.h"


class Flash2bitmapException
{
  public:
  Flash2bitmapException() {
    m_errno = 0;
    m_txt = NULL;
  };

  Flash2bitmapException(const char* txt) {
    m_errno = 0;
    m_txt = txt;
  };

  Flash2bitmapException(int err) {
    m_errno = err;
    m_txt = NULL;
  };

  virtual ~Flash2bitmapException() {}
  const char* get_text();

private:
  int m_errno;
  const char* m_txt;
};


class Flash2bitmap
{
public:
  Flash2bitmap();
  virtual ~Flash2bitmap();

  void  open(const char* file_name);
  void  close();
  void  start_render(size_t width=0, size_t height=0);
  void  render_single_frame();
  void  seek(long ofs=1);

  size_t  get_width() {
    return (m_frame.size.x1 - m_frame.size.x0)/20;
  };

  size_t  get_height() {
    return (m_frame.size.y1 - m_frame.size.y0)/20;
  };

  float get_framerate() {
    return ((float)m_frame.rate)/256.0;
  };

  const unsigned char* get_surface() {
    return m_image;
  };

  void  set_logger(Logger& logger) {
    delete m_msg;
    m_msg = &logger;
  };

protected:
  enum {
    NEXT_FRAME = SDL_USEREVENT+100,
  };
  void  init_sdl();
  cairo_t*  create_cairo(size_t width, size_t height, size_t stride);
  void  push_do_next_frame();
  void  do_sdl_event();

private:
  fad_frame_t     m_frame;
  fad_stream_t    m_stream;
  SDL_Surface*    m_surface;
  SDL_Event       m_event;
  cairo_t*        m_skip_frame;
  unsigned char*  m_image;
  FILE* m_fp;
  size_t  m_width, m_height, m_stride;
  Logger* m_msg;
};

#endif  //__INCLUDE_FLASH2BITMAP_H__
