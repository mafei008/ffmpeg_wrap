#ifndef __IMEDIADATA_HPP__
#define __IMEDIADATA_HPP__


enum  class   MediaDataType
{
	

	MEDIATYPE_VIDEO_ENC = 1,	//Filter可接收视频编码数据
	MEDIATYPE_VIDEO_YUV420P ,	//Filter可接收YUV数据
	MEDIATYPE_AUDIO_ENC,		//Filter可接收音频编码数据

	MEDIATYPE_ALL,			//Filter可接收各种类型数据

	MEDIATYPE_INVALID

};

class IMediaData
{

public:

	//TBD: 构造函数中不能调用纯虚函数FreeResource() ???
	IMediaData(void*  ptr) :m_inner_data(ptr)
	{
	
	}

	//析构函数必须要有函数体,不能是纯虚构函数
	//TBD: 析构函数中不能调用纯虚函数FreeResource() ???
	virtual ~IMediaData()
	{
	}



	void*  GetInnerData()
	{
		return m_inner_data;
	}


	bool  IsValid()
	{
		return (m_inner_data != nullptr) ? true : false;
	}


	MediaDataType  GetDataType()
	{
		return m_type;
	}

	void SetDataType(MediaDataType  type)
	{
		m_type = type;
	}

	
	
//protected:
//	///
//	///	@brief		释放资源
//	/// @details	定义该函数的目的在于，提供一个纯虚函数， 强制继承类实现该接口
//	///				如果只依靠析构函数来释放资源，那么继承类有可能忘记override 析构函数
//	///
//	virtual void   FreeResource() = 0;

protected:
	void*  m_inner_data;  //AVPacket、AVFrame.....

	MediaDataType  m_type;

};

#endif // __IMEDIADATA_HPP__
