#include "GLee.h"
#include <GLUT/glut.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <math.h>

using namespace std;

typedef struct {
        float x; float y; float z;
} POS;

typedef struct {
        GLfloat x; GLfloat y; GLfloat z;
} Vec3;

Vec3 cross(Vec3 a, Vec3 b)
{
	Vec3 tmp;
	
	tmp.x = a.y*b.z-b.y*a.z;
	tmp.y = b.x*a.z-a.x*b.z;
	tmp.z = a.x*b.y-b.x*a.y;
	
	return tmp;
}

double length(Vec3 v)
{
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

Vec3 normalize(Vec3 v)
{
	Vec3 tmp;
	double d;
	
	d=length(v);
	tmp.x = v.x/d;
	tmp.y = v.y/d;
	tmp.z = v.z/d;
	
	return tmp;
}

Vec3 getVertex(GLfloat _x,GLfloat _y,GLfloat _z)
{
	Vec3 tmp;
	tmp.x=_x;
	tmp.y=_y;
	tmp.z=_z;
	return tmp;
}

Vec3 subtract(Vec3 first,Vec3 second)
{
	Vec3 tmp;
	tmp.x=first.x-second.x;
	tmp.y=first.y-second.y;
	tmp.z=first.z-second.z;
	return tmp;
}

typedef struct {
	GLfloat r; GLfloat g; GLfloat b;
} COLOR;

typedef struct{
	GLfloat lightPos[4];
	GLfloat lightIntensity[4];
} PointLight;

typedef struct{
	GLfloat _ambient[4];
	GLfloat _diffuse[4];
	GLfloat _specular[4];
	GLfloat _specExp[1];
} Material;

typedef struct {
	GLfloat transCo[3];
}Translate;

typedef struct 
{	GLfloat parameter[3];
}Scale;

typedef struct 
{	GLfloat angle[1];
	GLfloat axis[3];
}Rotate;
typedef struct 
{
	char Type;
	int id;
}Transform;

typedef struct 
{
	GLuint vertexIndex[3];
}Triangle;

typedef struct 
{
	GLuint type;
	GLuint materialID;
	GLuint transformcount;
	GLuint triangleCount;
	std::vector<Transform> _transformation;
	std::vector<Triangle> _triangles;
}Mesh;

typedef struct{
	float mEl[4][4];
}Matrix;

Matrix setMatrix(float a, float b, float c, float d, float e, float f, float g, float h,
           float i, float j, float k, float l, float m, float n, float o, float p) {
		Matrix tmp;
        tmp.mEl[0][0] = a;
        tmp.mEl[0][1] = b;
        tmp.mEl[0][2] = c;
        tmp.mEl[0][3] = d;
        tmp.mEl[1][0] = e;
        tmp.mEl[1][1] = f;
        tmp.mEl[1][2] = g;
        tmp.mEl[1][3] = h;
        tmp.mEl[2][0] = i;
        tmp.mEl[2][1] = j;
        tmp.mEl[2][2] = k;
        tmp.mEl[2][3] = l;
        tmp.mEl[3][0] = m;
        tmp.mEl[3][1] = n;
        tmp.mEl[3][2] = o;
        tmp.mEl[3][3] = p;
        return tmp;
}
Matrix getRotation(float angle,Vec3 ax)
{		Matrix tmp;
		float theta=angle*M_PI/180;
		float cos=cosf(theta);
    	float sin=sinf(theta);
    	float k=1-cos;
    	Vec3 axis=normalize(ax);
    	Matrix result=setMatrix(cos + axis.x*axis.x*k, axis.x*axis.y*k - axis.z*sin, axis.x*axis.z*k + axis.y*sin, 0.0f,
              axis.y*axis.x*k + axis.z*sin, cos + axis.y*axis.y*k, axis.y*axis.z*k - axis.x*sin, 0.0f,
              axis.z*axis.x*k - axis.y*sin, axis.z*axis.y*k + axis.x*sin, cos + axis.z*axis.z*k, 0.0f,
              0.0f, 0.0f, 0.0f, 1.0f);
    	return result;
}

Vec3 matrixVec(Vec3 &src, Matrix tmp)
{
	float a, b, c;

        a = src.x * tmp.mEl[0][0] + src.y * tmp.mEl[0][1] + src.z * tmp.mEl[0][2];
        b = src.x * tmp.mEl[1][0] + src.y * tmp.mEl[1][1] + src.z * tmp.mEl[1][2];
        c = src.x * tmp.mEl[2][0] + src.y * tmp.mEl[2][1] + src.z * tmp.mEl[2][2];
    Vec3 neww;
    neww.x=a;
    neww.y=b;
    neww.z=c;
    return neww;
}

//GLOBALS
//utility
Vec3 _zero=getVertex(0,0,0);
Vec3 _yaxis=getVertex(0,1,0);
Matrix roMa;
//camera
POS camPos;
Vec3 _gaze;
Vec3 _up;
Vec3 _left;
Vec3 _forward;
//imageplane
float _imageLeft;
float _imageRight;
float _imageBottom;
float _imageTop;
float _imageNear;
float _imageFar;
int _imageWidth;
int _imageHeight;
//scene
COLOR backgroundColor;
GLfloat ambientlight[4];

int lightCount;
int matCount;
int translateCount;
int scaleCount;
int rotateCount;
int numOfVertices;
int numOfMeshes;

PointLight * _pointLights;
Material * materials;
Translate * _translations;
Scale * _scalings;
Rotate * _rotations;
std::vector<Mesh> _meshes;

GLfloat *vertexData;
GLfloat *normalData;
GLuint *indexData;
int *averager;

//light enable NOT NECESSARY
int l0_enable=1;
int l1_enable=1;
int l2_enable=1;
int l3_enable=1;

//GPU Descriptors
GLuint vboVertex;
GLuint vboNormal;
GLuint vboIndex;

void parseFiles(int argc, char **argv)
{	
	FILE * sceneFile;
	FILE * camFile;
	int counter;
	char dummy[100];
	int dummyindex;

	sceneFile = fopen(argv[1],"r");
	camFile=fopen(argv[2],"r");
	//Read Camera
	fscanf(camFile,"%f %f %f",&camPos.x,&camPos.y,&camPos.z);
	fscanf(camFile,"%f %f %f",&_gaze.x,&_gaze.y,&_gaze.z);
	fscanf(camFile,"%f %f %f",&_up.x,&_up.y,&_up.z);
	fscanf(camFile,"%f %f %f %f %f %f %d %d",&_imageLeft,&_imageRight,&_imageBottom,&_imageTop,&_imageNear,&_imageFar,&_imageWidth,&_imageHeight);

	_forward=subtract(_zero,_gaze);
	_left=normalize(cross(_up,_forward));
	
	fclose(camFile);
	//Read Scene
	fscanf(sceneFile,"%f %f %f",&backgroundColor.r,&backgroundColor.g,&backgroundColor.b);
	fscanf(sceneFile,"%f %f %f",&ambientlight[0],&ambientlight[1],&ambientlight[2]);
	ambientlight[3]=1;

	
	fscanf(sceneFile,"%d",&lightCount); //Number of point light sources
	_pointLights=new PointLight[lightCount];
	for(int i=0;i<lightCount;i++)
	{	fscanf(sceneFile,"%s %d",dummy,&dummyindex);
		fscanf(sceneFile,"%f %f %f",&_pointLights[i].lightPos[0],&_pointLights[i].lightPos[1],&_pointLights[i].lightPos[2]);
		fscanf(sceneFile,"%f %f %f",&_pointLights[i].lightIntensity[0],&_pointLights[i].lightIntensity[1],&_pointLights[i].lightIntensity[2]);
		_pointLights[i].lightPos[3]=1;
		_pointLights[i].lightIntensity[3]=1;
		memset(dummy,0,100);
	}
	fscanf(sceneFile,"%d",&matCount);//Number of materials
	materials=new Material[matCount];
	for(int i=0;i<matCount;i++)
	{	fscanf(sceneFile,"%s %d",dummy,&dummyindex);
		fscanf(sceneFile,"%f %f %f",&materials[i]._ambient[0],&materials[i]._ambient[1],&materials[i]._ambient[2]);
		fscanf(sceneFile,"%f %f %f",&materials[i]._diffuse[0],&materials[i]._diffuse[1],&materials[i]._diffuse[2]);
		fscanf(sceneFile,"%f %f %f %f",&materials[i]._specular[0],&materials[i]._specular[1],&materials[i]._specular[2],&materials[i]._specExp[0]);
		materials[i]._ambient[3]=1;
		materials[i]._diffuse[3]=1;
		materials[i]._specExp[3]=1;
		memset(dummy,0,100);
	}

	fscanf(sceneFile,"%s",dummy); // Translations                                
	fscanf(sceneFile,"%d",&translateCount);
	_translations=new Translate[translateCount];
	for(int i=0;i<translateCount;i++)
	{
		fscanf(sceneFile,"%f %f %f",&_translations[i].transCo[0],&_translations[i].transCo[1],&_translations[i].transCo[2]);
	}

	fscanf(sceneFile,"%s",dummy); // Scalings                                
	fscanf(sceneFile,"%d",&scaleCount);
	_scalings=new Scale[scaleCount];
	for(int i=0;i<scaleCount;i++)
	{
		fscanf(sceneFile,"%f %f %f",&_scalings[i].parameter[0],&_scalings[i].parameter[1],&_scalings[i].parameter[2]);
	}
	fscanf(sceneFile,"%s",dummy); // Rotations                                
	fscanf(sceneFile,"%d",&rotateCount);
	_rotations=new Rotate[rotateCount];
	for(int i=0;i<rotateCount;i++)
	{
		fscanf(sceneFile,"%f %f %f %f",&_rotations[i].angle[0],&_rotations[i].axis[0],&_rotations[i].axis[1],&_rotations[i].axis[2]);
	}
	memset(dummy,0,100);
	fscanf(sceneFile,"%d",&numOfVertices);	//Number of Vertices
	fscanf(sceneFile,"%s",dummy); 

	vertexData = (GLfloat*)malloc(sizeof(GLfloat)*numOfVertices*3);
	normalData= (GLfloat*)malloc(sizeof(GLfloat)*numOfVertices*3);
	averager=(int*)calloc(numOfVertices*3,sizeof(int));
	for (int i=0;i<numOfVertices;i++)
   	{
		fscanf(sceneFile,"%f %f %f ",&(vertexData[3*i]),&(vertexData[3*i+1]),&(vertexData[3*i+2]));
		
	}
	fscanf(sceneFile,"%d",&numOfMeshes);	//Number of Meshes
	for(int i=0;i<numOfMeshes;i++)
	{	fscanf(sceneFile,"%s %d",dummy,&dummyindex);
		
		Mesh newMesh;
		fscanf(sceneFile,"%d",&newMesh.type);
		fscanf(sceneFile,"%d",&newMesh.materialID);
		fscanf(sceneFile,"%d",&newMesh.transformcount);

		for(int j=0;j<newMesh.transformcount;j++)
		{
			Transform t;
			fscanf(sceneFile,"%s %d",&t.Type,&t.id);
			newMesh._transformation.push_back(t);
		}
		fscanf(sceneFile,"%d",&newMesh.triangleCount);
		
		for(int j=0;j<newMesh.triangleCount;j++)
		{
			Triangle tri;
			fscanf(sceneFile,"%d %d %d",&tri.vertexIndex[0],&tri.vertexIndex[1],&tri.vertexIndex[2]);
			Vec3 a=getVertex(vertexData[(tri.vertexIndex[0]-1)*3],vertexData[(tri.vertexIndex[0]-1)*3+1],vertexData[(tri.vertexIndex[0]-1)*3+2]);
			Vec3 b=getVertex(vertexData[(tri.vertexIndex[1]-1)*3],vertexData[(tri.vertexIndex[1]-1)*3+1],vertexData[(tri.vertexIndex[1]-1)*3+2]);
			Vec3 c=getVertex(vertexData[(tri.vertexIndex[2]-1)*3],vertexData[(tri.vertexIndex[2]-1)*3+1],vertexData[(tri.vertexIndex[2]-1)*3+2]);
			//Normal Calculations
			Vec3 e1=subtract(b,a);
			Vec3 e2=subtract(c,a);
			Vec3 prenormal=cross(e1,e2);
			Vec3 normal=normalize(prenormal);

			normalData[3*(tri.vertexIndex[0]-1)]+=normal.x;
			normalData[3*(tri.vertexIndex[0]-1)+1]+=normal.y;
			normalData[3*(tri.vertexIndex[0]-1)+2]+=normal.z;

			normalData[3*(tri.vertexIndex[1]-1)]+=normal.x;
			normalData[3*(tri.vertexIndex[1]-1)+1]+=normal.y;
			normalData[3*(tri.vertexIndex[1]-1)+2]+=normal.z;

			normalData[3*(tri.vertexIndex[2]-1)]+=normal.x;
			normalData[3*(tri.vertexIndex[2]-1)+1]+=normal.y;
			normalData[3*(tri.vertexIndex[2]-1)+2]+=normal.z;

			//How many triangles related those vertices
			averager[3*(tri.vertexIndex[0]-1)]++;
			averager[3*(tri.vertexIndex[0]-1)+1]++;
			averager[3*(tri.vertexIndex[0]-1)+2]++;

			averager[3*(tri.vertexIndex[1]-1)]++;
			averager[3*(tri.vertexIndex[1]-1)+1]++;
			averager[3*(tri.vertexIndex[1]-1)+2]++;

			averager[3*(tri.vertexIndex[2]-1)]++;
			averager[3*(tri.vertexIndex[2]-1)+1]++;
			averager[3*(tri.vertexIndex[2]-1)+2]++;

			newMesh._triangles.push_back(tri);
		}
		_meshes.push_back(newMesh);
		
		memset(dummy,0,100);
	}
	//Take average of normals
	for(int i=0;i<numOfVertices;i++)
	{	
		normalData[3*i]=normalData[3*i]/averager[3*i];
		normalData[(3*i)+1]=normalData[3*i+1]/averager[3*i+1];
		normalData[(3*i)+2]=normalData[3*i+2]/averager[3*i+2];


	}
	fclose(sceneFile);	
}

void draw()
{	
	
	for(int i=0;i<numOfMeshes;i++)
	{	Mesh currentMesh=_meshes[i];
		
		if(currentMesh.type==2)
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		else
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		int countofTriangle=currentMesh.triangleCount;

		indexData=(GLuint*) malloc(sizeof(GLuint)*3*countofTriangle);
		
		for(int j=0;j<countofTriangle;j++)
		{
			Triangle currentTriangle=currentMesh._triangles[j];
			indexData[j*3]=currentTriangle.vertexIndex[0]-1;
			indexData[j*3+1]=currentTriangle.vertexIndex[1]-1;
			indexData[j*3+2]=currentTriangle.vertexIndex[2]-1;
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vboIndex);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, countofTriangle * 3 * sizeof(GLuint), indexData, GL_STATIC_DRAW);

		glPushMatrix();
		for(int k=currentMesh.transformcount-1;k>=0;k--)
		{	
			Transform theTransform=currentMesh._transformation[k];
			int traID=theTransform.id;
			
			if(theTransform.Type=='r')
			{ 	
				glRotatef(_rotations[traID-1].angle[0],_rotations[traID-1].axis[0],_rotations[traID-1].axis[1],_rotations[traID-1].axis[2]);
				
			}
			else if(theTransform.Type=='s')
			{
				glScalef(_scalings[traID-1].parameter[0],_scalings[traID-1].parameter[1],_scalings[traID-1].parameter[2]);
				
			}
			else if(theTransform.Type=='t')
			{
				glTranslatef(_translations[traID-1].transCo[0],_translations[traID-1].transCo[1],_translations[traID-1].transCo[2]);	
			}


		}
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,materials[currentMesh.materialID-1]._ambient);
		glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,materials[currentMesh.materialID-1]._diffuse);
		glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,materials[currentMesh.materialID-1]._specular);
		glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,materials[currentMesh.materialID-1]._specExp);
		glDrawElements(GL_TRIANGLES,countofTriangle*3 , GL_UNSIGNED_INT,0);
		glPopMatrix();
		free(indexData);
	}
}


