

// The Stack Blur Algorithm was invented by Mario Klingemann, 
// mario@quasimondo.com and described here:
// http://incubator.quasimondo.com/processing/fast_blur_deluxe.php

// This is C++ RGBA (32 bit color) multi-threaded version 
// by Victor Laskin (victor.laskin@gmail.com)
// More details: http://vitiy.info/stackblur-algorithm-multi-threaded-blur-for-cpp

// This code is using MVThread class from my cross-platform framework 
// You can exchange it with any thread implementation you like


// -------------------------------------- stackblur ----------------------------------------->

static float const stackblur_mul[255] =
{
		512,512,456,512,328,456,335,512,405,328,271,456,388,335,292,512,
		454,405,364,328,298,271,496,456,420,388,360,335,312,292,273,512,
		482,454,428,405,383,364,345,328,312,298,284,271,259,496,475,456,
		437,420,404,388,374,360,347,335,323,312,302,292,282,273,265,512,
		497,482,468,454,441,428,417,405,394,383,373,364,354,345,337,328,
		320,312,305,298,291,284,278,271,265,259,507,496,485,475,465,456,
		446,437,428,420,412,404,396,388,381,374,367,360,354,347,341,335,
		329,323,318,312,307,302,297,292,287,282,278,273,269,265,261,512,
		505,497,489,482,475,468,461,454,447,441,435,428,422,417,411,405,
		399,394,389,383,378,373,368,364,359,354,350,345,341,337,332,328,
		324,320,316,312,309,305,301,298,294,291,287,284,281,278,274,271,
		268,265,262,259,257,507,501,496,491,485,480,475,470,465,460,456,
		451,446,442,437,433,428,424,420,416,412,408,404,400,396,392,388,
		385,381,377,374,370,367,363,360,357,354,350,347,344,341,338,335,
		332,329,326,323,320,318,315,312,310,307,304,302,299,297,294,292,
		289,287,285,282,280,278,275,273,271,269,267,265,263,261,259
};

static unsigned char const stackblur_shr[255] =
{
		9, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17,
		17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20,
		20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21,
		21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
		21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
		22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
		22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23,
		23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
		23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
		23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
		23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
		24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
		24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
		24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
		24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24
};

/// Stackblur algorithm body
void stackblurJob(float* src,				///< input image data
	   			  unsigned int w,					///< image width
				  unsigned int h,					///< image height
				  unsigned int radius,				///< blur intensity (should be in 2..254 range)
				  int cores,						///< total number of working threads
				  int core,							///< current thread number
				  int step,							///< step of processing (1,2)
				  float* stack				///< stack buffer
				  )
{
	unsigned int x, y, xp, yp, i;
	unsigned int sp;
	unsigned int stack_start;
	float* stack_ptr;

	float* src_ptr;
	float* dst_ptr;

	float sum_r;
	float sum_in_r;
	float sum_out_r;

	unsigned int wm = w - 1;
	unsigned int hm = h - 1;
	unsigned int  div = (radius * 2) + 1;
	float mul_sum = stackblur_mul[radius];
	unsigned char shr_sum = stackblur_shr[radius];
	
	if (step == 1)
	{
		int minY = core * h / cores;
		int maxY = (core + 1) * h / cores;

		for(y = minY; y < maxY; y++)
		{
			sum_r = 
			sum_in_r = 
			sum_out_r = 0;

			src_ptr = src + w * y; // start of line (0,y)

			for(i = 0; i <= radius; i++)
			{
				float v=src_ptr[(wm-radius+i+1)%w];
				stack_ptr    = &stack[ i ];
				stack_ptr[0] = v;
				sum_r		+= v * (i + 1);
				sum_out_r	+= v;
			}
			for(i = 1; i <= radius; i++)
			{
				if (i <= wm) src_ptr += 1;
				stack_ptr = &stack[ i + radius ];
				stack_ptr[0] = src_ptr[0];
				sum_r		+= src_ptr[0] * (radius + 1 - i);
				sum_in_r	+= src_ptr[0];
			}
			sp = radius;
			xp = radius;
			if (xp > wm) xp = wm;
			
			src_ptr = src + (xp + y * w); //   img.pix_ptr(xp, y);
			dst_ptr = src + y * w; // img.pix_ptr(0, y);

			for(x = 0; x < w+radius; x++)
			{
				if(x==w)dst_ptr-=w;

				if(x<w)
					dst_ptr[0] = (sum_r * mul_sum) / float(1<<shr_sum);
				else
				{
					float a=float(x-w)/float(radius);
					dst_ptr[0] = dst_ptr[0]*(a)+(1-a)*(sum_r * mul_sum) / float(1<<shr_sum);
				}

				dst_ptr += 1;

				sum_r -= sum_out_r;

				stack_start = sp + div - radius;
				if (stack_start >= div) stack_start -= div;
				stack_ptr = &stack[ stack_start];

				sum_out_r -= stack_ptr[0];

				if(xp < wm)
				{
					src_ptr += 1;
					++xp;
				}
				if(xp == wm)
				{
					src_ptr -= wm;
					xp=0;
				}

				stack_ptr[0] = src_ptr[0];

				sum_in_r += src_ptr[0];
				sum_r    += sum_in_r;

				++sp;
				if (sp >= div) sp = 0;
				stack_ptr = &stack[sp];

				sum_out_r += stack_ptr[0];
				sum_in_r  -= stack_ptr[0];
			}

		}
	}

	// step 2
	if (step == 2)
	{
		int minX = core * w / cores;
		int maxX = (core + 1) * w / cores;

		for(x = minX; x < maxX; x++)
		{
			sum_r =	
			sum_in_r = 
			sum_out_r = 0;

			src_ptr = src + x; // x,0
			for(i = 0; i <= radius; i++)
			{
				float v=src_ptr[((hm-radius+i+1)%h)*w];
				stack_ptr    = &stack[i ];
				stack_ptr[0] = v;
				sum_r       += v * (i + 1);
				sum_out_r   += v;
			}
			for(i = 1; i <= radius; i++)
			{
				if(i <= hm) src_ptr += w; // +stride

				stack_ptr = &stack[ (i + radius)];
				stack_ptr[0] = src_ptr[0];
				sum_r += src_ptr[0] * (radius + 1 - i);
				sum_in_r += src_ptr[0];
			}

			sp = radius;
			yp = radius;
			if (yp > hm) yp = hm;
			src_ptr = src +  (x + yp * w); // img.pix_ptr(x, yp);
			dst_ptr = src +  x; 			  // img.pix_ptr(x, 0);
			for(y = 0; y < h+radius; y++)
			{
				if(y==h)dst_ptr-=h*w;
				if(y<h)
					dst_ptr[0] = (sum_r * mul_sum) / float(1<< shr_sum);
				else
				{
					float a=float(y-h)/float(radius);
					dst_ptr[0] = a*dst_ptr[0]+(1-a)*(sum_r * mul_sum) / float(1<< shr_sum);
				}
				
				dst_ptr += w;

				sum_r -= sum_out_r;

				stack_start = sp + div - radius;
				if(stack_start >= div) stack_start -= div;
				stack_ptr = &stack[ stack_start];

				sum_out_r -= stack_ptr[0];

				if(yp < hm)
				{
					src_ptr += w; // stride
					++yp;
				}
				if(yp == hm)
				{
					src_ptr -= w*hm; // stride
					yp=0;
				}

				stack_ptr[0] =  src_ptr[0];
				sum_in_r +=		src_ptr[0];
				sum_r    += sum_in_r;

				++sp;
				if (sp >= div) sp = 0;
				stack_ptr = &stack[sp];
				sum_out_r += stack_ptr[0];
				sum_in_r  -= stack_ptr[0];
			}
		}
	}

}

