// Copied tvpgl functions, because we can't rely on the core to have them.

#include "tvpgl.h"

static unsigned char TVPOpacityOnOpacityTable[256*256];
static unsigned char TVPNegativeMulTable[256*256];

TVP_GL_FUNC_DECL(void, TVPCreateTable, (void))
{
	int a,b;

	for(a=0; a<256; a++)
	{
		for(b=0; b<256; b++)
		{
			float c;
			int ci;
			int addr = b*256+ a;

			if(a)
			{
				float at = (float)(a/255.0), bt = (float)(b/255.0);
				c = bt / at;
				c /= (float)( (1.0 - bt + c) );
				ci = (int)(c*255);
				if(ci>=256) ci = 255; /* will not overflow... */
			}
			else
			{
				ci=255;
			}

			TVPOpacityOnOpacityTable[addr]=(unsigned char)ci;
				/* higher byte of the index is source opacity */
				/* lower byte of the index is destination opacity */
		
			TVPNegativeMulTable[addr] = (unsigned char)
				( 255 - (255-a)*(255-b)/ 255 ); 
		}
	}
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPSaturatedAdd, (tjs_uint32 a, tjs_uint32 b))
{
	/* Add each byte of packed 8bit values in two 32bit uint32, with saturation. */
	tjs_uint32 tmp = (  ( a & b ) + ( ((a ^ b)>>1u) & 0x7f7f7f7fu)  ) & 0x80808080u;
	tmp = (tmp<<1u) - (tmp>>7u);
	return (a + b - tmp) | tmp;
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPMulColor, (tjs_uint32 color, tjs_uint32 fac))
{
	return (((((color & 0x00ff00u) * fac) & 0x00ff0000u) +
			(((color & 0xff00ffu) * fac) & 0xff00ff00u) ) >> 8u);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAlphaAndColorToAdditiveAlpha, (tjs_uint32 alpha, tjs_uint32 color))
{
	return TVP_GL_FUNCNAME(TVPMulColor)(color, alpha) + (color & 0xff000000u);

}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAlphaToAdditiveAlpha, (tjs_uint32 a))
{
	return TVP_GL_FUNCNAME(TVPAlphaAndColorToAdditiveAlpha)(a >> 24u, a);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_a_a, (tjs_uint32 dest, tjs_uint32 src))
{
	/*
		Di = sat(Si, (1-Sa)*Di)
		Da = Sa + Da - SaDa
	*/

	tjs_uint32 dopa = dest >> 24u;
	tjs_uint32 sopa = src >> 24u;
	dopa = dopa + sopa - (dopa*sopa >> 8u);
	dopa -= (dopa >> 8u); /* adjust alpha */
	sopa ^= 0xffu;
	src &= 0xffffffu;
	return (dopa << 24u) + 
		TVP_GL_FUNCNAME(TVPSaturatedAdd)((((dest & 0xff00ffu)*sopa >> 8u) & 0xff00ffu) +
			(((dest & 0xff00u)*sopa >> 8u) & 0xff00u), src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_a_a_o, (tjs_uint32 dest, tjs_uint32 src, tjs_int opa))
{
	src = (((src & 0xff00ffu)*opa >> 8u) & 0xff00ffu) + (((src >> 8u) & 0xff00ffu)*opa & 0xff00ff00u);
	return TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a)(dest, src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_a_d, (tjs_uint32 dest, tjs_uint32 src))
{
	return TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a)(dest, TVP_GL_FUNCNAME(TVPAlphaToAdditiveAlpha)(src));
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_a_d_o, (tjs_uint32 dest, tjs_uint32 src, tjs_int opa))
{
	src = (src & 0xffffffu) + ((((src >> 24u) * opa) >> 8u) << 24u);
	return TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_d)(dest, src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_n_a, (tjs_uint32 dest, tjs_uint32 src))
{
	tjs_uint32 sopa = (~src) >> 24u;
	return TVP_GL_FUNCNAME(TVPSaturatedAdd)((((dest & 0xff00ffu)*sopa >> 8u) & 0xff00ffu) + 
		(((dest & 0xff00u)*sopa >> 8u) & 0xff00u), src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_n_a_o, (tjs_uint32 dest, tjs_uint32 src, tjs_int opa))
{
	src = (((src & 0xff00ffu)*opa >> 8u) & 0xff00ffu) + (((src >> 8u) & 0xff00ffu)*opa & 0xff00ff00u);
	return TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a)(dest, src);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_HDA_n_a, (tjs_uint32 dest, tjs_uint32 src))
{
	return (dest & 0xff000000u) + (TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a)(dest, src) & 0xffffffu);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPAddAlphaBlend_HDA_n_a_o, (tjs_uint32 dest, tjs_uint32 src, tjs_int opa))
{
	return (dest & 0xff000000u) + (TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a_o)(dest, src, opa) & 0xffffffu);
}

