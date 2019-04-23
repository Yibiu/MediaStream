#pragma once


#include <QObject>
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QWindow>
//OpenGL Header
#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLPaintDevice>
//QWidget Header
#include <QPainter>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <Qstring>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QMath.h>
#include <QMouseEvent>
#include <QReadWriteLock>


typedef struct _vertex_data
{
	QVector3D pos_coord;
	QVector2D tex_coord;
} vertex_data_t;

typedef struct _camera_param
{
	QVector3D position;
	QVector3D direction;
	QVector3D up;

	float fov;
	float rotate_H;
	float rotate_V;
} camera_param_t;

typedef struct _mouse_param
{
	bool pressed;

	QVector2D cur_pos;
	QVector2D last_pos;
} mouse_param_t;


