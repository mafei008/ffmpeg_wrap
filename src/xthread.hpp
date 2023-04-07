#ifndef __XTHREAD_HPP__
#define __XTHREAD_HPP__


#include "xlog.hpp"
#include <thread>
#include <atomic>


///
/// @brief The XThread class:  继承该类， 但是如果不调用start()，就不会创建线程
///
class XThread
{
public:

	XThread() :m_IsExit(false), m_IsRunning(false)
	{

	}


    ///
    /// \brief Start  启动线程
    ///
	virtual bool StartThread()
	{
		m_IsExit = false;

		std::thread  th(&XThread::ThreadMain, this);
		th.detach();

		return true;
	}

	virtual  bool  IsThreadExit()
	{
		return  m_IsExit;
	}

    //通过控制isExit安全停止线程(不一定成功)
	virtual  bool  StopThread()
	{
		m_IsExit = true;

		for (int i = 0; i < 200; ++i)
		{
			if (m_IsRunning == false)
			{
				XLOGD("stop thread success, wait cnt = %d!\n", i);
				return  true;
			}

			xsleep(10);
		}

		XLOGD("stop thread failed!\n");
		return false;
	}

    //入口主函数， 子类要重载
	virtual  void  MainLoop() = 0;


public:
	//睡眠单位是ms
	static  void  xsleep(int mis)
	{
		std::chrono::milliseconds   du(mis);
		std::this_thread::sleep_for(du);
	}


protected:
	//子类中要访问该成员， 因此不能设置为private
	//子类中发现该标志置位， 那么线程函数就会退出
	//m_isExit是给外面要与线程通信的模块提供线程状态用的
	std::atomic_bool m_IsExit;

private:
	void ThreadMain()
	{
		m_IsRunning = true;

		this->MainLoop();

		m_IsRunning = false;
	}

	std::atomic_bool m_IsRunning;	//线程内部使用
};

#endif // XTHREAD_H
