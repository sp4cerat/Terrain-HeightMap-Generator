//#################################################################//
#include "Bmp.h"
//#include "error.h"
#include <IL/devil_cpp_wrapper.hpp>
//#################################################################//
void init_ilu_lib()
{
	static bool ini=false;

	if(!ini)
	{
		ilInit();
		iluInit();
		ini=true;
	}
}
//#################################################################//
Bmp::Bmp()
{
	init_ilu_lib();
	width=height=0;
	data=NULL;
}
//#################################################################//
Bmp::Bmp(const char*filename, bool convert32)
{
	init_ilu_lib();
	width=height=0;
	data=NULL;
	load(filename,convert32);
}
//#################################################################//
Bmp::Bmp(int x,int y,int b,unsigned char*buffer)
{
	width=height=0;
	data=NULL;
	set(x,y,b,buffer);
}
//#################################################################//
Bmp::~Bmp()
{
	if (data) free(data);
}
//#################################################################//
void  Bmp::flip()
{
	loopijk(0, 0, 0, width, height / 2, bpp / 8)
	{
		vswap(data[(i + j*width)*(bpp / 8) + k],
			data[(i + (height - 1 - j)*width)*(bpp / 8) + k]);
	}
}
void Bmp::save(const char*filename)
{
	flip();
	printf("saving bmp %dx%dx%d %s\n", width, height, bpp, filename);
	ILuint imageID;
	ilGenImages(1, &imageID);
	ilBindImage(imageID);
	if (bpp == 8) 	ilTexImage(width, height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, &data[0]); //the third agrument is usually always 1 (its depth. Its never 0 and I can't think of a time when its 2)
	if (bpp == 16) 	ilTexImage(width, height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_SHORT, &data[0]); //the third agrument is usually always 1 (its depth. Its never 0 and I can't think of a time when its 2)
	if (bpp == 24) 	ilTexImage(width, height, 1, bpp / 8, IL_RGB, IL_UNSIGNED_BYTE, &data[0]); //the third agrument is usually always 1 (its depth. Its never 0 and I can't think of a time when its 2)
	if (bpp == 32) 	ilTexImage(width, height, 1, bpp / 8, IL_RGBA, IL_UNSIGNED_BYTE, &data[0]); //the third agrument is usually always 1 (its depth. Its never 0 and I can't think of a time when its 2)
	ilEnable(IL_FILE_OVERWRITE);
	ilSaveImage(filename);
	ilDeleteImages(1, &imageID); // Delete the image name. 
	flip();
	return;
	/*
	unsigned char bmp[58]=
			{0x42,0x4D,0x36,0x30,0,0,0,0,0,0,0x36,0,0,0,0x28,0,0,0,
	           	0x40,0,0,0, // X-Size
	           	0x40,0,0,0, // Y-Size
                   	1,0,0x18,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	bmp[18]	=width;
	bmp[19]	=width>>8;
	bmp[22]	=height;
	bmp[23]	=height>>8;
	bmp[28]	=bpp;

	FILE* fn;
	if ((fn = fopen (filename,"wb")) != NULL)
	{
		fwrite(bmp ,1,54   ,fn);
		fwrite(data,1,width*height*(bpp/8),fn);
		fclose(fn);
	}
	else error_stop("Bmp::save %s",filename);*/
	/*
ILuint imageID;
ilGenImages(1, &imageID);
ilBindImage(imageID);

ilTexImage(imageWidth, imageHeight, 1, bytesperpixel, IL_RGB, IL_UNSIGNED_BYTE, pixels); //the third agrument is usually always 1 (its depth. Its never 0 and I can't think of a time when its 2)

ilSaveImage("image.jpg");	
	*/
	
}
//#################################################################//
void  Bmp::blur(int count)
{
	int x,y,b,c;
	int bytes=bpp/8;
	for(c=0;c<count;c++)
		for(x=0;x<width-1;x++)
			for(y=0;y<height-1;y++)
				for(b=0;b<bytes;b++)
					data[(y*width+x)*bytes+b]=
					    (	(int)data[((y+0)*width+x+0)*bytes+b]+
					      (int)data[((y+0)*width+x+1)*bytes+b]+
					      (int)data[((y+1)*width+x+0)*bytes+b]+
					      (int)data[((y+1)*width+x+1)*bytes+b] ) /4;

}
//#################################################################//
void Bmp::crop(int x,int y)
{
	if(data==NULL)return;

	unsigned char* newdata;
	int i,j;

	int bytes=bpp/8;

	newdata=(unsigned char*)malloc(x*y*bytes);

	if(!newdata) error_stop("Bmp::crop : out of memory");

	memset(newdata,0,x*y*bytes);

	for(i=0;i<y;i++)
		if(i<height)
			for(j=0;j<x*bytes;j++)
				if(j<width*bytes)
					newdata[i*x*bytes+j]=data[i*width*bytes+j];
	free(data);
	data=NULL;
	set(x,y,bpp,newdata);
}
//#################################################################//
void Bmp::convert_24_32()
{
	if(data==NULL)return ;

	int x=width, y=height;
	if(x==0)return ;
	if(y==0)return ;

	unsigned char* newdata=(unsigned char*)malloc(x*y*4);
	int bytes=bpp/8;

	loopijk(0,0,0, y,x,bytes)
		newdata[i*x*4+j*4+k]=
			data[i*x*bytes+j*bytes+k];

	loopi(0,x*y) newdata[i*4+3]=0;

	free(data);
	data=NULL;
	set(x,y,32,newdata);
}
//#################################################################//
void Bmp::scale(int x,int y)
{
	if(data==NULL)return ;
	if(x==0)return ;
	if(y==0)return ;

	unsigned char* newdata;
	int i,j,k;

	int bytes=bpp/8;
	newdata=(unsigned char*)malloc(x*y*bytes);
	if(!newdata) error_stop("Bmp::scale : out of memory");

	memset(newdata,0,x*y*bytes);

	for(i=0;i<y;i++)
		for(j=0;j<x;j++)
			for(k=0;k<bytes;k++)
				newdata[i*x*bytes+j*bytes+k]=data[(i*height/y)*(width*bytes)+(j*width/x)*bytes+k];

	free(data);
	data=NULL;
	set(x,y,bpp,newdata);
}
//#################################################################//
void Bmp::set(int x,int y,int b,unsigned char*buffer)
{
	width=x;
	height=y;
	bpp=b;
	if(data) free(data);

	data=(unsigned char*) malloc(width*height*(bpp/8));
	if(!data) error_stop("Bmp::set : out of memory");

	if(buffer==0)
		memset(data,0,width*height*(bpp/8));
	else
		memmove(data,buffer,width*height*(bpp/8));

	bmp[18]	=width;
	bmp[19]	=width>>8;
	bmp[22]	=height;
	bmp[23]	=height>>8;
	bmp[28]	=bpp;
}
//#################################################################//
void Bmp::load(const char *filename, bool convert32)
{
	ilImage i;

	if(!i.Load(filename))
	{
		printf("Bmp::load file %s not found\n",filename);
		set(8,8,24,0);
		memset(data,255,8*8*3);
		//while(1);;
		return;
	}
	
	if(i.GetData()==0)
	{
		printf("Bmp::load 0 pointer\n");
		while(1);;
	}
	
	if(i.Format()==IL_RGBA || convert32) i.Convert(IL_RGBA); else i.Convert(IL_RGB);

	printf("Bmp::loaded %s : %dx%dx%d \n",filename,i.Width(),i.Height(),i.Bpp());

	set(i.Width(),i.Height(),i.Bpp()*8,i.GetData());

	/*
	FILE* handle;

	if(filename==NULL)		
		{error_stop("File not found %s !\n",filename);}
	if((char)filename[0]==0)	
		{error_stop("File not found %s !\n",filename);}

	if ((handle = fopen(filename, "rb")) == NULL)
		{error_stop("File not found %s !\n",filename);}
		
	if(!fread(bmp, 11, 1, handle))
	{
		error_stop("Error reading file %s!\n",filename);
	}
	if(!fread(&bmp[11], (int)((unsigned char)bmp[10])-11, 1, handle))
	{
		error_stop("Error reading file %s!\n",filename);
	}

	width	=(int)((unsigned char)bmp[18])+((int)((unsigned char)(bmp[19]))<<8);
	height	=(int)((unsigned char)bmp[22])+((int)((unsigned char)(bmp[23]))<<8);
	bpp		=bmp[28];

	//printf("%s : %dx%dx%d Bit \n",filename,width,height,bpp);
	
	if(data)free(data);

	int size=width*height*(bpp/8);

	data=(unsigned char*)malloc(size+1);
	fread(data,size,1,handle);

	fclose(handle);

	if(convert32)convert_24_32();

	printf("read successfully %s ; %dx%dx%d Bit \n",filename,width,height,bpp);
*/
}

//#################################################################//
void Bmp::save_float(const char*filename, float* fdata)
{
	if(fdata==0)fdata=(float*)this->data;
	FILE* fn;
	if ((fn = fopen (filename,"wb")) == NULL)  error_stop("Bmp::save_float");
	fwrite(fdata,1,4*width*height,fn);
	fclose(fn);
}
//#################################################################//
bool Bmp::load_float(const char*filename, float* fdata)
{
	if (!fdata)fdata=(float*)data;

	FILE* fn;
	if ((fn = fopen (filename,"rb")) == NULL) return false;// error_stop("Bmp::load_float");
	fread(fdata,1,4*width*height,fn);
	fclose(fn);
	return true;
}
//#################################################################//