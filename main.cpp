#pragma comment(lib, "glew32.lib")

#include "glew.h"
#include "wglew.h"
#include "freeglut.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "math_3d.h"

#define WindowWidth 924
#define WindowHeight 600

using namespace std;
 
#define	NUM_VERTICES 5
#define	VERTEX_SIZE (5*sizeof(float))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

GLuint VBO;
GLuint IBO;
GLuint gWVPLoc;
GLuint gSampler;
GLuint program;
GLuint WorldMatrixLocation;
GLuint m_eyeWorldPosLocation;
GLuint m_matSpecularIntensityLocation;
GLuint m_matSpecularPowerLocation;

unsigned int Indi[] = {0, 3, 1, 1, 3, 2,  2, 3, 0,  0, 2, 1 };

struct lightLocation {
	GLuint Color;
	GLuint AmbinetIntensity;
	GLuint Direction;
	GLuint DiffuseIntensity;
};

struct DirectionalLight {
	Vector3f Color;
	float AmbientIntensity;
	Vector3f Direction;
	float DiffuseIntensity;
	float Intensity;
	float Power;
};	

struct Vertex {
	Vector3f m_pos;
	Vector2f m_tex;
	Vector3f m_normal;
	Vertex() {}
	Vertex(Vector3f pos, Vector2f tex) {
		m_pos = pos;
		m_tex = tex;
		m_normal = Vector3f(0.0f, 0.0f, 0.0f);
	}
};

DirectionalLight Light;
lightLocation m_dirLight; 
Vertex vert(Vector3f (0.0f, 0.0f, 0.0f), Vector2f (0.0f, 0.0f));

Vector3f m_target = Vector3f(0.0f, 0.0f, 1.0f);
Vector3f m_up = Vector3f(0.0f, 1.0f, 0.0f);
Vector3f m_os = Vector3f(0.0f, 0.0f, 0.0f);
Vector3f m_scale = Vector3f(1.0f, 1.0f, 1.0f);
Vector3f m_worldPos = Vector3f(0.0f, 0.0f, 0.0f);
Vector3f m_rotateInfo = Vector3f(0.0f, 0.0f, 0.0f);
Vector3f m_normal = Vector3f(0.0f, 0.0f, 0.0f);

const static float StepScale = 0.1f;

static const char* vCode = "                                                          \n\
#version 330                                                                        \n\
                                                                                    \n\
layout (location = 0) in vec3 Position;                                             \n\
layout (location = 1) in vec2 TexCoord;                                             \n\
layout (location = 2) in vec3 Normal;                                               \n\
                                                                                    \n\
uniform mat4 gWVP;                                                                  \n\
uniform mat4 gWorld;                                                                \n\
                                                                                    \n\
out vec4 Color;                                                                 \n\
out vec3 Normal0;                                                                   \n\
out vec3 WorldPos0;                                                               \n\
out vec2 TexCoord0;        \n\
                                                                                    \n\
void main()                                                                         \n\
{                                                                                   \n\
    gl_Position = gWVP * vec4(Position, 1.0);                                       \n\
	Color = vec4(1.0,0.0, 0.0,  1.0); \n\
	Normal0   = (gWorld * vec4(Normal, 0.0)).xyz;                                   \n\
    WorldPos0 = (gWorld * vec4(Position, 1.0)).xyz;   \n\
	TexCoord0 = TexCoord;        \n\
}";


