#version 120
uniform sampler2D tex_heightmap; 
uniform sampler2D tex_rock_sand_grass; 
uniform sampler2D tex_normalmap; 

uniform vec4 map_position;
uniform vec4 offset;
uniform vec4 scale;

varying vec3 vs_tex_coord;
varying vec4 vs_pos;

#define f_frac(_a_) (_a_-floor(_a_))

float bspline_2d_fp_fast( sampler2D texin, vec2 vecin) 
{
	vec2 tsize = vec2(4096.0,4096.0);
	vec2 one_tsize = vec2(1.0/tsize.x,1.0/tsize.y);
	vec2 coord_grid = vecin*tsize - vec2(0.5,0.5);
	vec2 fraction = f_frac(coord_grid);
	vec2 one_frac = 1.0 - fraction;
	vec2 one_frac2 = one_frac * one_frac;
	vec2 fraction2 = fraction * fraction;
	vec2 w0 = 1.0/6.0 * one_frac2 * one_frac;
	vec2 w1 = 2.0/3.0 - 0.5 * fraction2 * (2.0-fraction);
	vec2 w2 = 2.0/3.0 - 0.5 * one_frac2 * (2.0-one_frac);
	vec2 w3 = 1.0/6.0 * fraction2 * fraction;
	vec2 g0 = w0 + w1;
	vec2 g1 = w2 + w3;
	vec2 index = coord_grid-fraction;
	vec2 h0 = ((w1 / g0) - 0.5 + index)*one_tsize;
	vec2 h1 = ((w3 / g1) + 1.5 + index)*one_tsize;
	// fetch the four linear interpolations
	float tex000 = texture2D(texin, vec2(h0.x, h0.y)).x*16.0;
	float tex001 = texture2D(texin, vec2(h0.x, h1.y)).x*16.0;
	tex000 = mix(tex001, tex000, g0.y);
	float tex010 = texture2D(texin, vec2(h1.x, h0.y)).x*16.0;
	float tex011 = texture2D(texin, vec2(h1.x, h1.y)).x*16.0;
	tex010 = mix(tex011, tex010, g0.y);	
	return mix(tex010, tex000, g0.x)*(1.0/16.0);
}


vec4 bspline_2d_fp_fast4( sampler2D texin, vec2 vecin) 
{
	vec2 tsize = vec2(4096.0,4096.0);
	vec2 one_tsize = vec2(1.0/tsize.x,1.0/tsize.y);
	vec2 coord_grid = vecin*tsize - vec2(0.5,0.5);
	vec2 fraction = f_frac(coord_grid);
	vec2 one_frac = 1.0 - fraction;
	vec2 one_frac2 = one_frac * one_frac;
	vec2 fraction2 = fraction * fraction;
	vec2 w0 = 1.0/6.0 * one_frac2 * one_frac;
	vec2 w1 = 2.0/3.0 - 0.5 * fraction2 * (2.0-fraction);
	vec2 w2 = 2.0/3.0 - 0.5 * one_frac2 * (2.0-one_frac);
	vec2 w3 = 1.0/6.0 * fraction2 * fraction;
	vec2 g0 = w0 + w1;
	vec2 g1 = w2 + w3;
	vec2 index = coord_grid-fraction;
	vec2 h0 = ((w1 / g0) - 0.5 + index)*one_tsize;
	vec2 h1 = ((w3 / g1) + 1.5 + index)*one_tsize;
	// fetch the four linear interpolations
	vec4 tex000 = texture2D(texin, vec2(h0.x, h0.y))*16.0;
	vec4 tex001 = texture2D(texin, vec2(h0.x, h1.y))*16.0;
	tex000 = mix(tex001, tex000, g0.y);
	vec4 tex010 = texture2D(texin, vec2(h1.x, h0.y))*16.0;
	vec4 tex011 = texture2D(texin, vec2(h1.x, h1.y))*16.0;
	tex010 = mix(tex011, tex010, g0.y);	
	return mix(tex010, tex000, g0.x)*(1.0/16.0);
}

