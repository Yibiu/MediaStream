#include "opengl_skybox.h"


COpenGLSkyBox::COpenGLSkyBox(QWidget *parent)
	: QOpenGLWidget(parent)
{
	_window_width = 0;
	_window_height = 0;

	_program_ptr = NULL;
	_texture_ptr = NULL;
	_matrix_location = 0;

	memset(&_mouse_param, 0x00, sizeof(_mouse_param));
	memset(&_camera_param, 0x00, sizeof(_camera_param));
}

COpenGLSkyBox::~COpenGLSkyBox()
{
	makeCurrent();
	if (NULL != _program_ptr) {
		delete _program_ptr;
		_program_ptr = NULL;
	}
	if (NULL != _texture_ptr) {
		delete _texture_ptr;
		_texture_ptr = NULL;
	}
	_VAO.destroy();
	_VBO.destroy();
	doneCurrent();
}

void COpenGLSkyBox::initializeGL()
{
	initializeOpenGLFunctions();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	_init_shaders();
	_init_vertexs();
	_init_textures();
	_reset_camera();

	QString img_resources[] = {
		":/VLCPlayer/Resources/images/wall.jpg",
		":/VLCPlayer/Resources/images/wall.jpg",
		":/VLCPlayer/Resources/images/wall.jpg",
		":/VLCPlayer/Resources/images/wall.jpg",
		":/VLCPlayer/Resources/images/wall.jpg",
		":/VLCPlayer/Resources/images/wall.jpg"
	};
	_imgs.resize(6);
	for (int i = 0; i < 6; i++) {
		_imgs[i].load(img_resources[i]);
	}
}

void COpenGLSkyBox::resizeGL(int w, int h)
{
	_window_width = w;
	_window_height = h;
	glViewport(0, 0, w, h);
}

void COpenGLSkyBox::paintGL()
{
	glClearColor(0.678, 0.847, 0.902, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	_texture_ptr->bind();
	_program_ptr->bind();
	QMatrix4x4 matrix;
	matrix = _get_projection_matrix() * _get_view_matrix() * _get_model_matrix();
	_program_ptr->setUniformValue("matrix", matrix);
	glUniform1i(_program_ptr->uniformLocation("cube_sampler"), 0);
	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, _imgs[i].width(), _imgs[i].height(),
			0, GL_BGRA, GL_UNSIGNED_BYTE, _imgs[i].bits());
	}
	QOpenGLVertexArrayObject::Binder vao_binder(&_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);

	_program_ptr->release();
	_texture_ptr->release();
}

void COpenGLSkyBox::mousePressEvent(QMouseEvent *event)
{
	if (Qt::LeftButton == event->button()) {
		_mouse_param.pressed = true;
		_mouse_param.cur_pos = QVector2D(event->x(), event->y());
		_mouse_param.last_pos = _mouse_param.cur_pos;
	}

	QWidget::mousePressEvent(event);
}

void COpenGLSkyBox::mouseMoveEvent(QMouseEvent *event)
{
	if (_mouse_param.pressed) {
		float dx = _mouse_param.cur_pos.x() - event->x();
		float dy = _mouse_param.cur_pos.y() - event->y();
		_mouse_param.last_pos = _mouse_param.cur_pos;
		_mouse_param.cur_pos = QVector2D(event->x(), event->y());

		_camera_param.rotate_H += dx / 8.0f;
		_camera_param.rotate_V += dy / 4.0f;
		update();
	}

	QWidget::mouseMoveEvent(event);
}

void COpenGLSkyBox::mouseReleaseEvent(QMouseEvent *event)
{
	_mouse_param.pressed = false;

	QWidget::mouseReleaseEvent(event);
}

void COpenGLSkyBox::wheelEvent(QWheelEvent *event)
{
	// fov: [0.04 0.8] step 0.02
	// [0.04 0.08] step 0.01
	// [0.08 0.8] step 0.03
	//
	if (event->delta() > 0) {
		//_camera_param.fov = qMin(160.0f, _camera_param.fov + 5.0f);

		if (_camera_param.fov >= 0.04f && _camera_param.fov <= 0.08f) {
			_camera_param.fov = qMin(0.8f, _camera_param.fov + 0.01f);
		}
		else if (_camera_param.fov >= 0.08f && _camera_param.fov <= 0.8) {
			_camera_param.fov = qMin(0.3f, _camera_param.fov + 0.03f);
		}
		else {
			_camera_param.fov = qMin(0.8f - 0.01f, _camera_param.fov);
			_camera_param.fov = qMax(0.04f + 0.01f, _camera_param.fov);
		}
	}
	else {
		//_camera_param.fov = qMax(20.0f, _camera_param.fov - 5.0f);

		if (_camera_param.fov >= 0.04f && _camera_param.fov <= 0.08f) {
			_camera_param.fov = qMax(0.04f, _camera_param.fov - 0.01f);
		}
		else if (_camera_param.fov >= 0.08f && _camera_param.fov <= 0.8) {
			_camera_param.fov = qMax(0.04f, _camera_param.fov - 0.03f);
		}
		else {
			_camera_param.fov = qMin(0.8f - 0.01f, _camera_param.fov);
			_camera_param.fov = qMax(0.04f + 0.01f, _camera_param.fov);
		}
	}

	update();
	QWidget::wheelEvent(event);
}