static const char* fCode = "\n\
#version 330                                                                        \n\
                                                                                    \n\
in vec3 WorldPos0;\n\
in vec2 TexCoord0;                                                                  \n\
in vec4 Color;\n\
in vec3 Normal0;																	\n\
out vec4 FragColor;                                                                 \n\
uniform vec3 gEyeWorldPos;\n\
uniform float gMatSpecularIntensity;\n\
uniform float gSpecularPower;         \n\
															\n\
uniform sampler2D gSampler;                                                         \n\
struct DirectionalLight                                                             \n\
{                                                                                   \n\
    vec3 Color;                                                                     \n\
    float AmbientIntensity;															\n\
	float DiffuseIntensity;															\n\
	vec3 Direction;																	\n\
};                                                                                  \n\
                                                                                    \n\
uniform DirectionalLight gDirectionalLight;                                         \n\
                                                                                    \n\
void main()\n\
{\n\
	  vec4 AmbientColor = vec4(gDirectionalLight.Color, 1.0f) * gDirectionalLight.AmbientIntensity;\n\
	  vec3 LightDirection = -gDirectionalLight.Direction;\n\
	  vec3 Normal = normalize(Normal0);\n\
	  \n\
	  float DiffuseFactor = dot(Normal, LightDirection);\n\
	  \n\
	  vec4 DiffuseColor  = vec4(0, 0, 0, 0);\n\
	  vec4 SpecularColor = vec4(0, 0, 0, 0);\n\
	  \n\
	  if (DiffuseFactor > 0) {\n\
		DiffuseColor = vec4(gDirectionalLight.Color, 1.0f) *\n\
				gDirectionalLight.DiffuseIntensity *\n\
				DiffuseFactor;\n\
				\n\
		vec3 VertexToEye = normalize(gEyeWorldPos - WorldPos0);\n\
		vec3 LightReflect = normalize(reflect(gDirectionalLight.Direction, Normal));\n\
		float SpecularFactor = dot(VertexToEye, LightReflect);\n\
		SpecularFactor = pow(SpecularFactor, gSpecularPower);\n\
		if (SpecularFactor > 0) {\n\
			SpecularColor = vec4(gDirectionalLight.Color, 1.0f) \n\
					* gMatSpecularIntensity * \n\
					SpecularFactor;\n\
		}\n\
	  }\n\
	  \n\
	  FragColor = Color + AmbientColor + SpecularColor + DiffuseColor;  } \n\
	  ";

struct BaseLight {
	Vector3f Color;
	float AmbientIntensity;
	float DiffuseIntensity;
	BaseLight(){
		Color = Vector3f(0.1f, 0.1f, 0.1f);
		AmbientIntensity = 0.0f;
		DiffuseIntensity = 0.0f;
	}
};

struct PointLight : public BaseLight {
	Vector3f Position;
	struct{
		float Constant;
		float Linear;
		float Exp;
	} Attenuation;
	PointLight(){
		Position = Vector3f(0.0f, 0.0f, 0.0f);
		Attenuation.Constant = 1.0f;
		Attenuation.Linear = 0.0f;
		Attenuation.Exp = 0.0f;
	}
};

bool OnKeyboard(int Key) {
    bool Ret = false;
	
	switch (Key) {
	case GLUT_KEY_F10:{
		Light.AmbientIntensity -= 0.1;
		Ret = true;
		}
		break;
	case GLUT_KEY_F11:{
		Light.AmbientIntensity += 0.1;
		Ret = true;
		}
	case GLUT_KEY_F3:{
		Light.DiffuseIntensity -= 0.1;
		Ret = true;
		}
	case GLUT_KEY_F4:{
		Light.DiffuseIntensity += 0.1;
		Ret = true;
		}
		break;
	case GLUT_KEY_HOME:{
		Vector3f s = Vector3f(0.0f , sinf(StepScale * 5) , 0.0f);
		m_os += s;
		Ret = true;
		}
		break;
	case GLUT_KEY_END:{
		Vector3f s = Vector3f(0.0f , sinf(StepScale * 5) , 0.0f);
		m_os -= s;
		Ret = true;
		}
		break;
	case GLUT_KEY_PAGE_UP:{
		Vector3f s = Vector3f(sinf(StepScale * 5), 0.0f  , 0.0f);
		m_os += s;
		Ret = true;
		}
		break;
	case GLUT_KEY_PAGE_DOWN:{
		Vector3f s = Vector3f(sinf(StepScale * 5), 0.0f  , 0.0f);
		m_os -= s;
		Ret = true;
		}
		break;		   
    case GLUT_KEY_UP:{
            vert.m_pos += (m_target * StepScale);
            Ret = true;
        }
        break;
    case GLUT_KEY_DOWN:{
            vert.m_pos -= (m_target * StepScale);
            Ret = true;
        }
        break;
    case GLUT_KEY_LEFT:{
            Vector3f Right = m_target.Cross(m_up);
            Right.Normalize();
            Right *= StepScale;
            vert.m_pos += Right;
            Ret = true;
        }
        break;
    case GLUT_KEY_RIGHT:{
            Vector3f Left = m_up.Cross(m_target);
            Left.Normalize();
            Left *= StepScale;
            vert.m_pos += Left;
            Ret = true;
        }
        break;
    }

    return Ret;
}

