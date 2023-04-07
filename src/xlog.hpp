#ifndef __XLOG_HPP__
#define __XLOG_HPP__

#include <stdio.h>

//#include <QDebug>


#if 1
#define  XLOGD(...)  do {   \
                        printf(__VA_ARGS__);   \
                      }while(0)
#endif


//void  XLOGD(const char* format, ...);


#endif // __XLOG_H__
