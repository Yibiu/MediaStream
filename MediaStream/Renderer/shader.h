#pragma once


///////////////////////////////////////////////////
// Vertex shader
static const char *vertex_shader_source =
"attribute vec4 position;\n"
"attribute vec2 texture;\n"
"varying vec2 v_texcoord;\n"
"void main() {\n"
"	gl_Position = position;\n"
"	v_texcoord = texture;\n"
"}\n";


///////////////////////////////////////////////////
// Fragment - RGB24
static const char *fragment_shader_source_RGB =
"uniform sampler2D tex;\n"
"varying vec2 v_texcoord;\n"
"void main() {\n"
"	gl_FragColor = texture2D(tex, v_texcoord);\n"
"}\n";

// Fragment - RGBA
static const char *fragment_shader_source_RGBA =
"uniform sampler2D tex;\n"
"varying vec2 v_texcoord;\n"
"void main() {\n"
"	gl_FragColor = texture2D(tex, v_texcoord);\n"
"}\n";

// Fragment - YUV420P(I420)
static const char *fragment_shader_source_YUV420P =
"uniform sampler2D texY;\n"
"uniform sampler2D texU;\n"
"uniform sampler2D texV;\n"
"varying vec2 v_texcoord;\n"
"void main() {\n"
"	vec3 yuv;\n"
"	vec3 rgb;\n"
"	yuv.x = texture2D(texY, v_texcoord).r;\n"
"	yuv.y = texture2D(texU, v_texcoord).r - 0.5;\n"
"	yuv.z = texture2D(texV, v_texcoord).r - 0.5;\n"
"	rgb = mat3(1,1,1,0,-0.3455,1.779,1.4075,-0.7169,0)*yuv;\n"
"	gl_FragColor = vec4(rgb, 1);\n"
"}\n";

// Fragment - NV12(YUV601)
static const char *fragment_shader_source_YUV601 =
"uniform sampler2D texY;\n"
"uniform sampler2D texUV;\n"
"varying vec2 v_texcoord;\n"
"void main() {\n"
"	vec3 yuv;\n"
"	yuv.x = 1.164 * (texture2D(texY, v_texcoord).r - 0.062745); \n"
"	yuv.yz = texture2D(texUV, v_texcoord).rg - vec2(0.5, 0.5);\n"
// YCbCr->RGB
"	gl_FragColor[0] = clamp(yuv.x + 1.596 * yuv.z, 0.0, 1.0);\n"
"	gl_FragColor[1] = clamp(yuv.x - 0.8125 * yuv.z - 0.392 * yuv.y, 0.0, 1.0);\n"
"	gl_FragColor[2] = clamp(yuv.x + 2.017 * yuv.y, 0.0, 1.0);\n"
"	gl_FragColor[3] = 1.0;\n"
"}\n";

// Fragment - NV12(YUV709)
static const char *fragment_shader_source_YUV709 =
"uniform sampler2D texY;\n"
"uniform sampler2D texUV;\n"
"varying vec2 v_texcoord;\n"
"void main() {\n"
"	vec3 yuv;\n"
"	yuv.x = 1.164 * (texture2D(texY, v_texcoord).r - 0.062745);\n"
"	yuv.yz = texture2D(texUV, v_texcoord).rg - vec2(0.5, 0.5);\n"
"	gl_FragColor[0] = clamp(yuv.x + 1.793 * yuv.z, 0.0, 1.0);\n"
"	gl_FragColor[1] = clamp(yuv.x - 0.533 * yuv.z - 0.213 * yuv.y, 0.0, 1.0);\n"
"	gl_FragColor[2] = clamp(yuv.x + 2.112 * yuv.y, 0.0, 1.0);\n"
"	gl_FragColor[3] = 1.0;"
"}\n";


///////////////////////////////////////////////////
// Vertex/Fragment - for pano
const char *vertex_shader_source_pano =
"attribute vec3 pos_coords;\n"
"attribute vec2 tex_coords;\n"
"varying vec2 v_texcoord;\n"
"uniform mat4 matrix;"
"void main() {\n"
"	gl_Position = matrix * vec4(pos_coords, 1.0);\n"
"	v_texcoord = tex_coords;\n"
"}\n";

const char *fragment_shader_source_pano =
"uniform sampler2D tex_sampler;\n"
"varying vec2 v_texcoord;\n"
"void main() {\n"
"	gl_FragColor = texture(tex_sampler, v_texcoord);\n"
"}\n";

