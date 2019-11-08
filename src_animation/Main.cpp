/////////////////////////////////////////////
//
// Geometry Clip-Maps Tutorial
//
// (C) by Sven Forstmann in 2014
//
// License : MIT
// http://opensource.org/licenses/MIT
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
///////////////////////////////////////////
#include "Bmp.h"
#include "ogl.h"
#include "glsl.h"
///////////////////////////////////////////
vec4f sunlightvec(-1, 1, 2, 0);
#include "Mesh.h"
#include "Terrain.h"
#include "Volume.h"
///////////////////////////////////////////
uint   time_zero=timeGetTime();
double time_elapsed=0;
///////////////////////////////////////////
void DrawSky()
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	static Mesh skybox(
		"../data/skybox/ogre.material",
		"../data/skybox/ogre.mesh.xml"	);

	skybox.Draw(vec3f(0,0,0),vec3f(0,M_PI,M_PI));

	// moon

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glScalef(10,10,10);

	static Mesh ogremoon(
		"../data/moon/ogre.material",
		"../data/moon/ogre.mesh.xml");

	ogremoon.Draw(vec3f(1.5,1.0, 1.0),vec3f(0,time_elapsed*0.01,0));
	ogremoon.Draw(vec3f(0,  3.0,-4.0),vec3f(0,time_elapsed*0.02,0));

	glScalef(1.0/10,1.0/10,1.0/10);
	
	glDisable(GL_BLEND);
	glClear(GL_DEPTH_BUFFER_BIT);
}
///////////////////////////////////////////
void DrawScene()
{
	if ( GetAsyncKeyState(VK_ESCAPE) )  exit(0);

	POINT cursor;
	GetCursorPos(&cursor); // mouse pointer position

	bool	wireframe= GetAsyncKeyState(VK_SPACE);	// render wireframe
	bool	topdown	 = GetAsyncKeyState(VK_RETURN);	// view top-down
	float	viewangle_x = float(cursor.x-1280/2)/4.0;
	float	viewangle_y = float(cursor.y-768/2)/4.0;

	static double viewpos_x=0;
	static double viewpos_y=0;
	static double viewpos_z=0;

	//time
	time_elapsed=float(timeGetTime()-time_zero)/1000.0;
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearDepth(1.0f);
	glClearColor(1,0,0, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	
	glMatrixMode( GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	glDisable(GL_DEPTH_TEST);

	//move
	{
		static int t1=timeGetTime();
		static int t2=timeGetTime();
		t2=t1;t1=timeGetTime();

		double speed=double(t1-t2)/4.0;

		if(GetAsyncKeyState(VK_SHIFT))	speed*=8;

		double addx=-sin(viewangle_x*2*M_PI/360.0)*speed;
		double addz= cos(viewangle_x*2*M_PI/360.0)*speed;

		if(GetAsyncKeyState(VK_UP))		{viewpos_x+=addx;viewpos_z+=addz;}
		if(GetAsyncKeyState(VK_DOWN))	{viewpos_x-=addx;viewpos_z-=addz;}
		if(GetAsyncKeyState(VK_RIGHT))	{viewpos_z+=addx;viewpos_x-=addz;}
		if(GetAsyncKeyState(VK_LEFT))	{viewpos_z-=addx;viewpos_x+=addz;}
		viewpos_y = terrain::get_height(viewpos_x,viewpos_z);
	}

	// camera

	int vp[4];
	glMatrixMode( GL_PROJECTION);
	glLoadIdentity();
	glGetIntegerv(GL_VIEWPORT, vp);
	//gluPerspective(90.0, float(vp[2]) / float(vp[3]), 0.1, 5);// 1.0, terrain::map_scale );
	gluPerspective(90.0, float(vp[2]) / float(vp[3]), 0.00001, 1.5);
	glMatrixMode( GL_MODELVIEW);
	glLoadIdentity();


	// sky
	
	DrawSky();
	
	// ------------------
	// terrain rendering

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	terrain::wireframe=wireframe;
	terrain::draw(viewpos_x,viewpos_y,viewpos_z,viewangle_x,viewangle_y,0);

	// animtest
	/*
	static Mesh ogreobj(
		"../data/turrican/ogre.material",
		"../data/turrican/ogre.mesh.xml",
		"../data/turrican/ogre.skeleton.xml");
	
	glLoadIdentity();
	glRotatef( viewangle_y,1,0,0);		// set rotation
	glRotatef( viewangle_x,0,1,0);		// set rotation

	static double	posx=viewpos_x;
	static double	posz=viewpos_z;
	double			posy=terrain::get_height(posx,posz);

	double relx=viewpos_x-posx;
	double rely=viewpos_y-posy;
	double relz=viewpos_z-posz;

	//glScaled(1.0/terrain::map_scale,1.0/terrain::map_scale,1.0/terrain::map_scale);
	glTranslated(relx,rely,relz);
	//glRotatef(-90,1,0,0);	


	glCullFace(GL_BACK);
	
	int lod=0;
	if(GetAsyncKeyState(VK_F2)) lod=1;
	if(GetAsyncKeyState(VK_F3)) lod=2;
	if(GetAsyncKeyState(VK_F4)) lod=3;
	if(GetAsyncKeyState(VK_F5)) lod=4;
	
	if(GetAsyncKeyState(VK_F1))
		ogreobj.animation.SetPose(1,double(timeGetTime())/100.0);
	else
		ogreobj.animation.SetPose(0,double(timeGetTime())/1000.0);
	
	//if(0)
	glScalef(6,6,6);
	loopi(-0,1) loopj(-0,1)
	{
		ogreobj.Draw(vec3f(i*15,-11,j*15),vec3f(0,0*time_elapsed*0.1,0),lod);
	
		//glScalef(5,5,5);	
		//ogremoon.Draw(vec3f(5,1,5),vec3f(1*time_elapsed*0.4,time_elapsed*0.4,time_elapsed*0.3*1));
	}
	*/
	//glDisable(GL_DEPTH_TEST);
	/*
	glTranslated(0,0,-1);
	glRotatef(viewangle_y, 1, 0, 0);
	glRotatef(viewangle_x, 0, 1, 0);
	static Volume vol(256,256,256);
	vol.DrawCube();
	glutWireCube(1);
	*/
	// Swap
	glutSwapBuffers();
}
///////////////////////////////////////////
vec3f getff(float f)
{
	float add = (f < 0) ? 128 : 0;
	//float mul = (f < 0) ? -1 : 1;
	f = fabs(f);
	
	//f *= 256;

//	float ex = floor(log2(f));
	//float m = ((f / pow(2, ex))*65536);
	//cout << m <<" e " << ex << " f " << f <<endl;
	//printf("%e\n", f);

	float b = floor(f / 65536.0f);
	float g = floor((f - b * 65536.0f) / 256.0);
	float r = floor(f - b * 65536.0f - g * 256.0);

	return vec3f(r,g,b+add);
//	return vec3f(add + mul*r, add + mul*g, add + mul*b);
}


int main(int argc, char **argv)
{
	/*
	loopi(0, 20)
	{
		int a = float(i - 10);// *120.2567;
		vec3f v = getff(a);
		v.print();

		int b = uint(v.x) + uint(v.y) * 256 + (uint(v.z) & 127) * 65536;
		if (v.z >= 128) b = -b;

		printf("a: %d b: %d\n", a,b);
	}
	system("pause");
	return 0;*/
  glutInit(&argc, argv);  
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);  
  glutInitWindowSize(1280, 768);  
  glutInitWindowPosition(0, 0);  
  glutCreateWindow("Game 2014");
  glutDisplayFunc(DrawScene);
  glutIdleFunc(DrawScene);
  glewInit();
  wglSwapIntervalEXT(0);
  glutMainLoop();  
  return 0;
}
///////////////////////////////////////////
