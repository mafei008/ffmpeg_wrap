#ifndef  __MEDIAINFO_HPP__
#define  __MEDIAINFO_HPP__

extern "C"
{
#include "libavformat/avformat.h"    
}

class MediaInfo
{
public:
	MediaInfo(AVFormatContext  *fmtctx):m_pav_fmt_ctx(fmtctx)
	{

	}

	int  GetVideoSize(int& width,  int &height)
	{
		if ((m_video_width <= 0) || (m_video_height <= 0))
		{
			m_video_stream_idx = av_find_best_stream(m_pav_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
			if (m_video_stream_idx < 0)
			{
				XLOGD("%s: av_find_best_stream() failed\n", __FUNCTION__);
				return -1;
			}

			m_video_width = m_pav_fmt_ctx->streams[m_video_stream_idx]->codecpar->width;
			m_video_height = m_pav_fmt_ctx->streams[m_video_stream_idx]->codecpar->height;
		}
		
		width = m_video_width;
		height = m_video_height;

		return 0;
	}

private:

	AVFormatContext  *m_pav_fmt_ctx = nullptr;

	int m_video_width = -1;
	int m_video_height = -1;
	int m_video_stream_idx = -1;
	int m_audio_stream_idx = -1;
};




#endif
