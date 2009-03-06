#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "flash2bitmap.h"
#include "jpegenc.h"
#include "logger.h"

#define ERR(...)  msg.error(__VA_ARGS__)
#define INF(...)  msg.info(__VA_ARGS__)
#define DBG(...)  msg.debug(__VA_ARGS__)

class Config
{
public:
  long  width;
  long  height;
  long  quality;
  long  frame_by_sec;
  long  frame_by_idx;
  long  do_render_by_sec;
  long  do_render_by_idx;
  const char* source_filename;
  const char* output_filename;
  Logger::LEVEL msg_level;

  Config() {
    width = 0;
    height = 0;
    quality = 100;
    frame_by_sec = 0;
    frame_by_idx = 0;
    do_render_by_sec = 0;
    do_render_by_idx = 0;
    source_filename = NULL;
    output_filename = NULL;
    msg_level = Logger::ERROR;
  };
};  


static void show_version(const char* progname, Logger& msg)
{
  ERR("flashtojpeg version 0.1\n");
  ERR("   Copyright 2009 Toshiyuki Suzumura, under GPL license.\n\n");
}

static void show_usage(const char* progname, Logger& msg)
{
  ERR(" Usage: %s [options] <SWF-file> [write file|--]\n", progname);
  ERR("   -h           :show this message.\n");
  ERR("   -v           :verbose outputs.\n");
  ERR("   -vv          :very verbose outputs.\n");
  ERR("   -t <sec>     :output frame by sec. (default 0)\n");
  ERR("   -q <quality> :JPEG quality(1-100). (default 100)\n");
  // ERR("   -r <sec>     :render frames by sec.\n");
  ERR("   --           :write to stdout.\n");
  ERR("  When 'write file' is not specified, show SWF informations.\n");
  ERR("  If 'SWF-file' is not Flash, it will raise SIGSEGV.\n");
}


static void parse_args(int argc, const char* argv[], Config& cfg, Logger& msg)
{
  DBG("parse_args\n");

  if(argc==1) {
    show_version(argv[0], msg);    
    throw "";
  }
  for(int it=1; it<argc; it++) {
    const char* arg = argv[it];
    int sr;
    DBG("- %s\n", arg);
    if(strcmp(arg, "-h")==0) {
      show_version(argv[0], msg);
      show_usage(argv[0], msg);
      throw "";
   } else
    if(strcmp(arg, "-v")==0) {
      cfg.msg_level = Logger::INFO;
    } else
    if(strcmp(arg, "-vv")==0) {
      cfg.msg_level = Logger::DEBUG;
     } else
    if(strcmp(arg, "-t")==0) {
      if(it+1<argc) {
        sr = sscanf(argv[it+1], "%d", &cfg.frame_by_sec);
      }
      if(sr!=1 || cfg.frame_by_sec<=0) {
        ERR("Illegal option for '-t'.\n");
        throw "-t";
      }
      it++;
    } else
    if(strcmp(arg, "-q")==0) {
       if(it+1<argc) {
        sr = sscanf(argv[it+1], "%d", &cfg.quality);
      }
      if(sr!=1 || cfg.quality<1 || cfg.quality>100) {
        ERR("Illegal option for '-q'.\n");
        throw "-q'";
      }
      it++;
    } else
    if(strcmp(arg, "-r")==0) {
       if(it+1<argc) {
        sr = sscanf(argv[it+1], "%d", &cfg.do_render_by_sec);
      }
      if(sr!=1 || cfg.do_render_by_sec<=0) {
        ERR("Illegal option for '-r'.\n");
        throw "-r'";
      }
      it++;
    } else
    if(strcmp(arg, "--") && arg[0]=='-') {
      ERR("Illegal option: %s\n", arg);
      throw "-?";
    } else
    if(!cfg.source_filename) {
      cfg.source_filename = arg;
    } else {
      cfg.output_filename = arg;
    }
  }
}


static void show_swf_info(Flash2bitmap& flash)
{
  // Show SWF info.
  printf("%dx%d %f\n", flash.get_width(), flash.get_height(), flash.get_framerate());
}


int main(int argc, const char*argv[])
{
  Logger  msg;
  Flash2bitmap  flash;
  JpegEncoder   jpegenc;
  Config  cfg;

  msg.set_level(Logger::ERROR);
  flash.set_logger(msg);
  jpegenc.set_logger(msg);

  // Check arguments.
  try {
    parse_args(argc, argv, cfg, msg);
  }
  catch(const char* e) {
    return -1;
  }
  msg.set_level(cfg.msg_level);
  DBG("cfg.frame_by_sec: %d\n", cfg.frame_by_sec);
  DBG("cfg.do_render_by_sec: %d\n", cfg.do_render_by_sec);
  DBG("cfg.qwuality: %d\n", cfg.quality);
  DBG("cfg.source_filename: %s\n", cfg.source_filename);
  DBG("cfg.output_filename: %s\n", cfg.output_filename);
  if(cfg.source_filename==NULL) {
    ERR("Swf-file is not specified.\n");
    return -1;
  }
 
  // Open SWF.
  try {
    flash.open(cfg.source_filename);
 }
  catch(Flash2bitmapException *e) {
    ERR("Flash2bitmapException: %s\n", e->get_text());
  }

  // Setup render.
  if(cfg.width==0)  cfg.width  = flash.get_width();
  if(cfg.height==0) cfg.height = flash.get_height();
  INF("render    %dx%d\n", cfg.width, cfg.height);

  cfg.frame_by_idx  = (long)(cfg.frame_by_sec * flash.get_framerate());
  INF("frame     %dsec = %didx\n", cfg.frame_by_sec, cfg.frame_by_idx);

  cfg.do_render_by_idx  = (long)(cfg.do_render_by_sec * flash.get_framerate());
  INF("do_render %dsec = %didx\n", cfg.do_render_by_sec, cfg.do_render_by_idx);


  if(cfg.output_filename==NULL) {
    show_swf_info(flash);
    return 0;
  }
 
  // Render.
  try {
    // Render
    flash.start_render(cfg.width, cfg.height);

    int count = 0;
    for(; count<cfg.do_render_by_idx && count<cfg.frame_by_idx; count++) {
      flash.render_single_frame();
      DBG("Render #%4d: %dx%d %f\n", count, flash.get_width(), flash.get_height(), flash.get_framerate());
    }
    for(; count<cfg.frame_by_idx; count++) {
      flash.seek();
      DBG("Seek   #%4d: %dx%d %f\n", count+1, flash.get_width(), flash.get_height(), flash.get_framerate());
    }
    //if(count<cfg.frame_by_idx) flash.seek(cfg.frame_by_idx);
    flash.render_single_frame();
    INF("Render #%4d: %dx%d %f\n", cfg.frame_by_idx, flash.get_width(), flash.get_height(), flash.get_framerate());

    // Write to file by JPEG.
    jpegenc.open(cfg.output_filename);
    jpegenc.set_size(cfg.width, cfg.height);
    jpegenc.set_quality(cfg.quality);
    jpegenc.encode(flash.get_surface());
    jpegenc.close();
  }

  catch(Flash2bitmapException *e) {
    ERR("Flash2bitmapException: %s\n", e->get_text());
  }
  catch(JpegEncoderException *e) {
    ERR("JpegEncoderException: %s\n", e->get_text());
  }

  return 0;
}
