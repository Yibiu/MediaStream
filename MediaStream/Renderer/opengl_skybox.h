#pragma once
#include "../Common/media_defs.h"
#include "qt5_inc.h"
#include "shader.h"


/**
* @brief:
* CUBMAP for OpenGL skybox
*/
class COpenGLSkyBox : public QOpenGLWidget, public QOpenGLFunctions
{
	Q_OBJECT

public:
	COpenGLSkyBox(QWidget *parent = Q_NULLPTR);
	virtual ~COpenGLSkyBox();

	// override
	virtual void initializeGL();
	virtual void resizeGL(int w, int h);
	virtual void paintGL();
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void wheelEvent(QWheelEvent *event);

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

	QOpenGLShaderProgram *_program_ptr;
	QOpenGLTexture *_texture_ptr;
	QOpenGLVertexArrayObject _VAO;
	QOpenGLBuffer _VBO;
	int _matrix_location;

	mouse_param_t _mouse_param;
	camera_param_t _camera_param;

	QVector<QImage> _imgs; // {left, right, up, down, front, back}
};

