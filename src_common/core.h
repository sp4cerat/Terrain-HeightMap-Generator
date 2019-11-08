#pragma once
/*------------------------------------------------------*/
#define WIN32_DEFAULT_LIBS
#define _USE_MATH_DEFINES
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"winmm.lib")
#include <windows.h>
#include <mmsystem.h>
#include <malloc.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <time.h>
#include <math.h>
#include <vector>
#include "mathlib/vector.h"
#include "mathlib/matrix.h"
/*------------------------------------------------------*/
#define ushort unsigned short
#define uint unsigned int
#define uchar unsigned char
/*------------------------------------------------------*/
#define loop0i(end_l) for ( int i=0;i<end_l;++i )
#define loop0j(end_l) for ( int j=0;j<end_l;++j )
#define loopi(start_l,end_l) for ( int i=start_l;i<end_l;++i )
#define loopj(start_l,end_l) for ( int j=start_l;j<end_l;++j )
#define loopk(start_l,end_l) for ( int k=start_l;k<end_l;++k )
#define loopl(start_l,end_l) for ( int l=start_l;l<end_l;++l )
#define loopm(start_l,end_l) for ( int m=start_l;m<end_l;++m )
#define loopn(start_l,end_l) for ( int n=start_l;n<end_l;++n )
#define loop(a_l,start_l,end_l) for ( int a_l = start_l;a_l<end_l;++a_l )
/*------------------------------------------------------*/
#define loopij(_sti,_stj,_eni,_enj) loopi(_sti,_eni)loopj (_stj,_enj)
#define loopijk(_sti,_stj,_stk,_eni,_enj,_enk) loopi(_sti,_eni) loopj (_stj,_enj) loopk (_stk,_enk)
#define looplmn(_stl,_stm,_stn,_enl,_enm,_enn) loopl(_stl,_enl) loopm (_stm,_enm) loopn (_stn,_enn)
/*------------------------------------------------------*/
#define foreach(var, container) for( auto var = (container).begin(); var != (container).end(); ++var)
/*------------------------------------------------------*/
#define f_min min
#define f_max max
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#define clamp(a_,b_,c_) min(max(a_,b_),c_)
#define frac(a) (a-floor(a))
#define dot3(a,b) (a.x*b.x+a.y*b.y+a.z*b.z)
#define cross3(a,b) vec3f( a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z , a.x * b.y - a.y * b.x )
//#define lerp(t, a, b) ( a + t * (b - a) )
#define lerp(a_,b_,t_) ( a_*(1-t_) + b_*t_ )
#define vswap(a,b) { auto c=a;a=b;b=c; }
/*------------------------------------------------------*/
#define error_stop(fmt, ...) \
{\
	char text[10000];\
	sprintf(text,("" fmt "\n\nCallstack:\n%s [%s:%d]\n%s"),\
	##__VA_ARGS__,__FUNCTION__,__FILE__,  __LINE__, "");\
	printf(text);\
	MessageBoxA(0,text,"Error",0);exit(0);\
	while(1);;\
};
/*------------------------------------------------------*/
float cubicInterpolate (float p[4], float x) ;
float bicubicInterpolate (float p[4][4], float x, float y);
std::string get_pure_filename ( std::string filename );
std::string get_path ( std::string filename );
std::string int_to_str(const int x);
/*------------------------------------------------------*/
