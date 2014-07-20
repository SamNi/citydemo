#ifndef _GL_H_
#define _GL_H_

extern "C" {
#include <GL/glew.h>
}
#ifdef _WIN32
#include <Windows.h>
#endif 
#include <GL/GL.h>
#include <GL/GLU.h>


#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/random.hpp>

#endif // ~_GL_H_