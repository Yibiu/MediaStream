#include "opengl_window.h"


// CGLBaseWindow
CGLBaseWindow::CGLBaseWindow(QWindow *parent)
	: QWindow(parent)
{
	setSurfaceType(QSurface::OpenGLSurface);

	_context_ptr = NULL;
	_paint_device_ptr = NULL;
}

CGLBaseWindow::~CGLBaseWindow()
{
}

void CGLBaseWindow::initOpenGL()
{
	// Must be called in the same thread
	// ...
}

void CGLBaseWindow::renderPaint(QPainter *painter)
{
	// Must be called in the same thread
	// ...
}

void CGLBaseWindow::renderOpenGL(uint32_t size, const uint8_t *data_ptr)
{
	// Must be called in the same thread
	// ...
}

void CGLBaseWindow::exitOpenGL()
{
	// Must be called in the same thread
	// ...
}

// SLOTS
void CGLBaseWindow::renderNow(uint32_t size, const uint8_t *data_ptr)
{
	if (!isExposed())
		return;

	// First call, initialized to ensure in the same thread...
	// Initialization needs set context to current
	bool need_initialize = false;
	if (NULL == _context_ptr) {
		_context_ptr = new QOpenGLContext(this);
		_context_ptr->setFormat(requestedFormat());
		_context_ptr->create();
		need_initialize = true;
	}
	_context_ptr->makeCurrent(this);
	if (need_initialize) {
		initializeOpenGLFunctions();
		initOpenGL();
		if (NULL == _paint_device_ptr) {
			_paint_device_ptr = new QOpenGLPaintDevice;
		}
	}

	renderOpenGL(size, data_ptr);
	if (NULL != _paint_device_ptr) {
		_paint_device_ptr->setSize(this->size());
		QPainter painter(_paint_device_ptr);
		renderPaint(&painter);
	}

	_context_ptr->swapBuffers(this);
}

void CGLBaseWindow::exitGL()
{
	if (NULL == _context_ptr)
		return;

	_context_ptr->makeCurrent(this);
	exitOpenGL();
	if (NULL != _paint_device_ptr) {
		delete _paint_device_ptr;
		_paint_device_ptr = NULL;
	}
	_context_ptr->doneCurrent();
	delete _context_ptr;
	_context_ptr = NULL;
}


// COpenGLWindow
COpenGLWindow::COpenGLWindow(QWindow *parent)
	: CGLBaseWindow(parent),
	_IBO(QOpenGLBuffer::IndexBuffer)
{
	_program_ptr = NULL;
	_tex_Y = NULL;
	_tex_U = NULL;
	_tex_V = NULL;

	_width = 0;
	_height = 0;
	_fourcc = VIDEO_FOURCC_UNKNOWN;
}

COpenGLWindow::~COpenGLWindow()
{
}

void COpenGLWindow::initOpenGL()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.678, 0.847, 0.902, 1);

	_init_vertexs();
	_init_shaders();
	_init_textures();
}

void COpenGLWindow::renderPaint(QPainter *painter)
{
	// TODO:
	// ...
}

void COpenGLWindow::renderOpenGL(uint32_t size, const uint8_t *data_ptr)
{
	const qreal retinaScale = devicePixelRatio();
	glViewport(0, 0, width() * retinaScale, height() * retinaScale);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (0 == _width || 0 == _height)
		return;

	_program_ptr->bind();
	switch (_fourcc)
	{
	case VIDEO_FOURCC_RGBA:
	case VIDEO_FOURCC_BGRA:
	case VIDEO_FOURCC_ARGB:
	case VIDEO_FOURCC_ABGR:
		glActiveTexture(GL_TEXTURE0);
		_tex_Y->bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data_ptr);
		glUniform1i(_program_ptr->uniformLocation("tex"), 0);
		break;
	case VIDEO_FOURCC_RGB:
	case VIDEO_FOURCC_BGR:
		glActiveTexture(GL_TEXTURE0);
		_tex_Y->bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, data_ptr);
		glUniform1i(_program_ptr->uniformLocation("tex"), 0);
		break;
	case VIDEO_FOURCC_YUV420:
		glActiveTexture(GL_TEXTURE0);
		_tex_Y->bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _width, _height, 0, GL_RED, GL_UNSIGNED_BYTE, data_ptr);
		glActiveTexture(GL_TEXTURE1);
		_tex_U->bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _width / 2, _height / 2, 0, GL_RED, GL_UNSIGNED_BYTE,
			data_ptr + _width * _height);
		glActiveTexture(GL_TEXTURE2);
		_tex_V->bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _width / 2, _height / 2, 0, GL_RED, GL_UNSIGNED_BYTE,
			data_ptr + _width * _height * 5 / 4);
		glUniform1i(_program_ptr->uniformLocation("texY"), 0);
		glUniform1i(_program_ptr->uniformLocation("texU"), 1);
		glUniform1i(_program_ptr->uniformLocation("texV"), 2);
		break;
	case VIDEO_FOURCC_NV12:
		glActiveTexture(GL_TEXTURE0);
		_tex_Y->bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _width, _height, 0, GL_RED, GL_UNSIGNED_BYTE, data_ptr);
		glActiveTexture(GL_TEXTURE1);
		_tex_U->bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, _width / 2, _height / 2, 0, GL_RG, GL_UNSIGNED_BYTE,
			data_ptr + _width * _height);
		glUniform1i(_program_ptr->uniformLocation("texY"), 0);
		glUniform1i(_program_ptr->uniformLocation("texUV"), 1);
		break;
	default:
		return;
	}

	QOpenGLVertexArrayObject::Binder vao_binder(&_VAO);
	glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, 0);

	_program_ptr->release();
	_tex_Y->release();
	_tex_U->release();
	_tex_V->release();
}

