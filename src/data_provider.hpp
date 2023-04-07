#ifndef  __DATA_PROVIDER_HPP__
#define  __DATA_PROVIDER_HPP__

#include <stdint.h>

///
/// @brief 数据提供类， 抽象类
///
class DataProvider
{
	virtual  int   GetData(uint8_t*  data,  uint32_t  len) = 0;
};







#endif
