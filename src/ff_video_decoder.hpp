#ifndef  __FFVIDEO_DECODER_HPP__
#define  __FFVIDEO_DECODER_HPP__


#include "media_filter.hpp"
#include "avpacket_wrap.hpp"
#include "avframe_wrap.hpp"
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>
//#include <opencv2\opencv.hpp>


/*	头文件包含区    */
extern "C"
{
#include "libavcodec/avcodec.h"               //只用到封装器相关头文件
}




class  FFVideoDecoder	: public  MediaFilter
{
public:
	/**
	 * @brief  根据AVFormatContext中的信息来创建解码器
	 * 		   AVFormatContext中已经包含了编解码信息
	 * 
	 */
	ErrorCode  Open(AVFormatContext  *pavformt_ctx)
	{
		
        std::unique_lock<std::mutex>  lck(m_open_mutex);

        if (m_opened)
            return ErrorCode::ERR_OK;

        if (Init(pavformt_ctx) != 0)
        {
            return ErrorCode::ERR_DEC_OTHER_ERROR;
        }
        m_opened = true;

        return ErrorCode::ERR_OK;
	}




	/**
	 * @brief  根据解码器的名字来创建解码器
	 * 	 
	 */
	ErrorCode  Open(const char*  decoder_name)
	{
		
        std::unique_lock<std::mutex>  lck(m_open_mutex);

        if (m_opened)
            return ErrorCode::ERR_OK;

		AVCodec *cd = avcodec_find_decoder_by_name(decoder_name);
		if( cd == nullptr)
		{
			XLOGD("%s: %s: find avcodec_find_decoder_by_name failed\n", __FILE__, __FUNCTION__);
			return ErrorCode::ERR_DEC_FIND_CODEC_FAILED;
		}	
		XLOGD("codec id = 0x%x\n", cd->id);

		if(Init(cd) !=0)
		{
			return ErrorCode::ERR_DEC_OTHER_ERROR; 
		}

        m_opened = true;
        return ErrorCode::ERR_OK;
	}
	

	/**
	 * 	@brief  开始工作
	 *  @param  create_thread   解码器内部是否创建独立线程。
	 * 			true， 表示创建独立线程，该线程内部会去自动解码， 
	 * 			false， 不创建独立线程， 此时需要用户自己去调用解码函数做解码 	
	 */
    int Start(bool create_thread = true)
    {

        if (!create_thread)
            return 0;

        if (StartThread() != 0)
        {
            ReleaseResource();
            return -1;
        }

        return 0;
    }

	int Close()
	{
        std::unique_lock<std::mutex>  lck(m_open_mutex);
        if (!m_opened)
            return 0;

        StopThread();
        ReleaseResource();

        m_opened = true;

		return 0;
	}

	

	//future模型 发送数据到线程解码
	virtual bool   SendPacket(std::shared_ptr<IMediaData>  data)
	{
		if (!m_codec_ctx)
		{
			return false;
		}

		AVPacketWrap*  pkt_wrap = static_cast<AVPacketWrap*>(data.get());
		AVPacket*  pkt = static_cast<AVPacket*>(pkt_wrap->GetInnerData());
		
	//	XLOGD("size :%d, 0x%x\n", pkt->size, pkt->data);

	
		int ret = avcodec_send_packet(m_codec_ctx, pkt);   //如果是单线程，那么实际的解码在这个函数里面， 而不是在receive里面
		if (ret != 0)
		{
			/*
			* #define AVERROR_INVALIDDATA        FFERRTAG( 'I','N','D','A') ///< Invalid data found when processing input
			* 如果送进去的
			*/
			char buf[128];
			av_strerror(ret, buf, sizeof(buf));
			uint8_t  nalType = pkt->data[4] & 0x1F;
			XLOGD("avcodec_send_packet faild[error num = 0x%x, pkt_len = %d, nalType = %d]: %s，\n", ret, pkt->size, nalType, buf);
			return false;
		}
	
		std::string timestamp = pkt_wrap->GetTimestamp();
		//如果携带的时间戳是非空字符串，那么就说明用户需要展示时间戳
		//无论该值是否合理， 都需要展示
		std::unique_lock<std::mutex>  lck(m_timestamp_mutex);
		if (!timestamp.empty())	
		{
			m_timestamp_list.push_back(timestamp);
		}

		return true;
	}