float cubicInterpolate (float p[16], float x,int add) 
{
	return p[add+1] + 0.5 * x*(p[add+2] - p[add+0] + x*(2.0*p[add+0] - 5.0*p[add+1] + 4.0*p[add+2] - p[add+3] + x*(3.0*(p[add+1] - p[add+2]) + p[add+3] - p[add+0])));
}
float bspline_2d_fp( sampler2D texin, vec2 coord) 
{
	vec2 tsize = vec2(4096.0,4096.0);
	vec2 index = floor(coord*tsize);
	vec2 ipol  = f_frac(coord*tsize);
	vec2 one_tsize = vec2(1.0/tsize.x,1.0/tsize.y);

	index=index/tsize;

	float p[16];
	for(int i=0;i<4;i++)
	for(int j=0;j<4;j++)
	{
		p[i*4+j]=texture2D(texin, index+vec2(float(i-1)*one_tsize.x,float(j-1)*one_tsize.y )).x;
	}
	float arr[16];
	arr[0] = cubicInterpolate(p, ipol.y,0);
	arr[1] = cubicInterpolate(p, ipol.y,4);
	arr[2] = cubicInterpolate(p, ipol.y,8);
	arr[3] = cubicInterpolate(p, ipol.y,12);
	return   cubicInterpolate(arr , ipol.x,0);
}

void main(void)
{
	vec4 pos = scale*(offset+vec4(gl_Vertex.xyz,1.0));
	vec4 pos_map = pos*2.0+map_position;

	float bump_weight=1.0;

	if(scale.x<1.0/128.0)
	{
		if(scale.x<1.0/256.0)
			pos.z= bspline_2d_fp(tex_heightmap,f_frac(pos_map.xy));//.x;//*0.9+texture2D(tex_heightmap,pos_map.xy*8.0).x*0.1;
		else
			pos.z= bspline_2d_fp_fast(tex_heightmap,f_frac(pos_map.xy));//.x;//*0.9+texture2D(tex_heightmap,pos_map.xy*8.0).x*0.1;

		//float w_dist=clamp(1.0-length(pos.xy)*128.0,0.0,1.0);
		bump_weight=bspline_2d_fp_fast4(tex_normalmap,pos_map.xy).w;
	}
	else
	{
		pos.z= texture2D(tex_heightmap,pos_map.xy).x;//*0.9+texture2D(tex_heightmap,pos_map.xy*8.0).x*0.1;
		bump_weight=texture2D(tex_normalmap,pos_map.xy).w;
	}

	pos.z+=(texture2D(tex_heightmap,pos_map.xy*64.0).x-0.5)*0.01*bump_weight;//*w_dist;

	//pos.z=pos.z*0.89
	//	+0.1*texture2D(tex_heightmap,pos_map.xy*16.0)*texture2D(tex_heightmap,pos_map.xy*4.0)
	//	+0.01*texture2D(tex_heightmap,pos_map.xy*32.0);//*texture2D(tex_heightmap,pos_map.xy*16.0)
	//	;


	vs_tex_coord.z  = pos.z ;
	vs_tex_coord.xy = pos_map.xy ;

	pos.z*=-0.05;

	//vec3 n=bspline_2d_fp_fast4(tex_normalmap,frac(pos_map.xy)).xyz*2.0-1.0	;
	//n.xy*=-1.0;
	//float bump=0.0+texture2D(tex_heightmap,pos_map.xy*512.0).w;
	//float bump1=1.0-texture2D(tex_rock_sand_grass,pos_map.xy*128.0).w;
	//float bump2=1.0-texture2D(tex_heightmap,pos_map.xy*32.0).x;
	//pos.xyz=pos.xyz+n*(bump1)*0.004*0.05;
	//pos.z+=bump2*0.01*0.025*bump1;


	gl_Position = vs_pos = gl_ModelViewProjectionMatrix*pos;
}
