
#pragma once

#include "tjsCommHead.h"

#define TVPFillARGB TVPFillARGB_c
#define TVPFillColor TVPFillColor_c
#define TVPInterpLinTransAdditiveAlphaBlend TVPInterpLinTransAdditiveAlphaBlend_c
#define TVPInterpLinTransAdditiveAlphaBlend_o TVPInterpLinTransAdditiveAlphaBlend_o_c
#define TVPInterpLinTransConstAlphaBlend TVPInterpLinTransConstAlphaBlend_c
#define TVPInterpLinTransCopy TVPInterpLinTransCopy_c
#define TVPInterpStretchAdditiveAlphaBlend TVPInterpStretchAdditiveAlphaBlend_c
#define TVPInterpStretchAdditiveAlphaBlend_o TVPInterpStretchAdditiveAlphaBlend_o_c
#define TVPInterpStretchConstAlphaBlend TVPInterpStretchConstAlphaBlend_c
#define TVPInterpStretchCopy TVPInterpStretchCopy_c
#define TVPLinTransAdditiveAlphaBlend TVPLinTransAdditiveAlphaBlend_c
#define TVPLinTransAdditiveAlphaBlend_a TVPLinTransAdditiveAlphaBlend_a_c
#define TVPLinTransAdditiveAlphaBlend_ao TVPLinTransAdditiveAlphaBlend_ao_c
#define TVPLinTransAdditiveAlphaBlend_HDA TVPLinTransAdditiveAlphaBlend_HDA_c
#define TVPLinTransAdditiveAlphaBlend_HDA_o TVPLinTransAdditiveAlphaBlend_HDA_o_c
#define TVPLinTransAdditiveAlphaBlend_o TVPLinTransAdditiveAlphaBlend_o_c
#define TVPLinTransAlphaBlend TVPLinTransAlphaBlend_c
#define TVPLinTransAlphaBlend_a TVPLinTransAlphaBlend_a_c
#define TVPLinTransAlphaBlend_ao TVPLinTransAlphaBlend_ao_c
#define TVPLinTransAlphaBlend_d TVPLinTransAlphaBlend_d_c
#define TVPLinTransAlphaBlend_do TVPLinTransAlphaBlend_do_c
#define TVPLinTransAlphaBlend_HDA TVPLinTransAlphaBlend_HDA_c
#define TVPLinTransAlphaBlend_HDA_o TVPLinTransAlphaBlend_HDA_o_c
#define TVPLinTransAlphaBlend_o TVPLinTransAlphaBlend_o_c
#define TVPLinTransColorCopy TVPLinTransColorCopy_c
#define TVPLinTransConstAlphaBlend TVPLinTransConstAlphaBlend_c
#define TVPLinTransConstAlphaBlend_a TVPLinTransConstAlphaBlend_a_c
#define TVPLinTransConstAlphaBlend_d TVPLinTransConstAlphaBlend_d_c
#define TVPLinTransConstAlphaBlend_HDA TVPLinTransConstAlphaBlend_HDA_c
#define TVPLinTransCopy TVPLinTransCopy_c
#define TVPLinTransCopyOpaqueImage TVPLinTransCopyOpaqueImage_c
#define TVPStretchAdditiveAlphaBlend TVPStretchAdditiveAlphaBlend_c
#define TVPStretchAdditiveAlphaBlend_HDA TVPStretchAdditiveAlphaBlend_HDA_c
#define TVPStretchAdditiveAlphaBlend_HDA_o TVPStretchAdditiveAlphaBlend_HDA_o_c
#define TVPStretchAlphaBlend TVPStretchAlphaBlend_c
#define TVPStretchAlphaBlend_a TVPStretchAlphaBlend_a_c
#define TVPStretchAlphaBlend_ao TVPStretchAlphaBlend_ao_c
#define TVPStretchAlphaBlend_d TVPStretchAlphaBlend_d_c
#define TVPStretchAlphaBlend_do TVPStretchAlphaBlend_do_c
#define TVPStretchAlphaBlend_HDA TVPStretchAlphaBlend_HDA_c
#define TVPStretchAlphaBlend_HDA_o TVPStretchAlphaBlend_HDA_o_c
#define TVPStretchAlphaBlend_o TVPStretchAlphaBlend_o_c
#define TVPStretchColorCopy TVPStretchColorCopy_c
#define TVPStretchConstAlphaBlend TVPStretchConstAlphaBlend_c
#define TVPStretchConstAlphaBlend_a TVPStretchConstAlphaBlend_a_c
#define TVPStretchConstAlphaBlend_d TVPStretchConstAlphaBlend_d_c
#define TVPStretchConstAlphaBlend_HDA TVPStretchConstAlphaBlend_HDA_c
#define TVPStretchCopy TVPStretchCopy_c
#define TVPStretchCopyOpaqueImage TVPStretchCopyOpaqueImage_c

