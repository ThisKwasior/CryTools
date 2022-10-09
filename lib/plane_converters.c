#include <plane_converters.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
	Helpers
*/

int64_t clamp(const int64_t value, const int64_t down, const int64_t up)
{
	if(value < down) return down; 
	if(value > up) return up;
	return value;
}

/*
	Init and destroy
*/

void init_YUV420(yuv_s* yuv, const uint32_t width, const uint32_t height)
{
	yuv->y.w = width;
	yuv->y.h = height;
	yuv->u.w = width/2;
	yuv->u.h = height/2;
	yuv->v.w = width/2;
	yuv->v.h = height/2;
	
	yuv->y.data = calloc(yuv->y.w*yuv->y.h, 1);
	yuv->u.data = calloc(yuv->u.w*yuv->u.h, 1);
	yuv->v.data = calloc(yuv->v.w*yuv->v.h, 1);
}

void destroy_YUV_frame(yuv_s* yuv)
{
	free(yuv->y.data);
	free(yuv->u.data);
	free(yuv->v.data);
	
	memset(yuv, 0, sizeof(yuv_s));
}

/*
	Loaders
*/

void load_raw_YUV420(yuv_s* yuv, const uint8_t* data)
{
	memset(yuv->y.data, 0, yuv->y.w*yuv->y.h);
	memset(yuv->u.data, 0, yuv->u.w*yuv->u.h);
	memset(yuv->v.data, 0, yuv->v.w*yuv->v.h);
	
	memcpy(yuv->y.data, data, yuv->y.w*yuv->y.h);
	memcpy(yuv->u.data, &data[yuv->y.w*yuv->y.h], yuv->u.w*yuv->u.h);
	memcpy(yuv->v.data, &data[(yuv->y.w*yuv->y.h)+(yuv->u.w*yuv->u.h)], yuv->v.w*yuv->v.h);
}

/*
	Converters of YUV to RGB
*/

void conv_YUV_to_RGB(const uint8_t y, const uint8_t u, const uint8_t v,
					 uint8_t* r, uint8_t* g, uint8_t* b)
{
	int32_t rtmp = y + (v-128)*1.402f;
	int32_t gtmp = y - (u-128)*0.344f - (v-128)*0.714f;
	int32_t btmp = y + (u-128)*1.772f;
	
	/*int32_t rtmp = y + (v-128)*1.140f;
	int32_t gtmp = y - (u-128)*0.395f - (v-128)*0.581f;
	int32_t btmp = y + (u-128)*2.032f;*/
	
	*r = clamp(rtmp, 0, 255);
	*g = clamp(gtmp, 0, 255);
	*b = clamp(btmp, 0, 255);
}

void conv_YUV420_plane_to_RGB(const uint32_t width, const uint32_t height,
							  const uint8_t* y_data, const uint8_t* cr_data,
							  const uint8_t* cb_data, uint8_t* out)
{
	const uint32_t w_uv = width/2;
	
	for(uint32_t x = 0; x != width; ++x)
	{
		const uint32_t x_uv = x/2;
		
		for(uint32_t y = 0; y != height; ++y)
		{
			const uint32_t y_uv = y/2;
			
			const uint32_t pos_y = y*width + x;
			const uint32_t pos_uv = y_uv*w_uv + x_uv;
			const uint32_t pos_rgb = (y*width*3) + x*3;
			
			conv_YUV_to_RGB(y_data[pos_y], cb_data[pos_uv], cr_data[pos_uv],
							&out[pos_rgb], &out[pos_rgb+1], &out[pos_rgb+2]);	
		}
	}
}							  
				