#pragma once
#include "../Common/media_defs.h"
#include "qt5_inc.h"
#include "shader.h"


/**
* @brief:
* Base class based on QWindow for OpenGL
*
* These functions must be called in the same thread!!!
*/
class CGLBaseWindow : public QWindow, public QOpenGLFunctions
{
	Q_OBJECT

public:
	CGLBaseWindow(QWindow *parent = Q_NULLPTR);
	virtual ~CGLBaseWindow();

	// For overrider
	virtual void initOpenGL();
	virtual void renderPaint(QPainter *painter);
	virtual void renderOpenGL(uint32_t size, const uint8_t *data_ptr);
	virtual void exitOpenGL();

public slots:
	void renderNow(uint32_t size, const uint8_t *data_ptr);
	void exitGL();

protected:
	QOpenGLContext *_context_ptr;
	QOpenGLPaintDevice *_paint_device_ptr;
};


/**
* @brief:
* Direct openGL paint on QWindow using QOpenGLContext inherit from CGLBaseWindow
*/
class COpenGLWindow : public CGLBaseWindow
{
	Q_OBJECT

public:
	COpenGLWindow(QWindow *parent = Q_NULLPTR);
	virtual ~COpenGLWindow();

	// CGLBaseWindow
	virtual void initOpenGL();
	virtual void renderPaint(QPainter *painter);
	virtual void renderOpenGL(uint32_t size, const uint8_t *data_ptr);
	virtual void exitOpenGL();

	bool set_format(uint32_t width, uint32_t height, video_fourcc_t fourcc);

protected:
	void _init_vertexs();
	void _init_shaders();
	void _init_textures();

protected:
	QOpenGLShaderProgram *_program_ptr;
	QOpenGLTexture *_tex_Y;
	QOpenGLTexture *_tex_U;
	QOpenGLTexture *_tex_V;
	QOpenGLVertexArrayObject _VAO;
	QOpenGLBuffer _VBO;
	QOpenGLBuffer _IBO;

	uint32_t _width;
	uint32_t _height;
	video_fourcc_t _fourcc;
};