void CalcNormals(const GLuint* pIndi, unsigned int IndexCount,Vertex* pVertices,  unsigned int VertexCount) {
	for (unsigned int i = 0 ; i < IndexCount ; i += 3) {
		unsigned int Index0 = pIndi[i];
		unsigned int Index1 = pIndi[i + 1];
		unsigned int Index2 = pIndi[i + 2];
		Vector3f v1 = pVertices[Index1].m_pos - pVertices[Index0].m_pos;
		Vector3f v2 = pVertices[Index2].m_pos - pVertices[Index0].m_pos;
		Vector3f Normal = v1.Cross(v2);
		Normal.Normalize();

		pVertices[Index0].m_normal += Normal;
		pVertices[Index1].m_normal += Normal;
		pVertices[Index2].m_normal += Normal;
	}

	for (unsigned int i = 0 ; i < VertexCount ; i++) {
		    pVertices[i].m_normal.Normalize();
	}
}

static void Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	m_worldPos = Vector3f(0.0f, 0.0f, 3.0f);
    Matrix4f ScaleTrans, RotateTrans, TranslationTrans, CameraTranslationTrans, CameraRotateTrans, PersProjTrans, m_transformation, World ;
	ScaleTrans.InitScaleTransform(m_scale);
    RotateTrans.InitRotateTransform(m_os);
    TranslationTrans.InitTranslationTransform(m_worldPos);
    CameraTranslationTrans.InitTranslationTransform(vert.m_pos);
    CameraRotateTrans.InitCameraTransform(m_target, m_up);
    PersProjTrans.InitPersProjTransform(60.0f, WindowWidth, WindowHeight, 1.0f, 100.0f);
	World = TranslationTrans * RotateTrans * ScaleTrans;
	m_transformation = PersProjTrans * CameraRotateTrans * CameraTranslationTrans * World ;

	glUniformMatrix4fv(WorldMatrixLocation, 1, GL_TRUE, (const GLfloat*)&World);
	glUniformMatrix4fv(gWVPLoc, 1, GL_TRUE, (const GLfloat*)&m_transformation);
	glUniform3f(m_dirLight.Color, 0.4, 0.4f, 0.4f);
	glUniform1f(m_dirLight.AmbinetIntensity, Light.AmbientIntensity);	
	Light.Direction = Vector3f(0.0f , 0.0f , -1.0f);
	Vector3f Direction = Light.Direction;
	Direction.Normalize();
	glUniform3f(m_dirLight.Direction, Direction.x, Direction.y, Direction.z);
	glUniform1f(m_dirLight.DiffuseIntensity, Light.DiffuseIntensity);
    glUniform1f(m_matSpecularIntensityLocation, Light.Intensity);
    glUniform1f(m_matSpecularPowerLocation, Light.Power);

    glUniform3f(m_eyeWorldPosLocation, vert.m_pos.x, vert.m_pos.y, vert.m_pos.z);
	
	glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);	
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)12);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)20);
 
	glDrawElements(GL_TRIANGLES, 15, GL_UNSIGNED_INT, NULL);
	
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glutSwapBuffers();
}

static void SpecialKeyboardCB(int Key, int x, int y) {
    OnKeyboard(Key);
}

