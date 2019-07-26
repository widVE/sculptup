//
//  caveCalib.h
//  FionaUT
//
//  Created by Hyun Joon Shin on 7/2/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//


#include "FionaScene.h"

#define SZ -1.4478



typedef float Mat[9];

#define DEPTH_FACTOR 0.4f

inline void LUsolve( float a[], int n, float b[] )
{
	int i, j, k, imax=0;
	float big, dum, sum, temp;
	int index[10];
	
	float vv[10];
	
	for ( i=0; i<n; i++ )
	{
		big = 0.0f;
		for ( j=0; j<n; j++ )
			if ((temp = fabsf(a[i*n+j])) > big)	big = temp;
		
		if (big == 0.0f)
		{
			//			printf( "Singular matrix in routine LUdecompose\n" );
			return;
		}
		vv[i] = 1.0f / big;
	}
	
	for ( j=0; j<n; j++ )
	{
		for ( i=0; i<j; i++ )
		{
			sum = a[i*n+j];
			for ( k=0; k<i; k++ )	sum -= a[i*n+k] * a[k*n+j];
			a[i*n+j] = sum;
		}
		
		big = 0.0;
		for ( i=j; i<n; i++ )
		{
			sum = a[i*n+j];
			for ( k=0; k<j; k++ )	sum -= a[i*n+k] * a[k*n+j];
			a[i*n+j] = sum;
			if ((dum = vv[i] * fabsf(sum)) >= big)	big = dum, imax = i;
		}
		
		if ( j!=imax )
		{
			for ( k=0; k<n; k++ )
			{
				dum = a[imax*n+k];
				a[imax*n+k] = a[j*n+k];
				a[j*n+k] = dum;
			}
			vv[imax] = vv[j];
		}
		
		index[j] = imax;
		if (a[j*n+j] == 0.0f) a[j*n+j] = 0.0000000001f;
		
		if ( j!=n )
		{
			dum = 1.0f / a[j*n+j];
			for ( i=j+1; i<n; i++ )		a[i*n+j] *= dum;
		}
	}
	int ii = -1, ip;
	
	for ( i=0; i<n; i++ )
	{
		ip = index[i];
		sum = b[ip];
		b[ip] = b[i];
		
		if (ii>-1)	for ( j=ii; j<i; j++ )	sum -= a[i*n+j] * b[j];
		else		if (sum)				ii = i;
		b[i] = sum;
	}
	
	for ( i=n-1; i>=0; i-- )
	{
		sum = b[i];
		for ( j=i+1; j<n; j++ )	sum -= a[i*n+j] * b[j];
		b[i] = sum / a[i*n+i];
	}
}

/*
// Mathematical function to compute warping matrix
inline void computeWarpRectToQuad( float W, float H,
								  float x0, float y0, float x1, float y1,
								  float x2, float y2, float x3, float y3, Mat dst )
// W: width of the src rectangle
// H: height of the src rectangle
// x0, y0: bottom left corner (0,0) when untransformed
// x1, y1: bottom right corner (W,0) when untransformed
// x2, y2: top left corner (0,H) when untransformed
// x3, y3: top right corner (W,H) when untransformed
// a[]: output transform matrix
{
	static float M[36];
	M[ 0]=W; M[ 1]=0; M[ 2]=-W*x1; M[ 3]=0; M[ 4]=0; M[ 5]=0;
	M[ 6]=0; M[ 7]=0; M[ 8]=0;     M[ 9]=H; M[10]=0; M[11]=-H*x2;
	M[12]=W; M[13]=0; M[14]=-W*x3; M[15]=H; M[16]=0; M[17]=-H*x3;
	M[18]=0; M[19]=W; M[20]=-W*y1; M[21]=0; M[22]=0; M[23]=0;
	M[24]=0; M[25]=0; M[26]=0;     M[27]=0; M[28]=H; M[29]=-H*y2;
	M[30]=0; M[31]=W; M[32]=-W*y3; M[33]=0; M[34]=H; M[35]=-H*y3;
	
	static Mat a;
	
	a[0]=x1-x0; a[1]=x2-x0; a[2]=x3-x0;
	a[3]=y1-y0; a[4]=y2-y0; a[5]=y3-y0;
	LUsolve( M, 6, a );
	a[6] = x0;
	a[7] = y0;
	a[8] = 1;
	dst[0]=a[0]; dst[1]=a[3]; dst[2]=a[6];
	dst[3]=a[1]; dst[4]=a[4]; dst[5]=a[7];
	dst[6]=a[2]; dst[7]=a[5]; dst[8]=a[8];
}
*/

