#include "opengl_pano.h"


COpenGLPano::COpenGLPano(QWidget *parent)
	: QOpenGLWidget(parent),
	_EBO(QOpenGLBuffer::IndexBuffer)
{
	_window_width = 0;
	_window_height = 0;
	_width = 0;
	_height = 0;

	_program_ptr = NULL;
	_texture_ptr = NULL;
	_matrix_location = 0;
	_vertice_count = 0;

	memset(&_mouse_param, 0x00, sizeof(_mouse_param));
	memset(&_camera_param, 0x00, sizeof(_camera_param));

	_buf_size = 0;
	_buf_ptr = NULL;
}

COpenGLPano::~COpenGLPano()
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
	_EBO.destroy();
	doneCurrent();

	if (NULL != _buf_ptr) {
		delete[] _buf_ptr;
		_buf_ptr = NULL;
	}
	_buf_size = 0;
}

void COpenGLPano::initializeGL()
{
	initializeOpenGLFunctions();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	_init_vertexs();
	_init_shaders();
	_init_textures();
	_reset_camera();

	// :/VLCPlayer/Resources/images/wall.jpg
	//_img.load(":/VLCPlayer/Resources/images/nibiru.png");
}

void COpenGLPano::resizeGL(int w, int h)
{
	_window_width = w;
	_window_height = h;
	glViewport(0, 0, w, h);
}

void COpenGLPano::paintGL()
{
	glClearColor(0.678, 0.847, 0.902, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (0 == _width || 0 == _height || NULL == _buf_ptr)
		return;

	_texture_ptr->bind();
	_program_ptr->bind();
	QMatrix4x4 matrix;
	matrix = _get_projection_matrix() * _get_view_matrix() * _get_model_matrix();
	_program_ptr->setUniformValue("matrix", matrix);
	glUniform1i(_program_ptr->uniformLocation("tex_sampler"), 0);
	_locker.lockForRead();
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _img.width(), _img.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, _img.bits());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0, GL_BGR, GL_UNSIGNED_BYTE, _buf_ptr);
	_locker.unlock();
	QOpenGLVertexArrayObject::Binder vao_binder(&_VAO);
#ifdef ENABLE_VR360
	glDrawArrays(GL_TRIANGLES, 0, _vertice_count);
#else
	glDrawElements(GL_TRIANGLE_STRIP, _vertice_count, GL_UNSIGNED_SHORT, 0);
#endif

	_program_ptr->release();
	_texture_ptr->release();
}

void COpenGLPano::mousePressEvent(QMouseEvent *event)
{
	if (Qt::LeftButton == event->button()) {
		_mouse_param.pressed = true;
		_mouse_param.cur_pos = QVector2D(event->x(), event->y());
		_mouse_param.last_pos = _mouse_param.cur_pos;
	}

	QWidget::mousePressEvent(event);
}

void COpenGLPano::mouseMoveEvent(QMouseEvent *event)
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

void COpenGLPano::mouseReleaseEvent(QMouseEvent *event)
{
	_mouse_param.pressed = false;

	QWidget::mouseReleaseEvent(event);
}

void COpenGLPano::wheelEvent(QWheelEvent *event)
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

bool COpenGLPano::set_format(uint32_t width, uint32_t height)
{
	_width = width;
	_height = height;

	uint32_t new_size = width * height * 4;
	if (_buf_size < new_size) {
		if (NULL != _buf_ptr) {
			delete[] _buf_ptr;
		}
		_buf_ptr = new uint8_t[new_size];
		if (NULL == _buf_ptr) {
			_buf_size = 0;
			return false;
		}
		_buf_size = new_size;
	}

	return true;
}

bool COpenGLPano::send_data(uint32_t size, const uint8_t *data_ptr)
{
	if (NULL != data_ptr && size <= _buf_size) {
		_locker.lockForWrite();
		memcpy(_buf_ptr, data_ptr, size);
		_locker.unlock();
	}
	update();
	return true;
}


