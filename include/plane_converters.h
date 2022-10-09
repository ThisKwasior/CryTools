#include <stdint.h>

/*
	Plane types
*/

typedef struct PLANE
{
	uint32_t w;
	uint32_t h;
	uint8_t* data;
} plane_s;

typedef struct YUV_FRAME
{
	plane_s y;
	plane_s u;
	plane_s v;
} yuv_s;

/*
	Init and destroy
*/

void init_YUV420(yuv_s* yuv, const uint32_t width, const uint32_t height);

void destroy_YUV_frame(yuv_s* yuv);

/*
	Loaders
*/

void load_raw_YUV420(yuv_s* yuv, const uint8_t* data);

/*
	Converters of YUV to RGB
*/

void conv_YUV_to_RGB(const uint8_t y, const uint8_t u, const uint8_t v,
					 uint8_t* r, uint8_t* g, uint8_t* b);

void conv_YUV420_plane_to_RGB(const uint32_t width, const uint32_t height,
							  const uint8_t* y_data, const uint8_t* cr_data,
							  const uint8_t* cb_data, uint8_t* out);