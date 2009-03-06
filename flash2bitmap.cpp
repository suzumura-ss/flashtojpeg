#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "flash2bitmap.h"

#define ERR(...)  m_msg->error("f2bmp: "__VA_ARGS__)
#define DBG(...)  m_msg->debug("f2bmp: "__VA_ARGS__)


const char* Flash2bitmapException::get_text()
{
  if(m_txt) return m_txt;
  return strerror(m_errno);
}


Flash2bitmap::Flash2bitmap()
{
  m_msg = new Logger;

  DBG("Flash2bitmap()\n");
  m_surface = NULL;
  // m_skip_frame = NULL;
  m_image = NULL;
  m_fp = NULL;
  m_width = 0;
  m_height = 0;
  m_stride = 0;

  DBG("- fad_frame_init()\n");
  memset(&m_frame, 0, sizeof(m_frame));
  fad_frame_init(&m_frame);

  DBG("- fad_stream_init()\n");
  memset(&m_stream, 0, sizeof(m_stream));
  fad_stream_init(&m_stream, NULL);
}


Flash2bitmap::~Flash2bitmap()
{
  close();
  fad_stream_finish(&m_stream);
  fad_frame_finish(&m_frame);
}


void  Flash2bitmap::open(const char* file_name)
{
  DBG("open(%s)\n", file_name);

  DBG("- fopen()\n");
  if(m_fp) throw Flash2bitmapException("open: m_fp!=NULL");
  m_fp = fopen(file_name, "r");
  if(!m_fp) throw Flash2bitmapException(errno);

  DBG("- fad_stream_stdio()\n");
  fad_stream_stdio(&m_stream, m_fp);

  DBG("- fad_frame_decode()\n");
  fad_frame_decode(&m_frame, &m_stream);

  if(m_stream.err != FAD_ERROR_NONE) {
    close();
    throw Flash2bitmapException("open: Illegal format.");
  }
  m_width  = get_width();
  m_height = get_height();

  DBG("- init_sdl()\n");
  init_sdl();
}


void  Flash2bitmap::close()
{
  DBG("close()\n");

  if(m_surface) SDL_FreeSurface(m_surface);
  if(m_image) free(m_image);
  if(m_fp) fclose(m_fp);
  m_surface = NULL;
  m_image = NULL;
  m_fp = NULL;
}


void  Flash2bitmap::start_render(size_t width, size_t height)
{
  DBG("start_render(%d, %d)\n", width, height);

  if(width!=0)  m_width = width;
  if(height!=0) m_height = height;
  m_stride = m_width * 4;   // ARGB
  DBG(" - width = %d, height = %d, stride = %d\n", m_width, m_height, m_stride);

  m_image = (unsigned char*)calloc(1, m_stride * m_height);
  if(!m_image) {
    close();
    throw Flash2bitmapException("open: Memory allocation failed.");
  }
 
  DBG(" - SDL_CreateRGBSurfaceFrom()\n");
  m_surface = SDL_CreateRGBSurfaceFrom(m_image, m_width, m_height, 32, m_stride, 0, 0, 0, 0xff<<24);
  if(!m_surface) {
    throw Flash2bitmapException("init_sdl: SDL_CreateRGBSurfaceFrom():0 failed.\n");
  }
  m_frame.render->cr = create_cairo(m_width, m_height, m_stride);
  m_skip_frame = create_cairo(1, 1, 4);
}


void  Flash2bitmap::render_single_frame()
{
  DBG("render_single_frame()\n");

  do_sdl_event();
  if(m_frame.sta == FAD_STA_END)  return;
  if(m_frame.sta == FAD_STA_STOP) return;

  while(m_frame.sta!=FAD_STA_DORENDER &&
        m_frame.sta!=FAD_STA_FINISH &&
        m_frame.sta!=FAD_STA_UPDATEBTN ) {
    fad_frame_decode(&m_frame, &m_stream);
  }

  SDL_FillRect(m_surface, NULL, SDL_MapRGB(m_surface->format, 0, 0, 0));
  fad_frame_render_movie(&m_frame);
}


void  Flash2bitmap::seek(long ofs)
{
  DBG("seek(%d)\n", ofs);

  do_sdl_event();
  if(m_frame.sta == FAD_STA_END)  return;
  if(m_frame.sta == FAD_STA_STOP) return;

  while(m_frame.sta!=FAD_STA_DORENDER &&
        m_frame.sta!=FAD_STA_FINISH &&
        m_frame.sta!=FAD_STA_UPDATEBTN ) {
    fad_frame_decode(&m_frame, &m_stream);
  }
  // fad_frame_seek(&m_frame, &m_stream, ofs);
  cairo_t* cr = m_frame.render->cr;
  m_frame.render->cr = m_skip_frame;
  fad_frame_render_movie(&m_frame);
  m_frame.render->cr = cr;
}


void  Flash2bitmap::init_sdl()
{
  DBG("init_sdl()\n");

  // Initialize SDL.
  if(SDL_Init(SDL_INIT_VIDEO) < 0) {
    throw Flash2bitmapException("init_sdl: SDL_Init() failed.");
  }
  atexit(SDL_Quit);
  signal(SIGINT, SIG_DFL);
}


cairo_t*  Flash2bitmap::create_cairo(size_t width, size_t height, size_t stride)
{
  DBG("set_cairo()\n");
  cairo_t*  cr = NULL;

  // Initialize cairo surface.
  DBG(" - cairo_image_surface_for_data()\n");
  cairo_surface_t* cs = cairo_image_surface_create_for_data(m_image, CAIRO_FORMAT_ARGB32, width, height, stride);
  if(!cs) {
    throw Flash2bitmapException("set_cairo: cairo_image_surface_for_data() failed.\n");
  }

  DBG(" - cairo_create(%p)\n", cs);
  cr = cairo_create(cs);
  if(!cr) {
    throw Flash2bitmapException("set_cairo: cairo_create() failed.\n");
  }

  DBG(" - cairo_surface_destroy()\n");
  cairo_surface_destroy(cs);

  DBG(" - cairo_set_fill_rule()\n");
  cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

  DBG(" - cairo_set_tolerance()\n");
  cairo_set_tolerance(cr, 0.5);

  return cr;
}


void Flash2bitmap::do_sdl_event()
{
  DBG("do_sdl_event()\n");

  bool  quit = false;

  push_do_next_frame();
  while(!quit && SDL_WaitEvent(&m_event)) {
    switch(m_event.type) {
    case NEXT_FRAME:
      DBG(" - NEXT_FRAME\n");
    case SDL_QUIT:
      quit = true;
      break;
    default:
      break;
    }
  }
}


void Flash2bitmap::push_do_next_frame()
{
  SDL_Event event;

  event.type = NEXT_FRAME;
  event.user.code = 2;
  event.user.data1 = NULL;
  event.user.data2 = NULL;

  SDL_PushEvent(&event);
}