void COpenGLSkyBox::_init_shaders()
{
	_program_ptr = new QOpenGLShaderProgram;
	_program_ptr->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source_skybox);
	_program_ptr->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source_skybox);
	_program_ptr->bindAttributeLocation("pos_coords", 0);
	_matrix_location = _program_ptr->uniformLocation("matrix");
	_program_ptr->link();
}

void COpenGLSkyBox::_init_vertexs()
{
	QVector3D vertices[] = {
		QVector3D(-1.0f, 1.0f, -1.0f),
		QVector3D(-1.0f, -1.0f, -1.0f),
		QVector3D(1.0f, -1.0f, -1.0f),
		QVector3D(1.0f, -1.0f, -1.0f),
		QVector3D(1.0f, 1.0f, -1.0f),
		QVector3D(-1.0f, 1.0f, -1.0f),

		QVector3D(-1.0f, -1.0f, 1.0f),
		QVector3D(-1.0f, -1.0f, -1.0f),
		QVector3D(-1.0f, 1.0f, -1.0f),
		QVector3D(-1.0f, 1.0f, -1.0f),
		QVector3D(-1.0f, 1.0f, 1.0f),
		QVector3D(-1.0f, -1.0f, 1.0f),

		QVector3D(1.0f, -1.0f, -1.0f),
		QVector3D(1.0f, -1.0f, 1.0f),
		QVector3D(1.0f, 1.0f, 1.0f),
		QVector3D(1.0f, 1.0f, 1.0f),
		QVector3D(1.0f, 1.0f, -1.0f),
		QVector3D(1.0f, -1.0f, -1.0f),

		QVector3D(-1.0f, -1.0f, 1.0f),
		QVector3D(-1.0f, 1.0f, 1.0f),
		QVector3D(1.0f, 1.0f, 1.0f),
		QVector3D(1.0f, 1.0f, 1.0f),
		QVector3D(1.0f, -1.0f, 1.0f),
		QVector3D(-1.0f, -1.0f, 1.0f),

		QVector3D(-1.0f, 1.0f, -1.0f),
		QVector3D(1.0f, 1.0f, -1.0f),
		QVector3D(1.0f, 1.0f, 1.0f),
		QVector3D(1.0f, 1.0f, 1.0f),
		QVector3D(-1.0f, 1.0f, 1.0f),
		QVector3D(-1.0f, 1.0f, -1.0f),

		QVector3D(-1.0f, -1.0f, -1.0f),
		QVector3D(-1.0f, -1.0f, 1.0f),
		QVector3D(1.0f, -1.0f, -1.0f),
		QVector3D(1.0f, -1.0f, -1.0f),
		QVector3D(-1.0f, -1.0f, 1.0f),
		QVector3D(1.0f, -1.0f, 1.0f),
	};

	_VAO.create();
	QOpenGLVertexArrayObject::Binder vao_binder(&_VAO);
	_VBO.create();
	_VBO.bind();
	_VBO.allocate(vertices, 36 * sizeof(QVector3D));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
}

void COpenGLSkyBox::_init_textures()
{
	_texture_ptr = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
	_texture_ptr->setMinificationFilter(QOpenGLTexture::Nearest);
	_texture_ptr->setMagnificationFilter(QOpenGLTexture::Linear);
	_texture_ptr->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void COpenGLSkyBox::_reset_camera()
{
	_camera_param.position = QVector3D(0.0f, 0.0f, 0.0f);
	_camera_param.direction = QVector3D(0.0f, 0.0f, -1.0f);
	_camera_param.up = QVector3D(0.0f, 1.0f, 0.0f);
	_camera_param.fov = 0.06f;
	_camera_param.rotate_H = 0.0f;
	_camera_param.rotate_V = 0.0f;
}

QMatrix4x4 COpenGLSkyBox::_get_model_matrix()
{
	QMatrix4x4 model;

	return model;
}

QMatrix4x4 COpenGLSkyBox::_get_view_matrix()
{
	QMatrix4x4 view;

	view.rotate(_camera_param.rotate_H, QVector3D(0.0f, 1.0f, 0.0f));
	view.rotate(_camera_param.rotate_V, QVector3D(1.0f, 0.0f, 0.0f));
	view.lookAt(_camera_param.position, _camera_param.direction, _camera_param.up);

	return view;
}

QMatrix4x4 COpenGLSkyBox::_get_projection_matrix()
{
	QMatrix4x4 projection;

	float ratio = (float)_window_height / (float)_window_width;
	projection.frustum(-0.1f, 0.1f, -0.1f * ratio, 0.1f * ratio, _camera_param.fov, 300.0f);

	return projection;
}

