/**
 * Author: Jonathan Whitaker & Christopher Hartley
 * Last modified: Wed March 25, 2015
 * Name: bouncer.c
 * CS 3505 Spring 2015
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

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

  // set the pixel format and dimensions
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
    // otherwise an error occured
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

// Begin Dranger
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
	// convert the image from its native format to RGB24
	sws_scale(
	   sws_ctx,
	   (uint8_t const * const *)pFrame->data,
	   pFrame->linesize,
	   0,
	   pCodecCtx->height,
	   pFrameRGB->data,
	   pFrameRGB->linesize
	   );

	// set the RGB frame dimensions and format
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

  // return the AVFrame to the caller
  return pFrameRGB;
}
// End Dranger

void draw_circle(AVFrame *frame, int x0, int y0, int r) {
    int x, y;
    uint8_t *ptr;

    uint8_t red = 0; 
    uint8_t green = 0;
    uint8_t blue = 255;

    // iterate over each pixel in the image
    for(y = 0; y < frame->height; y++) {
	for(x = 0; x < frame->width; x++) {
	  // get a pointer to the Red byte value for the current
	  // pixel
	  ptr = frame->data[0] + y*frame->linesize[0] + x*3; // points to red
	  
	  // apply pythagorean theorem to get the square of the distance
	  // from the origin of the circle
	  int d = (x-x0)*(x-x0) + (y-y0)*(y-y0);

	  // if a^2 + b^2 <= r^2 then we are within the
	  // circle
	  if (d <= r*r) {
	    // apply linear interpolation to create a blue->pink'ish
	    // gradient
	    double dist = sqrt(d);
	    double perct = dist / (double) r;

	    // the red and green values are a percentage of the distance 
	    // from the origin to the outer edge of the circle
	    red = perct * 150; // perct * 150 creates a blue->pink'ish grad
	    green = perct * 150;

	    // copy the new RGB values into the frame
	    *ptr = red;
	    *(ptr+1) = green;
	    *(ptr+2) = blue;
     	  }
	}
   }
}

// main entry point
int main(int argc, char** argv)
{
  int i, x0, y0, r, grav;

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
  AVFrame *frame_copy;
  AVFrame *curr_frame;
  frame_copy = open_image(filename);
  
  // allocate a new frame for the copy and set its members
  curr_frame = av_frame_alloc();
  curr_frame->width = frame_copy->width;
  curr_frame->height = frame_copy->height;
  curr_frame->format = frame_copy->format;

  int ret;
  ret = av_image_alloc(curr_frame->data, curr_frame->linesize, 
		       curr_frame->width, curr_frame->height, PIX_FMT_RGB24, 32);

  // check if a buffer could be allocated
  if (ret < 0) {
      fprintf(stderr, "Could not allocate raw picture buffer\n");
      exit(1);
  }
  
  // create an array of values to 
  double height_mult[] = {0.9, 0.850, 0.825, 0.8, 0.750, 0.725, 0.7, 0.65, 0.625, 
   			  0.6, 0.6, 0.625, 0.65, 0.7, 0.725, 0.75, 0.8, 0.825, 0.85, 0.9};

  // the ball size and location should be a percentage 
  // of the image dimensions
  x0 = curr_frame->width / 2; // start the ball in the center
  //y0 = curr_frame->height * 0.9; // start the ball at the bottom
  if (curr_frame->width < curr_frame->height)
    r = curr_frame->width * 0.10;
  else
    r = curr_frame->height * 0.10;

  // output 300 files (into the current working directory) 
  // containing the frames of the animation. 
  char frame_name[14];
  for(i = 0; i < 300; i++) {
    snprintf(frame_name, sizeof(char) * 14, "frame%03d.mpff", i);
    
    // copy the frame data
    av_frame_copy(curr_frame, frame_copy);
    
    // draw the circle to the frame
    int phase_size = sizeof(height_mult) / sizeof(height_mult[0]);
    y0 = curr_frame->height * height_mult[i % phase_size];
    draw_circle(curr_frame, x0, y0, r);

    // save the current frame
    save_frame(frame_name, curr_frame);
  }

  // free the RGB buffer
  av_free(frame_copy);
  av_free(curr_frame);

  return 0;
}