/*
class MVImageUtilsStackBlurTask : public MVThread
{
public:
	unsigned char* src;
	unsigned int w;
	unsigned int h;
	unsigned int radius;
	int cores;
	int core;
	int step;
	unsigned char* stack;

	inline MVImageUtilsStackBlurTask(unsigned char* src, unsigned int w, unsigned int h, unsigned int radius, int cores, int core, int step, unsigned char* stack)
	{
		this->src = src;
		this->w = w;
		this->h = h;
		this->radius = radius;
		this->cores = cores;
		this->core = core;
		this->step = step;
		this->stack = stack;
	}

	inline void run()
	{
		stackblurJob(src, w, h, radius, cores, core, step, stack);
	}

};
*/

/// Stackblur algorithm by Mario Klingemann
/// Details here:
/// http://www.quasimondo.com/StackBlurForCanvas/StackBlurDemo.html
/// C++ implemenation base from:
/// https://gist.github.com/benjamin9999/3809142
/// http://www.antigrain.com/__code/include/agg_blur.h.html
/// This version works only with RGBA color
void 			   stackblur(float* src,				///< input image data
	   					     unsigned int w,					///< image width
							 unsigned int h,					///< image height
							 unsigned int radius,				///< blur intensity (should be in 2..254 range)
							 int cores = 1						///< number of threads (1 - normal single thread)
							 )
{
	if (radius > 254) return;
	if (radius < 1) return;

	unsigned int div = (radius * 2) + 1;
	float* stack = new float[div * cores];

	if (cores == 1)
	{
		// no multithreading
		stackblurJob(src, w, h, radius, 1, 0, 1, stack);
		stackblurJob(src, w, h, radius, 1, 0, 2, stack);
	}
	/*
	else
	{
		MVImageUtilsStackBlurTask** workers = new MVImageUtilsStackBlurTask*[cores];
		for (int i = 0; i < cores; i++)
		{
			workers[i] = new MVImageUtilsStackBlurTask(src, w, h, radius, cores, i, 1, stack + div * 4 * i);
			workers[i]->start();
		}

		for (int i = 0; i < cores; i++)
			workers[i]->wait();

		for (int i = 0; i < cores; i++)
		{
			workers[i]->step = 2;
			workers[i]->start();
		}

		for (int i = 0; i < cores; i++)
		{
			workers[i]->wait();
			delete workers[i];
		}

		delete[] workers;
	}*/

	delete[] stack;
}
