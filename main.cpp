#include <stdio.h>

extern "C"
{
#include <avcodec.h>
#include <avformat.h>
#include <avutil.h>
#include <imgutils.h>
#include <swscale.h>
};

void test(void)
{
    av_register_all();

    AVFormatContext * fmt_ctx = NULL;
    AVCodecContext * video_codec_ctx = NULL;
    AVCodec * video_codec = NULL;

    //const char * src_file_name = "./test.h264";
    const char * src_file_name = "./xxx";
//    const char * src_file_name = "/tmp/fifo";
    //need to close #1
    if (avformat_open_input(&fmt_ctx, src_file_name, NULL, NULL) < 0)
    {
        fprintf(stderr, "error-avformat_open_input\n");
        exit(1);
    }

    ////av_dump_format(fmt_ctx, 0, "./test.h264", 0);

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
    {
        fprintf(stderr, "error-avformat_find_stream_info\n");
        exit(1);
    }

    ///###################

    {
        printf("fmt_ctx:\n");
        printf("nb_streams %d\n", fmt_ctx->nb_streams);

        //can test NULL to codec
        //#2
        video_codec_ctx = avcodec_alloc_context3(NULL);

        avcodec_parameters_to_context(video_codec_ctx, fmt_ctx->streams[0]->codecpar);

        printf("stream width %d\n", video_codec_ctx->width);
        printf("stream height %d\n", video_codec_ctx->height);
        printf("video type no. : %d\n", AVMEDIA_TYPE_VIDEO);
        printf("stream type %d\n", video_codec_ctx->codec_type);
        printf("codec-id = %1d, (h264 = %d)\n", video_codec_ctx->codec_id, AV_CODEC_ID_H264);

        video_codec = avcodec_find_decoder(video_codec_ctx->codec_id);
        if (!video_codec)
        {
            fprintf(stderr, "error-avcodec_find_decoder\n");
            exit(1);
        }

        //
        if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0)
        {
            fprintf(stderr, "error-avcodec_open2\n");
            exit(1);
        }


        AVPacket avpkt;
        av_init_packet(&avpkt);

        AVFrame * video_frame = av_frame_alloc();
        if (!video_frame)
        {
            fprintf(stderr, "error-av_frame_alloc\n");
            exit(1);
        }


        AVFrame * bgr_frame = av_frame_alloc();

        if (!bgr_frame)
        {
            fprintf(stderr, "error-av_frame_alloc\n");
            exit(1);
        }

        {
            enum AVPixelFormat pix_fmt = AV_PIX_FMT_BGR24;

            int buf_size = av_image_alloc(bgr_frame->data, bgr_frame->linesize,
                       video_codec_ctx->width, video_codec_ctx->height, pix_fmt, 1);

            printf("convert buffer size = %d\n", buf_size);
        }


        //OK

//###########################3

        struct SwsContext * img_convert_ctx = NULL;

        int frame_cnt = 0;
        FILE *output=fopen("out.rgb","wb+");
        while (1)
        {
            printf("frame cnt = %d\n", frame_cnt);
            ++frame_cnt ;

            if (av_read_frame(fmt_ctx, &avpkt) < 0)
            {
                break;
            }


            int ret = 0;

            ret = avcodec_send_packet(video_codec_ctx, &avpkt);
            printf("[send_packet %d]\n", ret);
            ret = avcodec_receive_frame(video_codec_ctx, video_frame);
            printf("[recv frame%d]\n", ret);

            {

                if (!output)
                {
                    fprintf(stderr, "error open file\n");
                    exit(0);
                }

                const int w = video_codec_ctx->width;
                const int h = video_codec_ctx->height;

                SwsContext * img_convert_ctx
                    = sws_getContext(
                            w, h, video_codec_ctx->pix_fmt,
                            w, h, AV_PIX_FMT_BGR24,
                            SWS_BICUBIC, NULL, NULL, NULL);

                sws_scale(img_convert_ctx,
                        video_frame->data, video_frame->linesize, 0, h,
                        bgr_frame->data, bgr_frame->linesize);


                sws_freeContext(img_convert_ctx);
                fwrite(bgr_frame->data[0],w*h*3,1,output);
            }

            av_packet_unref(&avpkt);
        }
        fclose(output);

        av_freep(&bgr_frame->data[0]);
        av_frame_free(&bgr_frame);
        av_frame_free(&video_frame);

    }

    ///###################

    //#1
    avformat_close_input(&fmt_ctx);
    //#2
    avcodec_free_context(&video_codec_ctx);

  //  av_free(picture);

}

int main(int argc, const char * argv[])
{
    test();
    return 0;
}