#define TVP_INLINE_FUNC inline
#define TVP_GL_FUNCNAME(funcname) funcname
#define TVP_GL_FUNC_DECL(rettype, funcname, arg)  rettype TVP_GL_FUNCNAME(funcname) arg
#define TVP_GL_FUNC_STATIC_DECL(rettype, funcname, arg)  static rettype TVP_GL_FUNCNAME(funcname) arg
#define TVP_GL_FUNC_INLINE_DECL(rettype, funcname, arg)  static rettype TVP_INLINE_FUNC TVP_GL_FUNCNAME(funcname) arg
#define TVP_GL_FUNC_EXTERN_DECL(rettype, funcname, arg)  extern rettype TVP_GL_FUNCNAME(funcname) arg
#define TVP_GL_FUNC_PTR_EXTERN_DECL TVP_GL_FUNC_EXTERN_DECL

TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPFillARGB,  (tjs_uint32 *dest, tjs_int len, tjs_uint32 value));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPFillColor,  (tjs_uint32 *dest, tjs_int len, tjs_uint32 color));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPInterpLinTransAdditiveAlphaBlend,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPInterpLinTransAdditiveAlphaBlend_o,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPInterpLinTransConstAlphaBlend,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPInterpLinTransCopy,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPInterpStretchAdditiveAlphaBlend,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPInterpStretchAdditiveAlphaBlend_o,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPInterpStretchConstAlphaBlend,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPInterpStretchCopy,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int blend_y, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAdditiveAlphaBlend,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAdditiveAlphaBlend_a,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAdditiveAlphaBlend_ao,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAdditiveAlphaBlend_HDA,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAdditiveAlphaBlend_HDA_o,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAdditiveAlphaBlend_o,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAlphaBlend,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAlphaBlend_a,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAlphaBlend_ao,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAlphaBlend_d,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAlphaBlend_do,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAlphaBlend_HDA,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAlphaBlend_HDA_o,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransAlphaBlend_o,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransColorCopy,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransConstAlphaBlend,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransConstAlphaBlend_a,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransConstAlphaBlend_d,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransConstAlphaBlend_HDA,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransCopy,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPLinTransCopyOpaqueImage,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAdditiveAlphaBlend,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAdditiveAlphaBlend_HDA,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAdditiveAlphaBlend_HDA_o,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAlphaBlend,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAlphaBlend_a,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAlphaBlend_ao,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAlphaBlend_d,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAlphaBlend_do,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAlphaBlend_HDA,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAlphaBlend_HDA_o,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchAlphaBlend_o,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchColorCopy,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchConstAlphaBlend,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchConstAlphaBlend_a,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchConstAlphaBlend_d,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchConstAlphaBlend_HDA,  (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchCopy,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPStretchCopyOpaqueImage,  (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep));
TVP_GL_FUNC_PTR_EXTERN_DECL(void, TVPCreateTable, (void));