tran warpTrans(const std::vector<vec2>& s,const std::vector<vec2>& d)
{
	//We need 4 sample points
	static float M[64];
	static float a[8];
	M[ 0]=s[0].x; M[ 1]=s[0].y; M[ 2]=1; M[ 3]=0; M[ 4]=0; M[ 5]=0; M[ 6]=-s[0].x*d[0].x; M[ 7]=-s[0].y*d[0].x;
	M[16]=s[1].x; M[17]=s[1].y; M[18]=1; M[19]=0; M[20]=0; M[21]=0; M[22]=-s[1].x*d[1].x; M[23]=-s[1].y*d[1].x;
	M[32]=s[2].x; M[33]=s[2].y; M[34]=1; M[35]=0; M[36]=0; M[37]=0; M[38]=-s[2].x*d[2].x; M[39]=-s[2].y*d[2].x;
	M[48]=s[3].x; M[49]=s[3].y; M[50]=1; M[51]=0; M[52]=0; M[53]=0; M[54]=-s[3].x*d[3].x; M[55]=-s[3].y*d[3].x;
	M[ 8]=0; M[ 9]=0; M[10]=0; M[11]=s[0].x; M[12]=s[0].y; M[13]=1; M[14]=-s[0].x*d[0].y; M[15]=-s[0].y*d[0].y;
	M[24]=0; M[25]=0; M[26]=0; M[27]=s[1].x; M[28]=s[1].y; M[29]=1; M[30]=-s[1].x*d[1].y; M[31]=-s[1].y*d[1].y;
	M[40]=0; M[41]=0; M[42]=0; M[43]=s[2].x; M[44]=s[2].y; M[45]=1; M[46]=-s[2].x*d[2].y; M[47]=-s[2].y*d[2].y;
	M[56]=0; M[57]=0; M[58]=0; M[59]=s[3].x; M[60]=s[3].y; M[61]=1; M[62]=-s[3].x*d[3].y; M[63]=-s[3].y*d[3].y;
	
	a[ 0]=d[0].x; a[ 1]=d[0].y;
	a[ 2]=d[1].x; a[ 3]=d[1].y;
	a[ 4]=d[2].x; a[ 5]=d[2].y;
	a[ 6]=d[3].x; a[ 7]=d[3].y;
	LUsolve(M,8,a);
	return tran(a[ 0], a[ 1], 0, a[ 2], a[ 3], a[ 4], 0, a[ 5], 0, 0, 1 ,0, a[ 6], a[ 7], 0, 1);
}

#include <fstream>

void _FionaUTSyncProjectorCalib(FionaWindowConfig& conf);

inline int getWord(const std::string& con, std::string& w, int& k)
{
	w.clear();
	while(1)
	{
		if( k>=(int)con.length()) return 0;
		if( con[k]=='\n'){ k++; return 2; }
		if( con[k]==' ' ){ k++; return 1; }
		w+= con[k++];
	}
}

class ProjectorCalib: public FionaScene
{
public:
	int		targetProjector;
	int		targetPoint;
	std::vector<vec2>	s;
	std::vector<vec2>	d;
	