void initScene()
{	
	//bind vertex data
	glGenBuffers(1, &vboVertex);
	glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
	glBufferData(GL_ARRAY_BUFFER, numOfVertices * 3 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);
	//free(vertexData);

	//bind normal data
	glGenBuffers(1, &vboNormal);
	glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
	glBufferData(GL_ARRAY_BUFFER, numOfVertices * 3 * sizeof(GLfloat), normalData, GL_STATIC_DRAW);
	//free(normalData);

	glGenBuffers(1,&vboIndex);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
}

void keyboard(unsigned char key, int x, int y)
{
    switch(key) 
    {
        case 27:     // Escape
            exit(0); 
            break;
        case 'w':	//Go forward in the current gaze direction 0.05 units. The gaze vector is unchanged.
        	camPos.x+=0.05f*_gaze.x;
        	camPos.y+=0.05f*_gaze.y;
        	camPos.z+=0.05f*_gaze.z;
        	break; 
        case 's':	//Go backward in the current gaze direction 0.05 units. The gaze vector is unchanged.
        	camPos.x-=0.05f*_gaze.x;
        	camPos.y-=0.05f*_gaze.y;
        	camPos.z-=0.05f*_gaze.z;
        	break;
        case 'a':	//Look Left
        	roMa=getRotation(0.5,_yaxis);
        	_gaze=matrixVec(_gaze,roMa);
        	_up=matrixVec(_up,roMa);
        	_forward=subtract(_zero,_gaze);
			_left=normalize(cross(_up,_forward));
        
        	break;
        case 'd':	//Look Right
        	roMa=getRotation(-0.5,_yaxis);
        	_gaze=matrixVec(_gaze,roMa);
        	_up=matrixVec(_up,roMa);
        	_forward=subtract(_zero,_gaze);
			_left=normalize(cross(_up,_forward));
        	
        	break;
        case 'u':	//Look Up
        	
        	roMa=getRotation(0.5,_left);
        	_gaze=matrixVec(_gaze,roMa);
        	_up=matrixVec(_up,roMa);
        	_forward=subtract(_zero,_gaze);
			_left=normalize(cross(_up,_forward));
        	break;
        case 'j':	//Look Down
        	roMa=getRotation(-0.5,_left);
        	_gaze=matrixVec(_gaze,roMa);
        	_up=matrixVec(_up,roMa);
        	_forward=subtract(_zero,_gaze);
			_left=normalize(cross(_up,_forward));
        	break;
        case '0':	//Light Switch 0
        	if(l0_enable)
        		l0_enable=0;
        	else
        		l0_enable=1;
            break;
        case '1':	//Light Switch 1
        	if(l1_enable)
        		l1_enable=0;
        	else
        		l1_enable=1;
            break;
        case '2':	//Light Switch 2
        	if(l2_enable)
        		l2_enable=0;
        	else
        		l2_enable=1;
            break;
        default:  
            break;
    }

    glutPostRedisplay();
}
void renderScene()
{	cout<<"display"<<endl;
	glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER,vboVertex);
	glVertexPointer(3,GL_FLOAT,0,0);

    glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
    glNormalPointer(GL_FLOAT,0,0);
	
	glClearColor(backgroundColor.r,backgroundColor.g,backgroundColor.b,1.0);	//set background color
	glClearDepth(1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glEnable(GL_DEPTH_TEST);	//backface culling 
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

	//<set viewport and projection parameters>
   	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(_imageLeft,_imageRight,_imageBottom,_imageTop,_imageNear,_imageFar);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camPos.x,camPos.y,camPos.z, _gaze.x+camPos.x, _gaze.y+camPos.y,_gaze.z+camPos.z, _up.x,_up.y,_up.z); //// ???

    //<set light position using the glLightfv function>//
    glEnable(GL_LIGHTING);
	if(lightCount>0)
	{	
		glLightfv(GL_LIGHT0,GL_POSITION,_pointLights[0].lightPos);
		glLightfv(GL_LIGHT0,GL_DIFFUSE,_pointLights[0].lightIntensity);
		glLightfv(GL_LIGHT0,GL_SPECULAR,_pointLights[0].lightIntensity);
		glLightfv(GL_LIGHT0,GL_AMBIENT,ambientlight);
		if(l0_enable)
			glEnable(GL_LIGHT0);
		else
			glDisable(GL_LIGHT0);
	}
	if(lightCount>1)
	{	
		glLightfv(GL_LIGHT1,GL_POSITION,_pointLights[1].lightPos);
		glLightfv(GL_LIGHT1,GL_DIFFUSE,_pointLights[1].lightIntensity);
		glLightfv(GL_LIGHT1,GL_SPECULAR,_pointLights[1].lightIntensity);
		glLightfv(GL_LIGHT1,GL_AMBIENT,ambientlight);
		if(l1_enable)
			glEnable(GL_LIGHT1);
		else
			glDisable(GL_LIGHT1);
	}
	if(lightCount>2)
	{	
		glLightfv(GL_LIGHT2,GL_POSITION,_pointLights[2].lightPos);
		glLightfv(GL_LIGHT2,GL_DIFFUSE,_pointLights[2].lightIntensity);
		glLightfv(GL_LIGHT2,GL_SPECULAR,_pointLights[2].lightIntensity);
		glLightfv(GL_LIGHT2,GL_AMBIENT,ambientlight);
		if(l2_enable)
			glEnable(GL_LIGHT2);
		else
			glDisable(GL_LIGHT2);
	}
	if(lightCount>3)
	{	
		glLightfv(GL_LIGHT3,GL_POSITION,_pointLights[3].lightPos);
		glLightfv(GL_LIGHT3,GL_DIFFUSE,_pointLights[3].lightIntensity);
		glLightfv(GL_LIGHT3,GL_SPECULAR,_pointLights[3].lightIntensity);
		glLightfv(GL_LIGHT3,GL_AMBIENT,ambientlight);
		glEnable(GL_LIGHT3);
		if(l3_enable)
			glEnable(GL_LIGHT3);
		else
			glDisable(GL_LIGHT3);
	}
	if(lightCount>4)
	{	
		glLightfv(GL_LIGHT4,GL_POSITION,_pointLights[4].lightPos);
		glLightfv(GL_LIGHT4,GL_DIFFUSE,_pointLights[4].lightIntensity);
		glLightfv(GL_LIGHT4,GL_SPECULAR,_pointLights[4].lightIntensity);
		glLightfv(GL_LIGHT4,GL_AMBIENT,ambientlight);
		glEnable(GL_LIGHT4);
	}
	if(lightCount>5)
	{	
		glLightfv(GL_LIGHT5,GL_POSITION,_pointLights[5].lightPos);
		glLightfv(GL_LIGHT5,GL_DIFFUSE,_pointLights[5].lightIntensity);
		glLightfv(GL_LIGHT5,GL_SPECULAR,_pointLights[5].lightIntensity);
		glLightfv(GL_LIGHT5,GL_AMBIENT,ambientlight);
		glEnable(GL_LIGHT5);
	}
	if(lightCount>6)
	{	
		glLightfv(GL_LIGHT6,GL_POSITION,_pointLights[6].lightPos);
		glLightfv(GL_LIGHT6,GL_DIFFUSE,_pointLights[6].lightIntensity);
		glLightfv(GL_LIGHT6,GL_SPECULAR,_pointLights[6].lightIntensity);
		glLightfv(GL_LIGHT6,GL_AMBIENT,ambientlight);
		glEnable(GL_LIGHT6);
	}
	if(lightCount>7)
	{	
		glLightfv(GL_LIGHT7,GL_POSITION,_pointLights[7].lightPos);
		glLightfv(GL_LIGHT7,GL_DIFFUSE,_pointLights[7].lightIntensity);
		glLightfv(GL_LIGHT7,GL_SPECULAR,_pointLights[7].lightIntensity);
		glLightfv(GL_LIGHT7,GL_AMBIENT,ambientlight);
		glEnable(GL_LIGHT7);
	}
	//<apply modeling transformations and draw models>//
	draw();
	glutSwapBuffers();
}

int main(int argc, char **argv)
{
	parseFiles(argc,argv);
	glutInit(&argc,argv);
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);     // Set display mode
   	glutInitWindowSize(_imageWidth,_imageHeight);
   	glutCreateWindow("CENG477 - HW4");
   	initScene();
   	glutDisplayFunc(renderScene);
   	glutKeyboardFunc(keyboard);
   	glutMainLoop();

}