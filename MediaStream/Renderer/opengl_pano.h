#pragma once
#include "../Common/media_defs.h"
#include "qt5_inc.h"
#include "shader.h"


#define ENABLE_VR360		1


/**
* @brief:
* Renderer for VR videos.
* 1. For flat plane;
* 2. For 180/360 view.
*
* Mat = model * view * projection
* view if fix, projection is fix, model is changing!
* notice the difference of frustum and perspective!
*/
class COpenGLPano : public QOpenGLWidget, public QOpenGLFunctions
{
	Q_OBJECT

public:
	COpenGLPano(QWidget *parent = Q_NULLPTR);
	virtual ~COpenGLPano();

	// override
	virtual void initializeGL();
	virtual void resizeGL(int w, int h);
	virtual void paintGL();
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void wheelEvent(QWheelEvent *event);

	bool set_format(uint32_t width, uint32_t height);
	bool send_data(uint32_t size, const uint8_t *data_ptr);

protected:
	void _init_vertexs();
	void _init_shaders();
	void _init_textures();
	void _reset_camera();
	QMatrix4x4 _get_model_matrix();
	QMatrix4x4 _get_view_matrix();
	QMatrix4x4 _get_projection_matrix();

protected:
	uint32_t _window_width;
	uint32_t _window_height;
	uint32_t _width;
	uint32_t _height;

	QOpenGLShaderProgram *_program_ptr;
	QOpenGLTexture *_texture_ptr;
	QOpenGLVertexArrayObject _VAO;
	QOpenGLBuffer _VBO;
	QOpenGLBuffer _EBO;
	int _matrix_location;
	uint32_t _vertice_count;

	mouse_param_t _mouse_param;
	camera_param_t _camera_param;

	QReadWriteLock _locker;
	uint32_t _buf_size;
	uint8_t *_buf_ptr;
};