	void writeCalibration(void)
	{
#ifdef WIN32
		const char filename[] = "C:\\Documents and settings\\Administrator\\Desktop\\FionaConfig.txt";
#else
		const char filename[] = "/Users/joony/Desktop/FionaConfig.txt";
#endif
		std::ifstream ifs(filename);
		std::stringstream buffer;
		buffer<<ifs.rdbuf();
		std::string contents(buffer.str());
		ifs.close();
		
		std::ofstream ofs(filename);
		std::string word;
		int k=0;
		while(1)
		{
			int ret = getWord(contents,word,k);
			if(  word.compare("projCalib1x")==0
			   ||word.compare("projCalib1y")==0
			   ||word.compare("projCalib1z")==0
			   ||word.compare("projCalib2x")==0
			   ||word.compare("projCalib2y")==0
			   ||word.compare("projCalib2z")==0
			   ||word.compare("projCalib3x")==0
			   ||word.compare("projCalib3y")==0
			   ||word.compare("projCalib3z")==0
			   ||word.compare("projCalib4x")==0
			   ||word.compare("projCalib4y")==0
			   ||word.compare("projCalib4z")==0
			   )
			{
				getWord(contents,word,k);
				getWord(contents,word,k);
				getWord(contents,word,k);
				continue;
			}
			if( ret == 0 ) // End of File
				break;
			if( ret == 1 ) // add space
				ofs<<word<<" ";
			if( ret == 2 ) // add return
				ofs<<word<<"\n";
		}
		for( int i=0; i<4; i++ )
		{
			ofs<<"projCalib"<<(i+1)<<"x "<<fionaConf.projectorCalibx[i].x<<" "<<fionaConf.projectorCalibx[i].y<<" "<<fionaConf.projectorCalibx[i].z<<"\n";
			ofs<<"projCalib"<<(i+1)<<"y "<<fionaConf.projectorCaliby[i].x<<" "<<fionaConf.projectorCaliby[i].y<<" "<<fionaConf.projectorCaliby[i].z<<"\n";
			ofs<<"projCalib"<<(i+1)<<"z "<<fionaConf.projectorCalibz[i].x<<" "<<fionaConf.projectorCalibz[i].y<<" "<<fionaConf.projectorCalibz[i].z<<"\n";
		}
		ofs.close();
	}
	
	ProjectorCalib(void): FionaScene(), targetProjector(0), targetPoint(0)
	{
		s.resize(4);
		d.resize(4);
		d[0]=s[0]=vec2(-1,1);
		d[1]=s[1]=vec2(1,1);
		d[2]=s[2]=vec2(-1,-1);
		d[3]=s[3]=vec2(1,-1);
	}
	
	int	nodeType(void)
	{
		if( fionaConf.appType == FionaConfig::CAVE1 ) return 0;
		if( fionaConf.appType == FionaConfig::CAVE2 ) return 1;
		if( fionaConf.appType == FionaConfig::CAVE3 ) return 2;
		return -1;
	}
	
