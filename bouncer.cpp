//
//  bouncer.cpp
//
//
//  Created by Christopher Hartley on 3/21/15.
//
//

#include "bouncer.h"
#include "iostream"
#include "string"
#include "frame.h"
#include "avformat.h"
#include "avcodec.h"
#include "pixfmt.h"
using namespace std;

int main (int argc, char* argv[]) {
    //    The bouncer application should take a single command line argument.
    if ( argc != 2 ) {
        cout << "Invalid Argument Amount" << endl;
        cout << "usage: bouncer filename.jpg" << endl;
    }
    
    //This argument specifies the name of the background image file.  We will guarantee that the input file is always a .jpg file, and your application should reject non-jpg files (by simply testing the extension).
    string filename = argv[1];
    if (filename.substr(filename.find_last_of(".") + 1) != "jpg") {
        cout << "Invalid File Extension" << endl;
    }
    
    //    When drawn on the background image, the sphere(s) should occupy a 'reasonable' percentage of the image space (perhaps 5-20%), and should clearly bounce at a 'reasonable' rate.  The goal should simply be to allow the user to still see most of the background image, but also see detail in the sphere shading.
    
    
}


extern "C" {
    
    
    AVFrame* OpenImage(const char* imageFileName)
    {
        AVFormatContext *pFormatCtx;
        
        if(av_open_input_file(&pFormatCtx, imageFileName, NULL, 0, NULL)!=0)
        {
            printf("Can't open image file '%s'\n", imageFileName);
            return NULL;
        }
        
        dump_format(pFormatCtx, 0, imageFileName, false);
        
        AVCodecContext *pCodecCtx;
        
        pCodecCtx = pFormatCtx->streams[0]->codec;
        pCodecCtx->width = W_VIDEO;
        pCodecCtx->height = H_VIDEO;
        pCodecCtx->pix_fmt = PIX_FMT_YUV420P;
        
        // Find the decoder for the video stream
        AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
        if (!pCodec)
        {
            printf("Codec not found\n");
            return NULL;
        }
        
        // Open codec
        if(avcodec_open(pCodecCtx, pCodec)<0)
        {
            printf("Could not open codec\n");
            return NULL;
        }
        
        //
        AVFrame *pFrame;
        
        pFrame = avcodec_alloc_frame();
        
        if (!pFrame)
        {
            printf("Can't allocate memory for AVFrame\n");
            return NULL;
        }
        
        int frameFinished;
        int numBytes;
        
        // Determine required buffer size and allocate buffer
        numBytes = avpicture_get_size(PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height);
        uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
        
        avpicture_fill((AVPicture *) pFrame, buffer, PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height);
        
        // Read frame
        
        AVPacket packet;
        
        int framesNumber = 0;
        while (av_read_frame(pFormatCtx, &packet) >= 0)
        {
            if(packet.stream_index != 0)
                continue;
            
            int ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            if (ret > 0)
            {
                printf("Frame is decoded, size %d\n", ret);
                pFrame->quality = 4;
                return pFrame;
            }
            else
                printf("Error [%d] while decoding frame: %s\n", ret, strerror(AVERROR(ret)));
        }
    }
};
