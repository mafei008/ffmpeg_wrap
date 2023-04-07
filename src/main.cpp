#define  __STDC_CONSTANT_MACROS


#include <string>
#include "avpacket_wrap.hpp"
#include "ffdemux.hpp"
#include "media_info.hpp"
#include "xthread.hpp"
//#include "ff_video_decoder.hpp"

#include <cstdio>

#if 0
class  DummyYUV : public MediaFilter
{
public:
	int Open(const char*  filepath)
	{
		m_file = fopen(filepath, "wb"); //open(filepath,  O_RDWR | O_TRUNC);
		if(m_file == nullptr)
		{
			printf("fopen file %s failed\n", filepath);
			return -1;
		}

		return 0;
	}
	virtual MediaDataType MediaType() override
    {
        return MediaDataType::MEDIATYPE_VIDEO_ENC;
    }

	virtual void OnDataArrived(std::shared_ptr<IMediaData> data) override
	{
		printf("data arrived\n");

		
        AVFrameWrap  *frame_wrap = static_cast<AVFrameWrap*>(data.get());
        AVFrame  *frame = static_cast<AVFrame*>(frame_wrap->GetInnerData());

        if (!frame)return;

		//printf("frame: data0[0x%x, %u]\n", frame->data[0], frame->linesize[0]);

		if(frame->linesize[0] == frame->width)
		{
			fwrite(frame->data[0], 1, frame->width * frame->height, m_file);
			fwrite(frame->data[1], 1, frame->width * frame->height/4, m_file);
			fwrite(frame->data[2], 1, frame->width * frame->height/4, m_file);
		}
		else
		{
			for (int i = 0; i < frame->height; ++i) //Y 
			{
				fwrite(frame->data[0] + i * frame->linesize[0], 1, frame->width, m_file);
			}

			for (int i = 0; i < frame->height/2; ++i) //U
			{
				fwrite(frame->data[1] + i * frame->linesize[1], 1, frame->width/2, m_file);
			}

			for (int i = 0; i < frame->height/2; ++i) //V
			{
				fwrite(frame->data[2] + i * frame->linesize[2], 1, frame->width/2, m_file);
			}
		}

#if 0
        //容错，保证尺寸正确
        if (!datas_[0] || frame->width * frame->height == 0 || frame->width != this->width_ || frame->height != this->height_)
        {
          //  av_frame_free(&frame);
           // mux.unlock();
            return;
        }


        if (this->width_ == frame->linesize[0]) //无需对齐
        {
            memcpy(datas_[0], frame->data[0], this->width_* this->height_);
            memcpy(datas_[1], frame->data[1], this->width_* this->height_ / 4);
            memcpy(datas_[2], frame->data[2], this->width_* this->height_ / 4);

		//	int ret = av_buffer_get_ref_count(frame->buf[0]);

        }
        else//行对齐问题
        {
            for (int i = 0; i < this->height_; i++) //Y 
            {
                memcpy(datas_[0] + this->width_*i, frame->data[0] + frame->linesize[0] * i, this->width_ / 2);
            }

            for (int i = 0; i < this->height_ / 2; i++) //U
            {
                memcpy(datas_[1] + this->width_ / 2 * i, frame->data[1] + frame->linesize[1] * i, this->width_ / 2);
            }

            for (int i = 0; i < this->height_ / 2; i++) //V
            {
                memcpy(datas_[2] + this->width_ / 2 * i, frame->data[2] + frame->linesize[2] * i, this->width_ / 2);
            }

        }
	#endif
	}


private:

	FILE*  m_file;
};

int   user_read_data_cb(uint8_t*  data, int len)
{
	int ret;
	static FILE* fp = nullptr;

	if(fp == nullptr )
	{
		fp = fopen("test_2688x1520.avs3", "rw");
	}
	ret = fread(data, 1, len, fp);

	return ret;
}
#endif