	//从解码线程中获取解码结果  再次调用会复用上次空间，线程不安全
	virtual std::shared_ptr<IMediaData>  RecvFrame()
	{
		if (!m_codec_ctx)
		{
			return nullptr;
		}
		

		AVFrame  *frame = av_frame_alloc();
		std::shared_ptr<AVFrameWrap>  frameWrap = std::make_shared<AVFrameWrap>(frame);
	
		int ret = avcodec_receive_frame(m_codec_ctx, frame);
		if (ret != 0)
		{
		//	char buf[128];
		//	av_strerror(ret, buf, sizeof(buf));
		//	XLOGD("avcodec_receive_frame faild: %s\n", buf);
			return nullptr;
		}
	
		//软解码出来的格式是 AV_PIX_FMT_YUV420P
		frameWrap->SetPixFmt(AV_PIX_FMT_YUV420P);

		std::unique_lock<std::mutex>  lck(m_timestamp_mutex);
		if (!m_timestamp_list.empty())	//时间戳队列是否为空
		{
			frameWrap->SetTimestamp(m_timestamp_list.front());
			m_timestamp_list.pop_front();
		}

#if 0
		{
			int width = frame->width, height = frame->height;
			cv::Mat tmp_img = cv::Mat::zeros(height * 3 / 2, width, CV_8UC1);
			memcpy(tmp_img.data, frame->data[0], width*height);
			memcpy(tmp_img.data + width*height, frame->data[1], width*height / 4);
			memcpy(tmp_img.data + width*height * 5 / 4, frame->data[2], width*height / 4);
		//	cv::Mat bgr;
		//	cv::cvtColor(tmp_img, bgr, cv::CV_YUV2BGR_I420);
		}
#endif
		return frameWrap;

	}




	///
	/// @brief		实现父类IAVFilter的接口
	/// @details	接受上级Fillter传下来的数据
	///				该函数不是在decoder的内部线程中执行， 而是在上级filter的线程中执行
	///
	virtual   void  OnDataArrived(std::shared_ptr<IMediaData>  data)
	{
		/*	AVPacketWrap*  pkt_wrap = static_cast<AVPacketWrap*>(data.get());
		AVPacket*  pkt = static_cast<AVPacket*>(pkt_wrap->GetInnerData());
		printf("data = %p, len = %d\n", pkt->data, pkt->size);
		*/

		while (!m_decoder_thread->IsThreadExit())//这里不是循环
		{
			bool needWake = false;

			std::unique_lock<std::mutex>  lck(m_pkt_mutex);
			if (m_pkt_list.empty())
			{
				needWake = true;
			}

			//if (m_PackList.size() >= this->maxList)
			//{
			//	//生产者队列满了， 睡眠10ms
			//	XLOGD("%s: packList is full\n", __func__);
			//	xsleep(10);
			//	continue;
			//}

			m_pkt_list.push_back(data);
			if (needWake)
			{
				m_pkt_condwait.notify_one();
			}

			break;
		}

		return;
	}


	virtual MediaDataType MediaType() override
	{
		return MediaDataType::MEDIATYPE_VIDEO_ENC;
	}

private:
	std::vector<IMediaFilter*> m_sink_filters;
	std::mutex  m_filter_mutex;


	
protected:
	///
	/// @brief 定义嵌套类
	/// 
	class  FFDecodeThread : public XThread	//嵌套类可以直接访问外围内的私有成员
	{

	public:
		FFDecodeThread(FFVideoDecoder  &decoder) :m_decoder(decoder)
		{

		}

