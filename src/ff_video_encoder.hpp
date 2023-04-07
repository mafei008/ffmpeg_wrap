#ifndef  __FFVIDEO_ENCODER_HPP__
#define  __FFVIDEO_ENCODER_HPP__

#include "media_filter.hpp"
#include "avpacket_wrap.hpp"
#include "avframe_wrap.hpp"
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>


/*	C头文件包含区    */
extern "C"
{
#include "libavcodec/avcodec.h"               //只用到封装器相关头文件
}


class    FFVideoEncoder	: public  MediaFilter
{
public:
    /**
	 * @brief  根据编码器的名字来创建解码器
	 * 	 
	 */
	ErrorCode  Open(const char*  codec_name)
    {
        AVCodec *codec =  nullptr;
        codec = avcodec_find_encoder_by_name(codec_name);
		if (!m_codec) 
        {
			XLOGD("%s：%s :: Codec '%s' not found\n", __FILE__, __FUNCTION__,  codec_name);
			return ErrorCode::ERR_ENC_FIND_CODEC_FAILED;
		}
		printf("codec_id = %d\n", codec->id);

		m_codec_ctx = avcodec_alloc_context3(codec);
		if (!m_codec_ctx) 
        {
			XLOGD("%s:%s :: Could not allocate video codec context\n", __FILE__, __FUNCTION__);
			return ErrorCode::ERR_ENC_NOMEM;
		}

        return  ErrorCode::ERR_OK;
    }

    /**
     * @brief 发送一帧YUV数据给编码器编码
     */ 
    ErrorCode  SendFrame(AVFrame  *frame)
    {
        int ret;

        ret = avcodec_send_frame(m_codec_ctx, frame);
        if (ret < 0) 
        {    
           XLOGD("%s:%s :: Error sending a frame for encoding\n", __FILE__, __FUNCTION__);
           return  ErrorCode::ERR_ENC_SEND_FRAME_FAILED;
        }
        return  ErrorCode::ERR_OK;
    }




    /**
     * @brief 从编码器获取编码后的一帧数据
     */ 
    virtual std::shared_ptr<IMediaData>  ReceivePacket()
    {
        int ret;

        AVPacket*  pkt = av_packet_alloc();   //TBD:  这里的 pkt改为成员变量????
		if (pkt == nullptr)
		{
			XLOGD("%s: %s:  av_packet_alloc() failed.\n", __FILE__, __FUNCTION__);
			//return ErrorCode::ERR_ENC_NOMEM;
            return nullptr;
		}

        ret = avcodec_receive_packet(m_codec_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return nullptr;
        }
        else if (ret < 0) 
        {
            XLOGD("%s: %s:  avcodec_receive_packet() failed.\n", __FILE__, __FUNCTION__);
            return nullptr;
        }
    }
    

private:
    AVCodecContext *m_codec_ctx = NULL;
};



#endif