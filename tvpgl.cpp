// Copied tvpgl functions, because we can't rely on the core to have them.

#include "tvpgl.h"
#include <simde/x86/mmx.h>

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
	tjs_uint32   *v8;  // edi
	tjs_int       v9;  // ecx
	tjs_int       v10; // edx
	tjs_uint32 *  v11; // ebp
	tjs_uint32   *v12; // eax
	simde__m64    v14; // mm5
	simde__m64    v15; // mm5
	simde__m64    v17; // mm7
	simde__m64    v18; // mm1
	simde__m64    v19; // mm3
	simde__m64    v20; // mm1
	simde__m64    v21; // mm1
	simde__m64    v22; // mm3
	simde__m64    v23; // mm2
	simde__m64    v24; // mm3

	if (destlen > 0)
	{
		v8  = dest;
		v9  = sx;
		v10 = sy;
		v11 = &dest[destlen];
		if (dest < v11)
		{
			do
			{
				v12 = (tjs_uint32 *)((char *)&src[v9 >> 16] + srcpitch * (v10 >> 16));
				v14 = simde_mm_set1_pi16((tjs_uint32)(tjs_uint16)v9 >> 8);
				v15 = v14;
				v17 = simde_mm_set1_pi16(((tjs_uint32)(tjs_uint16)v10 >> 8) + ((tjs_uint32)(tjs_uint16)v10 >> 15));
				v18 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v12), simde_mm_setzero_si64());
				v19 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*(tjs_uint32 *)((char *)v12 + srcpitch)), simde_mm_setzero_si64());
				v20 = simde_m_paddb(v18, simde_m_psrlwi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(v12[1]), simde_mm_setzero_si64()), v18), v15), 8u));
				v21 = simde_m_paddb(
					v20,
					simde_m_psrlwi(
						simde_m_pmullw(
							simde_m_psubw(
								simde_m_paddb(
									v19,
									simde_m_psrlwi(
										simde_m_pmullw(
											simde_m_psubw(
												simde_m_punpcklbw(simde_mm_cvtsi32_si64(*(tjs_uint32 *)((char *)v12 + srcpitch + 4)), simde_mm_setzero_si64()),
												v19),
											v15),
										8u)),
								v20),
							v17),
						8u));
				v22 = simde_m_psrlqi(v21, 0x30u);
				v23 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v8), simde_mm_setzero_si64());
				v24 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v22));
				*v8 = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_paddw(v21, simde_m_psubw(v23, simde_m_psrlwi(simde_m_pmullw(v23, v24), 8u))), simde_mm_setzero_si64()));
				v9 += stepx;
				v10 += stepy;
				++v8;
			} while (v8 < v11);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPInterpLinTransAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	simde__m64    v10; // mm6
	simde__m64    v11; // mm6
	tjs_uint32   *v12; // edi
	tjs_int       v13; // ecx
	tjs_int       v14; // edx
	tjs_uint32 *  v15; // ebp
	tjs_uint32   *v16; // eax
	simde__m64    v18; // mm5
	simde__m64    v19; // mm5
	simde__m64    v21; // mm7
	simde__m64    v22; // mm1
	simde__m64    v23; // mm3
	simde__m64    v24; // mm1
	simde__m64    v25; // mm1
	simde__m64    v26; // mm3
	simde__m64    v27; // mm2
	simde__m64    v28; // mm3

	if (destlen > 0)
	{
		v10 = simde_mm_set1_pi16(((tjs_uint32)opa >> 7) + opa);
		v11 = v10;
		v12 = dest;
		v13 = sx;
		v14 = sy;
		v15 = &dest[destlen];
		if (dest < v15)
		{
			do
			{
				v16 = (tjs_uint32 *)((char *)&src[v13 >> 16] + srcpitch * (v14 >> 16));
				v18 = simde_mm_set1_pi16((tjs_uint32)(tjs_uint16)v13 >> 8);
				v19 = v18;
				v21 = simde_mm_set1_pi16(((tjs_uint32)(tjs_uint16)v14 >> 8) + ((tjs_uint32)(tjs_uint16)v14 >> 15));
				v22 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v16), simde_mm_setzero_si64());
				v23 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*(tjs_uint32 *)((char *)v16 + srcpitch)), simde_mm_setzero_si64());
				v24 = simde_m_paddb(v22, simde_m_psrlwi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(v16[1]), simde_mm_setzero_si64()), v22), v19), 8u));
				v25 = simde_m_psrlwi(
					simde_m_pmullw(
						simde_m_paddb(
							v24,
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_psubw(
										simde_m_paddb(
											v23,
											simde_m_psrlwi(
												simde_m_pmullw(
													simde_m_psubw(
														simde_m_punpcklbw(simde_mm_cvtsi32_si64(*(tjs_uint32 *)((char *)v16 + srcpitch + 4)), simde_mm_setzero_si64()),
														v23),
													v19),
												8u)),
										v24),
									v21),
								8u)),
						v11),
					8u);
				v26  = simde_m_psrlqi(v25, 0x30u);
				v27  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v12), simde_mm_setzero_si64());
				v28  = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v26));
				*v12 = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_paddw(v25, simde_m_psubw(v27, simde_m_psrlwi(simde_m_pmullw(v27, v28), 8u))), simde_mm_setzero_si64()));
				v13 += stepx;
				v14 += stepy;
				++v12;
			} while (v12 < v15);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPInterpLinTransConstAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	simde__m64    v10; // mm6
	simde__m64    v11; // mm6
	tjs_uint32   *v12; // edi
	tjs_int       v13; // ecx
	tjs_int       v14; // edx
	tjs_uint32 *  v15; // ebp
	tjs_uint32   *v16; // eax
	simde__m64    v18; // mm5
	simde__m64    v19; // mm5
	simde__m64    v21; // mm7
	simde__m64    v22; // mm1
	simde__m64    v23; // mm3
	simde__m64    v24; // mm1
	simde__m64    v25; // mm2

	if (destlen > 0)
	{
		v10 = simde_mm_set1_pi16(((tjs_uint32)opa >> 7) + opa);
		v11 = v10;
		v12 = dest;
		v13 = sx;
		v14 = sy;
		v15 = &dest[destlen];
		if (dest < v15)
		{
			do
			{
				v16  = (tjs_uint32 *)((char *)&src[v13 >> 16] + srcpitch * (v14 >> 16));
				v18  = simde_mm_set1_pi16((tjs_uint32)(tjs_uint16)v13 >> 8);
				v19  = v18;
				v21  = simde_mm_set1_pi16(((tjs_uint32)(tjs_uint16)v14 >> 8) + ((tjs_uint32)(tjs_uint16)v14 >> 15));
				v22  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v16), simde_mm_setzero_si64());
				v23  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*(tjs_uint32 *)((char *)v16 + srcpitch)), simde_mm_setzero_si64());
				v24  = simde_m_paddb(v22, simde_m_psrlwi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(v16[1]), simde_mm_setzero_si64()), v22), v19), 8u));
				v25  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v12), simde_mm_setzero_si64());
				*v12 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_paddb(
							v25,
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_psubw(
										simde_m_paddb(
											v24,
											simde_m_psrlwi(
												simde_m_pmullw(
													simde_m_psubw(
														simde_m_paddb(
															v23,
															simde_m_psrlwi(
																simde_m_pmullw(
																	simde_m_psubw(
																		simde_m_punpcklbw(
																			simde_mm_cvtsi32_si64(*(tjs_uint32 *)((char *)v16 + srcpitch + 4)),
																			simde_mm_setzero_si64()),
																		v23),
																	v19),
																8u)),
														v24),
													v21),
												8u)),
										v25),
									v11),
								8u)),
						simde_mm_setzero_si64()));
				v13 += stepx;
				v14 += stepy;
				++v12;
			} while (v12 < v15);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPInterpLinTransCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch))
{
	/* bilinear interpolation version */
	/* note that srcpitch unit is in byte */
	tjs_uint32 *  v8;  // edi
	tjs_int       v9;  // ecx
	tjs_int       v10; // edx
	tjs_uint32 *  v11; // ebp
	tjs_uint32   *v12; // eax
	simde__m64    v14; // mm5
	simde__m64    v15; // mm5
	simde__m64    v17; // mm7
	simde__m64    v18; // mm1
	simde__m64    v19; // mm3
	simde__m64    v20; // mm1

	if (destlen > 0)
	{
		v8  = dest;
		v9  = sx;
		v10 = sy;
		v11 = &dest[destlen];
		if (dest < v11)
		{
			do
			{
				v12 = (tjs_uint32 *)((char *)&src[v9 >> 16] + srcpitch * (v10 >> 16));
				v14 = simde_mm_set1_pi16((tjs_uint32)(tjs_uint16)v9 >> 8);
				v15 = v14;
				v17 = simde_mm_set1_pi16(((tjs_uint32)(tjs_uint16)v10 >> 8) + ((tjs_uint32)(tjs_uint16)v10 >> 15));
				v18 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v12), simde_mm_setzero_si64());
				v19 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*(tjs_uint32 *)((char *)v12 + srcpitch)), simde_mm_setzero_si64());
				v20 = simde_m_paddb(v18, simde_m_psrlwi(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(v12[1]), simde_mm_setzero_si64()), v18), v15), 8u));
				*v8 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_paddb(
							v20,
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_psubw(
										simde_m_paddb(
											v19,
											simde_m_psrlwi(
												simde_m_pmullw(
													simde_m_psubw(
														simde_m_punpcklbw(simde_mm_cvtsi32_si64(*(tjs_uint32 *)((char *)v12 + srcpitch + 4)), simde_mm_setzero_si64()),
														v19),
													v15),
												8u)),
										v20),
									v17),
								8u)),
						simde_mm_setzero_si64()));
				v9 += stepx;
				v10 += stepy;
				++v8;
			} while (v8 < v11);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPInterpStretchAdditiveAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep))
{
	/* stretching additive alpha blend with bilinear interpolation */
	simde__m64    v8;  // mm7
	simde__m64    v9;  // mm7
	tjs_uint32   *v10; // edi
	tjs_uint32    v11; // ebx
	tjs_uint32 *  v12; // ebp
	simde__m64    v13; // mm1
	simde__m64    v15; // mm3
	simde__m64    v16; // mm5
	simde__m64    v17; // mm5
	simde__m64    v18; // mm1
	simde__m64    v19; // mm1
	simde__m64    v20; // mm3
	simde__m64    v21; // mm2
	simde__m64    v22; // mm3

	if (destlen > 0)
	{
		v8  = simde_mm_set1_pi16(((tjs_uint32)blend_y >> 7) + blend_y);
		v9  = v8;
		v10 = dest;
		v11 = srcstart;
		v12 = &dest[destlen];
		if (dest < v12)
		{
			do
			{
				v13 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(src1[v11 >> 16]), simde_mm_setzero_si64());
				v15 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(src2[v11 >> 16]), simde_mm_setzero_si64());
				v16 = simde_mm_set1_pi16((tjs_uint16)v11 >> 8);
				v17 = v16;
				v18 = simde_m_paddb(
					v13,
					simde_m_psrlwi(
						simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(src1[(v11 >> 16) + 1]), simde_mm_setzero_si64()), v13), v17),
						8u));
				v19 = simde_m_paddb(
					v18,
					simde_m_psrlwi(
						simde_m_pmullw(
							simde_m_psubw(
								simde_m_paddb(
									v15,
									simde_m_psrlwi(
										simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(src2[(v11 >> 16) + 1]), simde_mm_setzero_si64()), v15), v17),
										8u)),
								v18),
							v9),
						8u));
				v20  = simde_m_psrlqi(v19, 0x30u);
				v21  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v10), simde_mm_setzero_si64());
				v22  = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v20));
				*v10 = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_paddw(v19, simde_m_psubw(v21, simde_m_psrlwi(simde_m_pmullw(v21, v22), 8u))), simde_mm_setzero_si64()));
				v11 += srcstep;
				++v10;
			} while (v10 < v12);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPInterpStretchAdditiveAlphaBlend_o_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	/* stretching additive alpha blend with bilinear interpolation */
	simde__m64    v9;  // mm7
	simde__m64    v10; // mm7
	simde__m64    v12; // mm6
	simde__m64    v13; // mm6
	tjs_uint32   *v14; // edi
	tjs_uint32    v15; // ebx
	tjs_uint32 *  v16; // ebp
	simde__m64    v17; // mm1
	simde__m64    v19; // mm3
	simde__m64    v20; // mm5
	simde__m64    v21; // mm5
	simde__m64    v22; // mm1
	simde__m64    v23; // mm1
	simde__m64    v24; // mm3
	simde__m64    v25; // mm2
	simde__m64    v26; // mm3

	if (destlen > 0)
	{
		v9  = simde_mm_set1_pi16(((tjs_uint32)blend_y >> 7) + blend_y);
		v10 = v9;
		v12 = simde_mm_set1_pi16(((tjs_uint32)opa >> 7) + opa);
		v13 = v12;
		v14 = dest;
		v15 = srcstart;
		v16 = &dest[destlen];
		if (dest < v16)
		{
			do
			{
				v17 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(src1[v15 >> 16]), simde_mm_setzero_si64());
				v19 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(src2[v15 >> 16]), simde_mm_setzero_si64());
				v20 = simde_mm_set1_pi16((tjs_uint16)v15 >> 8);
				v21 = v20;
				v22 = simde_m_paddb(
					v17,
					simde_m_psrlwi(
						simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(src1[(v15 >> 16) + 1]), simde_mm_setzero_si64()), v17), v21),
						8u));
				v23 = simde_m_psrlwi(
					simde_m_pmullw(
						simde_m_paddb(
							v22,
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_psubw(
										simde_m_paddb(
											v19,
											simde_m_psrlwi(
												simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(src2[(v15 >> 16) + 1]), simde_mm_setzero_si64()), v19), v21),
												8u)),
										v22),
									v10),
								8u)),
						v13),
					8u);
				v24  = simde_m_psrlqi(v23, 0x30u);
				v25  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v14), simde_mm_setzero_si64());
				v26  = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v24));
				*v14 = simde_mm_cvtsi64_si32(simde_m_packuswb(simde_m_paddw(v23, simde_m_psubw(v25, simde_m_psrlwi(simde_m_pmullw(v25, v26), 8u))), simde_mm_setzero_si64()));
				v15 += srcstep;
				++v14;
			} while (v14 < v16);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPInterpStretchConstAlphaBlend_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa))
{
	/* stretch copy with bilinear interpolation */
	simde__m64    v9;  // mm7
	simde__m64    v10; // mm7
	simde__m64    v12; // mm6
	simde__m64    v13; // mm6
	tjs_uint32   *v14; // edi
	tjs_uint32    v15; // ebx
	tjs_uint32 *  v16; // ebp
	simde__m64    v17; // mm1
	simde__m64    v19; // mm3
	simde__m64    v20; // mm5
	simde__m64    v21; // mm5
	simde__m64    v22; // mm1
	simde__m64    v23; // mm2

	if (destlen > 0)
	{
		v9  = simde_mm_set1_pi16(((tjs_uint32)blend_y >> 7) + blend_y);
		v10 = v9;
		v12 = simde_mm_set1_pi16(((tjs_uint32)opa >> 7) + opa);
		v13 = v12;
		v14 = dest;
		v15 = srcstart;
		v16 = &dest[destlen];
		if (dest < v16)
		{
			do
			{
				v17 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(src1[v15 >> 16]), simde_mm_setzero_si64());
				v19 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(src2[v15 >> 16]), simde_mm_setzero_si64());
				v20 = simde_mm_set1_pi16((tjs_uint16)v15 >> 8);
				v21 = v20;
				v22 = simde_m_paddb(
					v17,
					simde_m_psrlwi(
						simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(src1[(v15 >> 16) + 1]), simde_mm_setzero_si64()), v17), v21),
						8u));
				v23  = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v14), simde_mm_setzero_si64());
				*v14 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_paddb(
							v23,
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_psubw(
										simde_m_paddb(
											v22,
											simde_m_psrlwi(
												simde_m_pmullw(
													simde_m_psubw(
														simde_m_paddb(
															v19,
															simde_m_psrlwi(
																simde_m_pmullw(
																	simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(src2[(v15 >> 16) + 1]), simde_mm_setzero_si64()), v19),
																	v21),
																8u)),
														v22),
													v10),
												8u)),
										v23),
									v13),
								8u)),
						simde_mm_setzero_si64()));
				v15 += srcstep;
				++v14;
			} while (v14 < v16);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPInterpStretchCopy_c, (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep))
{
	/* stretch copy with bilinear interpolation */
	simde__m64   v8;  // mm7
	simde__m64   v9;  // mm7
	tjs_uint32 * v10; // edi
	tjs_uint32   v11; // ebx
	tjs_uint32 * v12; // ebp
	simde__m64   v13; // mm1
	simde__m64   v15; // mm3
	simde__m64   v16; // mm5
	simde__m64   v17; // mm5
	simde__m64   v18; // mm1

	if (destlen > 0)
	{
		v8  = simde_mm_set1_pi16(((tjs_uint32)blend_y >> 7) + blend_y);
		v9  = v8;
		v10 = dest;
		v11 = srcstart;
		v12 = &dest[destlen];
		if (dest < v12)
		{
			do
			{
				v13 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(src1[v11 >> 16]), simde_mm_setzero_si64());
				v15 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(src2[v11 >> 16]), simde_mm_setzero_si64());
				v16 = simde_mm_set1_pi16((tjs_uint16)v11 >> 8);
				v17 = v16;
				v18 = simde_m_paddb(
					v13,
					simde_m_psrlwi(
						simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(src1[(v11 >> 16) + 1]), simde_mm_setzero_si64()), v13), v17),
						8u));
				*v10 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_paddb(
							v18,
							simde_m_psrlwi(
								simde_m_pmullw(
									simde_m_psubw(
										simde_m_paddb(
											v15,
											simde_m_psrlwi(
												simde_m_pmullw(
													simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(src2[(v11 >> 16) + 1]), simde_mm_setzero_si64()), v15),
													v17),
												8u)),
										v18),
									v9),
								8u)),
						simde_mm_setzero_si64()));
				v11 += srcstep;
				++v10;
			} while (v10 < v12);
		}
	}
	simde_m_empty();
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
	tjs_uint32   *v9;  // edi
	simde__m64    v11; // mm3
	simde__m64    v12; // mm3
	simde__m64    v13; // mm0
	tjs_uint32 *  v14; // ebp
	simde__m64    v15; // mm5
	simde__m64    v16; // mm1
	simde__m64    v17; // [esp+0h] [ebp-28h]

	if (len > 0)
	{
		v9  = dest;
		v11 = simde_mm_set1_pi16(opa);
		v12 = v11;
		v13 = simde_m_por(simde_mm_cvtsi32_si64(sx), simde_m_psllqi(simde_mm_cvtsi32_si64(sy), 0x20u));
		v17 = simde_m_por(simde_mm_cvtsi32_si64(stepx), simde_m_psllqi(simde_mm_cvtsi32_si64(stepy), 0x20u));
		v14 = &dest[len];
		if (dest < v14)
		{
			do
			{
				v15 = simde_m_psradi(v13, 0x10u);
				v16 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v9), simde_mm_setzero_si64());
				*v9 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_psrlwi(
							simde_m_paddw(
								simde_m_psllwi(v16, 8u),
								simde_m_pmullw(
									simde_m_psubw(
										simde_m_punpcklbw(
											simde_mm_cvtsi32_si64(*(const tjs_uint32 *)((char *)&src[simde_mm_cvtsi64_si32(v15)] + srcpitch * simde_mm_cvtsi64_si32(simde_m_psrlqi(v15, 0x20u)))),
											simde_mm_setzero_si64()),
										v16),
									v12)),
							8u),
						simde_mm_setzero_si64()));
				v13 = simde_m_paddd(v13, v17);
				++v9;
			} while (v9 < v14);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPLinTransConstAlphaBlend_a_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa))
{
	tjs_uint32   *v9;  // edi
	simde__m64    v11; // mm3
	simde__m64    v12; // mm3
	simde__m64    v13; // mm0
	tjs_uint32 *  v14; // ebp
	simde__m64    v15; // mm5
	simde__m64    v16; // mm1
	simde__m64    v17; // [esp+0h] [ebp-28h]

	if (len > 0)
	{
		v9  = dest;
		v11 = simde_mm_set1_pi16(opa);
		v12 = v11;
		v13 = simde_m_por(simde_mm_cvtsi32_si64(sx), simde_m_psllqi(simde_mm_cvtsi32_si64(sy), 0x20u));
		v17 = simde_m_por(simde_mm_cvtsi32_si64(stepx), simde_m_psllqi(simde_mm_cvtsi32_si64(stepy), 0x20u));
		v14 = &dest[len];
		if (dest < v14)
		{
			do
			{
				v15 = simde_m_psradi(v13, 0x10u);
				v16 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v9), simde_mm_setzero_si64());
				*v9 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_psrlwi(
							simde_m_paddw(
								simde_m_psllwi(v16, 8u),
								simde_m_pmullw(
									simde_m_psubw(
										simde_m_punpcklbw(
											simde_mm_cvtsi32_si64(*(const tjs_uint32 *)((char *)&src[simde_mm_cvtsi64_si32(v15)] + srcpitch * simde_mm_cvtsi64_si32(simde_m_psrlqi(v15, 0x20u)))),
											simde_mm_setzero_si64()),
										v16),
									v12)),
							8u),
						simde_mm_setzero_si64()));
				v13 = simde_m_paddd(v13, v17);
				++v9;
			} while (v9 < v14);
		}
	}
	simde_m_empty();
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
	tjs_uint32 *v8;  // edi
	simde__m64  v9;  // mm0
	simde__m64  v10; // mm2
	tjs_uint32 *v11; // ebp
	simde__m64  v12; // mm5

	if (destlen > 0)
	{
		v8  = dest;
		v9  = simde_m_por(simde_mm_cvtsi32_si64(sx), simde_m_psllqi(simde_mm_cvtsi32_si64(sy), 0x20u));
		v10 = simde_m_por(simde_mm_cvtsi32_si64(stepx), simde_m_psllqi(simde_mm_cvtsi32_si64(stepy), 0x20u));
		v11 = &dest[destlen];
		if (dest < v11)
		{
			do
			{
				v12 = simde_m_psradi(v9, 0x10u);
				*v8 = *(const tjs_uint32 *)((char *)&src[simde_mm_cvtsi64_si32(v12)] + srcpitch * simde_mm_cvtsi64_si32(simde_m_psrlqi(v12, 0x20u)));
				v9  = simde_m_paddd(v9, v10);
				++v8;
			} while (v8 < v11);
		}
	}
	simde_m_empty();
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
	tjs_uint32   *v5;  // edi
	tjs_uint32    v6;  // ebx
	tjs_uint32 *  v7;  // ebp
	simde__m64    v8;  // mm4
	simde__m64    v9;  // mm2
	simde__m64    v10; // mm2
	simde__m64    v11; // mm1

	if (len > 0)
	{
		v5 = dest;
		v6 = srcstart;
		v7 = &dest[len];
		if (dest < v7)
		{
			do
			{
				v8  = simde_mm_cvtsi32_si64(src[v6 >> 16]);
				v9  = simde_m_psrlqi(v8, 0x18u);
				v10 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v9));
				v11 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v5), simde_mm_setzero_si64());
				*v5 = simde_mm_cvtsi64_si32(simde_m_paddusb(simde_m_packuswb(simde_m_psubw(v11, simde_m_psrlwi(simde_m_pmullw(v11, v10), 8u)), simde_mm_setzero_si64()), v8));
				v6 += srcstep;
				++v5;
			} while (v5 < v7);
		}
	}
	simde_m_empty();
}

