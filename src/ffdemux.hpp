#ifndef  __FFDEMUX_HPP__
#define  __FFDEMUX_HPP__


/*	头文件包含区    */
extern "C"
{
#include "libavformat/avformat.h"               //只用到封装器相关头文件
#include "libavutil/dict.h"

#include <stdio.h>
//#include <stdlib.h>
//#include <fcntl.h>
}

#include "json.hpp"
#include "xlog.hpp"
#include "media_filter.hpp"
#include "avpacket_wrap.hpp"
#include "xthread.hpp"
#include "xerror.h"

#include <functional>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>




using json_t = nlohmann::json;

/*	ffmpeg IO回调函数模板	*/
using IOCallBack = std::function <int(uint8_t* data,  int len)>;

#if 0
class  FFDemuxMediaFilter : public IMediaFilter
{
public:

	

private:
	std::vector<IMediaFilter*> m_video_sink_filters;
	std::vector<IMediaFilter*> m_audio_sink_filters;
	std::mutex  m_filter_mutex;
};
#endif

//
/// @brief  解析码流类，从码流中提取完整的一帧数据
///
class  FFDemux : public  MediaFilter 
{
public:
	FFDemux()
	{

	//	av_log_set_level(AV_LOG_TRACE);
	//	av_register_all();
	//	avformat_network_init();
	}


	virtual  ~FFDemux()
	{
		m_io_input = nullptr;

		ReleaseResource();
	}




	
	/// 
	/// @brief		配置demux的各种策略
	/// @details	目前支持以下类型：
    ///				rtsp_transport : tcp/udp，  rtsp拉流使用TCP/UDP传输 
	/// @return
	/// 
    int SetConfig(json_t&  config)
    {
        //必须要open之前设置好Config
        if (m_opened == true)
            return -1;

        for (auto &element : config.items())
        {

            if(element.key() ==  "rtsp_transport")
            {
                 if (element.value() == "tcp")
                 {
                     av_dict_set(&m_options, "rtsp_transport", "tcp", 0);  //默认是UDP拉流， 这里指定TCP就是TCP拉流
                   
                 }
                 else if (element.value() == "udp")
                 {
                     av_dict_set(&m_options, "rtsp_transport", "udp", 0); 
                  
                 }
                 else
                 {
                    // XLOGD("");
                 }
            }
			//某些类型的码流， 可能ffmpeg无法探测出其类型，在调用avformat_open_input()时
			//必须提供AVInputFormat参数， 这样ffmpeg才能探测出类型
			else if(element.key() == "AVInputFormat")	
			{
				m_input_fmt = element.value();
			}

        }

		return 0;
    }

	/// 
	/// @brief	初始化各种资源
	/// @param  url[in]   码流地址
    ///
    /// @return  
	///
	ErrorCode  Open(const char*  url)
	{
		int ret = -1;
		ErrorCode retcode;

		if (m_opened)	//防止重复打开
			return ErrorCode::ERR_OK;

		if (!url )
		{
			XLOGD("%s: error!! url and m_input IO all NULL\n", __FUNCTION__);
			return ErrorCode::ERR_DEMUX_INVALID_PARAM;
		}

		std::lock_guard<std::mutex> lock(m_open_mutex);
		retcode = Init(url);    //Init内部会去ReleaseResource()
		if (retcode != ErrorCode::ERR_OK)
		{
			XLOGD("%s: Open demux failed\n", __FUNCTION__);
			return retcode;
		}

        m_opened = true;
		return ErrorCode::ERR_OK;
	}