void COpenGLPano::_init_vertexs()
{
#ifdef ENABLE_VR360
	QVector3D center(0.0f, 0.0f, 0.0f);
	const float R = 200.0f;
	const int num_H = 50;
	const int num_V = 50;
	double per_H = 2 * M_PI / num_H;
	double per_V = M_PI / num_V;

	QVector<vertex_data_t> vertices;
	for (int v = 0; v <= num_V; v++) {
		float y = R * cos(v * per_V);
		float r = fabs(R * sin(v * per_V));

		for (int h = 0; h <= num_H; h++) {
			vertex_data_t coords;

			int h_tmp = h == num_H ? 0 : h;
			float x = r * cos(h_tmp * per_H);
			float z = r * sin(h_tmp * per_H);
			coords.pos_coord = QVector3D(x, y, z) + center;

			float tex_u = 1.0f / num_H * h;
			float tex_v = 1.0f / num_V * v;
			coords.tex_coord = QVector2D(1.0f - tex_u, tex_v);

			vertices.push_back(coords);
		}
	}

	QVector<vertex_data_t> vertices_ordered;
	for (int v = 0; v < num_V; v++) {
		for (int h = 0; h < num_H; h++) {
			int index = v * (num_H + 1) + h;
			int index_up = index + (num_H + 1);

			vertices_ordered.push_back(vertices[index]);
			vertices_ordered.push_back(vertices[index + 1]);
			vertices_ordered.push_back(vertices[index_up]);

			vertices_ordered.push_back(vertices[index_up]);
			vertices_ordered.push_back(vertices[index + 1]);
			vertices_ordered.push_back(vertices[index_up + 1]);
		}
	}

	_VAO.create();
	QOpenGLVertexArrayObject::Binder vao_binder(&_VAO);
	_VBO.create();
	_VBO.bind();
	_VBO.allocate(&vertices_ordered[0], vertices_ordered.size() * sizeof(vertex_data_t));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
	_vertice_count = vertices_ordered.size();

#else
	vertice_value_t vertices[] = {
		{ QVector3D(-1.0f, -1.0f, 0.0f), QVector2D(0.0f, 1.0f) },
		{ QVector3D(1.0f, -1.0f, 0.0f), QVector2D(1.0f, 1.0f) },
		{ QVector3D(-1.0f, 1.0f, 0.0f), QVector2D(0.0f, 0.0f) },
		{ QVector3D(1.0f, 1.0f, 0.0f), QVector2D(1.0f, 0.0f) },
	};
	GLushort indices[] = {
		0, 1, 3,
		0, 2, 3
	};

	_VAO.create();
	QOpenGLVertexArrayObject::Binder vao_binder(&_VAO);
	_VBO.create();
	_VBO.bind();
	_VBO.allocate(vertices, 4 * sizeof(vertices));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
	_EBO.create();
	_EBO.bind();
	_EBO.allocate(indices, 6 * sizeof(GLushort));
	_vertice_count = 6;
#endif
}

void COpenGLPano::_init_shaders()
{
	_program_ptr = new QOpenGLShaderProgram;
	_program_ptr->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source_pano);
	_program_ptr->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source_pano);
	_program_ptr->bindAttributeLocation("pos_coords", 0);
	_program_ptr->bindAttributeLocation("tex_coords", 1);
	_matrix_location = _program_ptr->uniformLocation("matrix");
	_program_ptr->link();
}

void COpenGLPano::_init_textures()
{
	_texture_ptr = new QOpenGLTexture(QOpenGLTexture::Target2D);
	_texture_ptr->setMinificationFilter(QOpenGLTexture::Nearest);
	_texture_ptr->setMagnificationFilter(QOpenGLTexture::Linear);
	_texture_ptr->setWrapMode(QOpenGLTexture::ClampToBorder);
}

void COpenGLPano::_reset_camera()
{
	_camera_param.position = QVector3D(0.0f, 0.0f, 0.0f);
	_camera_param.direction = QVector3D(0.0f, 0.0f, -1.0f);
	_camera_param.up = QVector3D(0.0f, 1.0f, 0.0f);
	_camera_param.fov = 0.06f;
	_camera_param.rotate_H = 0.0f;
	_camera_param.rotate_V = 0.0f;
}

QMatrix4x4 COpenGLPano::_get_model_matrix()
{
	QMatrix4x4 model;

#ifdef ENABLE_VR360
#else
#endif

	return model;
}

QMatrix4x4 COpenGLPano::_get_view_matrix()
{
	QMatrix4x4 view;

#ifdef ENABLE_VR360
	view.rotate(_camera_param.rotate_H, QVector3D(0.0f, 1.0f, 0.0f));
	view.rotate(_camera_param.rotate_V, QVector3D(1.0f, 0.0f, 0.0f));
	view.lookAt(_camera_param.position, _camera_param.direction, _camera_param.up);
#else
#endif

	return view;
}

QMatrix4x4 COpenGLPano::_get_projection_matrix()
{
	QMatrix4x4 projection;

#ifdef ENABLE_VR360
	float ratio = (float)_window_height / (float)_window_width;
	projection.frustum(-0.1f, 0.1f, -0.1f * ratio, 0.1f * ratio, _camera_param.fov, 300.0f); // Resize not change.
	//projection.perspective(_camera_param.fov, (float)_window_height / (float)_window_width, 0.1f, 400.0f); // Resize changing.
#else
#endif

	return projection;
}

