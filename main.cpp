#include <stdio.h>

#include <iostream>
//#include <time.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};

#include <string>
#include <assert.h>

//=============================
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <pthread.h>
#include <vector>
#include <unistd.h>

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
//=============================

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

    int getWidth(void) const
    {
        return m_video_codec_ctx->width;
    }

    int getHeight(void) const
    {
        return m_video_codec_ctx->height;
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

        return m_bgr_frame->data[0];
    }

    void cleanup(void)
    {
        avformat_close_input(&m_fmt_ctx);
        avcodec_free_context(&m_video_codec_ctx);

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

void testCPP(const char * file_name)
{
    HyStreamerBase ctx;
    ctx.init(file_name);
   // ctx.init("./test.h264");
    ctx.init_video_stream();

    AVPacket avpkt;

    int frame_cnt = 0;

    FILE * output = fopen("rgbfile","wb+");

    const int w = ctx.getWidth();
    const int h = ctx.getHeight();

    while (ctx.readVideoPkt(&avpkt) >= 0)
    {
        printf("frame_cnt = %d\n", frame_cnt);
        ++frame_cnt;

        ctx.frameDecode(&avpkt);

        uint8_t * cvt_buf = ctx.getConvertedFormtFrame();

        av_packet_unref(&avpkt);

        {
            using namespace cv;

            int dim = 2;
            int dims[2] = {h ,w};
            printf("h, w = %d %d\n", h, w);
            Mat src = cv::Mat(dim, dims, CV_8UC3, cvt_buf);

            imshow("opencv", src);
            waitKey(1);
        }
        printf("%d\n", w*h);
        fwrite(cvt_buf,w*h*3,1,output);
    }
    fclose(output);
    ctx.cleanup();
}

int main(int argc, const char * argv[])
{
    testCPP("./test.h264");
    return 0;
}