TVP_GL_FUNC_INLINE_DECL(tjs_uint32, TVPBlendARGB, (tjs_uint32 b, tjs_uint32 a, tjs_int ratio))
{
	/* returns a * ratio + b * (1 - ratio) */
	tjs_uint32 b2;
	tjs_uint32 t;
	b2 = b & 0x00ff00ffu;
	t = (b2 + (((a & 0x00ff00ffu) - b2) * ratio >> 8u)) & 0x00ff00ffu;
	b2 = (b & 0xff00ff00u) >> 8u;
	return t + 
		(((b2 + (( ((a & 0xff00ff00u) >> 8u) - b2) * ratio >> 8u)) << 8u)& 0xff00ff00u);
}

TVP_GL_FUNC_DECL(void, TVPFillARGB_c, (tjs_uint32 *dest, tjs_int len, tjs_uint32 value))
{
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			{
				dest[___index] = value;
			}
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPFillColor_c, (tjs_uint32 *dest, tjs_int len, tjs_uint32 color))
{
	tjs_uint32 t1;

	color &= 0xffffffu;
	{
		for(int ___index = 0; ___index < len; ___index++)
		{
			t1 = dest[___index];;
			t1 &= 0xff000000u;;
			t1 += color;;
			dest[___index] = t1;;
		}
	}
}

