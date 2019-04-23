#pragma once
#include <stdint.h>


typedef enum _rt_status
{
	RT_STATUS_SUCCESS = 0,

	// Warning
	RT_STATUS_WARNING_MIN = 0x1000,
	RT_STATUS_BUSY,
	RT_STATUS_WARNING_MAX,

	// Error
	RT_STATUS_ERROR_MIN = 0x2000,
	RT_STATUS_INVALID_PARAMETER,
	RT_STATUS_MEMORY_ALLOCATE,
	RT_STATUS_INITIALIZED,
	RT_STATUS_UNINITIALIZED,
	RT_STATUS_UNSUPPORT,
	RT_STATUS_EXECUTE,
	RT_STATUS_ERROR_MAX
} rt_status_t;


static inline bool rt_is_success(rt_status_t status)
{
	return (RT_STATUS_SUCCESS == status);
}

static inline bool rt_is_warning(rt_status_t status)
{
	return (RT_STATUS_WARNING_MIN <= status && RT_STATUS_WARNING_MAX >= status);
}

static inline bool rt_is_error(rt_status_t status)
{
	return (RT_STATUS_ERROR_MIN <= status && RT_STATUS_ERROR_MAX >= status);
}