int main()
{
	//相机的RTSP地址
	//const char*  m_urlpath = "rtsp://admin:admin123@192.168.3.95:554/cam/realmonitor?channel=1&subtype=0";
	const char*  m_urlpath = "./test_2688x1520.avs3";
	//const char*  m_urlpath = "./000.ts";
	FFDemux  *m_demuxer = nullptr;
//	FFVideoDecoder  *m_videodecoder = nullptr;
//	DummyYUV  *m_dummy =  nullptr;




#if 0
	m_demuxer = new FFDemux();
	m_videodecoder =  new FFVideoDecoder();
	m_dummy = new DummyYUV();
	m_dummy->Open("test_1280x720.yuv");

	m_demuxer->AddSinkFilter(m_videodecoder);
	m_videodecoder->AddSinkFilter(m_dummy);

	
	IOCallBack  cb = user_read_data_cb;
	ErrorCode ret = m_demuxer->Open(cb);
//	ErrorCode ret = m_demuxer->Open(m_urlpath);
	if(ret!=ErrorCode::ERR_OK)
	{
		printf("open rtsp failed\n");
		return -1;
	}
	//printf("m_demuxer->Open()  OK\n");
	AVFormatContext *avfmt_ctx = m_demuxer->GetDemuxHandler();
    ret = m_videodecoder->Open(avfmt_ctx);
//	ret = m_videodecoder->Open("libdavs3");
   	if(ret != ErrorCode::ERR_OK)
	{
		XLOGD("open libdavs failed: ret = 0x%x\n", static_cast<uint32_t>(ret));
		return -1;
	}
	//printf("m_demuxer->Open()  OK\n");

	m_demuxer->Start(true);
	m_videodecoder->Start(true);

	while(1)
	{
		XThread::xsleep(10000);
	}
#endif
#if 1
	m_demuxer = new FFDemux();	
	ErrorCode ret = m_demuxer->Open(m_urlpath);
	if(ret!=ErrorCode::ERR_OK)
	{
		printf("open rtsp failed\n");
		return -1;
	}
	while(true)
	{
		//printf("1111\n");	
		std::shared_ptr<IMediaData>  data;
		//获取完整的一帧数据
        ErrorCode  err = m_demuxer->GetPacket(data);
		if ( err != ErrorCode::ERR_OK)
		{
			printf("Get data failed\n");
			XThread::xsleep(10);
			continue;
		}


		AVPacketWrap*  pkt_wrap = static_cast<AVPacketWrap*>(data.get());
		AVPacket*  pkt = static_cast<AVPacket*>(pkt_wrap->GetInnerData());

		//pkt->data 是该帧编码数据的指针，  pkt->size是该帧编码数据的长度
		printf("pkt->data = 0x%x, pkt->size = %d\n", pkt->data, pkt->size);
	}
#endif	

#if 0
	m_demuxer = new FFDemux();	
	ErrorCode ret = m_demuxer->Open(m_urlpath);
	if(ret!=ErrorCode::ERR_OK)
	{
		printf("open rtsp failed\n");
		return -1;
	}

	m_videodecoder =  new FFVideoDecoder();
	ret = m_videodecoder->Open("libdavs3");
   	if(ret != ErrorCode::ERR_OK)
	{
		XLOGD("open libdavs failed: ret = 0x%x\n", static_cast<uint32_t>(ret));
		return -1;
	}

	while(true)
	{
		//printf("1111\n");	
		std::shared_ptr<IMediaData>  data;
		//获取完整的一帧书
        ErrorCode  err = m_demuxer->GetPacket(data);
		if ( err != ErrorCode::ERR_OK)
		{
			printf("Get data failed\n");
			XThread::xsleep(10);
			continue;
		}


		AVPacketWrap*  pkt_wrap = static_cast<AVPacketWrap*>(data.get());
		AVPacket*  pkt = static_cast<AVPacket*>(pkt_wrap->GetInnerData());

		//pkt->data 是该帧编码数据的指针，  pkt->size是该帧编码数据的长度
		printf("pkt->data = 0x%x, pkt->size = %d\n", pkt->data, pkt->size);

		if(!m_videodecoder->SendPacket(data))
		{
			XLOGD("send packet failed\n");
			continue;
		}

		std::shared_ptr<IMediaData> frame = m_videodecoder->RecvFrame();
		if (!frame)
		{
			continue;
		}
		XLOGD("get YUV data!\n");
	}
#endif
	
	return 0;
	  
}
