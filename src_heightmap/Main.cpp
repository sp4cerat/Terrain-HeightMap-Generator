///////////////////////////////////////////
#include "Bmp.h"
#include "stackblur.h"
// Result image
Bmp image(4096,4096,24);
std::vector<float> data;
///////////////////////////////////////////
//int terrain_seed=11;
//int terrain_seed=2;
int terrain_seed=134137;
int terrain_max_points = 300;
//int terrain_seed=392137;
//int terrain_seed=71911831;
//int terrain_seed=11937;
//int terrain_seed=8943537; // gut
///////////////////////////////////////////
inline int getpixelofs(int i,int j)
{
	while(i<0)i+=image.width;
	while(j<0)j+=image.height;
	i=i%image.width;
	j=j%image.height;
	return i + j*image.width;
}
///////////////////////////////////////////
inline void setpixelmax(int i,int j,float z,std::vector<float> &pixels=data)
{
	int ofs=getpixelofs(i,j);
	pixels[ofs]=max(z,data[ofs]);
}
///////////////////////////////////////////
inline void setpixel(int i,int j,float z,std::vector<float> &pixels=data)
{
	pixels[getpixelofs(i,j)]=z;
}
///////////////////////////////////////////
inline float getpixel(int i,int j,std::vector<float> &pixels=data)
{
	return pixels[getpixelofs(i,j)];
}
///////////////////////////////////////////
namespace perlin
{
	int rnd_seed = 0;
	float sample_rnd(int x,int y)
	{
		int a=rnd_seed+x+y+(x*2561)+(y*5131);
		return float((rnd_seed+
			x*1531359+
			y*8437113+
			x*a*353+y*a*241+x*y*21+
			532515)&65535)/65535;
	}

