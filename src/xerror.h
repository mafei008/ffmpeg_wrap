#ifndef  __XERROR_H__
#define  __XERROR_H__

/// 
/// @brief      定义错误码
/// @details    错误码采用  ERR_模块名_错误原因  来定义
///
enum  class   ErrorCode : uint32_t  
{
    ERR_OK      = 0x0,      //成功


	/// ImediaFilter模块
	ERR_MEDIAFILTER_FILTER_PTR_NULL = 0xA0000000,   //MediaFiter 指针为空
    ERR_MEDIAFILTER_FILTER_DUPLICATE,       //重复的 MediaFilter指针      
	ERR_MEDIAFILTER_NOT_SUPPORT,
	//ERR_MEDIAFILTER_DATATYPE_NOTMATCH ,//MediaFilter数据类型不匹配


    /// Demux模块
    ERR_DEMUX_FILE_END  =    0xA0001000,    ///  Demux 读到文件末尾
    ERR_DEMUX_NOMEM,        /// Demux内存申请失败
    ERR_DEMUX_NOTREADY, /// Demux没有初始化
    ERR_DEMUX_READFRAME_FAIL,  /// readframe失败, 不是read_file_end
    ERR_DEMUX_DATA_INDEX_ERR,   /// 读取到的数据索引错误
    ERR_DEMUX_INVALID_PARAM,    /// Demux模块输入参数无效
    ERR_DEMUX_DEMUX_BUSY,   ///  Demux模块忙
    ERR_DEMUX_FILTER_DATATYPE_NOTMATCH,    /// demux和下级filter的数据类型不匹配
//	ERR_DEMUX_FILTER_PTR_NULL,	//添加的filter是空指针
  //  ERR_DEMUX_FILTER_DATATYPE_INVALID,  /// 添加的 filter的 data type是无效的
	ERR_DEMUX_OTHER_ERROR,



    // FFDecode 模块
    ERR_DEC_INVALID_PARAM   =   0xA0002000, //输入参数无效
    ERR_DEC_FILTER_DATATYPE_NOTMATCH,
    ERR_DEC_FIND_CODEC_FAILED,  // //ffmpeg内没有找到解码codec
    ERR_DEC_OTHER_ERROR,
//	ERR_DEC_FILTER_PTR_NULL,	//添加的filter是空指针


    // FFEncoder模块
    ERR_ENC_FIND_CODEC_FAILED = 0xA0003000,   //ffmpeg内没有找到编码codec
    ERR_ENC_NOMEM,      //enc模块申请内存失败
    ERR_ENC_SEND_FRAME_FAILED,    //调用 avcodec_send_frame失败
};




#endif // 
