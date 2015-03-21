//
//  bouncer.cpp
//
//
//  Created by Christopher Hartley on 3/21/15.
//
//

#include "bouncer.h"
#include <iostream>
#include <string>

using namespace std;

int main (int argc, char* argv[]) {
    // The bouncer application should take a single command line argument.
    if ( argc != 2 ) {
        cout << "Invalid Argument Amount" << endl;
        cout << "usage: bouncer filename.jpg" << endl;
    }
    
    // This argument specifies the name of the background image file.  We will guarantee that the input file is always a .jpg file, and your application should reject non-jpg file
    // (by simply testing the extension).
    string filename = argv[1];
    if (filename.substr(filename.find_last_of(".") + 1) != "jpg") {
        cout << "Invalid File Extension" << endl;
    }

    OpenImage(filename);
    
    return 0;
}


extern "C" {
#include <stdio.h>
#include "libavutil/frame.h"
#include "libavutil/pixfmt.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

    AVFrame* OpenImage(const char* imageFileName)
    {
      av_register_all();
        AVFormatContext *pFormatCtx;
        
	// open the JPG file.
        if(av_open_input_file(&pFormatCtx, filename, NULL, 0, NULL) != 0)
        {
            printf("Can't open image file '%s'\n", filename);
            return NULL;
        }
        
	// dump file header info to std error
        av_dump_format(pFormatCtx, 0, imageFileName, false);
        
        AVCodecContext *pCodecCtx;
        
        pCodecCtx = pFormatCtx->streams[0]->codec;
        pCodecCtx->width = W_VIDEO;
        pCodecCtx->height = H_VIDEO;
        pCodecCtx->pix_fmt = AV_PIX_FMT_RGB8;
        
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
        int n_bytes;
        
        // determine required buffer size and allocate buffer
        n_bytes = avpicture_get_size(AV_PIX_FMT_RGB8, pCodecCtx->width, pCodecCtx->height);
        uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
        
        avpicture_fill((AVPicture *) pFrame, buffer, AV_PIX_FMT_RGB8, pCodecCtx->width, pCodecCtx->height);
        
        // read the frame
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