	///
	/// @brief   设置用户自定义的ffmpeg IO函数.
	/// @details 例如， 对于ffmpeg从内存中读取数据， 就需要设置自定义 Input函数
	///	@return:  0， 表示设置成功； < 0, 表示设置失败。
	///
	ErrorCode  Open(IOCallBack  cb)
	{
		int ret = -1;
		ErrorCode  retcode;

		if (m_opened)	//防止重复打开
			return ErrorCode::ERR_OK;

		if ( m_io_input != nullptr)
		{
			XLOGD("%s: error!! m_io_input not  NULL\n", __FUNCTION__);
			return ErrorCode::ERR_DEMUX_OTHER_ERROR;
		}
		m_io_input = cb;

		std::lock_guard<std::mutex> lock(m_open_mutex);
		ret = AllocIOResource();	//如果失败，内部回去调用ReleaseResource()
		if (ret < 0)
		{
			XLOGD("%s:%s: AllocIOResource failed\n", __FILE__, __FUNCTION__);
			goto fail;
		}
		
		//注意，这里不能传""， 而是要传空指针
		retcode = Init(m_custom_io_context);    //Init()如果失败， 内部会去ReleaseResource()
		if (retcode !=  ErrorCode::ERR_OK)
		{
			XLOGD("%s: Open demux failed\n", __FUNCTION__);
			goto fail;
		}

		m_opened = true;
		return ErrorCode::ERR_OK;

	fail:
		m_io_input = nullptr;
		return ErrorCode::ERR_DEMUX_OTHER_ERROR;
	}

    /// 
    /// @brief  开启线程，开始读取数据
    /// @param  create_thread   demux内部是否要创建独立线程
    int  Start(bool  create_thread = true)
    {
        int ret;

        if (!create_thread)
            return 0;

        //  ReleaseResource()资源的释放比较混乱， 有的是在函数内部，例如Init()， 有的是在函数外部， 考虑统一
        ret = StartThread();
        if (ret != 0)
        {
            ReleaseResource();
            return -1;
        }

         return 0;

    }

    /// @brief 关闭线程， 清理各种资源
	int Close()
	{

        std::lock_guard<std::mutex> lock(m_open_mutex);

        if (!m_opened)
            return 0;

        //这里加锁是为了防止在多个线程中都去调用Stop()
      //  std::lock_guard<std::mutex> lock(m_thread_lock);

      //先停止内部线程
        StopThread();
       
		ReleaseResource();
        m_opened = false;

		return 0;
	}


	///
	/// @brief   设置用户自定义的ffmpeg IO函数.
	/// @details 例如， 对于ffmpeg从内存中读取数据， 就需要设置自定义 Input函数
	///	@warning 该设置必须在调用 Open()之前完成
	///	@return:  0， 表示设置成功； < 0, 表示设置失败， 一般发生在 FFDemux已经启动的情况下。
	///
	//template <typename  Fn>
	//int   SetIOCallback(Fn &&fn)
	//{

 //       std::lock_guard<std::mutex> lock(m_open_mutex);

 //       //必须要open之前设置好自定义IO
	//	if (m_opened == true)
	//		return -1;

 //       m_io_input = std::move(fn);

	//	return 0;
	//}



	

	///
	/// @brief  读取整帧编码数据
	///
	virtual ErrorCode   GetPacket(std::shared_ptr<IMediaData>&   media_data  )
	{
		int ret = 0;

		if (!m_opened)
		{
			XLOGD("%s: %s:FFDemux is not opened\n",__FILE__,  __FUNCTION__);
			return ErrorCode::ERR_DEMUX_NOTREADY;
		}

		AVPacket*  pkt = av_packet_alloc();   //TBD:  这里的 pkt改为成员变量????
		if (pkt == NULL)
		{
			XLOGD("%s: %s: FFDemux::read():  av_packet_alloc failed\n", __FILE__, __FUNCTION__);
			return ErrorCode::ERR_DEMUX_NOMEM;
		}


		ret = av_read_frame(m_pav_fmt_ctx, pkt);
		if (ret != 0)
		{
            av_packet_unref(pkt);

            if (ret != AVERROR_EOF)
            {
                char buf[128];
                av_strerror(ret, buf, sizeof(buf));
                //如果是从MEM中读取数据， 那么这里会不停打印: End of file。AVERROR_EOF
                XLOGD("FFDemux::Read fail: ret = %d, %s\n", ret, buf);
                return ErrorCode::ERR_DEMUX_READFRAME_FAIL;
            }

            //返回 EOF
            return ErrorCode::ERR_DEMUX_FILE_END;
		
		}
	//	XLOGD("pkt->len = %d\n", pkt->size);
       
        media_data = std::make_shared<AVPacketWrap>(pkt);

		// d.data = reinterpret_cast<uint8_t*>(pkt);   //这里强制转换不能使用static_cast<unsigned char*>，编译会报错，static_cast不支持不相关类型的指针转换， 因为这样有危险，所以会报错
		//  d.size = pkt->size;

		if (pkt->stream_index == m_video_stream_idx)
		{
            media_data->SetDataType(MediaDataType::MEDIATYPE_VIDEO_ENC);
		}
		else if (pkt->stream_index == m_audio_stream_idx)
		{
            media_data->SetDataType(MediaDataType::MEDIATYPE_AUDIO_ENC);
		}
		else
		{
			av_packet_unref(pkt);
			return ErrorCode::ERR_DEMUX_DATA_INDEX_ERR;
		}


        return ErrorCode::ERR_OK;
	}