TVP_GL_FUNC_DECL(void, TVPStretchAdditiveAlphaBlend_HDA_c, (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep))
{
	tjs_uint32   *v5;  // edi
	tjs_uint32    v6;  // ebx
	tjs_uint32 *  v7;  // ebp
	simde__m64    v8;  // mm4
	simde__m64    v9;  // mm2
	simde__m64    v10; // mm2
	simde__m64    v11; // mm1

	if (len > 0)
	{
		v5 = dest;
		v6 = srcstart;
		v7 = &dest[len];
		if (dest < v7)
		{
			do
			{
				v8  = simde_mm_cvtsi32_si64(src[v6 >> 16]);
				v9  = simde_m_psrlqi(v8, 0x18u);
				v10 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v9));
				v11 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v5), simde_mm_setzero_si64());
				*v5 = simde_mm_cvtsi64_si32(simde_m_paddusb(simde_m_packuswb(simde_m_psubw(v11, simde_m_psrlwi(simde_m_pmullw(v11, v10), 8u)), simde_mm_setzero_si64()), v8));
				v6 += srcstep;
				++v5;
			} while (v5 < v7);
		}
	}
	simde_m_empty();
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
	tjs_uint32   *v5;  // edi
	tjs_uint32    v6;  // ebx
	tjs_uint32 *  v7;  // ebp
	simde__m64    v8;  // mm3
	simde__m64    v9;  // mm5
	simde__m64    v10; // mm4
	simde__m64    v11; // mm5

	if (len > 0)
	{
		v5 = dest;
		v6 = srcstart;
		v7 = &dest[len];
		if (dest < v7)
		{
			do
			{
				v8  = simde_mm_cvtsi32_si64(src[v6 >> 16]);
				v9  = simde_m_psrlqi(v8, 0x18u);
				v10 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v5), simde_mm_setzero_si64());
				v11 = simde_mm_set1_pi16((tjs_uint16)simde_mm_cvtsi64_si32(v9));
				*v5 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_psrlwi(simde_m_paddw(simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(v8, simde_mm_setzero_si64()), v10), v11), simde_m_psllwi(v10, 8u)), 8u),
						simde_mm_setzero_si64()));
				v6 += srcstep;
				++v5;
			} while (v5 < v7);
		}
	}
	simde_m_empty();
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
	tjs_uint32   *v6;  // edi
	tjs_uint32    v7;  // ebx
	simde__m64    v9;  // mm7
	tjs_uint32 *  v10; // ebp
	simde__m64    v11; // mm7
	simde__m64    v12; // mm1

	if (len > 0)
	{
		v6  = dest;
		v7  = srcstart;
		v9  = simde_mm_set1_pi16(opa);
		v10 = &dest[len];
		v11 = v9;
		if (dest < v10)
		{
			do
			{
				v12 = simde_m_punpcklbw(simde_mm_cvtsi32_si64(*v6), simde_mm_setzero_si64());
				*v6 = simde_mm_cvtsi64_si32(
					simde_m_packuswb(
						simde_m_psrlwi(
							simde_m_paddw(
								simde_m_psllwi(v12, 8u),
								simde_m_pmullw(simde_m_psubw(simde_m_punpcklbw(simde_mm_cvtsi32_si64(src[v7 >> 16]), simde_mm_setzero_si64()), v12), v11)),
							8u),
						simde_mm_setzero_si64()));
				v7 += srcstep;
				++v6;
			} while (v6 < v10);
		}
	}
	simde_m_empty();
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
	tjs_uint32 * v5; // edi
	tjs_uint32   v6; // ebx
	tjs_uint32 * v7; // ebp

	if (destlen > 0)
	{
		v5 = dest;
		v6 = srcstart;
		v7 = &dest[destlen];
		if (dest < v7)
		{
			do
			{
				*v5 = simde_mm_cvtsi64_si32(simde_mm_cvtsi32_si64(src[v6 >> 16]));
				v6 += srcstep;
				++v5;
			} while (v5 < v7);
		}
	}
	simde_m_empty();
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

