#include <stdio.h>

#include <iostream>
//#include <time.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
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
        static int64_t tspm = 0;
        int ret = 0;
        // printf("ddd %lld, %lld\n" ,pavpkt->dts, pavpkt->pts);
        // printf("ddd %lld\n", pavpkt->duration);
        // printf("ddd %lld\n", av_gettime_relative());

        ret = avcodec_send_packet(m_video_codec_ctx, pavpkt);
        printf("[send_packet %d]\n", ret);
        ret = avcodec_receive_frame(m_video_codec_ctx, m_video_frame);
        printf("[recv frame%d]\n", ret);

        // AVRational fps = 
        //     av_guess_frame_rate(m_fmt_ctx, m_fmt_ctx->streams[0],m_video_frame);
        //
        //
        // printf("fps = %.2lf\n", (double)fps.num/fps.den);


        int64_t cur = av_gettime_relative();

//        printf(">> %.2f\n", 1000000.0/(cur - tspm));

        tspm = av_gettime_relative();

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

//    FILE * output = fopen("rgbfile","wb+");

    const int w = ctx.getWidth();
    const int h = ctx.getHeight();

    uint64_t ts = 0;

    {
        using namespace std;
        vector<cv::Mat> fq;
        int dim = 2;
        int dims[2] = {h ,w};

        while (ctx.readVideoPkt(&avpkt) >= 0)
        {
            ctx.frameDecode(&avpkt);
            uint8_t * cvt_buf = ctx.getConvertedFormtFrame();

    //        cv::Mat src = cv::Mat(dim, dims, CV_8UC3, cvt_buf);
        //    cv::imshow("x", src);
        //    cv::waitKey(1);
            fq.push_back(cv::Mat(dim, dims, CV_8UC3, cvt_buf).clone());
            av_packet_unref(&avpkt);
        }
       // return;

        for (int i = 0; i < fq.size(); ++i)
        {
            printf("i = %d\n", i);
            ts = av_gettime_relative();
            imshow("opencv", fq[i]);

            fprintf(stderr, "(showimg)dt = %lld\n", (av_gettime_relative() - ts)/1000);

            ts = av_gettime_relative();
            cv::waitKey(1);
            fprintf(stderr, "(wait key)dt = %lld\n", (av_gettime_relative() - ts)/1000);

        }

    }
    return ;

    while (ctx.readVideoPkt(&avpkt) >= 0)
    {
        printf("frame_cnt = %d\n", frame_cnt);
        ++frame_cnt;

        ts = av_gettime_relative();

        ctx.frameDecode(&avpkt);

        fprintf(stderr, "(decode)dt = %lld\n", (av_gettime_relative() - ts)/1000);

        ts = av_gettime_relative();
        uint8_t * cvt_buf = ctx.getConvertedFormtFrame();
        fprintf(stderr, "(convert)dt = %lld\n", (av_gettime_relative() - ts)/1000);

    //    cv::namedWindow("opencv");

//        if (0)
        {
            using namespace cv;

            int dim = 2;
            int dims[2] = {h ,w};
            printf("h, w = %d %d\n", h, w);

            ts = av_gettime_relative();
            Mat src = cv::Mat(dim, dims, CV_8UC3, cvt_buf);
            fprintf(stderr, "(init-mat)dt = %lld\n", (av_gettime_relative() - ts)/1000);
            //cvtColor(src, gray_image, CV_BGR2GRAY );

            ts = av_gettime_relative();
            imshow("opencv", src);

            fprintf(stderr, "(showimg)dt = %lld\n", (av_gettime_relative() - ts)/1000);

            ts = av_gettime_relative();
            waitKey(1);
            fprintf(stderr, "(wait key)dt = %lld\n", (av_gettime_relative() - ts)/1000);
        }
        printf("%d\n", w*h);
        //fwrite(cvt_buf,w*h*3,1,output);
        av_packet_unref(&avpkt);

    }
//    fclose(output);
    ctx.cleanup();
}

int main(int argc, const char * argv[])
{
    testCPP("./test.h264");
    //testCPP("./ooo");
//    testCPP("/tmp/fifo");
    return 0;
}
