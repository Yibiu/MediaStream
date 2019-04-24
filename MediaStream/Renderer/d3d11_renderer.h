#pragma once
#include <vector>
#include <d3d11.h>
#include "../Common/media_defs.h"
#include "../Common/rt_status.h"


#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)		{if (p) {(p)->Release(); (p) = NULL;}}
#endif


/**
* @brief:
* Video renderer using D3D11
*
* SUPPORT:
* Texture mode
* Data mode
*/
class CD3D11Renderer
{
public:
	CD3D11Renderer();
	virtual ~CD3D11Renderer();

	rt_status_t create(uint32_t width, uint32_t height, video_fourcc_t fourcc, HWND wnd);
	void destroy();
	ID3D11Device* get_device();

	rt_status_t create_textures(uint32_t count);
	void destroy_textures();
	ID3D11Texture2D* get_texture(uint32_t index);
	void render_texture(uint32_t index);

	void render_data(uint32_t stride, const uint8_t *data_ptr);

protected:
	ID3D11Device *_device_ptr;
	IDXGISwapChain *_swapchain_ptr;
	ID3D11DeviceContext *_context_ptr;
	ID3D11Texture2D *_backbuf_ptr;
	ID3D11DepthStencilView *_depthview_ptr;
	ID3D11RenderTargetView *_renderview_ptr;
	ID3D11Query *_query_ptr;

	std::vector<ID3D11Texture2D *> _textures;

	uint32_t _width;
	uint32_t _height;
	video_fourcc_t _fourcc;
};

