#ifndef  __AVPACKET_WRAP_HPP__
#define  __AVPACKET_WRAP_HPP__



extern "C"
{
	#include "libavcodec/avcodec.h"
}


#include "imediadata.hpp"




class AVPacketWrap : public  IMediaData
{
public:

	AVPacketWrap(AVPacket*  pkt) :IMediaData(pkt)
	{
	}

	virtual ~AVPacketWrap()
	{
		AVPacket*  pkt = static_cast<AVPacket*>(m_inner_data);
		av_packet_free(&pkt);
		m_inner_data = nullptr;
	}

	void SetTimestamp(std::string time)
	{
		m_timestamp = time;
	}

	std::string GetTimestamp()
	{
		return m_timestamp;
	}

private:
	std::string  m_timestamp;

	// void FreeResource()
	//{
	//	AVPacket*  pkt = static_cast<AVPacket*>(m_inner_data);
	//	av_packet_free(&pkt);
	//	m_inner_data = nullptr;
	//}
};



#endif
