#include <stdio.h>

extern "C"
{
#include <avcodec.h>
#include <avformat.h>
#include <avutil.h>
#include <imgutils.h>
#include <swscale.h>
};

#include <string>
#include <assert.h>

class HyStreamerBase
{

protected:
    AVFormatContext * m_fmt_ctx;
    std::string m_url;

    //to be subclass
    AVCodecContext * m_video_codec_ctx;
    AVCodec * m_video_codec;
    AVFrame * m_video_frame;
    AVFrame * m_bgr_frame;
public:
    HyStreamerBase(void);

    int init(const std::string & url);

    int init_video_stream(void);

    int readVideoPkt(AVPacket * pavpkt);

    int frameDecode(AVPacket * pavpkt)
    {
        int ret = 0;
        ret = avcodec_send_packet(m_video_codec_ctx, pavpkt);
        printf("[send_packet %d]\n", ret);
        ret = avcodec_receive_frame(m_video_codec_ctx, m_video_frame);
        printf("[recv frame%d]\n", ret);

        return ret;
    }

    uint8_t * getConvertedFormtFrame(void)
    {
        enum AVPixelFormat pix_fmt = AV_PIX_FMT_BGR24;

        const int w = m_video_codec_ctx->width;
        const int h = m_video_codec_ctx->height;

        struct SwsContext * img_convert_ctx
            = sws_getContext(
                    w, h, m_video_codec_ctx->pix_fmt,
                    w, h, pix_fmt,
                    SWS_BICUBIC, NULL, NULL, NULL);

        sws_scale(img_convert_ctx,
                m_video_frame->data, m_video_frame->linesize, 0, h,
                m_bgr_frame->data, m_bgr_frame->linesize);


        sws_freeContext(img_convert_ctx);

        return 0;
    }

};

HyStreamerBase::HyStreamerBase(void):
    m_fmt_ctx(NULL),
    m_url(),

    m_video_codec_ctx(NULL),
    m_video_codec(NULL),
    m_video_frame(NULL),
    m_bgr_frame(NULL)
{

}

#if 0
av_freep(&bgr_frame->data[0]);
av_frame_free(&bgr_frame);
av_frame_free(&video_frame);
avformat_close_input(&fmt_ctx);
avcodec_free_context(&video_codec_ctx);
#endif

int HyStreamerBase::init(const std::string & url)
{
    av_register_all();

    int ret = 0;
    ret = avformat_open_input(&m_fmt_ctx, url.c_str(), NULL, NULL);
    if (ret < 0)
    {
        goto fail;
    }

    ret = avformat_find_stream_info(m_fmt_ctx, NULL);

    if (ret < 0)
    {
        avformat_close_input(&m_fmt_ctx);
        goto fail;
    }


    return 0;

fail:
    fprintf(stderr, ">%s\n", av_err2str(ret));
    return -1;

}

int HyStreamerBase::init_video_stream(void)
{
    printf("fmt_ctx:\n");
    printf("nb_streams %d\n", m_fmt_ctx->nb_streams);

    //can test NULL to codec
    //#2
    m_video_codec_ctx = avcodec_alloc_context3(NULL);

    avcodec_parameters_to_context(m_video_codec_ctx
            , m_fmt_ctx->streams[0]->codecpar);

    printf("stream width %d\n", m_video_codec_ctx->width);
    printf("stream height %d\n", m_video_codec_ctx->height);
    printf("video type no. : %d\n", AVMEDIA_TYPE_VIDEO);
    printf("stream type %d\n", m_video_codec_ctx->codec_type);
    printf("codec-id = %1d, (h264 = %d)\n", m_video_codec_ctx->codec_id, AV_CODEC_ID_H264);

    m_video_codec = avcodec_find_decoder(m_video_codec_ctx->codec_id);
    if (!m_video_codec)
    {
        fprintf(stderr, "error-avcodec_find_decoder\n");
        exit(1);
    }

    if (avcodec_open2(m_video_codec_ctx, m_video_codec, NULL) < 0)
    {
        fprintf(stderr, "error-avcodec_open2\n");
        exit(1);
    }


    m_video_frame = av_frame_alloc();
    if (!m_video_frame)
    {
        fprintf(stderr, "error-av_frame_alloc\n");
        exit(1);
    }


    m_bgr_frame = av_frame_alloc();

    if (!m_bgr_frame)
    {
        fprintf(stderr, "error-av_frame_alloc\n");
        exit(1);
    }

    {
        enum AVPixelFormat pix_fmt = AV_PIX_FMT_BGR24;

        int buf_size = av_image_alloc(
                m_bgr_frame->data,
                m_bgr_frame->linesize,
                m_video_codec_ctx->width, m_video_codec_ctx->height,
                 pix_fmt, 1);

        printf("convert buffer size = %d\n", buf_size);
    }

    return 0;
}

int HyStreamerBase::readVideoPkt(AVPacket * pavpkt)
{
    assert(pavpkt);

    av_init_packet(pavpkt);
    int ret = av_read_frame(m_fmt_ctx, pavpkt);

    if (ret < 0)
    {
        av_freep(&m_bgr_frame->data[0]);
        av_frame_free(&m_bgr_frame);
        av_frame_free(&m_video_frame);
    }

    //AVERROR_EOF
    return ret;
}

void testCPP(void)
{
    HyStreamerBase ctx;
    ctx.init("./xxx");
    ctx.init_video_stream();

    AVPacket avpkt;

    int frame_cnt = 0;
    while (ctx.readVideoPkt(&avpkt) >= 0)
    {
        printf("frame_cnt = %d\n", frame_cnt);
        ++frame_cnt;


        ctx.frameDecode(&avpkt);

        ctx.getConvertedFormtFrame();

        av_packet_unref(&avpkt);
    }

}

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

        avcodec_parameters_to_context(video_codec_ctx
                , fmt_ctx->streams[0]->codecpar);

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

            int buf_size = av_image_alloc(
                    bgr_frame->data, bgr_frame->linesize,
                       video_codec_ctx->width,
                       video_codec_ctx->height, pix_fmt, 1);

            printf("convert buffer size = %d\n", buf_size);
        }


        //OK

//###########################3


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

                struct SwsContext * img_convert_ctx
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
   // test();
    testCPP();
    return 0;

    test();
    return 0;
}
