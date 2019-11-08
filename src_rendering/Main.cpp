/////////////////////////////////////////////
//
// Geometry Clip-Maps Renderer
//
// (C) by Sven Forstmann in 2014
//
// License : Commercial
// Private use, also in commercial projects, is permitted.
// Reselling this code or publishing this code is NOT permitted.
/////////////////////////////////////////////
// Mathlib included from 
// http://sourceforge.net/projects/nebuladevice/
/////////////////////////////////////////////
#include <iostream> 
#include <vector> 
#include <string> 
#include <stdio.h>
#include <glew.h>
#include <wglew.h>
#include <windows.h>
#include <mmsystem.h>
#include <GL/glut.h>
using namespace std;
#pragma comment(lib,"winmm.lib")
#include "Bmp.h"
///////////////////////////////////////////
vec4f sunlightvec(-1, 1, 2, 0);
int grid = 64;				// patch resolution
int levels = 10;			// LOD levels
float snowfall = 0.0;		// height of the snowfall
float speedfactor = 2;		// speed
float heightmap_avg = 0.5;
float heightmap_scale = 0.8;
int width = 4096, height = 4096; // heightmap dimensions
///////////////////////////////////////////
#include "ogl.h"
#include "glsl.h"
#include "Mesh.h"
///////////////////////////////////////////
void make_terrain_normal(float *heightmap, Bmp &bmp_path, Bmp &bmp_normal)
{
	loopj(0, height)	loopi(0, width)
	{
		int ii = i;
		int jj = j;
		float h = heightmap[ii + jj*width];
		float hx = heightmap[(ii + 1) % width + jj*width];
		float hy = heightmap[ii + ((jj + 1) % height)*width];
		vec3f d1(1, 0, (hx - h) * 1000);
		vec3f d2(0, 1, (hy - h) * 1000);
		bmp_normal.data[(i + j*width) * 4 + 0] = clamp(d1.z*32.0 + 128.0, 0, 255);
		bmp_normal.data[(i + j*width) * 4 + 1] = clamp(d2.z*32.0 + 128.0, 0, 255);
		//bmp_normal.data[(i+j*width)*4+2]=255.0*h;
		bmp_normal.data[(i + j*width) * 4 + 2] = bmp_path.data[(i + j*width) * 3 + 0];

		vec3f n; n.cross(d1, d2); n.norm();
		float w1 = clamp((n.z*0.5 - h)*2.0, 0.0, 1.0);				// grass-sand
		float w2 = clamp((n.z*0.5 - h)*8.0, 0.0, 1.0);				// sand-rock
		//		float w3=clamp((n.z*0.7-0.14-clamp(1.0-h*1.5,0,1.0))*16.0,0.0,1.0);		// rock-snow
		//			float w3=clamp((n.z*1.0-0.40-clamp(1.0-(h+0.05)*snowfall,0,1.0))*16.0,0.0,1.0);			// rock-snow
		float w3 = clamp((n.z - 0.5 - clamp((snowfall - h)*3.0, 0, 0.5))*32.0, 0.0, 1.0);		// rock-snow snowy
		float c1 = 1;		// rock
		float c2 = 0;		// 
		float c3 = 0;		// 
		float c4 = 0;		// 	
		float col = c1;
		c2 = lerp(c2, c3, w1);
		col = lerp(col, c2, w2);
		col = lerp(col, c4, w3);
		bmp_normal.data[(i + j*width) * 4 + 3] = col*255.0f;
	}
}
///////////////////////////////////////////
void DrawScene()
{
	if ( GetAsyncKeyState(VK_ESCAPE) )  exit(0);

	POINT cursor;
	GetCursorPos(&cursor); // mouse pointer position

	bool	wireframe= GetAsyncKeyState(VK_SPACE);	// render wireframe
	bool	topdown	 = GetAsyncKeyState(VK_RETURN);	// view top-down
	float	viewangle_x = -float(cursor.x)/4.0;
	float	viewangle_y = -float(cursor.y) / 4.0;
	float	viewangle_z = 0;

	static double viewpos_x=0;
	static double viewpos_y=0;
	static double viewpos_z=0;

	static int tex_heightmap=0;
	static int tex_terrain=0;
	static int tex_rock_sand_grass=0;
	static float *heightmap=0;
	static bool init=true;
	static Shader shader("../shader/terrain");
	static int vbo=0;
	static std::vector<float> vert;
	static Bmp bmp_normal(width,height,32);
	static Bmp bmp_path("../data/terrain/path.png");
	
	if(init)
	{
		static Bmp bmp(width,height,32);
		//static Bmp tmp(width,height,32);
		if(!bmp.load_float("../data/terrain/result.f32")) error_stop("Bmp::load_float"); 

		heightmap=(float*)bmp.data;
		loopi(0,width*height)heightmap[i]*= heightmap_scale;

		Bmp bmp_grass("../data/terrain/grass.bmp");
		Bmp bmp_rock ("../data/terrain/rock.bmp");
		Bmp bmp_sand ("../data/terrain/sand.bmp");
		Bmp bmp_road ("../data/terrain/road.png");
	
		/*+++++++++++++++++++++++++++++++++++++*/
		// terrain texture

		Bmp bmp_merged(1024,1024,32);

		loopi(0,bmp_grass.width*bmp_grass.height)
		{
			bmp_merged.data[i*4+0]=bmp_rock .data[i*3+1];
			bmp_merged.data[i*4+1]=bmp_sand .data[i*3+2];
			bmp_merged.data[i*4+2]=bmp_grass.data[i*3+1];
			bmp_merged.data[i*4+3]=bmp_road .data[i*3+0];//0;//bmp_elev .data[i*3+0];			
		}
		tex_rock_sand_grass = ogl_tex_new(bmp_merged.width,bmp_merged.height,GL_LINEAR_MIPMAP_LINEAR,GL_REPEAT,GL_RGBA,GL_RGBA,bmp_merged.data, GL_UNSIGNED_BYTE);

		make_terrain_normal(heightmap, bmp_path, bmp_normal);
		tex_terrain = ogl_tex_new(width, height, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_RGBA, GL_RGBA, bmp_normal.data, GL_UNSIGNED_BYTE);
		/*+++++++++++++++++++++++++++++++++++++*/
		// terrain heightmap
		
		heightmap_avg=0;
		float heightmap_min=1;
		float heightmap_max=0;
		loopi(0,width*height){float h=heightmap[i];heightmap_min=min(heightmap_min,h);heightmap_max=max(heightmap_max,h);}
		heightmap_avg=(heightmap_min+heightmap_max)/2;
		
		tex_heightmap = ogl_tex_new(width,height,GL_LINEAR_MIPMAP_LINEAR,GL_REPEAT,GL_LUMINANCE16F_ARB,GL_LUMINANCE,bmp.data, GL_FLOAT);
		/*+++++++++++++++++++++++++++++++++++++*/
		// driver info
		std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;			//std::cout << "GL_EXTENSIONS: " << glGetString(GL_EXTENSIONS) << std::endl;
		std::cout << "GL_RENDERER: " << glGetString(GL_RENDERER) << std::endl;
		std::cout << "GL_VENDOR: " << glGetString(GL_VENDOR) << std::endl;
		std::cout << "GLU_VERSION: " << gluGetString(GLU_VERSION) << std::endl;			//std::cout << "GLU_EXTENSIONS: " << gluGetString(GLU_EXTENSIONS) << std::endl;
		std::cout << "GLUT_API_VERSION: " << GLUT_API_VERSION << std::endl;
		/*+++++++++++++++++++++++++++++++++++++*/
		// make vbo quad patch
		loopj(0,grid+1)
		loopi(0,grid+2)
		{
			loopk(0, ((i==0) ? 2 : 1) )
			{
				vert.push_back(float(i)/grid);
				vert.push_back(float(j)/grid);
				vert.push_back(0 );
			}			
			++j;
			loopk(0, ((i==grid+1) ? 2 : 1) )
			{
				vert.push_back(float(i)/grid);
				vert.push_back(float(j)/grid);
				vert.push_back(0 );
			}
			--j;
		}
		/*+++++++++++++++++++++++++++++++++++++*/
		glGenBuffers(1, (GLuint *)(&vbo));
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vert.size(),&vert[0], GL_DYNAMIC_DRAW_ARB );
		/*+++++++++++++++++++++++++++++++++++++*/
		init=false;
		/*+++++++++++++++++++++++++++++++++++++*/
	}

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glClearDepth(1.0f);
	glClearColor(0,0,0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode( GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	glDisable(GL_DEPTH_TEST);

	loopi(0,5)
	{
		glActiveTextureARB( GL_TEXTURE0+i );
		glDisable(GL_TEXTURE_2D);
	}

	//move
	{
		static int t1=timeGetTime();
		static int t2=timeGetTime();
		t2=t1;t1=timeGetTime();
		double dt = double(t1 - t2);

		double speed=dt/2.0;

		if(GetAsyncKeyState(VK_SHIFT))	speed*=80;

		static double addrotx = 0;

		viewangle_x += addrotx;

		double addx = sin(viewangle_x * 2 * M_PI / 360.0)*speed*speedfactor;
		double addz = cos(viewangle_x * 2 * M_PI / 360.0)*speed*speedfactor;

		if(GetAsyncKeyState(VK_UP))		{viewpos_x+=addx;viewpos_z+=addz;}
		if(GetAsyncKeyState(VK_DOWN))	{viewpos_x-=addx;viewpos_z-=addz;}
		if (GetAsyncKeyState(VK_RIGHT))	addrotx -= dt*0.1;// { viewpos_z += addx; viewpos_x -= addz; }
		if (GetAsyncKeyState(VK_LEFT))	addrotx += dt*0.1;//{viewpos_z-=addx;viewpos_x+=addz;}

		if (GetAsyncKeyState(VK_F1)) snowfall = clamp(snowfall + 0.01, 0, 1);
		if (GetAsyncKeyState(VK_F2)) snowfall = clamp(snowfall - 0.01, 0, 1);

		if (GetAsyncKeyState(VK_F3)) speedfactor = clamp(speedfactor * 1.02, 0.1, 100);
		if (GetAsyncKeyState(VK_F4)) speedfactor = clamp(speedfactor * 0.98, 0.1, 100);

		//update normal
		{
			static uint t = timeGetTime(); static float s = snowfall;
			if (timeGetTime() - t>5000)
			{ 
				t = timeGetTime();
				if (s != snowfall)
				{
					make_terrain_normal(heightmap, bmp_path, bmp_normal);
					glDeleteTextures(1, (uint*)&tex_terrain);
					tex_terrain = ogl_tex_new(width, height, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_RGBA, GL_RGBA, bmp_normal.data, GL_UNSIGNED_BYTE);
					s = snowfall;
				}
			}
		}
	}

	int    map_res=8192;
	double map_scale=map_res*grid;
	double map_pos_x=-double(int(viewpos_x) % int(map_scale))/map_scale;
	double map_pos_y=-double(int(viewpos_z) % int(map_scale))/map_scale;

	// camera
	glMatrixMode( GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode( GL_PROJECTION);
	glLoadIdentity();

	if (topdown)
	{
		glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		glRotatef(180,1,0,0);
		wireframe^=1;
	}
	else		 
	{
		int vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		gluPerspective(90.0,float(vp[2])/float(vp[3]) , 0.00001, 1.5);
		{
			auto get_height = [](float map_pos_x, float map_pos_y)
			{
				float px = (map_pos_x + 2)*width;
				float py = (map_pos_y + 2)*height;
				int x = int(px) % width;
				int y = int(py) % height;

				float h44[4][4];
				float w44[4][4];
				loopi(-2, 2)	loopj(-2, 2)
				{
					int ox = (x + i + width) % width;
					int oy = (y + j + height) % height;
					h44[i + 2][j + 2] = heightmap[ox + oy*width];
					w44[i + 2][j + 2] = float(bmp_normal.data[(ox + oy*width) * 4 + 3]) / 255.0; // rock intensity
				}
				float terrain_height = bicubicInterpolate(h44, frac(px), frac(py));
				float bump_weight = bicubicInterpolate(w44, frac(px), frac(py));

				// details
				int x64 = int(px * 64) % width;
				int y64 = int(py * 64) % height;
				terrain_height += (heightmap[x64 + y64*width] - heightmap_avg)*0.01*bump_weight;//*w_dist;

				return -(terrain_height + 0.003)*0.05;
			};

			viewpos_y = get_height(map_pos_x, map_pos_y);

			// Tilt
			{
				static float tilt = 0;
				static float roll = 0;
				float a = viewangle_x * 2 * M_PI / 360, r = 0.0005;
				float ytilt = get_height(map_pos_x + sin(a)*r, map_pos_y + cos(a)*r);
				float yroll = get_height(map_pos_x - cos(a)*r, map_pos_y + sin(a)*r);

				float add = 0;
				if (GetAsyncKeyState(VK_UP))	add = 5;
				if (GetAsyncKeyState(VK_DOWN))	add = -5;

				viewangle_y = tilt = tilt*0.95 + 0.05* (add  + atan2(ytilt - viewpos_y, r*0.5) * 180 / M_PI);
				viewangle_z = roll = roll*0.95 + 0.05* ( - atan2(yroll - viewpos_y, r*0.5) * 180 / M_PI);

				//printf(" %f %f %f  \r", map_pos_x, map_pos_y, viewpos_y);

			}
		}

		// rotate
		glRotatef(90, 1, 0, 0);		// set rotation
		glRotatef(-viewangle_y, 1, 0, 0);		// set rotation
		glRotatef(viewangle_z * 1 + 0 * timeGetTime()*0.1, 0, 1, 0);		// set rotation
		glRotatef(viewangle_x, 0, 0, 1);		// set rotation

		//skybox
		static Mesh skybox(
			"../data/skybox/ogre.material",
			"../data/skybox/ogre.mesh.xml");
		glDisable(GL_CULL_FACE);
		glPushMatrix();
		glScalef(1.0/1000.0,1.0/1000.0,1.0/1000.0);
		skybox.Draw(vec3f(0,0,0),vec3f(M_PI/2,0,0));
		glMatrixMode( GL_PROJECTION);
		glPopMatrix();

		// translate vertical
		glTranslatef(0,0,-viewpos_y);	// set height

		glEnable(GL_CULL_FACE);
	}

	// ------------------
	// terrain rendering

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	matrix44 mat;
	glGetFloatv(GL_PROJECTION_MATRIX, &mat.m[0][0]);		ogl_check_error();
	
	// Enable VBO
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);				ogl_check_error();
	glEnableClientState(GL_VERTEX_ARRAY);					ogl_check_error();
	glVertexPointer  ( 3, GL_FLOAT,0, (char *) 0);			ogl_check_error();

	ogl_bind(0,tex_heightmap);
	ogl_bind(1,tex_terrain);
	ogl_bind(2,tex_rock_sand_grass);

	// Triangle Mesh
	shader.begin();
	shader.setUniform1i("tex_heightmap",0);
	shader.setUniform1i("tex_normalmap",1);
	shader.setUniform1i("tex_rock_sand_grass",2);

	float sxy=2; // scale x/y
	shader.setUniform4f("map_position", map_pos_x,map_pos_y,0,0);
	shader.setUniform1f("snowfall", snowfall);
	shader.setUniform1f("heightmap_avg", heightmap_avg);
	//if(0)
	loopj(0,levels)
	{
		int i=levels-1-j;
		float sxy=2.0/float(1<<i);
		float ox=(int(int(viewpos_x)*(1<<i))%map_res)/map_scale;
		float oy=(int(int(viewpos_z)*(1<<i))%map_res)/map_scale;

		vec3f scale	(sxy*0.25,sxy*0.25,1);
		shader.setUniform4f("scale" , scale.x,scale.y,1,1);	

		loopk(-2,2) loopj(-2,2) // each level has 4x4 patches
		{
			if(i!=levels-1) if(k==-1||k==0) if(j==-1||j==0) continue;

			vec3f offset(ox+float(j),oy+float(k),0);
			if(k>=0) offset.y-=1.0/float(grid); // adjust offset for proper overlapping
			if(j>=0) offset.x-=1.0/float(grid); // adjust offset for proper overlapping

			//cull
			int xp=0,xm=0,yp=0,ym=0,zp=0;
			looplmn(0,0,0,2,2,2)
			{
				vec3f v = scale*(offset+vec3f(l,m,float(-n)*0.05)); // bbox vector
				vec4f cs = mat * vec4f(v.x,v.y,v.z,1); // clipspace
				if(cs.z< cs.w) zp++;				
				if(cs.x<-cs.w) xm++;	if(cs.x>cs.w) xp++;
				if(cs.y<-cs.w) ym++;	if(cs.y>cs.w) yp++;
			}
			if(zp==0 || xm==8 || xp==8 || ym==8 || yp==8)continue; // skip if invisible
			
			//render
			shader.setUniform4f("offset", offset.x,offset.y,0,0);
			if(wireframe)	glDrawArrays( GL_LINES, 0, vert.size()/3);
			else			glDrawArrays( GL_TRIANGLE_STRIP, 0, vert.size()/3);
		}
	}	
	shader.end();

	loopi(0,3) ogl_bind(2-i,0);

	// Disable VBO
	glDisableClientState(GL_VERTEX_ARRAY);									ogl_check_error();
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);								ogl_check_error();

	// Swap
	glutSwapBuffers();
}
///////////////////////////////////////////
int main(int argc, char **argv) 
{ 
  glutInit(&argc, argv);  
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);  
  glutInitWindowSize(1280, 768);  
  glutInitWindowPosition(0, 0);  
  glutCreateWindow("Geometry Clipmaps Renderer (c) Sven Forstmann 2014");
  glutDisplayFunc(DrawScene);
  glutIdleFunc(DrawScene);
  glewInit();
  wglSwapIntervalEXT(0);
  glutMainLoop();  
  return 0;
}
///////////////////////////////////////////