void COpenGLWindow::exitOpenGL()
{
	if (NULL != _program_ptr) {
		delete _program_ptr;
		_program_ptr = NULL;
	}
	if (NULL != _tex_Y) {
		delete _tex_Y;
		_tex_Y = NULL;
	}
	if (NULL != _tex_U) {
		delete _tex_U;
		_tex_U = NULL;
	}
	if (NULL != _tex_V) {
		delete _tex_V;
		_tex_V = NULL;
	}
	_VAO.destroy();
	_VBO.destroy();
	_IBO.destroy();
}

bool COpenGLWindow::set_format(uint32_t width, uint32_t height, video_fourcc_t fourcc)
{
	_width = width;
	_height = height;
	_fourcc = fourcc;
	return true;
}


void COpenGLWindow::_init_vertexs()
{
	vertex_data_t vertices[] = {
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
	_VBO.allocate(vertices, 4 * sizeof(vertex_data_t));
	// Attribute 0 - position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
	// Attribute 1 - texture
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
	_IBO.create();
	_IBO.bind();
	_IBO.allocate(indices, 6 * sizeof(GLushort));
}

void COpenGLWindow::_init_shaders()
{
	_program_ptr = new QOpenGLShaderProgram;
	_program_ptr->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source);
	switch (_fourcc)
	{
	case VIDEO_FOURCC_RGBA:
	case VIDEO_FOURCC_BGRA:
	case VIDEO_FOURCC_ARGB:
	case VIDEO_FOURCC_ABGR:
		_program_ptr->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source_RGBA);
		break;
	case VIDEO_FOURCC_RGB:
	case VIDEO_FOURCC_BGR:
		_program_ptr->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source_RGB);
		break;
	case VIDEO_FOURCC_YUV420:
		_program_ptr->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source_YUV420P);
		break;
	case VIDEO_FOURCC_NV12:
		if (_width * _height < 1280 * 720) {
			_program_ptr->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source_YUV601);
		}
		else {
			_program_ptr->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source_YUV709);
		}
		break;
	default:
		break;
	}
	_program_ptr->bindAttributeLocation("position", 0);
	_program_ptr->bindAttributeLocation("texture", 1);
	_program_ptr->link();
}

void COpenGLWindow::_init_textures()
{
	_tex_Y = new QOpenGLTexture(QOpenGLTexture::Target2D);
	_tex_Y->setMinificationFilter(QOpenGLTexture::Nearest); // nearest filtering mode
	_tex_Y->setMagnificationFilter(QOpenGLTexture::Linear); // bilinear filtering mode
	_tex_Y->setWrapMode(QOpenGLTexture::ClampToBorder);

	_tex_U = new QOpenGLTexture(QOpenGLTexture::Target2D);
	_tex_U->setMinificationFilter(QOpenGLTexture::Nearest); // nearest filtering mode
	_tex_U->setMagnificationFilter(QOpenGLTexture::Linear); // bilinear filtering mode
	_tex_U->setWrapMode(QOpenGLTexture::ClampToBorder);

	_tex_V = new QOpenGLTexture(QOpenGLTexture::Target2D);
	_tex_V->setMinificationFilter(QOpenGLTexture::Nearest); // nearest filtering mode
	_tex_V->setMagnificationFilter(QOpenGLTexture::Linear); // bilinear filtering mode
	_tex_V->setWrapMode(QOpenGLTexture::ClampToBorder);
}

