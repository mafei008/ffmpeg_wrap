#ifndef    __MEDIA_FILTER_HPP__
#define    __MEDIA_FILTER_HPP__


#include "imediafilter.hpp"
#include "xlog.hpp"
#include <mutex>
#include <list>
#include <vector>
#include <set>

#if 1
class MediaFilter : public IMediaFilter
{

public:
	virtual ErrorCode AddSinkFilter(IMediaFilter *filter) override
	{
		if (filter == nullptr)
		{
			XLOGD("%s: %s:: filter is NULLL\n", __FILE__, __FUNCTION__);
			return ErrorCode::ERR_MEDIAFILTER_FILTER_PTR_NULL;
		}

		std::lock_guard<std::mutex> lock(m_filter_mutex);
		if(m_sink_filters.count(filter) != 0)
		{
			XLOGD("%s: %s:: filter duplicate, can not add.\n", __FILE__, __FUNCTION__);
			return ErrorCode::ERR_MEDIAFILTER_FILTER_DUPLICATE;
		}

		m_sink_filters.insert(filter);

		return ErrorCode::ERR_OK;
	}


	virtual void RemoveSinkFilter(IMediaFilter *filter) override
	{
		if (filter == nullptr)
		{
			XLOGD("%s: %s: filter is NULLL\n", __FILE__, __FUNCTION__);
			return ;
		}

		if(m_sink_filters.count(filter) == 0)
		{
			XLOGD("%s: %s: has no filter to remove.\n", __FILE__, __FUNCTION__);
			return ;
		}

		std::lock_guard<std::mutex> lock(m_filter_mutex);
		m_sink_filters.erase(filter);

		return;
	}


	virtual void Notify(std::shared_ptr<IMediaData> data) override
	{
		//throw std::logic_error("The method or operation is not implemented.");
		std::lock_guard<std::mutex> lock(m_filter_mutex);

		// for (int i = 0; i < m_sink_filters.size(); ++i)	//集合不支持该种方法
		// {
		// 	m_sink_filters[i].OnDataArrived(data);
		// }

			for(const auto& elem : m_sink_filters)	//这是所有容器公共的访问元素方法
			{
				elem->OnDataArrived(data);
			}
	}


	virtual void OnDataArrived(std::shared_ptr<IMediaData> data) = 0;


	// virtual MediaDataType MediaType() override
	// {
	// 	return   MediaDataType::MEDIATYPE_ALL;
	// }

private:
//	std::vector<IMediaFilter*> m_sink_filters;
	std::set<IMediaFilter*> m_sink_filters;
	std::mutex  m_filter_mutex;
};
#endif



#if 0
class  MediaFilterOutPort
{
public:
	virtual ErrorCode Connect(MediaFilterInPort *filter) override
	{
		if (filter == nullptr)
		{
			XLOGD("%s: %s:: filter is NULLL\n", __FILE__, __FUNCTION__);
			return ErrorCode::ERR_MEDIAFILTER_FILTER_PTR_NULL;
		}

		std::lock_guard<std::mutex> lock(m_filter_mutex);
		m_sink_filters.push_back(filter);

		return ErrorCode::ERR_OK;
	}


	virtual void Disconnect(MediaFilterInPort *filter) override
	{
		if (filter == nullptr)
		{
			XLOGD("%s: %s: filter is NULLL\n", __FILE__, __FUNCTION__);
			return;
		}

		return;
	}


	virtual void Notify(std::shared_ptr<IMediaData> data) override
	{
		//throw std::logic_error("The method or operation is not implemented.");
		std::lock_guard<std::mutex> lock(m_filter_mutex);

		for (int i = 0; i < m_sink_filters.size(); ++i)
		{
			m_sink_filters[i]->OnDataArrived(data);
		}
	}


	/// 
	/// @brief		��ȡ��MediaFilter����˿ڿ��Խ��յ���������
	/// @dwarning	����Source Filter�� �ǲ���Ҫ�������ݵģ� ��˸ýӿڻ᷵�� ��Чֵ
	virtual MediaDataType  MediaType() = 0;

private:
	std::vector<MediaFilterInPort*> m_sink_filters;
	std::mutex  m_filter_mutex;

};


class  MediaFilterInPort
{
	///
	/// @brief Filter���յ��ϼ�Filter���������ݺ� �ڸú����ڶ����ݽ��д���
	///		   �����ڶ����߽�������
	///
	virtual   void  OnDataArrived(std::shared_ptr<IMediaData>  data) = 0;



	/// 
	/// @brief		��ȡ��MediaFilter����˿ڿ��Խ��յ���������
	/// @dwarning	����Source Filter�� �ǲ���Ҫ�������ݵģ� ��˸ýӿڻ᷵�� ��Чֵ
	virtual MediaDataType  MediaType() = 0;
};
#endif









#endif

