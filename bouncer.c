#include <stdio.h>
#include <string.h>
#include "libavutil/frame.h"
#include "libavutil/pixfmt.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

AVFrame* OpenImage(const char* filename)
{
  AVFormatContext *pFormatCtx;
  if (avformat_open_input(&pFormatCtx,filename,NULL,NULL)!=0)
  {
      printf("Error in opening input file %s",filename);
      return ERROR_CODE;
  }

  av_dump_format(pFormatCtx, 0, filename, false);

  AVCodecContext *pCodecCtx;

  pCodecCtx = pFormatCtx->streams[0]->codec;
  pCodecCtx->pix_fmt = AV_PIX_FMT_RGB24;

  // find the decoder for the stream
  AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  if (!pCodec)
  {
      printf("Codec not found\n");
      return NULL;
  }

  // open codec
  if(avcodec_open(pCodecCtx, pCodec) < 0)
  {
      printf("Could not open codec\n");
      return NULL;
  }

  AVFrame *pFrame;

  pFrame = avcodec_alloc_frame();
  if (!pFrame)
  {
      printf("Can't allocate memory for AVFrame\n");
      return NULL;
  }

  int frameFinished;
  int numBytes;

  // determine required buffer size and allocate buffer
  numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
  uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

  avpicture_fill((AVPicture *) pFrame, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

  // read the frame
  AVPacket packet;

  int framesNumber = 0;
  while (av_read_frame(pFormatCtx, &packet) >= 0)
  {
      if(packet.stream_index != 0) {
	     continue;
      }

      int ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
      if (ret > 0)
      {
	       printf("Frame is decoded, size %d\n", ret);
	       pFrame->quality = 4;
	       return pFrame;
      }
      else {
	     printf("Error [%d] while decoding frame: %s\n", ret, strerror(AVERROR(ret)));
       return NULL;
     }
   }
}

// main entry point
int main(int argc, char** argv)
{
  // the bouncer application should take a single command line argument.
  if (argc != 2) {
    printf("Illegal Argument Amount\n");
    printf("usage: bouncer filename.jpg\n");
    return 1;
  }

  // application should reject non-jpg files (by simply testing the extension).
  char * filename = argv[1];
  char * ext = strrchr(filename, '.');
  if (ext) {
    if (strcmp(ext + 1, "jpg") != 0) {
      printf("Illegal File Extension\n");
      return 1;
    }
  }
  else {
    printf("Illegal File Extension\n");
    return 1;
  }

  return 0;
}
