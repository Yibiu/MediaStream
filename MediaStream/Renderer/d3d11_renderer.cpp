#include "d3d11_renderer.h"


CD3D11Renderer::CD3D11Renderer()
{
	_device_ptr = NULL;
	_swapchain_ptr = NULL;
	_context_ptr = NULL;
	_backbuf_ptr = NULL;
	_depthview_ptr = NULL;
	_renderview_ptr = NULL;
	_query_ptr = NULL;

	_textures.clear();

	_width = 0;
	_height = 0;
	_fourcc = VIDEO_FOURCC_UNKNOWN;
}

CD3D11Renderer::~CD3D11Renderer()
{
}

rt_status_t CD3D11Renderer::create(uint32_t width, uint32_t height, video_fourcc_t fourcc, HWND wnd)
{
	rt_status_t status = RT_STATUS_SUCCESS;

	do {
		// Create device and swap chain
		D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
		DXGI_SWAP_CHAIN_DESC sp_desc = { 0 };
		if (VIDEO_FOURCC_RGBA == fourcc) {
			sp_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (VIDEO_FOURCC_BGRA == fourcc) {
			sp_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		}
		else {
			status = RT_STATUS_UNSUPPORT;
			break;
		}
		sp_desc.BufferCount = 2;
		sp_desc.BufferDesc.Width = width;
		sp_desc.BufferDesc.Height = height;
		sp_desc.BufferDesc.RefreshRate.Numerator = 60;
		sp_desc.BufferDesc.RefreshRate.Denominator = 1;
		sp_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sp_desc.OutputWindow = wnd;
		sp_desc.SampleDesc.Count = 1;
		sp_desc.SampleDesc.Quality = 0;
		sp_desc.Windowed = TRUE;
		HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
			D3D11_CREATE_DEVICE_SINGLETHREADED, NULL, 0, D3D11_SDK_VERSION, &sp_desc,
			&_swapchain_ptr, &_device_ptr, &feature_level, &_context_ptr);
		if (FAILED(hr)) {
			status = RT_STATUS_INITIALIZED;
			break;
		}

		// Create render target view
		hr = _swapchain_ptr->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&_backbuf_ptr);
		if (FAILED(hr)) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		hr = _device_ptr->CreateRenderTargetView(_backbuf_ptr, NULL, &_renderview_ptr);
		if (FAILED(hr)) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		// Create depth stencil view
		ID3D11Texture2D *texture_stencil = NULL;
		D3D11_TEXTURE2D_DESC tx_desc;
		tx_desc.Width = width;
		tx_desc.Height = height;
		tx_desc.MipLevels = 1;
		tx_desc.ArraySize = 1;
		tx_desc.Format = DXGI_FORMAT_D16_UNORM;
		tx_desc.SampleDesc.Count = 1;
		tx_desc.SampleDesc.Quality = 0;
		tx_desc.Usage = D3D11_USAGE_DEFAULT;
		tx_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		tx_desc.CPUAccessFlags = 0;
		tx_desc.MiscFlags = 0;
		hr = _device_ptr->CreateTexture2D(&tx_desc, NULL, &texture_stencil);
		if (FAILED(hr)) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		D3D11_DEPTH_STENCIL_VIEW_DESC dp_desc;
		dp_desc.Format = tx_desc.Format;
		dp_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dp_desc.Texture2D.MipSlice = 0;
		dp_desc.Flags = 0;
		hr = _device_ptr->CreateDepthStencilView(texture_stencil, &dp_desc, &_depthview_ptr);
		if (FAILED(hr)) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		if (NULL != texture_stencil) {
			texture_stencil->Release();
		}
		// Link to pipeline
		_context_ptr->OMSetRenderTargets(1, &_renderview_ptr, _depthview_ptr);

		// Viewport
		D3D11_VIEWPORT vp;
		vp.Width = (FLOAT)width;
		vp.Height = (FLOAT)height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		_context_ptr->RSSetViewports(1, &vp);

		D3D11_QUERY_DESC pQueryDesc;
		pQueryDesc.Query = D3D11_QUERY_EVENT;
		pQueryDesc.MiscFlags = 0;
		hr = _device_ptr->CreateQuery(&pQueryDesc, &_query_ptr);
		if (FAILED(hr)) {
			status = RT_STATUS_INITIALIZED;
			break;
		}

		_textures.clear();
		_width = width;
		_height = height;
		_fourcc = fourcc;
	} while (false);

	if (!rt_is_success(status)) {
		destroy();
	}

	return status;
}

void CD3D11Renderer::destroy()
{
	SAFE_RELEASE(_query_ptr);
	SAFE_RELEASE(_depthview_ptr);
	SAFE_RELEASE(_renderview_ptr);
	SAFE_RELEASE(_backbuf_ptr);
	SAFE_RELEASE(_context_ptr);
	SAFE_RELEASE(_swapchain_ptr);
	SAFE_RELEASE(_device_ptr);
}

ID3D11Device* CD3D11Renderer::get_device()
{
	return _device_ptr;
}

rt_status_t CD3D11Renderer::create_textures(uint32_t count)
{
	rt_status_t status = RT_STATUS_SUCCESS;

	do {
		if (NULL == _device_ptr) {
			status = RT_STATUS_UNINITIALIZED;
			break;
		}

		_textures.clear();
		for (int i = 0; i < count; i++) {
			D3D11_TEXTURE2D_DESC tex_desc = { 0 };
			tex_desc.Width = _width;
			tex_desc.Height = _height;
			tex_desc.MipLevels = 1;
			tex_desc.ArraySize = 1;
			tex_desc.Format = VIDEO_FOURCC_RGBA == _fourcc ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM;
			tex_desc.SampleDesc.Count = 1;
			tex_desc.SampleDesc.Quality = 0;
			tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			tex_desc.Usage = D3D11_USAGE_DEFAULT;

			ID3D11Texture2D *texture_ptr = NULL;
			HRESULT hr = _device_ptr->CreateTexture2D(&tex_desc, NULL, &texture_ptr);
			if (FAILED(hr)) {
				status = RT_STATUS_MEMORY_ALLOCATE;
				break;
			}

			_textures.push_back(texture_ptr);
		}
	} while (false);

	if (!rt_is_success(status)) {
		destroy_textures();
	}

	return status;
}

void CD3D11Renderer::destroy_textures()
{
	for (int i = 0; i < _textures.size(); i++) {
		SAFE_RELEASE(_textures[i]);
	}
	_textures.clear();
}

ID3D11Texture2D* CD3D11Renderer::get_texture(uint32_t index)
{
	if (index >= _textures.size())
		return NULL;

	return _textures[index];
}

void CD3D11Renderer::render_texture(uint32_t index)
{
	if (index >= _textures.size() || NULL == _context_ptr
		|| NULL == _backbuf_ptr || NULL == _swapchain_ptr)
		return;

	_context_ptr->CopyResource(_backbuf_ptr, _textures[index]);
	_swapchain_ptr->Present(0, 0);
}

void CD3D11Renderer::render_data(uint32_t stride, const uint8_t *data_ptr)
{
	if (NULL == _context_ptr || NULL == _backbuf_ptr || NULL == _swapchain_ptr)
		return;

	_context_ptr->UpdateSubresource(_backbuf_ptr, 0, NULL, data_ptr, stride, 0);
	_swapchain_ptr->Present(0, 0);
}

