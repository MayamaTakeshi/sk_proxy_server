# COMSPEC is defined only on Windows (but this is for Cygwin).
# The app will compile in cygwin, but it is exiting with code 129 (it seems the exit is being done by some thread from SK).
# curiously, if we run the app under gdb and stop it for a while, after we command 'continue' it will not exit.

SK_INCLUDE=-I /opt/switchkit/8.3/include 

BRS_STREAM_UTILS_INCLUDE=-I ../brs_stream_utils
BRS_STREAM_UTILS_LIBS=-L ../brs_stream_utils -lbrs_stream_utils

MINIJSON_INCLUDE=-I ../minijson
MINIJSON_LIBS=-L ../minijson -lminijson 

SWITCHKITJSON_INCLUDE=-I ../switchkitjson 
SWITCHKITJSON_LIBS=-L ../switchkitjson -lswitchkitjson 

ifdef COMSPEC 
        SK_LIBS = -L /opt/switchkit/8.3/lib -l SKApi
else 
       SK_LIBS = -L /opt/switchkit/8.3/lib -l skapi -l skcapi -l ACE 
endif

all: sk_proxy_server

sk_proxy_server: brs_stream_utils minijson switchkitjson sk_proxy_server.c
	gcc -g sk_proxy_server.c ${SK_INCLUDE} ${BRS_STREAM_UTILS_INCLUDE} ${MINIJSON_INCLUDE} ${SWITCHKITJSON_INCLUDE} ${SWITCHKITJSON_LIBS} ${SK_LIBS} ${BRS_STREAM_UTILS_LIBS} ${MINIJSON_LIBS} -o sk_proxy_server

brs_stream_utils:
	make -C ../brs_stream_utils

minijson:
	make -C ../minijson

switchkitjson:
	make -C ../switchkitjson

#switchkitjson_test: static_lib switchkitjson_test.c
#        gcc -g sk_proxy_server.c ${SK_INCLUDE} ${MINIJSON_INCLUDE} -L. -lswitchkitjson ${SK_LIBS} ${MINIJSON_LIBS} -lm -o switchkitjson_test

clean:
	rm -f *.a *.o sk_proxy_server
