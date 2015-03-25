#include <stdio.h>
#include <string.h>
#include <math.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

// Ref: https://github.com/chelyaev/ffmpeg-tutorial/blob/master/tutorial01.c
void save_frame(char *filename, AVFrame *pFrame) {
  FILE *f;
  AVCodec *avc;
  AVCodecContext *avctx = NULL;
  AVPacket pkt;
  int got_output;

  // get MPFF encoder
  avc = avcodec_find_encoder(AV_CODEC_ID_MPFF);
  if (!avc) {
    fprintf(stderr, "MPFF codec could not be found\n");
    exit(1);
  }

  // get the codec context
  avctx = avcodec_alloc_context3(avc);

  avctx->pix_fmt = PIX_FMT_RGB24;
  avctx->width = pFrame->width;
  avctx->height = pFrame->height;

  // open the codec
  if (avcodec_open2(avctx, avc, NULL) < 0) {
    fprintf(stderr, "MPFF codec could not be opened\n");
    exit(1);
  }

  // initialize a packet
 
  av_init_packet(&pkt);
  pkt.data = NULL;
  pkt.size = 0;

  // encode the data into the packet
  avcodec_encode_video2(avctx, &pkt, pFrame, &got_output);

  // open file
  f = fopen(filename, "wb");
  if(!f) {
    fprintf(stderr, "save_frame: Couldn't open file %s", filename);
    exit(1);
  }
  
  if (got_output)
    // write the packet data to the output file
    fwrite(pkt.data, 1, pkt.size, f);
  else {
    fprintf(stderr, "save_frame: Encoder didn't get any output.");
    exit(1);
  }

  // close the output file
  fclose(f);

  // close and free up the codec context
  avcodec_close(avctx);
  av_free(avctx);

  // free the packet
  av_free_packet(&pkt);
}

// Ref: https://github.com/chelyaev/ffmpeg-tutorial/blob/master/tutorial01.c
AVFrame* open_image(const char* filename)
{
  AVFormatContext *pFormatCtx = NULL;
  int i, videoStream;
  AVCodecContext *pCodecCtx = NULL;
  AVCodec *pCodec = NULL;
  AVFrame *pFrame = NULL;
  AVFrame *pFrameRGB = NULL;
  AVPacket packet;
  int frameFinished;
  int numBytes;
  uint8_t *buffer = NULL;

  struct SwsContext *sws_ctx = NULL;

  // register all formats and codecs
  av_register_all();

  // open video file
  if(avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
    fprintf(stderr, "Can't open image file '%s'\n", filename);
    return NULL;
  }

  // retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    fprintf(stderr, "Couldn't find stream information\n");
    return NULL;
  }

  // dump information about file onto std out
  av_dump_format(pFormatCtx, 0, filename, 0);

  // find the first video stream
  videoStream=-1;
  for(i=0; i < pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }

  if(videoStream == -1) {
    fprintf(stderr, "Didn't find a video stream.\n");
    return NULL;
  }

  // get a pointer to the codec context for the video stream
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;

  // find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec == NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return NULL;
  }

  // open the found codec
  if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
    fprintf(stderr, "Could not open the given codec.\n");
    return NULL;
  }

  // allocate video frame
  pFrame=av_frame_alloc();

  // allocate an AVFrame structure
  pFrameRGB=av_frame_alloc();
  if(pFrameRGB == NULL) {
    fprintf(stderr, "Can't allocate memory for AVFrame\n");
    return NULL;
  }

  // determine required buffer size and allocate buffer
  numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
  buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

  sws_ctx = sws_getContext(
			   pCodecCtx->width,
			   pCodecCtx->height,
			   pCodecCtx->pix_fmt,
			   pCodecCtx->width,
			   pCodecCtx->height,
			   PIX_FMT_RGB24,
			   SWS_BILINEAR,
			   NULL,
			   NULL,
			   NULL
			   );

  // assign appropriate parts of buffer to image planes in pFrameRGB
  avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
		 pCodecCtx->width, pCodecCtx->height);

  // read frames
  while(av_read_frame(pFormatCtx, &packet) >= 0) {
    if(packet.stream_index == videoStream) {
      // decode video frame
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

      if(frameFinished) {
	// convert the image from its native format to RGB
	sws_scale(
	   sws_ctx,
	   (uint8_t const * const *)pFrame->data,
	   pFrame->linesize,
	   0,
	   pCodecCtx->height,
	   pFrameRGB->data,
	   pFrameRGB->linesize
	   );

	pFrameRGB->width = pCodecCtx->width;
	pFrameRGB->height = pCodecCtx->height;
	pFrameRGB->format = PIX_FMT_RGB24;
	
      }
    }

    // free the packet that was allocated by av_read_frame
    av_free_packet(&packet);
  }

  // free the YUV frame
  av_free(pFrame);

  // close the codec
  avcodec_close(pCodecCtx);

  // close the video file
  avformat_close_input(&pFormatCtx);

  return pFrameRGB;
}

void draw_circle(AVFrame *frame, int x0, int y0, int r) {
    int x, y;
    uint8_t *ptr;

    uint8_t red = 255; 
    uint8_t green = 255;
    uint8_t blue = 255;
    for(y = 0; y < frame->height; y++) {
	for(x = 0; x < frame->width; x++) {
		ptr = frame->data[0] + y*frame->linesize[0] + x*3; // points to red
		if ((x-x0)^2 + (y-y0)^2 <= r^2) {
			memset(ptr, red, 1);
			memset(ptr+1, green, 1);
			//memset(ptr+2, blue, 1);
		}
	}
   }

}

// main entry point
int main(int argc, char** argv)
{
  int i, x, y, r, grav;

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

  // decode the JPG image to an AVFrame and create a copy
  const AVFrame *frame_copy;
  AVFrame *curr_frame;
  frame_copy = open_image(filename);
  
  // allocate a new frame for the copy and set its members
  curr_frame = av_frame_alloc();
  curr_frame->width = frame_copy->width;
  curr_frame->height = frame_copy->height;
  curr_frame->format = frame_copy->format;

  int ret;
  ret = av_image_alloc(curr_frame->data, curr_frame->linesize, curr_frame->width, curr_frame->height,  
                        PIX_FMT_RGB24, 32);

  if (ret < 0) {
      fprintf(stderr, "Could not allocate raw picture buffer\n");
      exit(1);
  }

  // the ball size and location should be a percentage 
  // of the image dimensions
  x = curr_frame->width / 2;
  y = curr_frame->height * curr_frame->width / 3;
  r = 150;
  printf("x0: %d\n", x);
  printf("y0: %d\n", y);
  printf("radius: %d\n", r);
  // create some color enums
  enum {BLACK=0x000000, WHITE=0xFFFFFF};

  // output 300 files (into the current working directory) 
  // containing the frames of the animation. 
  char frame_name[14];
  for(i = 0; i < 300; i++) {
    snprintf(frame_name, sizeof(char) * 14, "frame%03d.mpff", i);
    
    // copy the frame data
    av_frame_copy(curr_frame, frame_copy);
    
    // draw the circle to the frame
    draw_circle(curr_frame, x, y, r);

    // save the current frame
    save_frame(frame_name, curr_frame);


  }

  // free the RGB buffer
  //av_free(frame_copy);
  //av_free(curr_frame);

  return 0;
}
