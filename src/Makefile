#LIB_PATH = /home/11411/tools/ffmpeg-4.0/x86_build/static_lib
LIB_PATH=/home/11411/tools/ffmpeg-4.4/cv22_bin/static_lib
INC_PATH = /home/11411/tools/ffmpeg-4.0/x86_build/include
#AVS3_LIB = /home/11411/tools/ffmpeg-4.0/third_lib/avs3/lib
 CROSS_COMPILE=aarch64-linux-gnu

ffdemux:
	$(CROSS_COMPILE)-g++  -std=c++11 -I$(INC_PATH)   main.cpp   \
		$(LIB_PATH)/libavformat.a  \
		$(LIB_PATH)/libavcodec.a  \
		$(LIB_PATH)/libavfilter.a  \
		$(LIB_PATH)/libavutil.a   \
		$(LIB_PATH)/libswresample.a 	\
		   -lrt -lm     -lpthread  -o main

dec_cplus:
	$(CROSS_COMPILE)-g++  -std=c++11 -I$(INC_PATH)   main.cpp   \
		$(LIB_PATH)/libavformat.a  \
		$(LIB_PATH)/libavcodec.a  \
		$(LIB_PATH)/libavfilter.a  \
		$(LIB_PATH)/libavutil.a   \
		$(LIB_PATH)/libswresample.a 	\
		-L$(AVS3_LIB)	\
		   -lrt -lm -lz    -lpthread -lSvtAvs3Enc  -ldavs3  -o main



dec_c:
	gcc   -I$(INC_PATH)   main_c.c  \
		$(LIB_PATH)/libavformat.a  \
		$(LIB_PATH)/libavcodec.a  \
		$(LIB_PATH)/libavfilter.a  \
		$(LIB_PATH)/libavutil.a   \
		$(LIB_PATH)/libswresample.a 	\
		-L$(AVS3_LIB)	\
		   -lrt -lm -lz    -lpthread -lSvtAvs3Enc  -ldavs3  -o main_c



	