	void updateJoystick(const vec3& v)
	{
/*
		d[targetPoint] += vec2(v.x,v.z)*0.1f;
//		static Mat temp;
//		computeWarpRectToQuad(1,1,p[0].x,p[0].y,p[1].x,p[1].y,p[2].x,p[2].y,p[3].x,p[3].y,temp);

		tran temp = warpTrans(s,d);
//		fionaConf.caveCalibx[targetProjector] = vec3(temp[0],temp[1],temp[2]);
//		fionaConf.caveCaliby[targetProjector] = vec3(temp[3],temp[4],temp[5]);
//		fionaConf.caveCalibz[targetProjector] = vec3(temp[6],temp[7],temp[8]);
		fionaConf.caveCalibx[targetProjector] = vec3(temp.a00,temp.a01,temp.a03);
		fionaConf.caveCaliby[targetProjector] = vec3(temp.a10,temp.a11,temp.a13);
		fionaConf.caveCalibz[targetProjector] = vec3(temp.a30,temp.a31,temp.a33);
		
		std::cout<<caveDetailCalibration(1,1)<<std::endl;
*/
		printf("%f\n",v.x);
		float fa = 0.001f;
		int proj = targetProjector%4;
		int node = targetProjector/4;
		if( node != nodeType() ) return;
		switch(targetPoint)
		{
			case 0:
				fionaConf.projectorCalibx[proj].x-=v.x*fa/2;
				fionaConf.projectorCalibx[proj].z+=v.x*fa/2;
				fionaConf.projectorCaliby[proj].y-=v.z*fa/2;
				fionaConf.projectorCaliby[proj].z+=v.z*fa/2;
				break;
			case 1:
				fionaConf.projectorCalibx[proj].x+=v.x*fa/2;
				fionaConf.projectorCalibx[proj].z+=v.x*fa/2;
				fionaConf.projectorCaliby[proj].y+=v.z*fa/2;
				fionaConf.projectorCaliby[proj].z+=v.z*fa/2;
				break;
		}
		_FionaUTSyncProjectorCalib(fionaWinConf[0]);
	}
	