		virtual void MainLoop() override
		{

			while (!IsThreadExit())
			{
				std::shared_ptr<IMediaData> data = nullptr;
				//这里加{}的目的是为了减少m_pkt_mutex锁住的范围
				{
					std::unique_lock<std::mutex>  lck(m_decoder.m_pkt_mutex);
					while (m_decoder.m_pkt_list.empty())
					{
						m_decoder.m_pkt_condwait.wait(lck);
					}

					//取出packet 消费者
					data = m_decoder.m_pkt_list.front();
					m_decoder.m_pkt_list.pop_front();
				}

				//发送数据到解码线程，一个数据包，可能解码多个结果
				if (m_decoder.SendPacket(data))
				{
					while (!IsThreadExit())
					{
						//获取解码后的数据

						std::shared_ptr<IMediaData> frame = m_decoder.RecvFrame();
						if (!frame)
						{
							break;
						}
						//发送数据给观察者
	
						m_decoder.Notify(frame);
						//	break;
					}

				}
			}
		}

		FFVideoDecoder&  m_decoder;
	};

	FFDecodeThread*  m_decoder_thread = nullptr;



private:

    int  Init(AVFormatContext  *pavformt_ctx)
    {
        int ret = 0;
        if (!pavformt_ctx)
        {
            return -1;
        }

        int video_stream_idx = av_find_best_stream(pavformt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
        if (video_stream_idx < 0)
        {
            XLOGD("%s: av_find_best_stream() failed\n", __FUNCTION__);
            return -1;
        }

        AVCodecParameters*  pcodec_param = pavformt_ctx->streams[video_stream_idx]->codecpar;
        if (!pcodec_param)
        {
            XLOGD("%s: AVCodecParameters is NULL\n", __FUNCTION__);
            return -1;
        }

        do
        {
            //1 查找解码器
		//	pcodec_param->codec_id = AV_CODEC_ID_CAVS;
            AVCodec *cd = avcodec_find_decoder(pcodec_param->codec_id);
            if (!cd)
            {
                ret = -1;
                XLOGD("avcodec_find_decoder %d failed!\n", pcodec_param->codec_id);
                break;
            }
			printf("codec_id = 0x%x\n", pcodec_param->codec_id);


            //2 创建解码上下文，并复制参数
            m_codec_ctx = avcodec_alloc_context3(cd);
            if (!m_codec_ctx)
            {
                ret = -1;
                XLOGD("%s: avcodec_alloc_context3 failed\n", __FUNCTION__);
                break;
            }

            /*
            * 下面这段代码其实也可以不要。 将数据送给解码器后，解码器内部自然会填充
            */
            if ((ret = avcodec_parameters_to_context(m_codec_ctx, pcodec_param)) < 0)
            {
                char buf[256] = { 0 };
                av_strerror(ret, buf, sizeof(buf) - 1);
				ret = -1;
                XLOGD("avcodec_parameters_to_context failed:%s\n", buf);
                break;
            }


            m_codec_ctx->thread_count = 1;
            m_codec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;  //对H264解码貌似无用


                                                            //设置加速解码
                                                            //  m_CodecCtx->lowres = cd->max_lowres;
                                                            //  m_CodecCtx->flags2 |= AV_CODEC_FLAG2_FAST;
            AVDictionary* options = NULL;
            ////优化解码延时
            //if (p->codec_id == AV_CODEC_ID_H264)
            //{
            //	//这里的值是优化编码速度用的，而不是解码速度
            //	//   av_dict_set(&options, "preset", "superfast", 0);
            //	//   av_dict_set(&options, "tune", "zerolatency", 0);
            //}
            //else if (p->codec_id == AV_CODEC_ID_H265)
            //{
            //	//   av_dict_set(&options, "x265-params", "qp=20", 0);
            //	//   av_dict_set(&options, "preset", "ultrafast", 0);
            //	//   av_dict_set(&options, "tune", "zero-latency", 0);
            //}


            //3 打开解码器
            ret = avcodec_open2(m_codec_ctx, NULL, &options);
            if (ret != 0)
            {
                av_dict_free(&options);
                char buf[256] = { 0 };
                av_strerror(ret, buf, sizeof(buf) - 1);
                XLOGD("%s\n", buf);
                break;
            }

            av_dict_free(&options);

            //AVCodecParameters中并没有pix_fmt字段， 并且， avcodec_open2也
            //不会给pix_fmt赋值， 因此， 需要自己手动赋值
            m_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;


			/*for (int i = 0; ; ++i)
			{
				auto config = avcodec_get_hw_config(cd, i);
				if (!config)
					break;
				if (config->device_type)
				{
					XLOGD("hw device = %s\n", av_hwdevice_get_type_name(config->device_type));
				}
			}*/

        } while (0);


        if (ret == 0)
            return 0;

    fail:
        ReleaseResource();
		return -1;
    }


	int  Init(AVCodec *cd)
    {
        int ret = -1;

        if (!cd)
        {
            return -1;
        }


        do
        {
            //2 创建解码上下文，并复制参数
            m_codec_ctx = avcodec_alloc_context3(cd);
            if (!m_codec_ctx)
            {
                ret = -1;
                XLOGD("%s: avcodec_alloc_context3 failed\n", __FUNCTION__);
                break;
            }

            m_codec_ctx->thread_count = 1;
            m_codec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;  //对H264解码貌似无用

            AVDictionary* options = NULL;
        
            //3 打开解码器
            ret = avcodec_open2(m_codec_ctx, NULL, &options);
            if (ret != 0)
            {
                av_dict_free(&options);
                char buf[256] = { 0 };
                av_strerror(ret, buf, sizeof(buf) - 1);
                XLOGD("%s\n", buf);
                break;
            }
	
            av_dict_free(&options);
            //AVCodecParameters中并没有pix_fmt字段， 并且， avcodec_open2也
            //不会给pix_fmt赋值， 因此， 需要自己手动赋值
            m_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;


			/*for (int i = 0; ; ++i)
			{
				auto config = avcodec_get_hw_config(cd, i);
				if (!config)
					break;
				if (config->device_type)
				{
					XLOGD("hw device = %s\n", av_hwdevice_get_type_name(config->device_type));
				}
			}*/

        } while (0);

        if (ret == 0)
            return 0;

    fail:
        ReleaseResource();
		return -1;
    }



	void ReleaseResource()
	{
		if (m_codec_ctx)
		{
			//avcodec_close的使用场景： 调用avcodec_alloc_context3申请了内存，
			//但是还没有调用avcodec_open2时， 可以用avcodec_close来释放内存。
			//avcodec_free_context的使用场景： 无论什么情况下， 
			//都可以调用avcodec_free_context。 因此，ffmpeg中avcodec_close()
			//头文件的注释中也写明了， 不建议使用avcodec_close， 都用avcodec_free_context
			avcodec_free_context(&m_codec_ctx);
			m_codec_ctx = nullptr;
		}
	}


    int StartThread()
    {

        m_decoder_thread = new FFDecodeThread(*this);
        if (!m_decoder_thread)
            return -1;

        m_decoder_thread->StartThread();
        return 0;
    }


    int StopThread()
    {
        if (m_decoder_thread)
        {
            if (!m_decoder_thread->StopThread())
                return -1;

            delete m_decoder_thread;
            m_decoder_thread = nullptr;
        }

        return 0;
    }


private:

//	int  parse_timestamp = 1;

    bool  m_opened = false;
    std::mutex  m_open_mutex;

	AVCodecContext *m_codec_ctx = nullptr;

	std::list<std::shared_ptr<IMediaData>>  m_pkt_list;
	std::mutex  m_pkt_mutex;
	std::condition_variable  m_pkt_condwait;

	//std::vector<IMediaFilter*> m_sink_filters;
	//std::mutex  m_filter_mutex;
	//IMediaFilter*  m_filter = nullptr;


	std::list<std::string>  m_timestamp_list;
	std::mutex  m_timestamp_mutex;

};

#endif
