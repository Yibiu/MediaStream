#pragma once
#include "../Common/media_defs.h"
#include "qt5_inc.h"
#include "shader.h"


/**
* @brief:
* OpenGL of qt using QOpenGLWidget
*
* Update is in different thread!!!
*/
class COpenGLWidget : public QOpenGLWidget, public QOpenGLFunctions
{
	Q_OBJECT

public:
	COpenGLWidget(QWidget *parent = Q_NULLPTR);
	virtual ~COpenGLWidget();

	// override
	void initializeGL() Q_DECL_OVERRIDE;
	void resizeGL(int w, int h) Q_DECL_OVERRIDE;
	void paintGL() Q_DECL_OVERRIDE;

	bool set_format(uint32_t width, uint32_t height, video_fourcc_t fourcc);
	bool send_data(uint32_t size, const uint8_t *data_ptr);

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

	QReadWriteLock _locker;
	uint32_t _buf_size;
	uint8_t *_buf_ptr;
};