void CreateVertexBuffer(const unsigned int* pIndi, unsigned int IndexCount) {
	Vertex Vertices[4] = { 
		Vertex(Vector3f(-1.0f, -1.0f, 0.5773f), Vector2f(0.0f, 0.0f)),
        Vertex(Vector3f(0.0f, -1.0f, -1.15475), Vector2f(0.5f, 0.0f)),
        Vertex(Vector3f(1.0f, -1.0f, 0.5773f),  Vector2f(1.0f, 0.0f)),
        Vertex(Vector3f(0.0f, 1.0f, 0.0f),      Vector2f(0.5f, 1.0f)) 
	};

    unsigned int VertexCount = ARRAY_SIZE_IN_ELEMENTS(Vertices);

    CalcNormals(pIndi, IndexCount, Vertices, VertexCount);
		
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
}

static void CreateIndexBuffer() {
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indi), Indi, GL_STATIC_DRAW);
}

GLuint loadShader(const char * source, GLenum type) {
	GLsizei * length = 0;
	GLchar infolog[1000];

	GLuint id = glCreateShader(type);
	GLint len = strlen(source);
    GLint compileStatus;

	glShaderSource(id, 1, &source, &len);
	glCompileShader(id);
	
	glGetShaderiv (id, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == 0){
		printf ("Error compiling shader.\n");
		glGetShaderInfoLog(id, NUM_VERTICES * VERTEX_SIZE ,length, infolog); 
		printf (infolog);
		return 0;
	}

    return id;
}

GLuint LoadShaderProgram() {
	GLsizei* length = 0;
	GLchar infolog[1000];

	GLuint program = glCreateProgram();
	GLuint vShader = loadShader(vCode, GL_VERTEX_SHADER);
	GLuint fShader = loadShader(fCode, GL_FRAGMENT_SHADER);

	glAttachShader(program, vShader); 
	glAttachShader(program, fShader);
	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked) ;

	if (linked ==  0) {
		glGetProgramInfoLog(program, NUM_VERTICES * VERTEX_SIZE ,length, infolog );
		printf(infolog);
	}
	
	glUseProgram(program);
	
    WorldMatrixLocation = glGetUniformLocation(program, "gWorld");

    gWVPLoc = glGetUniformLocation(program, "gWVP");
	gSampler = glGetUniformLocation(program, "gSampler");
	m_dirLight.Color = glGetUniformLocation ( program, "gDirectionalLight.Color");
	m_dirLight.AmbinetIntensity = glGetUniformLocation ( program, "gDirectionalLight.AmbientIntensity");

	m_dirLight.Direction = glGetUniformLocation ( program, "gDirectionalLight.Direction");
    m_dirLight.DiffuseIntensity =glGetUniformLocation ( program, "gDirectionalLight.DiffuseIntensity");
    m_eyeWorldPosLocation = glGetUniformLocation ( program,"gEyeWorldPos");
    m_matSpecularIntensityLocation = glGetUniformLocation ( program,"gMatSpecularIntensity");
    m_matSpecularPowerLocation = glGetUniformLocation ( program,"gSpecularPower");

    assert(gWVPLoc != 0xFFFFFFFF);

	return program;
}


int main(int argc, char** argv) {
	Light.AmbientIntensity = 0.5f;
    Light.DiffuseIntensity = 0.75f;
    Light.Direction = Vector3f(0.0f, 0.0, -1.0);
	Light.Intensity=1.0f;
	Light.Power = 32;

	glutInit(&argc, argv);
	glutInitContextVersion ( 3, 0 );
    glutInitContextProfile ( GLUT_CORE_PROFILE );
	glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Задание по КГ - Вероника Пестрецова");
	glutDisplayFunc(Render);
    glutIdleFunc(Render);
    glutSpecialFunc(SpecialKeyboardCB);
	
    GLenum res = glewInit();
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }

	glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
	glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
	CreateVertexBuffer(Indi, ARRAY_SIZE_IN_ELEMENTS(Indi));
	CreateIndexBuffer();
	
    LoadShaderProgram();

    glutMainLoop();
}