	AVFormatContext*  GetDemuxHandler()
	{
		return m_pav_fmt_ctx;
	}




	

	///
	/// @brief		实现父类IMediaFilter的接口
	/// @details	FFDemux属于Source Filter， 因此Update()接口为空实现
	///
	virtual   void  OnDataArrived(std::shared_ptr<IMediaData>  data)
	{
		return;
	}


	/// 
	/// @details  FFDemux是Source Filter， 不需要接受输入数据， 因此该实现返回无效值
	/// 
	virtual MediaDataType MediaType() override
	{
		return MediaDataType::MEDIATYPE_ALL;
	}


	

private:
	///
	/// @brief FFMPEG 自定义IO函数
	/// @details 在该函数内， 会去调用 用户传入的读入数据方法
	///
	static int io_read_cb(void*  opaque, uint8_t *data, int len)
	{
	//	static FILE* fp = nullptr;

		int ret = 0;
		if (data == nullptr || len <= 0)
		{
			return 0;
		}

		FFDemux*  demux = static_cast<FFDemux*>(opaque);
		ret = demux->m_io_input(data, len);
		
		return (ret < 0 ? 0 : ret);
	}



    int StartThread()
    {
       
        m_demux_thread = new DemuxThread(*this);
        if (!m_demux_thread)
        {
            return -1;
        }
        m_demux_thread->StartThread();
        

        return 0;
    }


    ///// 
    ///// @brief  停止FFDemux
    ///// 
    int  StopThread()
    {
   
    	//先停止内部线程
    	if (m_demux_thread)
    	{
    		m_demux_thread->StopThread();
    		delete m_demux_thread;
    		m_demux_thread = nullptr;
    	}
    
    	return 0;
    }


	int  AllocIOResource()
	{
		int ret = -1;

		do
		{
			m_custom_io_buffer = (uint8_t*)av_mallocz(m_custom_io_buffersize);
			if (!m_custom_io_buffer)
			{
				XLOGD("%s: malloc m_customIOBuffer failed\n", __func__);
				ret = -1;
				break;
			}

			m_custom_io_context = avio_alloc_context(
				m_custom_io_buffer,
				m_custom_io_buffersize,
				0,
				(void*)this,
				io_read_cb,
				nullptr,
				nullptr);

			if (!m_custom_io_context)
			{
				XLOGD("%s: avio_alloc_context failed\n", __func__);
				ret = -1;
				break;
			}

		//	m_pav_fmt_ctx->pb = m_custom_io_context;
		//	m_pav_fmt_ctx->flags = AVFMT_FLAG_CUSTOM_IO;

			ret = 0;
		} while (0);

		if (ret < 0)
		{
			ReleaseResource();
			return -1;
		}

		return ret;
	}

