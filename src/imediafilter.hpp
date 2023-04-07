#ifndef  __IMEDIAFILTER_HPP__
#define  __IMEDIAFILTER_HPP__


#include "imediadata.hpp"
#include "xerror.h"
#include <memory>	//智能指针相关

///
/// @brief		
/// @details	IMediaFilter采用观察者模式来处理数据，将订阅者和发布者合在一个类中实现
///
class IMediaFilter
{
public:
	///
	/// @brief 添加下一级Filter
	///
	virtual ErrorCode   AddSinkFilter(IMediaFilter  *filter) = 0;


	///
	/// @brief 删除下一级Filter
	///
	virtual void   RemoveSinkFilter(IMediaFilter  *filter) = 0;

	///
	/// @brief 将数据发送给下一级Filter，类似于发布者发布消息
	///
	virtual  void Notify(std::shared_ptr<IMediaData>  data) = 0;


	///
	/// @brief Filter接收到上级Filter传来的数据后， 在该函数内对数据进行处理
	///		   类似于订阅者接受数据
	///
	virtual   void  OnDataArrived(std::shared_ptr<IMediaData>  data) = 0;


	/// 
	/// @brief		获取该MediaFilter输入端口可以接收的数据类型
	/// @dwarning	对于Source Filter， 是不需要接收数据的， 因此该接口会返回 无效值
	virtual MediaDataType  MediaType() = 0;

};





#endif