TVP_GL_FUNC_DECL(void, TVPInterpLinTransAdditiveAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffffu) >> 8u;
		blend_x += blend_x >> 7u;
		blend_y = (sy & 0xffffu) >> 8u;
		blend_y += blend_y >> 7u;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16u)  )*srcpitch) + (sx>>16u);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0u] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a)(dest[0u], TVP_GL_FUNCNAME(TVPBlendARGB)(
			TVP_GL_FUNCNAME(TVPBlendARGB)(p0[0u], p0[1u], blend_x),
			TVP_GL_FUNCNAME(TVPBlendARGB)(p1[0u], p1[1u], blend_x),
				blend_y));
		sx += stepx, sy += stepy;

		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPInterpLinTransAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	opa += opa >> 7u;
	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffffu) >> 8u;
		blend_x += blend_x >> 7u;
		blend_y = (sy & 0xffffu) >> 8u;
		blend_y += blend_y >> 7u;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16u)  )*srcpitch) + (sx>>16u);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0u] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a_o)(dest[0u], TVP_GL_FUNCNAME(TVPBlendARGB)(
			TVP_GL_FUNCNAME(TVPBlendARGB)(p0[0u], p0[1u], blend_x),
			TVP_GL_FUNCNAME(TVPBlendARGB)(p1[0u], p1[1u], blend_x),
				blend_y), opa);
		sx += stepx, sy += stepy;

		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPInterpLinTransConstAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	opa += opa >> 7u; /* adjust opacity */
	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffffu) >> 8u;
		blend_x += blend_x >> 7u;
		blend_y = (sy & 0xffffu) >> 8u;
		blend_y += blend_y >> 7u;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16u)  )*srcpitch) + (sx>>16u);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0u] = TVP_GL_FUNCNAME(TVPBlendARGB)(dest[0u], TVP_GL_FUNCNAME(TVPBlendARGB)(
			TVP_GL_FUNCNAME(TVPBlendARGB)(p0[0u], p0[1u], blend_x),
			TVP_GL_FUNCNAME(TVPBlendARGB)(p1[0u], p1[1u], blend_x),
				blend_y), opa);
		sx += stepx, sy += stepy;

		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPInterpLinTransCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	while(destlen > 0)
	{
		int blend_x, blend_y;
		const tjs_uint32 *p0, *p1;

		blend_x = (sx & 0xffffu) >> 8u;
		blend_x += blend_x >> 7u;
		blend_y = (sy & 0xffffu) >> 8u;
		blend_y += blend_y >> 7u;
		p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16u)  )*srcpitch) + (sx>>16u);
		p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);
		dest[0u] = TVP_GL_FUNCNAME(TVPBlendARGB)(
			TVP_GL_FUNCNAME(TVPBlendARGB)(p0[0u], p0[1u], blend_x),
			TVP_GL_FUNCNAME(TVPBlendARGB)(p1[0u], p1[1u], blend_x),
				blend_y);
		sx += stepx, sy += stepy;

		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPInterpStretchAdditiveAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep))
{
	/* stretching additive alpha blend with bilinear interpolation */
	tjs_int blend_x;
	tjs_int sp;

	blend_y += blend_y >> 7u; /* adjust blend ratio */


	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffffu) >> 8u;
		sp = srcstart >> 16u;
		dest[0u] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a)(dest[0u], TVP_GL_FUNCNAME(TVPBlendARGB)(
			TVP_GL_FUNCNAME(TVPBlendARGB)(src1[sp], src1[sp+1u], blend_x),
			TVP_GL_FUNCNAME(TVPBlendARGB)(src2[sp], src2[sp+1u], blend_x),
				blend_y));
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPInterpStretchAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	/* stretching additive alpha blend with bilinear interpolation */
	tjs_int blend_x;
	tjs_int sp;

	blend_y += blend_y >> 7u; /* adjust blend ratio */

	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffffu) >> 8u;
		sp = srcstart >> 16u;
		dest[0u] = TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a_o)(dest[0u], TVP_GL_FUNCNAME(TVPBlendARGB)(
			TVP_GL_FUNCNAME(TVPBlendARGB)(src1[sp], src1[sp+1u], blend_x),
			TVP_GL_FUNCNAME(TVPBlendARGB)(src2[sp], src2[sp+1u], blend_x),
				blend_y), opa);
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPInterpStretchConstAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	/* stretch copy with bilinear interpolation */
	tjs_int blend_x;
	tjs_int sp;

	blend_y += blend_y >> 7u; /* adjust blend ratio */
	opa += opa >> 7u; /* adjust opa */

	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffffu) >> 8u;
		sp = srcstart >> 16u;
		dest[0u] = TVP_GL_FUNCNAME(TVPBlendARGB)(dest[0u], TVP_GL_FUNCNAME(TVPBlendARGB)(
			TVP_GL_FUNCNAME(TVPBlendARGB)(src1[sp], src1[sp+1u], blend_x),
			TVP_GL_FUNCNAME(TVPBlendARGB)(src2[sp], src2[sp+1u], blend_x),
				blend_y), opa);
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPInterpStretchCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep))
{
	/* stretch copy with bilinear interpolation */
	tjs_int blend_x;
	tjs_int sp;

	blend_y += blend_y >> 7u; /* adjust blend ratio */
	while(destlen > 0)
	{
		blend_x = (srcstart & 0xffffu) >> 8u;
		sp = srcstart >> 16u;
		dest[0u] = TVP_GL_FUNCNAME(TVPBlendARGB)(
			TVP_GL_FUNCNAME(TVPBlendARGB)(src1[sp], src1[sp+1u], blend_x),
			TVP_GL_FUNCNAME(TVPBlendARGB)(src2[sp], src2[sp+1u], blend_x),
				blend_y);
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a)(*dest, 
				*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u)));
			sx += stepx;
			sy += stepy;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a)(*dest, 
				*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u)));
			sx += stepx;
			sy += stepy;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_ao_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a_o)(*dest, 
				*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u)), opa);
			sx += stepx;
			sy += stepy;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_HDA_n_a)(*dest, 
				*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u)));
			sx += stepx;
			sy += stepy;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_HDA_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_HDA_n_a_o)(*dest, 
				*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u)), opa);
			sx += stepx;
			sy += stepy;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a_o)(*dest, 
				*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u)), opa);
			sx += stepx;
			sy += stepy;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
			sx += stepx;
			sy += stepy;
			d = *dest;
			sopa = s >> 24u;
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_d)(*dest, 
				*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u)));
			sx += stepx;
			sy += stepy;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_ao_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_d_o)(*dest, 
				*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u)), opa);
			sx += stepx;
			sy += stepy;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
			sx += stepx;
			sy += stepy;
			d = *dest;
			addr = ((s >> 16u) & 0xff00u) + (d>>24u);
			destalpha = TVPNegativeMulTable[addr]<<24u;
			sopa = TVPOpacityOnOpacityTable[addr];
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u) + destalpha;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_do_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
			sx += stepx;
			sy += stepy;
			d = *dest;
			addr = (( (s>>24u)*opa) & 0xff00u) + (d>>24u);
			destalpha = TVPNegativeMulTable[addr]<<24u;
			sopa = TVPOpacityOnOpacityTable[addr];
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u) + destalpha;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
			sx += stepx;
			sy += stepy;
			d = *dest;
			sopa = s >> 24u;
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu) + (d & 0xff000000u); /* hda */
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_HDA_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
			sx += stepx;
			sy += stepy;
			d = *dest;
			sopa = ((s >> 24u) * opa) >> 8u;
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu) + (d & 0xff000000u);
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
			sx += stepx;
			sy += stepy;
			d = *dest;
			sopa = ((s >> 24u) * opa) >> 8u;
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransColorCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	/* note that srcpitch unit is in byte */
	while(destlen > 0)
	{
		dest[0u] = (dest[0u] & 0xff000000u) + (0x00ffffffu & *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u)));
		sx += stepx, sy += stepy;
		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransConstAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
			sx += stepx;
			sy += stepy;
			d = *dest;
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * opa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 | ((d + ((s - d) * opa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransConstAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	opa <<= 24u;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a)(*dest, 
				((*( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u))) & 0xffffffu) | opa);
			sx += stepx;
			sy += stepy;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransConstAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d, addr;
	tjs_int alpha;
	opa <<= 8u;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
			sx += stepx;
			sy += stepy;
			d = *dest;
			addr = opa + (d>>24u);
			alpha = TVPOpacityOnOpacityTable[addr];
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * alpha >> 8u)) & 0xff00ffu) +
				(TVPNegativeMulTable[addr]<<24u);
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 | ((d + ((s - d) * alpha >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransConstAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32 d1, s, d;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
			sx += stepx;
			sy += stepy;
			d = *dest;
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * opa >> 8u)) & 0xff00ffu) + (d & 0xff000000u);
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 | ((d + ((s - d) * opa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	/* note that srcpitch unit is in byte */
	while(destlen > 0)
	{
		dest[0u] = *( (const tjs_uint32*)((tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
		sx += stepx, sy += stepy;
		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPLinTransCopyOpaqueImage_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	while(destlen > 0)
	{
		dest[0u] = 0xff000000u | *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16u)*srcpitch) + (sx>>16u));
		sx += stepx;
		sy += stepy;
		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_n_a)(*dest, src[srcstart >> 16u]);
			srcstart += srcstep;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_HDA_n_a)(*dest, src[srcstart >> 16u]);
			srcstart += srcstep;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_HDA_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_HDA_n_a_o)(*dest, src[srcstart >> 16u], opa);
			srcstart += srcstep;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = src[srcstart >> 16u];
			srcstart += srcstep;
			d = *dest;
			sopa = s >> 24u;
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_d)(*dest, src[srcstart >> 16u]);
			srcstart += srcstep;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_ao_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_d_o)(*dest, src[srcstart >> 16u], opa);
			srcstart += srcstep;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = src[srcstart >> 16u];
			srcstart += srcstep;
			d = *dest;
			addr = ((s >> 16u) & 0xff00u) + (d>>24u);
			destalpha = TVPNegativeMulTable[addr]<<24u;
			sopa = TVPOpacityOnOpacityTable[addr];
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u) + destalpha;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_do_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa, addr, destalpha;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = src[srcstart >> 16u];
			srcstart += srcstep;
			d = *dest;
			addr = (( (s>>24u)*opa) & 0xff00u) + (d>>24u);
			destalpha = TVPNegativeMulTable[addr]<<24u;
			sopa = TVPOpacityOnOpacityTable[addr];
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u) + destalpha;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = src[srcstart >> 16u];
			srcstart += srcstep;
			d = *dest;
			sopa = s >> 24u;
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu) + (d & 0xff000000u); /* hda */
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_HDA_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = src[srcstart >> 16u];
			srcstart += srcstep;
			d = *dest;
			sopa = ((s >> 24u) * opa) >> 8u;
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu) + (d & 0xff000000u);
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d, sopa;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = src[srcstart >> 16u];
			srcstart += srcstep;
			d = *dest;
			sopa = ((s >> 24u) * opa) >> 8u;
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * sopa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 + ((d + ((s - d) * sopa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchColorCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	/* this performs only color(main) copy */
	while(destlen > 0)
	{
		dest[0u] = (dest[0u] & 0xff0000u) + (src[srcstart >> 16u] & 0xffffffu);
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPStretchConstAlphaBlend_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = src[srcstart>>16u];
			srcstart += srcstep;
			d = *dest;
			d1 = d & 0xff00ffu;
			d1 = (d1 + (((s & 0xff00ffu) - d1) * opa >> 8u)) & 0xff00ffu;
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 | ((d + ((s - d) * opa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchConstAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	opa <<= 24u;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			*dest = TVP_GL_FUNCNAME(TVPAddAlphaBlend_a_a)(*dest, (src[srcstart >> 16u] & 0xffffffu) | opa);
			srcstart += srcstep;
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchConstAlphaBlend_d_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d, addr;
	tjs_int alpha;
	if(opa > 128u) opa ++; /* adjust for error */
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = src[srcstart >> 16u];
			srcstart += srcstep;
			d = *dest;
			addr = (( (s>>24u)*opa) & 0xff00u) + (d>>24u);
			alpha = TVPOpacityOnOpacityTable[addr];
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * alpha >> 8u)) & 0xff00ffu) +
				(TVPNegativeMulTable[addr]<<24u);
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 | ((d + ((s - d) * alpha >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchConstAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	tjs_uint32 d1, s, d;
	for(int lu_n = 0; lu_n < len; lu_n++)
	{
		{
			s = src[srcstart >> 16u];
			srcstart += srcstep;
			d = *dest;
			d1 = d & 0xff00ffu;
			d1 = ((d1 + (((s & 0xff00ffu) - d1) * opa >> 8u)) & 0xff00ffu) + (d & 0xff000000u);
			d &= 0xff00u;
			s &= 0xff00u;
			*dest = d1 | ((d + ((s - d) * opa >> 8u)) & 0xff00u);
			dest++;
		}

	}
}

TVP_GL_FUNC_DECL(void, TVPStretchCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	while(destlen > 0)
	{
		dest[0u] = src[srcstart >> 16u];
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

TVP_GL_FUNC_DECL(void, TVPStretchCopyOpaqueImage_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	while(destlen > 0)
	{
		dest[0u] = 0xff000000u | src[srcstart >> 16u];
		srcstart += srcstep;
		dest ++;
		destlen --;
	}
}