	///
	/// @brief  释放ffmpeg内部各种资源
	///
	void ReleaseResource()
	{
		av_dict_free(&m_options);

		//1. 放在最前面
		//2. 无论m_pAvFmtCtx是否为空， 都可以调用该函数进行释放
		avformat_close_input(&m_pav_fmt_ctx);
		m_pav_fmt_ctx = nullptr;


		// @warning the internal buffer could have changed, and be != avio_ctx_buffer */
		//
		//注意： 在初始化m_custom_io_context时， 其内部的buffer指向的就是m_custom_io_buffer;
		//但是在内部运行过程中， m_custom_io_context其内部的buffer有可能发生了变化， 不再是
		//外界传入的m_custom_io_buffer了。 但是在改变的过程中， 函数内部应该已经将外部传入的
		//m_custom_io_buffer个释放掉了， 因此这里千万不要再次去手动释放m_custom_io_buffer，
		//而是要去释放m_customIOContext->buffer。
		if (m_custom_io_context)
		{
			av_freep(&m_custom_io_context->buffer);
		}

		//该函数内部不会去释放  m_custom_io_context->buffer
		//这里也不需要去判断 m_custom_io_context是否为空。 
		avio_context_free(&m_custom_io_context);
		m_custom_io_context = nullptr;


		//不能有这一步
		/*if (m_customIOBuffer)
		{
		av_freep(m_customIOBuffer);
		}*/
	}