	virtual void buttons(int button,int state)
	{
		if( state!=1 ) return;
		switch(button)
		{
			case 1: targetProjector=targetProjector-1; break;
			case 0: targetProjector=targetProjector+1; break;
			case 2: targetPoint=0; break;
			case 3: targetPoint=1; break;
			case 5: 
			{
//				if(targetProjector/4 == nodeType())
				writeCalibration();
			} break;
		}
		if( targetProjector>=12 ) targetProjector-=12;
		if( targetProjector<0 ) targetProjector+=12;
	}
	void keyboard(unsigned int key, int x, int y)
	{
		switch(key)
		{
			case 'a': targetProjector=0; break;
			case 'q': targetProjector=1; break;
			case 's': targetProjector=2; break;
			case 'w': targetProjector=3; break;
			case 'z': targetPoint=0; break;
			case 'x': targetPoint=1; break;
		}
	}
	void render(void)
	{
		glBindTexture(GL_TEXTURE_2D,0);
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glLineWidth(3);
		
		glDisable(GL_LIGHTING);
		glColor4f(1,1,1,1);
		if( targetProjector/4 == nodeType())
		{
			glColor4f(1,1,0,1);
		}
		glLine(vec3(    0,   SZ,  -SZ),vec3(    0,  -SZ,  -SZ));
		glLine(vec3( SZ/2,   SZ,  -SZ),vec3( SZ/2,  -SZ,  -SZ));
		glLine(vec3(-SZ/2,   SZ,  -SZ),vec3(-SZ/2,  -SZ,  -SZ));
		glLine(vec3(  -SZ,    0,  -SZ),vec3(   SZ,    0,  -SZ));
		glLine(vec3(  -SZ, SZ/2,  -SZ),vec3(   SZ, SZ/2,  -SZ));
		glLine(vec3(  -SZ,-SZ/2,  -SZ),vec3(   SZ,-SZ/2,  -SZ));

		glLine(vec3(    0,   SZ,   SZ),vec3(    0,  -SZ,   SZ));
		glLine(vec3( SZ/2,   SZ,   SZ),vec3( SZ/2,  -SZ,   SZ));
		glLine(vec3(-SZ/2,   SZ,   SZ),vec3(-SZ/2,  -SZ,   SZ));
		glLine(vec3(  -SZ,    0,   SZ),vec3(   SZ,    0,   SZ));
		glLine(vec3(  -SZ, SZ/2,   SZ),vec3(   SZ, SZ/2,   SZ));
		glLine(vec3(  -SZ,-SZ/2,   SZ),vec3(   SZ,-SZ/2,   SZ));

		
		glLine(vec3(   SZ,   SZ,    0),vec3(   SZ,  -SZ,    0));
		glLine(vec3(   SZ,   SZ, SZ/2),vec3(   SZ,  -SZ, SZ/2));
		glLine(vec3(   SZ,   SZ,-SZ/2),vec3(   SZ,  -SZ,-SZ/2));
		glLine(vec3(   SZ,    0,  -SZ),vec3(   SZ,    0,   SZ));
		glLine(vec3(   SZ, SZ/2,  -SZ),vec3(   SZ, SZ/2,   SZ));
		glLine(vec3(   SZ,-SZ/2,  -SZ),vec3(   SZ,-SZ/2,   SZ));

		glLine(vec3(  -SZ,   SZ,    0),vec3(  -SZ,  -SZ,    0));
		glLine(vec3(  -SZ,   SZ, SZ/2),vec3(  -SZ,  -SZ, SZ/2));
		glLine(vec3(  -SZ,   SZ,-SZ/2),vec3(  -SZ,  -SZ,-SZ/2));
		glLine(vec3(  -SZ,    0,  -SZ),vec3(  -SZ,    0,   SZ));
		glLine(vec3(  -SZ, SZ/2,  -SZ),vec3(  -SZ, SZ/2,   SZ));
		glLine(vec3(  -SZ,-SZ/2,  -SZ),vec3(  -SZ,-SZ/2,   SZ));
		
		
		glLine(vec3(  -SZ,  -SZ,    0),vec3(   SZ,  -SZ,    0));
		glLine(vec3(  -SZ,  -SZ, SZ/2),vec3(   SZ,  -SZ, SZ/2));
		glLine(vec3(  -SZ,  -SZ,-SZ/2),vec3(   SZ,  -SZ,-SZ/2));
		glLine(vec3(    0,  -SZ,  -SZ),vec3(    0,  -SZ,   SZ));
		glLine(vec3( SZ/2,  -SZ,  -SZ),vec3( SZ/2,  -SZ,   SZ));
		glLine(vec3(-SZ/2,  -SZ,  -SZ),vec3(-SZ/2,  -SZ,   SZ));

		glLine(vec3(  -SZ,   SZ,    0),vec3(   SZ,   SZ,    0));
		glLine(vec3(  -SZ,   SZ, SZ/2),vec3(   SZ,   SZ, SZ/2));
		glLine(vec3(  -SZ,   SZ,-SZ/2),vec3(   SZ,   SZ,-SZ/2));
		glLine(vec3(    0,   SZ,  -SZ),vec3(    0,   SZ,   SZ));
		glLine(vec3( SZ/2,   SZ,  -SZ),vec3( SZ/2,   SZ,   SZ));
		glLine(vec3(-SZ/2,   SZ,  -SZ),vec3(-SZ/2,   SZ,   SZ));
		

		
		glLine(vec3(-SZ,-SZ,-SZ),vec3(-SZ,-SZ, SZ));
		glLine(vec3(-SZ, SZ,-SZ),vec3(-SZ, SZ, SZ));
		glLine(vec3( SZ,-SZ,-SZ),vec3( SZ,-SZ, SZ));
		glLine(vec3( SZ, SZ,-SZ),vec3( SZ, SZ, SZ));

		glLine(vec3(-SZ,-SZ,-SZ),vec3(-SZ, SZ,-SZ));
		glLine(vec3(-SZ,-SZ, SZ),vec3(-SZ, SZ, SZ));
		glLine(vec3( SZ,-SZ,-SZ),vec3( SZ, SZ,-SZ));
		glLine(vec3( SZ,-SZ, SZ),vec3( SZ, SZ, SZ));

		glLine(vec3(-SZ,-SZ,-SZ),vec3( SZ,-SZ,-SZ));
		glLine(vec3(-SZ,-SZ, SZ),vec3( SZ,-SZ, SZ));
		glLine(vec3(-SZ, SZ,-SZ),vec3( SZ, SZ,-SZ));
		glLine(vec3(-SZ, SZ, SZ),vec3( SZ, SZ, SZ));
	}
};