	float sample_bicubic(int px,int py,int level)
	{
		int rndx=px>>level; float x01=(px&((1<<level)-1))/float(1<<level);
		int rndy=py>>level; float y01=(py&((1<<level)-1))/float(1<<level);

		int sizex=image.width >>level;
		int sizey=image.height>>level;

		float a[4][4];
		loopi(0,4)loopj(0,4) a[i][j]=sample_rnd( (i+rndx)%sizex, (j+rndy)%sizey);

		return bicubicInterpolate (a,  x01,  y01 );
	}
	void get_perlin(int seed=0)
	{
		rnd_seed = seed+seed*3456+seed*23521;

		int sx=image.width;float fx=sx;
		int sy=image.height;float fy=sy;

		loopj(0,sy)loopi(0,sx)
		{
			float v=0;
			loopk(0,7) v=v*0.5+sample_bicubic(i,j,k);				
			setpixel(i,j,v);
		}
	}
///
};//namespace
///////////////////////////////////////////
int getRnd ()
{
	static int seed = 453413+terrain_seed;
	seed = (seed*751423^234423) - seed*seed*346 + seed*seed*seed*342521 - 93337524 + (seed/2346) - (seed^234621356) + ((seed*seed)>>16);
	return seed;
}
///////////////////////////////////////////
float getRnd256 ()
{
	return (getRnd()&511)-256;
}
///////////////////////////////////////////
vec3f rndv3f()
{
	vec3f rnd( getRnd256() , getRnd256() , getRnd256());
	rnd.norm();
	return rnd;
}
///////////////////////////////////////////
void line_with_rnd( vec3f a, vec3f b )
{
	float dx=a.x-b.x,dy=a.y-b.y;
	float length = sqrt(dx*dx+dy*dy);//(a-b).len();

	if ( length < 1 ) return;

	vec3f rnd=rndv3f();
	rnd.z+=float(terrain_seed%15+2)/7.0f; // z=height=intensity inside the image

	vec3f middle = (a+b)/2 + rnd*length*0.4f;

	setpixelmax(middle.x,middle.y,middle.z);

	line_with_rnd (a,middle);
	line_with_rnd (middle,b);
}
///////////////////////////////////////////
void DLA()
{
	printf("Diffusion-limited aggregation (DLA)\n\n");

	std::vector<vec3f> v;
	std::vector<int  > w; 
	
	v.push_back(vec3f(image.width/2,image.height/2,100));
	w.push_back(0);

	//DLA
	int pmax = terrain_max_points;// (terrain_seed * 4536 + 234563) % 100; pmax *= pmax; pmax += 250; pmax = pmax*image.width * 14 / 2048;
	int radmax = image.width*16/1024;
	int radmax2= image.width/6;//(4+((terrain_seed*163)%3));

	loopi(0,pmax)
	{
		if(i%100==0) printf("%d of %d points\r", i,pmax);

		int id=(getRnd()%v.size());//*3/4+v.size()*1/4-4; id=max(id,0);
		if(id>=v.size())continue;

		float angle  = getRnd()%2127;
		int   radius = ((getRnd()%radmax )+radmax )*(((i*i)&3)+1)*((i&7)+1);

		if (radius>radmax2)radius=radmax2;

		vec3f p(sin(angle)*radius ,cos(angle)*radius ,0);
		p.z=(getRnd()%111)-v[id].z  ;

		int steps=4*radius;
		int skip=0;

		loopj(steps/5,steps*5/4+1)
		{
			float a=float(j)/float(steps);
			vec3f q=v[id]+p*a;
			if(getpixel(q.x,q.y)>0){skip=1;break;}
		}
		if(skip)continue;

		p=p+v[id];

		line_with_rnd( v[id],p );

		// add new point
		v.push_back(p);
		w.push_back(id);
	}
	printf("\n\n");
}
///////////////////////////////////////////
void BoxBlur(std::vector<float> &data, int r)
{
	int w=image.width;
	int h=image.height;
	float r2=r*2;

	loopj(0,h)
	{
		float a1=0,a2=0,a=0;

		loopi(-r,0)
		{
			a+=getpixel(i,j,data);
			a1+=getpixel(i,j,data);
		}
		loopi(1,r)
		{
			a+=getpixel(i,j,data);
			a2+=getpixel(i,j,data);
		}

		loopi(0,w)
		{			
			setpixel(i,j,a/r2,data);
			a+=getpixel(i+r,j,data);
			a-=getpixel(i-r,j,data);
		}
	}
	loopj(0,w)
	{
		float a=0;
		loopi(-r,r)
			a+=getpixel(j,i,data);

		loopi(0,h)
		{			
			setpixel(j,i,a/r2,data);
			a+=getpixel(j,i+r,data);
			a-=getpixel(j,i-r,data);
		}
	}
}
void Blur(std::vector<float> &data, int radius)
{
	if(radius==0)return;

	radius=radius*image.width/800;//(400+(terrain_seed*17)%400);//1024;

	if(radius>254)
	{
		loopi(0,radius/128)
			stackblur(&data[0],image.width,image.height,254);		
	}
	else
		stackblur(&data[0],image.width,image.height,radius);		

	return;

	std::vector<float> tmp=data;

	float diameter=radius*2;
	std::vector<float> w;
	w.resize(diameter);
	loopi(0,diameter) w[i]=sin(float(i)*M_PI/diameter );

	loopj(0,image.height)
	loopi(0,image.width)
	{
		int s=i-radius, e=i+radius, w_of=0;
		
		int t_ofs=j*image.width; 
		float a_sum=0,w_sum=0;

		loopk(s,e)
		{
			float weight=w[w_of++];
			a_sum+=tmp[t_ofs+(k+image.width)%image.width]*weight;
			w_sum+=weight;
		}

		data[i+j*image.width]=a_sum/w_sum;
	}
	tmp=data;

	loopi(0,image.width)
	loopj(0,image.height)
	{
		int s=j-radius, e=j+radius, w_of=0;
		
		int t_of=i; 
		float a_sum=0,w_sum=0;

		loopk(s,e)
		{
			float weight=w[w_of++];
			a_sum+=tmp[t_of+image.width*((k+image.height)%image.height)]*weight;
			w_sum+=weight;
		}
		data[i+j*image.width]=a_sum/w_sum;
	}
}
///////////////////////////////////////////
void Normalize(std::vector<float> &dat)
{
	printf("Normalize\n\n");

	float fmin,fmax;
	fmin=fmax=dat[0];

	loopi(0,image.width*image.height)
	{
		fmin=min(fmin,dat[i]);
		fmax=max(fmax,dat[i]);
	}
	loopi(0,image.width*image.height)
		dat[i]=(dat[i]-fmin)*200/(fmax-fmin);
}
///////////////////////////////////////////
std::vector<float> perlin_weight;
///////////////////////////////////////////
void Sum_Blurred()
{
	printf("Sum Blurred Images\n\n");
	std::vector<float> result=data;

	const int numsteps=8;

//	float weight[numsteps]={0.01,0.4,0.8,2,4,4,5,5};
	float weight[numsteps]={0.1,0.4,0.8,2,4,4,5,5};
//	float weight[numsteps]={0.07,0.2,0.8,2,4,4,5,5};

	loopj(0,data.size()) result[j]=0;//weight[0];

	loopi(0,numsteps)
	{
		int radius=(2<<i)-1;

		printf("Blur Step %d of %d radius %d \n", i,numsteps, radius);

		std::vector<float> tmp;tmp=data;

		Blur(tmp,radius*2/3+1);
		Blur(tmp,radius*2/3);
		Normalize(tmp);

		if(i==6) perlin_weight=tmp;
		if(i==7) loopi(0,tmp.size()) perlin_weight[i]=tmp[i]-perlin_weight[i]*0.5;
		
		char txt[1000];sprintf(txt,"%d.bmp",i);
		loopi(0,image.width*image.height)
			image.data[i*3]=
			image.data[i*3+1]=
			image.data[i*3+2]=min(max(tmp[i],0),255);

		//image.save(txt);

		/*
		if(i==7)
		{
			Bmp bmp(image.width*2,image.height*2,24);
			loopi(0,image.width*2)
			loopj(0,image.height*2)
			{
				int o1=((i%image.width)+(j%image.height)*image.width)*3;
				int o2=(i+j*image.width*2)*3;
				bmp.data[o2+0]=
				bmp.data[o2+1]=
				bmp.data[o2+2]=image.data[o1];
			}
			bmp.save("test.bmp");
		}
		*/

		loopj(0,data.size()) result[j]+=tmp[j]*weight[i];
	}
	data=result;
	Normalize(data);
}
///////////////////////////////////////////
void write_obj(char* name,int scale=1)
{
	printf("Writing OBJ\n");
	FILE* f=fopen(name,"w");
	if(!f) return;

	loopj(0,image.height/scale)
		loopi(0, image.width / scale)
	{
		float x=i;
		float y=j;
		float z = data[i*scale + j*scale*image.width] * (100*4096 / image.height);
		fprintf(f,"v %f %f %f\n",x,y,z);

	}
	loopj(0, image.height / scale - 1)
		loopi(0, image.width / scale - 1)
	{
		fprintf(f,"f");		
		loopm(0,2)
		loopk(0,2)
		{
			int ox = i + (k^m);
			int oy = j + m;
			int o = 1 + ox + oy*(image.width / scale);
			fprintf(f," %d",o);
		}
		fprintf(f,"\n");
	}
	fclose(f);
}
///////////////////////////////////////////
void Add_Perlin()
{
	printf("Add_Perlin\n");

	Normalize(data);
	Normalize(perlin_weight);

	std::vector<float> tmp=data;

	if(!image.load_float("../data/perlin.f32",&data[0]))
	{
		perlin::get_perlin(terrain_seed);
		image.save_float("../data/perlin.f32",&data[0]);
	}

	Normalize(data);	
	/*
	loopi(0,image.width*image.height)
		image.data[i*3]=
		image.data[i*3+1]=
		image.data[i*3+2]=min(max(data[i],0),255);
	image.save("perlin.bmp");
	*/
	
	loopj(0,image.height) 
	loopi(0,image.width)
	{
		int o=i+j*image.width;
		float w=// fabs(getpixel(i+10,j,tmp)-getpixel(i,j,tmp))
				//+fabs(getpixel(i,j+10,tmp)-getpixel(i,j,tmp));
			    ((perlin_weight[i])/200.0f+0.05);
		data[o]=tmp[o]+(data[o]-100)*sqrt(w)*0.075;//0.125*(w);
	}
//	loopi(0,image.width*image.height) 
	//	data[i]=tmp[i]+(data[i]-100)*0.06125*((perlin_weight[i]+10)*0.9/200.0f+0.1);

	Normalize(data);
};
///////////////////////////////////////////
void Erosion()
{
	printf("Erosion\n\n");

	std::vector<vec3f> v;
	std::vector<float> tmp_in=data;
	std::vector<float> tmp_out=data;

	v.clear();
	loopi(0,20000)
	{
		vec3f p(abs(float(getRnd()%image.width)),abs(float(getRnd()%image.height)),1);
		v.push_back(p);		
	}
	
	loopi(0,20000)
	loopj(0,400)	
	{
		float w_path= (j<5) ? float(j+1)/5.0f : 1;

		vec3f a(v[i].x,v[i].y,getpixel(v[i].x,v[i].y));
		vec3f mina=a;
		loopl(0,4)
		{
			int k=(l+getRnd())&3;
			float ax[4]={-1, 1, 1,-1};
			float ay[4]={-1,-1, 1, 1};
			float b=getpixel(v[i].x+ax[k],v[i].y+ay[k]);
			if(b<mina.z) mina=vec3f(v[i].x+ax[k],v[i].y+ay[k],b);
		}
		if(mina.z<a.z)
		{
			setpixel(a.x,a.y,a.z*(1.0-0.2*w_path)+mina.z*0.2*w_path);
			v[i].x=mina.x;
			v[i].y=mina.y;
		}
		else continue;
	}

	std::vector<float> orig=tmp_in;
	loopi(0,data.size())data[i]=sqrt(sqrt(sqrt(tmp_in[i]-data[i])));//(data[i]==tmp[i]) ? 0 : 255;
	tmp_in=data;
	loopi(0,data.size())data[i]=0;

	loopj(0,7)
	{
		int r[7]={0   ,1    ,2  ,4  ,8   ,16 , 32};
		//int w[7]={0.02,0.05 ,0.5,6  ,40 ,100 , 200};
		//int w[7]={0.04,0.1 ,0.5,6  ,40 ,100 , 200};
		//int w[7]={0.08,0.16 ,0.5,6  ,40 ,100 , 200};
		int w[7]={0.16,0.32 ,0.9,6  ,40 ,100 , 200};
		std::vector<float> tmp=tmp_in;
		Blur(tmp,r[j]*1/2);
		loopi(0,data.size())data[i]+=tmp[i]*w[j];
	}	
	Normalize(orig);

	loopi(0,data.size())data[i]=orig[i]-data[i]*0.6*(0.05+0.95*orig[i]/200);
	Normalize(data);
}
///////////////////////////////////////////
int main(int a,char**args)
{
	data.resize(image.width*image.height,0);
	//image.load_float("result.f32",&data[0]);
	int t1=timeGetTime();
	DLA();
	Sum_Blurred();	//Add_Perlin();
	//system("start test.bmp");
	//return(0);

	std::vector<float> tmp=data;
	if(0)
	loopi(0,image.width)
	loopj(0,image.height)
	{
		float a1=getpixel(i,j,data);
		float a2=getpixel(i*8,8*j,tmp);
		setpixel(i,j,a1+a2*(a1/200.0)*0.1,data);
	}

	Erosion();
	//Add_Perlin();

	loopi(0,data.size())data[i]=data[i]/200.0;
	
	//fractalize
	if (0)
	{
		std::vector<float> tmp=data;
		int width=image.width;
		int height=image.height;
		int rnd[6];
		loopi(0,6)rnd[i]=getRnd()%width;
		loopi(0,width)loopj(0,height)
		{
			int x1=rnd[0]+i*4;
			int y1=rnd[1]+j*4;
			int x2=rnd[2]+i*16;
			int y2=rnd[3]+j*16;
			int x3=rnd[4]+i*32;
			int y3=rnd[5]+j*32;
			float h1=getpixel(i,j,tmp);
			float h2=getpixel(x1,y1,tmp);
			float h3=1.0-getpixel(x2,y2,tmp);
			float h4=1.0-getpixel(x3,y3,tmp);
			float final=h1*0.89+h3*h2*0.1+h2*h4*h3*0.01;
			setpixel(i,j,final,data);
		}
	}

	float final_scale=float(getRnd()&511)/511;
	//loopi(0,data.size())data[i]=data[i]*(0.25+final_scale*0.75);

	int t2=timeGetTime()-t1;
	printf("time: %d ms\n",t2);

	image.save_float("../data/terrain/result.f32",&data[0]);		
	//system("start ..\\bin64\\Rendering.exe");

	std::vector<ushort> map16;
	map16.resize(image.width*image.height);
	
	loopi(0,image.width*image.height)
	{
		image.data[i*3]=
		image.data[i*3+1]=
		image.data[i * 3 + 2] = min(255, max(0, 255*data[i]));
		map16[i] = min(65535, max(0, 65535 * data[i]));
	}

	if (1)
	{
		FILE* fn;
		if ((fn = fopen("../heightmap.raw", "wb")) != NULL)
		{
			fwrite(&map16[0], 1, image.width*image.height*2, fn);
			fclose(fn);
		}
		Bmp bmp(image.width , image.height , 16);
		memcpy(&bmp.data[0], &map16[0], image.width * image.height * 2);
		bmp.save("../heightmap16.png");
	}

	if (0)
	{
		Bmp bmp(image.width*2,image.height*2,24);
		loopi(0,image.width*2)
		loopj(0,image.height*2)
		{
			int o1=((i%image.width)+(j%image.height)*image.width)*3;
			int o2=(i+j*image.width*2)*3;
			bmp.data[o2+0]=
			bmp.data[o2+1]=
			bmp.data[o2+2]=image.data[o1];
		}
		bmp.save("test.bmp");
	}

	//image.save("result.bmp");
	//write_obj("result.obj");
	//system("start test.bmp");
	
	write_obj("../data/result.obj",8);
	image.save("../data/result.bmp");
	return 0;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR szCmdLine, int sw){ return main( 0,NULL );}