	/**
	 * @brief 用户传入码流地址时使用该函数
	 */ 
	ErrorCode  Init(const char*  url)
	{
		ErrorCode  code;
		int ret = -1;
		do
		{
			m_pav_fmt_ctx = avformat_alloc_context();
			if (!m_pav_fmt_ctx)
			{
				ret = -1;
				code = ErrorCode::ERR_DEMUX_NOMEM;
				XLOGD("FFDemux avformat_alloc_context failed\n");
				break;
			}

			AVInputFormat *infmt = nullptr;
			if(!m_input_fmt.empty())
			{
				infmt = av_find_input_format(m_input_fmt.c_str());
			}
			XLOGD("%s:%s:: AVInputFormat ptr is %p\n",__FILE__,  __FUNCTION__,  infmt);
			ret = avformat_open_input(&m_pav_fmt_ctx, url, infmt, &m_options);
			if (ret != 0)
			{
				code = ErrorCode::ERR_DEMUX_OTHER_ERROR;
				char buf[128];
				av_strerror(ret, buf, sizeof(buf));
				XLOGD("%s:%s:avformat_open_input() faild: %s\n", __FILE__, __FUNCTION__, buf);

				break;
			}

			/*
			！！！！！！下面这句话一定要加上，不然有可能探测不出视频的长宽等信息！！！！！
			*/
            ret = avformat_find_stream_info(m_pav_fmt_ctx, nullptr);
            if (ret < 0) {
				code = ErrorCode::ERR_DEMUX_OTHER_ERROR;
                XLOGD("avformat_find_stream_info failed!\n");
                break;
            }
			/*
			下面这两句话不是必须的， 但是为了防止代码逻辑出错， 还是加上。
			*/
			m_video_stream_idx = av_find_best_stream(m_pav_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
			if (m_video_stream_idx < 0)
			{
				XLOGD("av_find_best_stream[VIDEO] failed!\n");
			}
			else
			{
				//m_pAvFmtCtx->streams[m_VideoStream]->discard = AVDISCARD_NONINTRA;
			}

			m_audio_stream_idx = av_find_best_stream(m_pav_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);
			if (m_audio_stream_idx < 0) {
			//	XLOGD("av_find_best_stream[AUDIO] failed!\n");
			}
	

			if ((m_video_stream_idx < 0) && (m_audio_stream_idx < 0))
			{
				code = ErrorCode::ERR_DEMUX_OTHER_ERROR;
				ret = -1;
				break;
			}
			ret = 0;

		} while (0);

		if (ret == 0)
			return ErrorCode::ERR_OK;

		ReleaseResource();
		return code;
	}

	/**
	 *	@brief  用户自定义AVIOContext时使用该函数
	 */
	ErrorCode  Init(AVIOContext  *ioctx)
	{
		ErrorCode  code;
		int ret = -1;
		do
		{
			m_pav_fmt_ctx = avformat_alloc_context();
			if (!m_pav_fmt_ctx)
			{
				ret = -1;
				code = ErrorCode::ERR_DEMUX_NOMEM;
				XLOGD("FFDemux avformat_alloc_context failed\n");
				break;
			}

			m_pav_fmt_ctx->pb = ioctx;
			m_pav_fmt_ctx->flags = AVFMT_FLAG_CUSTOM_IO;

			AVInputFormat *infmt = nullptr;
			if(!m_input_fmt.empty())
			{
				infmt = av_find_input_format(m_input_fmt.c_str());
			}
			XLOGD("%s:%s:: AVInputFormat ptr is %p\n",__FILE__,  __FUNCTION__,  infmt);
			ret = avformat_open_input(&m_pav_fmt_ctx, nullptr, infmt, &m_options);
			if (ret != 0)
			{
				code = ErrorCode::ERR_DEMUX_OTHER_ERROR;
				char buf[128];
				av_strerror(ret, buf, sizeof(buf));
				XLOGD("%s:%s:avformat_open_input() faild: %s\n", __FILE__, __FUNCTION__, buf);

				break;
			}

			/*
			！！！！！！下面这句话一定要加上，不然有可能探测不出视频的长宽等信息！！！！！
			*/
            ret = avformat_find_stream_info(m_pav_fmt_ctx, nullptr);
            if (ret < 0) {
				code = ErrorCode::ERR_DEMUX_OTHER_ERROR;
                XLOGD("avformat_find_stream_info failed!\n");
                break;
            }
			/*
			下面这两句话不是必须的， 但是为了防止代码逻辑出错， 还是加上。
			*/
			m_video_stream_idx = av_find_best_stream(m_pav_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
			if (m_video_stream_idx < 0)
			{
				XLOGD("av_find_best_stream[VIDEO] failed!\n");
			}
			else
			{
				//m_pAvFmtCtx->streams[m_VideoStream]->discard = AVDISCARD_NONINTRA;
			}

			m_audio_stream_idx = av_find_best_stream(m_pav_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);
			if (m_audio_stream_idx < 0) {
			//	XLOGD("av_find_best_stream[AUDIO] failed!\n");
			}
	

			if ((m_video_stream_idx < 0) && (m_audio_stream_idx < 0))
			{
				code = ErrorCode::ERR_DEMUX_OTHER_ERROR;
				ret = -1;
				break;
			}
			ret = 0;

		} while (0);

		if (ret == 0)
			return ErrorCode::ERR_OK;

		ReleaseResource();
		return code;
	}
	

protected:
	///
	/// @brief 定义嵌套类
	/// 
	class  DemuxThread : public XThread
	{

	public:
		DemuxThread(FFDemux  &demux):m_demux(demux)
		{

		}


		virtual void MainLoop() override
		{
			while (!m_IsExit)
			{

                std::shared_ptr<IMediaData>  data;
                ErrorCode  err = m_demux.GetPacket(data);
				if ( err != ErrorCode::ERR_OK)
				{
					xsleep(10);
					continue;
				}
				m_demux.Notify(data);
			
			//	std::this_thread::sleep_for(std::chrono::milliseconds(20));	//睡眠40ms
                xsleep(20);
			}
		}


		FFDemux&  m_demux;

	};

	DemuxThread*  m_demux_thread = nullptr;
//	std::mutex  m_thread_lock;

private:
  //  std::atomic_bool  m_start;	//对应 Start()和Stop()的控制
	bool  m_opened = false;	//对应Open()和Close()的控制
    std::mutex  m_open_mutex;   //在开启/关闭时防止竞争

	AVFormatContext  *m_pav_fmt_ctx = nullptr;

	AVDictionary* m_options = nullptr;	//保存FFmpeg 

	/* 用户自定义IO */
	AVIOContext  *m_custom_io_context = nullptr;
	uint8_t*  m_custom_io_buffer = nullptr;
	size_t   m_custom_io_buffersize = 0x200000;



//	using IOCallBack = std::function <void(int arg)>;
	//typedef  std::function <void(int arg)>  CallBack;
	IOCallBack  m_io_input = nullptr;


	int m_video_stream_idx = -1;
	int m_audio_stream_idx = -1;

	std::string m_input_fmt; //AVInputFormat


// private:
// 	std::vector<IMediaFilter*> m_video_sink_filters;
// 	std::vector<IMediaFilter*> m_audio_sink_filters;
// 	std::mutex  m_filter_mutex;


//	IMediaFilter  *m_filter = nullptr;	//在构造函数中由外部提供
};


#endif
