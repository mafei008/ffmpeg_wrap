#ifndef  __AVFRAME_WRAP_HPP
#define  __AVFRAME_WRAP_HPP



#include "imediadata.hpp"

extern "C"
{
#include "libavutil/pixfmt.h"
#include "libavutil/frame.h"
//#include "libavutil/buffer_internal.h"
}

#include <string>

class  AVFrameWrap : public  IMediaData
{
public:
	AVFrameWrap(AVFrame*  frame) :IMediaData(frame)
	{
	}

	~AVFrameWrap()
	{
		
		AVFrame  *frame = static_cast<AVFrame*>(m_inner_data);
		av_frame_free(&frame);
		m_inner_data = nullptr;
	}

	void SetPixFmt(enum AVPixelFormat  fmt)
	{
		m_pix_fmt = fmt;
	}

	void SetTimestamp(std::string  &time)
	{
		m_timestamp = time;
	}


	enum AVPixelFormat GetPixFmt()
	{
		return m_pix_fmt;
	}

	std::string  GetTimestamp()
	{
		return m_timestamp;
	}



private:
	enum AVPixelFormat m_pix_fmt;

	std::string  m_timestamp;
};


#endif // 
