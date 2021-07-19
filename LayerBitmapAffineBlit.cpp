
#include "tjsCommHead.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapAffineBlit.h"

template <typename tFuncStretch, typename tFuncAffine>
static void TVPDoAffineLoop(
		tFuncStretch stretch,
		tFuncAffine affine,
		tjs_int sxs,
		tjs_int sys,
		tjs_uint8 *dest,
		tjs_int l,
		tjs_int len,
		const tjs_uint8 *src,
		tjs_int srcpitch,
		tjs_int sxl,
		tjs_int syl,
		const tTVPRect & srcrect);

template <typename tFuncStretch, typename tFuncAffine>
static void TVPDoBilinearAffineLoop(
		tFuncStretch stretch,
		tFuncAffine affine,
		tjs_int sxs,
		tjs_int sys,
		tjs_uint8 *dest,
		tjs_int l,
		tjs_int len,
		const tjs_uint8 *src,
		tjs_int srcpitch,
		tjs_int sxl,
		tjs_int syl,
		const tTVPRect & srccliprect,
		const tTVPRect & srcrect);

struct PartialAffineBltParam {
  tTVPBaseBitmap *self;
  tjs_uint8 *dest;
  tjs_int destpitch;
  tjs_int yc;
  tjs_int yclim;
  tjs_int scanlinestart;
  tjs_int scanlineend;
  tjs_int *points_x;
  tjs_int *points_y;
  tTVPRect *refrect;
  tjs_int sxstep;
  tjs_int systep;
  tTVPRect *destrect;
  tTVPBBBltMethod method;
  tjs_int opa;
  bool hda;
  tTVPBBStretchType type;
  bool clear;
  tjs_uint32 clearcolor;
  const tjs_uint8 *src;
  tjs_int srcpitch;
  tTVPRect *srccliprect;
  tTVPRect *srcrect;
  // below variable returns value to main thread.
  tjs_int leftlimit;
  tjs_int rightlimit;
  tjs_int mostupper;
  tjs_int mostbottom;
  bool firstline;
};
static void TJS_USERENTRY PartialAffineBltEntry(void *param);
void PartialAffineBlt(PartialAffineBltParam *param);

int InternalAffineBlt(tTVPBaseBitmap *dest, tTVPRect destrect, const tTVPBaseBitmap *ref,
	tTVPRect refrect, const tTVPPointD * points,
		tTVPBBBltMethod method, tjs_int opa,
		tTVPRect * updaterect = NULL,
		bool hda = true, tTVPBBStretchType mode = stNearest, bool clear = false,
			tjs_uint32 clearcolor = 0);


#define TVP_GL_FUNC_PTR_DECL(rettype, funcname, arg) rettype (__cdecl * funcname) arg

/* add here compiler specific inline directives */
#if defined( __BORLANDC__ ) || ( _MSC_VER )
	#define TVP_INLINE_FUNC __inline
#elif defined(__GNUC__)
	#define TVP_INLINE_FUNC inline
#else
	#define TVP_INLINE_FUNC 
#endif

static tjs_uint32 TVP_INLINE_FUNC TVPSaturatedAdd(tjs_uint32 a, tjs_uint32 b)
{
	/* Add each byte of packed 8bit values in two 32bit uint32, with saturation. */
	tjs_uint32 tmp = (  ( a & b ) + ( ((a ^ b)>>1) & 0x7f7f7f7f)  ) & 0x80808080;
	tmp = (tmp<<1) - (tmp>>7);
	return (a + b - tmp) | tmp;
}

/*
	TVPAddAlphaBlend_dest_src[_o]
	dest/src    :    a(additive-alpha)  d(alpha)  n(none alpha)
	_o          :    with opacity
*/

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_n_a(tjs_uint32 dest, tjs_uint32 src)
{
	tjs_uint32 sopa = (~src) >> 24;
	return TVPSaturatedAdd((((dest & 0xff00ff)*sopa >> 8) & 0xff00ff) + 
		(((dest & 0xff00)*sopa >> 8) & 0xff00), src);
}

static tjs_uint32 TVP_INLINE_FUNC TVPAddAlphaBlend_n_a_o(tjs_uint32 dest, tjs_uint32 src, tjs_int opa)
{
	src = (((src & 0xff00ff)*opa >> 8) & 0xff00ff) + (((src >> 8) & 0xff00ff)*opa & 0xff00ff00);
	return TVPAddAlphaBlend_n_a(dest, src);
}

static tjs_uint32 TVP_INLINE_FUNC TVPBlendARGB(tjs_uint32 b, tjs_uint32 a, tjs_int ratio)
{
	/* returns a * ratio + b * (1 - ratio) */
	tjs_uint32 b2;
	tjs_uint32 t;
	b2 = b & 0x00ff00ff;
	t = (b2 + (((a & 0x00ff00ff) - b2) * ratio >> 8)) & 0x00ff00ff;
	b2 = (b & 0xff00ff00) >> 8;
	return t + 
		(((b2 + (( ((a & 0xff00ff00) >> 8) - b2) * ratio >> 8)) << 8)& 0xff00ff00);
}


//---------------------------------------------------------------------------
// To forcing bilinear interpolation, define TVP_FORCE_BILINEAR.

#ifdef TVP_FORCE_BILINEAR
	#define TVP_BILINEAR_FORCE_COND true
#else
	#define TVP_BILINEAR_FORCE_COND false
#endif
//---------------------------------------------------------------------------
static float sBmFactor[] =
{
  59, // bmCopy,
  59, // bmCopyOnAlpha,
  52, // bmAlpha,
  52, // bmAlphaOnAlpha,
  61, // bmAdd,
  59, // bmSub,
  45, // bmMul,
  10, // bmDodge,
  58, // bmDarken,
  56, // bmLighten,
  42, // bmScreen,
  52, // bmAddAlpha,
  52, // bmAddAlphaOnAddAlpha,
  52, // bmAddAlphaOnAlpha,
  52, // bmAlphaOnAddAlpha,
  52, // bmCopyOnAddAlpha,
  32, // bmPsNormal,
  30, // bmPsAdditive,
  29, // bmPsSubtractive,
  27, // bmPsMultiplicative,
  27, // bmPsScreen,
  15, // bmPsOverlay,
  15, // bmPsHardLight,
  10, // bmPsSoftLight,
  10, // bmPsColorDodge,
  10, // bmPsColorDodge5,
  10, // bmPsColorBurn,
  29, // bmPsLighten,
  29, // bmPsDarken,
  29, // bmPsDifference,
  26, // bmPsDifference5,
  66, // bmPsExclusion
};

//---------------------------------------------------------------------------
static tjs_int GetAdaptiveThreadNum(tjs_int pixelNum, float factor)
{
  if (pixelNum >= factor * 500)
    return TVPGetThreadNum();
  else
    return 1;
}

//---------------------------------------------------------------------------
// template function for strech loop
//---------------------------------------------------------------------------

// define function pointer types for stretching line
typedef TVP_GL_FUNC_PTR_DECL(void, tTVPStretchFunction,
	(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int srcstart, tjs_int srcstep));
typedef TVP_GL_FUNC_PTR_DECL(void, tTVPStretchWithOpacityFunction,
	(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int srcstart, tjs_int srcstep, tjs_int opa));
typedef TVP_GL_FUNC_PTR_DECL(void, tTVPBilinearStretchFunction,
	(tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2,
	tjs_int blend_y, tjs_int srcstart, tjs_int srcstep));
typedef TVP_GL_FUNC_PTR_DECL(void, tTVPBilinearStretchWithOpacityFunction,
	(tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2,
	tjs_int blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa));

//---------------------------------------------------------------------------

// declare stretching function object class
class tTVPStretchFunctionObject
{
	tTVPStretchFunction Func;
public:
	tTVPStretchFunctionObject(tTVPStretchFunction func) : Func(func) {;}
	void operator () (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int srcstart, tjs_int srcstep)
	{
		Func(dest, len, src, srcstart, srcstep);
	}
};

class tTVPStretchWithOpacityFunctionObject
{
	tTVPStretchWithOpacityFunction Func;
	tjs_int Opacity;
public:
	tTVPStretchWithOpacityFunctionObject(tTVPStretchWithOpacityFunction func, tjs_int opa) :
		Func(func), Opacity(opa) {;}
	void operator () (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int srcstart, tjs_int srcstep)
	{
		Func(dest, len, src, srcstart, srcstep, Opacity);
	}
};

class tTVPBilinearStretchFunctionObject
{
protected:
	tTVPBilinearStretchFunction Func;
public:
	tTVPBilinearStretchFunctionObject(tTVPBilinearStretchFunction func) : Func(func) {;}
	void operator () (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2,
	tjs_int blend_y, tjs_int srcstart, tjs_int srcstep)
	{
		Func(dest, destlen, src1, src2, blend_y, srcstart, srcstep);
	}
};

#define TVP_DEFINE_BILINEAR_STRETCH_FUNCTION(func, one) class \
t##func##FunctionObject : \
	public tTVPBilinearStretchFunctionObject \
{ \
public: \
	t##func##FunctionObject() : \
		tTVPBilinearStretchFunctionObject(func) {;} \
	void DoOnePixel(tjs_uint32 *dest, tjs_uint32 color) \
	{ one; } \
};


class tTVPBilinearStretchWithOpacityFunctionObject
{
protected:
	tTVPBilinearStretchWithOpacityFunction Func;
	tjs_int Opacity;
public:
	tTVPBilinearStretchWithOpacityFunctionObject(tTVPBilinearStretchWithOpacityFunction func, tjs_int opa) :
		Func(func), Opacity(opa) {;}
	void operator () (tjs_uint32 *dest, tjs_int destlen, const tjs_uint32 *src1, const tjs_uint32 *src2,
	tjs_int blend_y, tjs_int srcstart, tjs_int srcstep)
	{
		Func(dest, destlen, src1, src2, blend_y, srcstart, srcstep, Opacity);
	}
};

#define TVP_DEFINE_BILINEAR_STRETCH_WITH_OPACITY_FUNCTION(func, one) class \
t##func##FunctionObject : \
	public tTVPBilinearStretchWithOpacityFunctionObject \
{ \
public: \
	t##func##FunctionObject(tjs_int opa) : \
		tTVPBilinearStretchWithOpacityFunctionObject(func, opa) {;} \
	void DoOnePixel(tjs_uint32 *dest, tjs_uint32 color) \
	{ one; } \
};

//---------------------------------------------------------------------------

// declare streting function object for bilinear interpolation
TVP_DEFINE_BILINEAR_STRETCH_FUNCTION(
	TVPInterpStretchCopy,
	*dest = color);

TVP_DEFINE_BILINEAR_STRETCH_WITH_OPACITY_FUNCTION(
	TVPInterpStretchConstAlphaBlend,
	*dest = TVPBlendARGB(*dest, color, Opacity));

TVP_DEFINE_BILINEAR_STRETCH_FUNCTION(
	TVPInterpStretchAdditiveAlphaBlend,
	*dest = TVPAddAlphaBlend_n_a(*dest, color));

TVP_DEFINE_BILINEAR_STRETCH_WITH_OPACITY_FUNCTION(
	TVPInterpStretchAdditiveAlphaBlend_o,
	*dest = TVPAddAlphaBlend_n_a_o(*dest, color, Opacity));

//---------------------------------------------------------------------------

// declare stretching loop function
#define TVP_DoStretchLoop_ARGS  x_ref_start, y_ref_start, x_len, y_len, \
						destp, destpitch, x_step, \
						y_step, refp, refpitch
template <typename tFunc>
void TVPDoStretchLoop(
		tFunc func,
		tjs_int x_ref_start,
		tjs_int y_ref_start,
		tjs_int x_len, tjs_int y_len,
		tjs_uint8 * destp, tjs_int destpitch,
		tjs_int x_step, tjs_int y_step,
		const tjs_uint8 * refp, tjs_int refpitch)
{
	tjs_int y_ref = y_ref_start;
	while(y_len--)
	{
		func(
			(tjs_uint32*)destp,
			x_len,
			(const tjs_uint32*)(refp + (y_ref>>16)*refpitch),
			x_ref_start,
			x_step);
		y_ref += y_step;
		destp += destpitch;
	}
}
//---------------------------------------------------------------------------


// declare stretching loop function for bilinear interpolation

#define TVP_DoBilinearStretchLoop_ARGS  rw, rh, dw, dh, \
						srccliprect, x_ref_start, y_ref_start, x_len, y_len, \
						destp, destpitch, x_step, \
						y_step, refp, refpitch
template <typename tStretchFunc>
void TVPDoBiLinearStretchLoop(
		tStretchFunc stretch,
		tjs_int rw, tjs_int rh,
		tjs_int dw, tjs_int dh,
		const tTVPRect & srccliprect,
		tjs_int x_ref_start,
		tjs_int y_ref_start,
		tjs_int x_len, tjs_int y_len,
		tjs_uint8 * destp, tjs_int destpitch,
		tjs_int x_step, tjs_int y_step,
		const tjs_uint8 * refp, tjs_int refpitch)
{
/*
	memo
          0         1         2         3         4
          .         .         .         .                  center = 1.5 = (4-1) / 2
          ------------------------------                 reference area
     ----------++++++++++----------++++++++++
                         ^                                 4 / 1  step 4   ofs =  1.5   = ((4-1) - (1-1)*4) / 2
               ^                   ^                       4 / 2  step 2   ofs =  0.5   = ((4-1) - (2-1)*2) / 2
          ^         ^         ^         *                  4 / 4  step 1   ofs =  0     = ((4-1) - (4-1)*1) / 2
         *       ^       ^       ^       *                 4 / 5  steo 0.8 ofs = -0.1   = ((4-1) - (5-1)*0.8) / 2
        *    ^    ^    ^    ^    ^    ^    *               4 / 8  step 0.5 ofs = -0.25

*/



	// adjust start point
	if(x_step >= 0)
		x_ref_start += (((rw-1)<<16) - (dw-1)*x_step)/2;
	else
		x_ref_start -= (((rw-1)<<16) + (dw-1)*x_step)/2 - x_step;
	if(y_step >= 0)
		y_ref_start += (((rh-1)<<16) - (dh-1)*y_step)/2;
	else
		y_ref_start -= (((rh-1)<<16) + (dh-1)*y_step)/2 - y_step;

	// horizontal destination line is splitted into three parts;
	// 1. left fraction (x_ref < srccliprect.left               (lf)
	//                or x_ref >= srccliprect.right - 1)
	// 2. center                                 (c)
	// 3. right fraction (x_ref >= srccliprect.right - 1        (rf)
	//                or x_ref < srccliprect.left)

	tjs_int ref_left_limit  = (srccliprect.left)<<16;
	tjs_int ref_right_limit = (srccliprect.right-1)<<16;

	tjs_int y_ref = y_ref_start;
	while(y_len--)
	{
		tjs_int y1 = y_ref >> 16;
		tjs_int y2 = y1+1;
		tjs_int y_blend = (y_ref & 0xffff) >> 8;
		if(y1 < srccliprect.top)
			y1 = srccliprect.top;
		else if(y1 >= srccliprect.bottom)
			y1 = srccliprect.bottom - 1;
		if(y2 < srccliprect.top)
			y2 = srccliprect.top;
		else if(y2 >= srccliprect.bottom)
			y2 = srccliprect.bottom - 1;

		const tjs_uint32 * l1 =
			(const tjs_uint32*)(refp + refpitch * y1);
		const tjs_uint32 * l2 =
			(const tjs_uint32*)(refp + refpitch * y2);


		// perform left and right fractions
		tjs_int x_remain = x_len;
		tjs_uint32 * dp;
		tjs_int x_ref;

		// from last point
		if(x_remain)
		{
			dp = (tjs_uint32*)destp + (x_len - 1);
			x_ref = x_ref_start + (x_len - 1) * x_step;
			if(x_ref < ref_left_limit)
			{
				tjs_uint color =
					TVPBlendARGB(
						*(l1 + srccliprect.left),
						*(l2 + srccliprect.left), y_blend);
				do
					stretch.DoOnePixel(dp, color), dp-- , x_ref -= x_step, x_remain --;
				while(x_ref < ref_left_limit && x_remain);
			}
			else if(x_ref >= ref_right_limit)
			{
				tjs_uint color =
					TVPBlendARGB(
						*(l1 + srccliprect.right - 1),
						*(l2 + srccliprect.right - 1), y_blend);
				do
					stretch.DoOnePixel(dp, color), dp-- , x_ref -= x_step, x_remain --;
				while(x_ref >= ref_right_limit && x_remain);
			}
		}

		// from first point
		if(x_remain)
		{
			dp = (tjs_uint32*)destp;
			x_ref = x_ref_start;
			if(x_ref < ref_left_limit)
			{
				tjs_uint color =
					TVPBlendARGB(
						*(l1 + srccliprect.left),
						*(l2 + srccliprect.left), y_blend);
				do
					stretch.DoOnePixel(dp, color), dp++ , x_ref += x_step, x_remain --;
				while(x_ref < ref_left_limit && x_remain);
			}
			else if(x_ref >= ref_right_limit)
			{
				tjs_uint color =
					TVPBlendARGB(
						*(l1 + srccliprect.right - 1),
						*(l2 + srccliprect.right - 1), y_blend);
				do
					stretch.DoOnePixel(dp, color), dp++ , x_ref += x_step, x_remain --;
				while(x_ref >= ref_right_limit && x_remain);
			}
		}

		// perform center part
		// (this may take most time of this function)
		if(x_remain)
		{
			stretch(
				dp,
				x_remain,
				l1, l2, y_blend,
				x_ref,
				x_step);
		}

		// step to the next line
		y_ref += y_step;
		destp += destpitch;
	}
}


//---------------------------------------------------------------------------
// template function for affine loop
//---------------------------------------------------------------------------
// define function pointer types for transforming line
typedef TVP_GL_FUNC_PTR_DECL(void, tTVPAffineFunction,
	(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
typedef TVP_GL_FUNC_PTR_DECL(void, tTVPAffineWithOpacityFunction,
	(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));
typedef TVP_GL_FUNC_PTR_DECL(void, tTVPBilinearAffineFunction,
	(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch));
typedef TVP_GL_FUNC_PTR_DECL(void, tTVPBilinearAffineWithOpacityFunction,
	(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa));

//---------------------------------------------------------------------------

// declare affine function object class
class tTVPAffineFunctionObject
{
	tTVPAffineFunction Func;
public:
	tTVPAffineFunctionObject(tTVPAffineFunction func) : Func(func) {;}
	void operator () (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch)
	{
		Func(dest, len, src, sx, sy, stepx, stepy, srcpitch);
	}
};

class tTVPAffineWithOpacityFunctionObject
{
	tTVPAffineWithOpacityFunction Func;
	tjs_int Opacity;
public:
	tTVPAffineWithOpacityFunctionObject(tTVPAffineWithOpacityFunction func, tjs_int opa) :
		Func(func), Opacity(opa) {;}
	void operator () (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch)
	{
		Func(dest, len, src, sx, sy, stepx, stepy, srcpitch, Opacity);
	}
};

class tTVPBilinearAffineFunctionObject
{
protected:
	tTVPBilinearAffineFunction Func;
public:
	tTVPBilinearAffineFunctionObject(tTVPBilinearAffineFunction func) : Func(func) {;}
	void operator () (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch)
	{
		Func(dest, len, src, sx, sy, stepx, stepy, srcpitch);
	}
};

#define TVP_DEFINE_BILINEAR_AFFINE_FUNCTION(func, one) class \
t##func##FunctionObject : \
	public tTVPBilinearAffineFunctionObject \
{ \
public: \
	t##func##FunctionObject() : \
		tTVPBilinearAffineFunctionObject(func) {;} \
	void DoOnePixel(tjs_uint32 *dest, tjs_uint32 color) \
	{ one; } \
};


class tTVPBilinearAffineWithOpacityFunctionObject
{
protected:
	tTVPBilinearAffineWithOpacityFunction Func;
	tjs_int Opacity;
public:
	tTVPBilinearAffineWithOpacityFunctionObject(tTVPBilinearAffineWithOpacityFunction func, tjs_int opa) :
		Func(func), Opacity(opa) {;}
	void operator () (tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src,
	tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch)
	{
		Func(dest, len, src, sx, sy, stepx, stepy, srcpitch, Opacity);
	}
};

#define TVP_DEFINE_BILINEAR_AFFINE_WITH_OPACITY_FUNCTION(func, one) class \
t##func##FunctionObject : \
	public tTVPBilinearAffineWithOpacityFunctionObject \
{ \
public: \
	t##func##FunctionObject(tjs_int opa) : \
		tTVPBilinearAffineWithOpacityFunctionObject(func, opa) {;} \
	void DoOnePixel(tjs_uint32 *dest, tjs_uint32 color) \
	{ one; } \
};

//---------------------------------------------------------------------------

// declare affine transformation function object for bilinear interpolation
TVP_DEFINE_BILINEAR_AFFINE_FUNCTION(
	TVPInterpLinTransCopy,
	*dest = color);

TVP_DEFINE_BILINEAR_AFFINE_WITH_OPACITY_FUNCTION(
	TVPInterpLinTransConstAlphaBlend,
	*dest = TVPBlendARGB(*dest, color, Opacity));


TVP_DEFINE_BILINEAR_AFFINE_FUNCTION(
	TVPInterpLinTransAdditiveAlphaBlend,
	*dest = TVPAddAlphaBlend_n_a(*dest, color));

TVP_DEFINE_BILINEAR_AFFINE_WITH_OPACITY_FUNCTION(
	TVPInterpLinTransAdditiveAlphaBlend_o,
	*dest = TVPAddAlphaBlend_n_a_o(*dest, color, Opacity));

//---------------------------------------------------------------------------

// declare affine loop function
#define TVP_DoAffineLoop_ARGS  sxs, sys, \
		dest, l, len, src, srcpitch, sxl, syl, srcrect
template <typename tFuncStretch, typename tFuncAffine>
static void TVPDoAffineLoop(
		tFuncStretch stretch,
		tFuncAffine affine,
		tjs_int sxs,
		tjs_int sys,
		tjs_uint8 *dest,
		tjs_int l,
		tjs_int len,
		const tjs_uint8 *src,
		tjs_int srcpitch,
		tjs_int sxl,
		tjs_int syl,
		const tTVPRect & srcrect)
{
	tjs_int len_remain = len;

	// skip "out of source rectangle" points
	// from last point
	sxl += 32768; // do +0.5 to rounding
	syl += 32768; // do +0.5 to rounding

	tjs_int spx, spy;
	tjs_uint32 *dp;
	dp = (tjs_uint32*)dest + l + len-1;
	spx = (len-1)*sxs + sxl;
	spy = (len-1)*sys + syl;

	while(len_remain > 0)
	{
		tjs_int sx, sy;
		sx = spx >> 16;
		sy = spy >> 16;
		if(sx >= srcrect.left && sx < srcrect.right &&
			sy >= srcrect.top && sy < srcrect.bottom)
			break;
		dp--;
		spx -= sxs;
		spy -= sys;
		len_remain --;
	}

	// from first point
	spx = sxl;
	spy = syl;
	dp = (tjs_uint32*)dest + l;

	while(len_remain > 0)
	{
		tjs_int sx, sy;
		sx = spx >> 16;
		sy = spy >> 16;
		if(sx >= srcrect.left && sx < srcrect.right &&
			sy >= srcrect.top && sy < srcrect.bottom)
			break;
		dp++;
		spx += sxs;
		spy += sys;
		len_remain --;
	}

	if(len_remain > 0)
	{
		// transfer a line
		if(sys == 0)
			stretch(dp, len_remain,
				(tjs_uint32*)(src + (spy>>16) * srcpitch), spx, sxs);
		else
			affine(dp, len_remain,
				(tjs_uint32*)src, spx, spy, sxs, sys, srcpitch);
	}
}

//---------------------------------------------------------------------------
// declare affine loop function for bilinear interpolation

#define TVP_DoBilinearAffineLoop_ARGS  sxs, sys, \
		dest, l, len, src, srcpitch, sxl, syl, srccliprect, srcrect
template <typename tFuncStretch, typename tFuncAffine>
void TVPDoBilinearAffineLoop(
		tFuncStretch stretch,
		tFuncAffine affine,
		tjs_int sxs,
		tjs_int sys,
		tjs_uint8 *dest,
		tjs_int l,
		tjs_int len,
		const tjs_uint8 *src,
		tjs_int srcpitch,
		tjs_int sxl,
		tjs_int syl,
		const tTVPRect & srccliprect,
		const tTVPRect & srcrect)
{
	// bilinear interpolation copy
	tjs_int len_remain = len;
	tjs_int spx, spy;
	tjs_int sx, sy;
	tjs_uint32 *dp;

	// skip "out of source rectangle" points
	// from last point
	sxl += 32768; // do +0.5 to rounding
	syl += 32768; // do +0.5 to rounding

	dp = (tjs_uint32*)dest + l + len-1;
	spx = (len-1)*sxs + sxl;
	spy = (len-1)*sys + syl;

	while(len_remain > 0)
	{
		tjs_int sx, sy;
		sx = spx >> 16;
		sy = spy >> 16;
		if(sx >= srcrect.left && sx < srcrect.right &&
			sy >= srcrect.top && sy < srcrect.bottom)
			break;
		dp--;
		spx -= sxs;
		spy -= sys;
		len_remain --;
	}

	// from first point
	spx = sxl;
	spy = syl;
	dp = (tjs_uint32*)dest + l;

	while(len_remain > 0)
	{
		tjs_int sx, sy;
		sx = spx >> 16;
		sy = spy >> 16;
		if(sx >= srcrect.left && sx < srcrect.right &&
			sy >= srcrect.top && sy < srcrect.bottom)
			break;
		dp++;
		l++; // step l forward
		spx += sxs;
		spy += sys;
		len_remain --;
	}

	sxl = spx;
	syl = spy;

	sxl -= 32768; // take back the original
	syl -= 32768; // take back the original

#define FIX_SX_SY	\
	if(sx < srccliprect.left) \
		sx = srccliprect.left, fixed_count ++; \
	if(sx >= srccliprect.right) \
		sx = srccliprect.right - 1, fixed_count++; \
	if(sy < srccliprect.top) \
		sy = srccliprect.top, fixed_count++; \
	if(sy >= srccliprect.bottom) \
		sy = srccliprect.bottom - 1, fixed_count++;


	// from last point
	spx = (len_remain-1)*sxs + sxl/* - 32768*/;
	spy = (len_remain-1)*sys + syl/* - 32768*/;
	dp = (tjs_uint32*)dest + l + len_remain-1;

	while(len_remain > 0)
	{
		tjs_int fixed_count = 0;
		tjs_uint32 c00, c01, c10, c11;
		tjs_int blend_x, blend_y;

		sx = (spx >> 16);
		sy = (spy >> 16);
		FIX_SX_SY
		c00 = *((const tjs_uint32*)(src + sy * srcpitch) + sx);

		sx = (spx >> 16) + 1;
		sy = (spy >> 16);
		FIX_SX_SY
		c01 = *((const tjs_uint32*)(src + sy * srcpitch) + sx);

		sx = (spx >> 16);
		sy = (spy >> 16) + 1;
		FIX_SX_SY
		c10 = *((const tjs_uint32*)(src + sy * srcpitch) + sx);

		sx = (spx >> 16) + 1;
		sy = (spy >> 16) + 1;
		FIX_SX_SY
		c11 = *((const tjs_uint32*)(src + sy * srcpitch) + sx);

		if(!fixed_count) break;

		blend_x = (spx & 0xffff) >> 8;
		blend_x += blend_x>>7; // adjust blend ratio
		blend_y = (spy & 0xffff) >> 8;
		blend_y += blend_y>>7;

		affine.DoOnePixel(dp, TVPBlendARGB(
			TVPBlendARGB(c00, c01, blend_x),
			TVPBlendARGB(c10, c11, blend_x),
				blend_y));

		dp--;
		spx -= sxs;
		spy -= sys;
		len_remain --;
	}

	// from first point
	spx = sxl/* - 32768*/;
	spy = syl/* - 32768*/;
	dp = (tjs_uint32*)dest + l;

	while(len_remain > 0)
	{
		tjs_int fixed_count = 0;
		tjs_uint32 c00, c01, c10, c11;
		tjs_int blend_x, blend_y;

		sx = (spx >> 16);
		sy = (spy >> 16);
		FIX_SX_SY
		c00 = *((const tjs_uint32*)(src + sy * srcpitch) + sx);

		sx = (spx >> 16) + 1;
		sy = (spy >> 16);
		FIX_SX_SY
		c01 = *((const tjs_uint32*)(src + sy * srcpitch) + sx);

		sx = (spx >> 16);
		sy = (spy >> 16) + 1;
		FIX_SX_SY
		c10 = *((const tjs_uint32*)(src + sy * srcpitch) + sx);

		sx = (spx >> 16) + 1;
		sy = (spy >> 16) + 1;
		FIX_SX_SY
		c11 = *((const tjs_uint32*)(src + sy * srcpitch) + sx);

		if(!fixed_count) break;

		blend_x = (spx & 0xffff) >> 8;
		blend_x += blend_x>>7; // adjust blend ratio
		blend_y = (spy & 0xffff) >> 8;
		blend_y += blend_y>>7;

		affine.DoOnePixel(dp, TVPBlendARGB(
			TVPBlendARGB(c00, c01, blend_x),
			TVPBlendARGB(c10, c11, blend_x),
				blend_y));

		dp++;
		spx += sxs;
		spy += sys;
		len_remain --;
	}

	if(len_remain > 0)
	{
		// do center part (this may takes most time)
		if(sys == 0)
		{
			// do stretch
			const tjs_uint8 * l1 = src + (spy >> 16) * srcpitch;
			const tjs_uint8 * l2 = l1 + srcpitch;
			stretch(
				dp,
				len_remain,
				(const tjs_uint32*)l1,
				(const tjs_uint32*)l2,
				(spy & 0xffff) >> 8,
				spx,
				sxs);
		}
		else
		{
			// do affine
			affine(dp, len_remain,
				(tjs_uint32*)src, spx, spy, sxs, sys, srcpitch);
		}
	}
}
//---------------------------------------------------------------------------
static inline tjs_int floor_16(tjs_int x)
{
	// take floor of 16.16 fixed-point format
	return (x >> 16) - (x < 0);
}
static inline tjs_int div_16(tjs_int x, tjs_int y)
{
	// return x * 65536 / y
	return (tjs_int)((tjs_int64)x * 65536 / y);
}
static inline tjs_int mul_16(tjs_int x, tjs_int y)
{
	// return x * y / 65536
	return (tjs_int)((tjs_int64)x * y / 65536);
}
//---------------------------------------------------------------------------
int InternalAffineBlt(tTVPBaseBitmap *destbmp, tTVPRect destrect, const tTVPBaseBitmap *ref,
		tTVPRect refrect, const tTVPPointD * points_in,
			tTVPBBBltMethod method, tjs_int opa,
			tTVPRect * updaterect,
			bool hda, tTVPBBStretchType mode, bool clear, tjs_uint32 clearcolor)
{
	// unlike other drawing methods, 'destrect' is the clip rectangle of the
	// destination bitmap.
	// affine transformation coordinates are to be applied on zero-offset
	// source rectangle:
	// (0, 0) - (refreft.right - refrect.left, refrect.bottom - refrect.top)

	// points are given as destination points, corresponding to three source
	// points; upper-left, upper-right, bottom-left.

	// if 'clear' is true, area which is out of the affine destination and
	// within the destination bounding box, is to be filled with value 'clearcolor'.

	// returns 0 if the updating rect is not updated, 1 if error
	// otherwise returns 2

	// extract stretch type
	tTVPBBStretchType type = (tTVPBBStretchType)(mode & stTypeMask);

	// check source rectangle
	if(refrect.left >= refrect.right ||
		refrect.top >= refrect.bottom) return 1;
	if(refrect.left < 0 || refrect.top < 0 ||
		refrect.right > (tjs_int)ref->GetWidth() ||
		refrect.bottom > (tjs_int)ref->GetHeight())
		TVPThrowExceptionMessage(TJS_W("Out of Rectangle"));

	// multiply source rectangle points by 65536 (16.16 fixed-point)
	// note that each pixel has actually 1.0 x 1.0 size
	// eg. a pixel at 0,0 may have (-0.5, -0.5) - (0.5, 0.5) area
	tTVPRect srcrect = refrect; // unmultiplied source rectangle is saved as srcrect
	refrect.left   = refrect.left   * 65536 - 32768;
	refrect.top    = refrect.top    * 65536 - 32768;
	refrect.right  = refrect.right  * 65536 - 32768;
	refrect.bottom = refrect.bottom * 65536 - 32768;

	// create point list in fixed point real format
	tTVPPoint points[3];
	for(tjs_int i = 0; i < 3; i++)
		points[i].x = static_cast<tjs_int>(points_in[i].x * 65536), points[i].y = static_cast<tjs_int>(points_in[i].y * 65536);

	// check destination rectangle
	if(destrect.left < 0) destrect.left = 0;
	if(destrect.top < 0) destrect.top = 0;
	if(destrect.right > (tjs_int)destbmp->GetWidth()) destrect.right = destbmp->GetWidth();
	if(destrect.bottom > (tjs_int)destbmp->GetHeight()) destrect.bottom = destbmp->GetHeight();

	if(destrect.left >= destrect.right ||
		destrect.top >= destrect.bottom) return 1; // not drawable

	// vertex points
	tjs_int points_x[4];
	tjs_int points_y[4];

	// check each vertex and find most-top/most-bottom/most-left/most-right points
	tjs_int scanlinestart, scanlineend; // most-top/most-bottom
	tjs_int leftlimit, rightlimit; // most-left/most-right

	// - upper-left (p0)
	points_x[0] = points[0].x;
	points_y[0] = points[0].y;
	leftlimit = points_x[0];
	rightlimit = points_x[0];
	scanlinestart = points_y[0];
	scanlineend = points_y[0];

	// - upper-right (p1)
	points_x[1] = points[1].x;
	points_y[1] = points[1].y;
	if(leftlimit > points_x[1]) leftlimit = points_x[1];
	if(rightlimit < points_x[1]) rightlimit = points_x[1];
	if(scanlinestart > points_y[1]) scanlinestart = points_y[1];
	if(scanlineend < points_y[1]) scanlineend = points_y[1];

	// - bottom-right (p2)
	points_x[2] = points[1].x - points[0].x + points[2].x;
	points_y[2] = points[1].y - points[0].y + points[2].y;
	if(leftlimit > points_x[2]) leftlimit = points_x[2];
	if(rightlimit < points_x[2]) rightlimit = points_x[2];
	if(scanlinestart > points_y[2]) scanlinestart = points_y[2];
	if(scanlineend < points_y[2]) scanlineend = points_y[2];

	// - bottom-left (p3)
	points_x[3] = points[2].x;
	points_y[3] = points[2].y;
	if(leftlimit > points_x[3]) leftlimit = points_x[3];
	if(rightlimit < points_x[3]) rightlimit = points_x[3];
	if(scanlinestart > points_y[3]) scanlinestart = points_y[3];
	if(scanlineend < points_y[3]) scanlineend = points_y[3];

	// rough check destrect intersections
	if(floor_16(leftlimit) >= destrect.right) return 0;
	if(floor_16(rightlimit) < destrect.left) return 0;
	if(floor_16(scanlinestart) >= destrect.bottom) return 0;
	if(floor_16(scanlineend) < destrect.top) return 0;

	// compute sxstep and systep (step count for source image)
	tjs_int sxstep, systep;

	{
		double dv01x = (points[1].x - points[0].x) * (1.0 / 65536.0);
		double dv01y = (points[1].y - points[0].y) * (1.0 / 65536.0);
		double dv02x = (points[2].x - points[0].x) * (1.0 / 65536.0);
		double dv02y = (points[2].y - points[0].y) * (1.0 / 65536.0);

		double x01, x02, s01, s02;

		if     (points[1].y == points[0].y)
		{
			sxstep = (tjs_int)((refrect.right - refrect.left) / dv01x);
			systep = 0;
		}
		else if(points[2].y == points[0].y)
		{
			sxstep = 0;
			systep = (tjs_int)((refrect.bottom - refrect.top) / dv02x);
		}
		else
		{
			x01 = dv01x / dv01y;
			s01 = (refrect.right - refrect.left) / dv01y;

			x02 = dv02x / dv02y;
			s02 = (refrect.top - refrect.bottom) / dv02y;

			double len = x01 - x02;

			sxstep = (tjs_int)(s01 / len);
			systep = (tjs_int)(s02 / len);
		}
	}

	// prepare to transform...
	tjs_int yc    = (scanlinestart + 32768) / 65536;
	tjs_int yclim = (scanlineend   + 32768) / 65536;

	if(destrect.top > yc) yc = destrect.top;
	if(destrect.bottom <= yclim) yclim = destrect.bottom - 1;
	if(yc >= destrect.bottom || yclim < 0)
		return 0; // not drawable

	tjs_uint8 * dest = (tjs_uint8*)destbmp->GetScanLineForWrite(yc);
	tjs_int destpitch = destbmp->GetPitchBytes();
	const tjs_uint8 * src = (const tjs_uint8 *)ref->GetScanLine(0);
	tjs_int srcpitch = ref->GetPitchBytes();

//	yc    = yc    * 65536;
//	yclim = yclim * 65536;

	// make srccliprect
	tTVPRect srccliprect;
	if(mode & stRefNoClip)
		srccliprect.left = 0,
		srccliprect.top = 0,
		srccliprect.right = (tjs_int)ref->GetWidth(),
		srccliprect.bottom = (tjs_int)ref->GetHeight(); // no clip; all the bitmap will be the source
	else
		srccliprect = srcrect; // clip; the source is limited to the source rectangle


	// process per a line
	tjs_int mostupper  = -1;
	tjs_int mostbottom = -1;
	bool firstline = true;

        tjs_int ych = yclim - yc + 1;
        tjs_int w = destrect.right - destrect.left;
        tjs_int h = destrect.bottom - destrect.top;

        tjs_int taskNum = GetAdaptiveThreadNum(w * h, sBmFactor[method] * 13 / 59);
        TVPBeginThreadTask(taskNum);
        PartialAffineBltParam params[TVPMaxThreadNum];
        for (tjs_int i = 0; i < taskNum; i++) {
          tjs_int y0, y1;
          y0 = ych * i / taskNum;
          y1=  ych * (i + 1) / taskNum - 1;
          PartialAffineBltParam *param = params + i;
          param->self = destbmp;
          param->dest = dest + destpitch * y0;
          param->destpitch = destpitch;
          param->yc = (yc + y0) * 65536;
          param->yclim = (yc + y1) * 65536;
          param->scanlinestart = scanlinestart;
          param->scanlineend = scanlineend;
          param->points_x = points_x;
          param->points_y = points_y;
          param->refrect = &refrect;
          param->sxstep = sxstep;
          param->systep = systep;
          param->destrect = &destrect;
          param->method = method;
          param->opa = opa;
          param->hda = hda;
          param->type = type;
          param->clear = clear;
          param->clearcolor = clearcolor;
          param->leftlimit = leftlimit;
          param->rightlimit = rightlimit;
          param->mostupper = mostupper;
          param->mostbottom = mostbottom;
          param->firstline = firstline;
          param->src = src;
          param->srcpitch = srcpitch;
          param->srccliprect = &srccliprect;
          param->srcrect = &srcrect;
          TVPExecThreadTask(&PartialAffineBltEntry, TVP_THREAD_PARAM(param));
        }
        TVPEndThreadTask();

        // update area param
        for (tjs_int i = 0; i < taskNum; i++) {
          PartialAffineBltParam *param = params + i;
          if (param->firstline)
            continue;
          if (firstline) {
            firstline = false;
            leftlimit = param->leftlimit;
            rightlimit = param->rightlimit;
            mostupper = param->mostupper;
            mostbottom  = param->mostbottom;
          } else {
            if (param->leftlimit < leftlimit) leftlimit = param->leftlimit;
            if (param->rightlimit > rightlimit) rightlimit = param->rightlimit;
            if (param->mostupper < mostupper) mostupper = param->mostupper;
            if (param->mostbottom > mostbottom) mostbottom = param->mostbottom;
          }
        }

	// clear upper and lower area of the affine transformation
	if(clear)
	{
		tjs_int h;
		tjs_uint8 * dest = (tjs_uint8*)destbmp->GetScanLineForWrite(0);
		tjs_uint8 * p;
		if(mostupper == -1 && mostbottom == -1)
		{
			// special case: nothing was drawn;
			// clear entire area of the destrect
			mostupper  = destrect.bottom;
			mostbottom = destrect.bottom - 1;
		}

		h = mostupper - destrect.top;
		if(h > 0)
		{
			p = dest + destrect.top * destpitch;
			do
				(hda?TVPFillColor:TVPFillARGB)((tjs_uint32*)p + destrect.left,
					destrect.right - destrect.left, clearcolor),
				p += destpitch;
			while(--h);
		}

		h = destrect.bottom - (mostbottom + 1);
		if(h > 0)
		{
			p = dest + (mostbottom + 1) * destpitch;
			do
				(hda?TVPFillColor:TVPFillARGB)((tjs_uint32*)p + destrect.left,
					destrect.right - destrect.left, clearcolor),
				p += destpitch;
			while(--h);
		}

	}

	// fill members of updaterect
	if(updaterect)
	{
		if(clear)
		{
			// clear is specified
			*updaterect = destrect;
				// update rectangle is the same as the destination rectangle
		}
		else if(!firstline)
		{
			// update area is being
			updaterect->left = leftlimit;
			updaterect->right = rightlimit + 1;
			updaterect->top = mostupper;
			updaterect->bottom = mostbottom + 1;
		}
	}

	return (clear || !firstline)?2:0;
}

static void TJS_USERENTRY PartialAffineBltEntry(void *v)
{
  PartialAffineBltParam *param = (PartialAffineBltParam *)v;
  PartialAffineBlt(param);
}

void PartialAffineBlt(PartialAffineBltParam *param)
{
  tjs_uint8 *dest = param->dest;
  const tjs_int destpitch = param->destpitch;
  tjs_int yc = param->yc;
  tjs_int yclim = param->yclim;
  const tjs_int scanlinestart = param->scanlinestart;
  const tjs_int scanlineend = param->scanlineend;
  const tjs_int *points_x = param->points_x;
  const tjs_int *points_y = param->points_y;
  const tTVPRect &refrect = *(param->refrect);
  const tjs_int sxstep = param->sxstep;
  const tjs_int systep = param->systep;
  const tTVPRect &destrect = *(param->destrect);
  const tTVPBBBltMethod method = param->method;
  const tjs_int opa = param->opa;
  const bool hda = param->hda;
  const tTVPBBStretchType type = param->type;
  const bool clear = param->clear;
  const tjs_uint32 clearcolor = param->clearcolor;
  const tjs_uint8 *const src  = param->src;
  const tjs_int srcpitch = param->srcpitch;
  const tTVPRect &srccliprect = *(param->srccliprect);
  const tTVPRect &srcrect = *(param->srcrect);

  tjs_int &leftlimit = param->leftlimit;
  tjs_int &rightlimit = param->rightlimit;
  tjs_int &mostupper = param->mostupper;
  tjs_int &mostbottom = param->mostbottom;
  bool &firstline = param->firstline;

	for(; yc <= yclim; yc+=65536, dest += destpitch)
	{
		// transfer a line

		// skip out-of-range lines
		tjs_int yl = yc;
		if(yl < scanlinestart)
			continue;
		if(yl >= scanlineend)
			continue;

		// actual write line
		tjs_int y = (yc+32768) / 65536;

		// find line intersection
		// line codes are:
		// p0 .. p1  : 0
		// p1 .. p2  : 1
		// p2 .. p3  : 2
		// p3 .. p0  : 3
		tjs_int line_code0, line_code1;
		tjs_int where0, where1;
		tjs_int where, code;

		for(code = 0; code < 4; code ++)
		{
			tjs_int ip0 = code;
			tjs_int ip1 = (code + 1) & 3;
			if     (points_y[ip0] == yl && points_y[ip1] == yl)
			{
				where = points_x[ip1] > points_x[ip0] ? 0 : 65536;
				code += 8;
				break;
			}
		}
		if(code < 8)
		{
			for(code = 0; code < 4; code ++)
			{
				tjs_int ip0 = code;
				tjs_int ip1 = (code + 1) & 3;
				if(points_y[ip0] <= yl && points_y[ip1] > yl)
				{
					where = div_16(yl - points_y[ip0], points_y[ip1] - points_y[ip0]);
					break;
				}
				else if(points_y[ip0] >  yl && points_y[ip1] <= yl)
				{
					where = div_16(points_y[ip0] - yl, points_y[ip0] - points_y[ip1]);
					break;
				}
			}
		}
		line_code0 = code;
		where0 = where;

		if(line_code0 < 4)
		{
			for(code = 0; code < 4; code ++)
			{
				if(code == line_code0) continue;
				tjs_int ip0 = code;
				tjs_int ip1 = (code + 1) & 3;
				if     (points_y[ip0] == yl && points_y[ip1] == yl)
				{
					where = points_x[ip1] > points_x[ip0] ? 0 : 65536;
					break;
				}
				else if(points_y[ip0] <= yl && points_y[ip1] >  yl)
				{
					where = div_16(yl - points_y[ip0], points_y[ip1] - points_y[ip0]);
					break;
				}
				else if(points_y[ip0] >  yl && points_y[ip1] <= yl)
				{
					where = div_16(points_y[ip0]- yl , points_y[ip0] - points_y[ip1]);
					break;
				}
			}
			line_code1 = code;
			where1 = where;
		}
		else
		{
			line_code0 &= 3;
			line_code1 = line_code0;
			where1 = 65536 - where0;
		}

		// compute intersection point
		tjs_int ll, rr, sxl, syl, sxr, syr;
		switch(line_code0)
		{
		case 0:
			ll  = mul_16(points_x[1] - points_x[0]   , where0) + points_x[0];
			sxl = mul_16(refrect.right - refrect.left, where0) + refrect.left;
			syl = refrect.top;
			break;
		case 1:
			ll  = mul_16(points_x[2] - points_x[1]   , where0) + points_x[1];
			sxl = refrect.right;
			syl = mul_16(refrect.bottom - refrect.top, where0) + refrect.top;
			break;
		case 2:
			ll  = mul_16(points_x[3] - points_x[2]   , where0) + points_x[2];
			sxl = mul_16(refrect.left - refrect.right, where0) + refrect.right;
			syl = refrect.bottom;
			break;
		case 3:
			ll  = mul_16(points_x[0] - points_x[3]   , where0) + points_x[3];
			sxl = refrect.left;
			syl = mul_16(refrect.top - refrect.bottom, where0) + refrect.bottom;
			break;
		}

		switch(line_code1)
		{
		case 0:
			rr  = mul_16(points_x[1] - points_x[0]   , where1) + points_x[0];
			sxr = mul_16(refrect.right - refrect.left, where1) + refrect.left;
			syr = refrect.top;
			break;
		case 1:
			rr  = mul_16(points_x[2] - points_x[1]   , where1) + points_x[1];
			sxr = refrect.right;
			syr = mul_16(refrect.bottom - refrect.top, where1) + refrect.top;
			break;
		case 2:
			rr  = mul_16(points_x[3] - points_x[2]   , where1) + points_x[2];
			sxr = mul_16(refrect.left - refrect.right, where1) + refrect.right;
			syr = refrect.bottom;
			break;
		case 3:
			rr  = mul_16(points_x[0] - points_x[3]   , where1) + points_x[3];
			sxr = refrect.left;
			syr = mul_16(refrect.top - refrect.bottom, where1) + refrect.bottom;
			break;
		}


		// l, r, sxs, sys, and len
		tjs_int sxs, sys, len;
		sxs = sxstep;
		sys = systep;
		if(ll > rr)
		{
			std::swap(ll, rr);
			std::swap(sxl, sxr);
			std::swap(syl, syr);
		}

		// round l and r to integer
		tjs_int l, r;

		// 0x8000000 were choosed to avoid special divition behavior around zero
		l = ((tjs_uint32)(ll + 0x80000000ul-1)/65536)-(0x80000000ul/65536)+1;
		sxl += mul_16(65535 - ((tjs_uint32)(ll+0x80000000ul-1) % 65536), sxs); // adjust source start point x
		syl += mul_16(65535 - ((tjs_uint32)(ll+0x80000000ul-1) % 65536), sys); // adjust source start point y

		r = ((tjs_uint32)(rr + 0x80000000ul-1)/65536)-(0x80000000ul/65536)+1; // note that at this point r is *NOT* inclusive

		// - clip widh destrect.left and destrect.right
		if(l < destrect.left)
		{
			tjs_int d = destrect.left - l;
			sxl += d * sxs;
			syl += d * sys;
			l = destrect.left;
		}
		if(r > destrect.right) r = destrect.right;

		// - compute horizontal length
		len = r - l;

		// - transfer
		if(len > 0)
		{
			// fill left and right of the line if 'clear' is specified
			if(clear)
			{
				tjs_int clen;

				int ll = l + 1;
				if(ll > destrect.right) ll = destrect.right;
				clen = ll - destrect.left;
				if(clen > 0)
					(hda?TVPFillColor:TVPFillARGB)((tjs_uint32*)dest + destrect.left, clen, clearcolor);
				int rr = r - 1;
				if(rr < destrect.left) rr = destrect.left;
				clen = destrect.right - rr;
				if(clen > 0)
					(hda?TVPFillColor:TVPFillARGB)((tjs_uint32*)dest + rr, clen, clearcolor);
			}


			// update updaterect
			if(firstline)
			{
				leftlimit = l;
				rightlimit = r;
				firstline = false;
				mostupper = mostbottom = y;
			}
			else
			{
				if(l < leftlimit) leftlimit = l;
				if(r > rightlimit) rightlimit = r;
				mostbottom = y;
			}

			// transfer using each blend function
			switch(method)
			{
			case bmCopy:
				if(opa == 255)
				{
					if(TVP_BILINEAR_FORCE_COND || !hda)
					{
						if(TVP_BILINEAR_FORCE_COND || type >= stFastLinear)
						{
							// bilinear interpolation
							TVPDoBilinearAffineLoop(
								tTVPInterpStretchCopyFunctionObject(),
								tTVPInterpLinTransCopyFunctionObject(),
								TVP_DoBilinearAffineLoop_ARGS);
						}
						else
						{
							// point on point (nearest) copy
							TVPDoAffineLoop(
								tTVPStretchFunctionObject(TVPStretchCopy),
								tTVPAffineFunctionObject(TVPLinTransCopy),
								TVP_DoAffineLoop_ARGS);
						}
					}
					else
					{
						TVPDoAffineLoop(
							tTVPStretchFunctionObject(TVPStretchColorCopy),
							tTVPAffineFunctionObject(TVPLinTransColorCopy),
							TVP_DoAffineLoop_ARGS);
					}
				}
				else
				{
					if(TVP_BILINEAR_FORCE_COND || !hda)
					{
						if(TVP_BILINEAR_FORCE_COND || type >= stFastLinear)
						{
							// bilinear interpolation
							TVPDoBilinearAffineLoop(
								tTVPInterpStretchConstAlphaBlendFunctionObject(opa),
								tTVPInterpLinTransConstAlphaBlendFunctionObject(opa),
								TVP_DoBilinearAffineLoop_ARGS);
						}
						else
						{
							TVPDoAffineLoop(
								tTVPStretchWithOpacityFunctionObject(TVPStretchConstAlphaBlend, opa),
								tTVPAffineWithOpacityFunctionObject(TVPLinTransConstAlphaBlend, opa),
								TVP_DoAffineLoop_ARGS);
						}
					}
					else
					{
						TVPDoAffineLoop(
							tTVPStretchWithOpacityFunctionObject(TVPStretchConstAlphaBlend_HDA, opa),
							tTVPAffineWithOpacityFunctionObject(TVPLinTransConstAlphaBlend_HDA, opa),
							TVP_DoAffineLoop_ARGS);
					}
				}
				break;

			case bmCopyOnAlpha:
				// constant ratio alpha blending, with consideration of destination alpha
				if(opa == 255)
					TVPDoAffineLoop(
						tTVPStretchFunctionObject(TVPStretchCopyOpaqueImage),
						tTVPAffineFunctionObject(TVPLinTransCopyOpaqueImage),
						TVP_DoAffineLoop_ARGS);
				else
					TVPDoAffineLoop(
						tTVPStretchWithOpacityFunctionObject(TVPStretchConstAlphaBlend_d, opa),
						tTVPAffineWithOpacityFunctionObject(TVPLinTransConstAlphaBlend_d, opa),
						TVP_DoAffineLoop_ARGS);
				break;

			case bmAlpha:
				// alpha blending, ignoring destination alpha
				if(opa == 255)
				{
					if(!hda)
						TVPDoAffineLoop(
							tTVPStretchFunctionObject(TVPStretchAlphaBlend),
							tTVPAffineFunctionObject(TVPLinTransAlphaBlend),
							TVP_DoAffineLoop_ARGS);
					else
						TVPDoAffineLoop(
							tTVPStretchFunctionObject(TVPStretchAlphaBlend_HDA),
							tTVPAffineFunctionObject(TVPLinTransAlphaBlend_HDA),
							TVP_DoAffineLoop_ARGS);
				}
				else
				{
					if(!hda)
						TVPDoAffineLoop(
							tTVPStretchWithOpacityFunctionObject(TVPStretchAlphaBlend_o, opa),
							tTVPAffineWithOpacityFunctionObject(TVPLinTransAlphaBlend_o, opa),
							TVP_DoAffineLoop_ARGS);
					else
						TVPDoAffineLoop(
							tTVPStretchWithOpacityFunctionObject(TVPStretchAlphaBlend_HDA_o, opa),
							tTVPAffineWithOpacityFunctionObject(TVPLinTransAlphaBlend_HDA_o, opa),
							TVP_DoAffineLoop_ARGS);
				}
				break;

			case bmAlphaOnAlpha:
				// alpha blending, with consideration of destination alpha
				if(opa == 255)
					TVPDoAffineLoop(
						tTVPStretchFunctionObject(TVPStretchAlphaBlend_d),
						tTVPAffineFunctionObject(TVPLinTransAlphaBlend_d),
						TVP_DoAffineLoop_ARGS);
				else
					TVPDoAffineLoop(
						tTVPStretchWithOpacityFunctionObject(TVPStretchAlphaBlend_do, opa),
						tTVPAffineWithOpacityFunctionObject(TVPLinTransAlphaBlend_do, opa),
						TVP_DoAffineLoop_ARGS);
				break;

			case bmAddAlpha:
				// additive alpha blending, ignoring destination alpha
				if(opa == 255)
				{
					if(TVP_BILINEAR_FORCE_COND || !hda)
					{
						if(TVP_BILINEAR_FORCE_COND || type >= stFastLinear)
						{
							// bilinear interpolation
							TVPDoBilinearAffineLoop(
								tTVPInterpStretchAdditiveAlphaBlendFunctionObject(),
								tTVPInterpLinTransAdditiveAlphaBlendFunctionObject(),
								TVP_DoBilinearAffineLoop_ARGS);
						}
						else
						{
							TVPDoAffineLoop(
								tTVPStretchFunctionObject(TVPStretchAdditiveAlphaBlend),
								tTVPAffineFunctionObject(TVPLinTransAdditiveAlphaBlend),
								TVP_DoAffineLoop_ARGS);
						}
					}
					else
					{
						TVPDoAffineLoop(
							tTVPStretchFunctionObject(TVPStretchAdditiveAlphaBlend_HDA),
							tTVPAffineFunctionObject(TVPLinTransAdditiveAlphaBlend_HDA),
							TVP_DoAffineLoop_ARGS);
					}
				}
				else
				{
					if(TVP_BILINEAR_FORCE_COND || !hda)
					{
						if(TVP_BILINEAR_FORCE_COND || type >= stFastLinear)
						{
							// bilinear interpolation
							TVPDoBilinearAffineLoop(
								tTVPInterpStretchAdditiveAlphaBlend_oFunctionObject(opa),
								tTVPInterpLinTransAdditiveAlphaBlend_oFunctionObject(opa),
								TVP_DoBilinearAffineLoop_ARGS);
						}
						else
						{
							TVPDoAffineLoop(
								tTVPStretchWithOpacityFunctionObject(TVPStretchAlphaBlend_o, opa),
								tTVPAffineWithOpacityFunctionObject(TVPLinTransAdditiveAlphaBlend_o, opa),
								TVP_DoAffineLoop_ARGS);
						}
					}
					else
					{
						TVPDoAffineLoop(
							tTVPStretchWithOpacityFunctionObject(TVPStretchAdditiveAlphaBlend_HDA_o, opa),
							tTVPAffineWithOpacityFunctionObject(TVPLinTransAdditiveAlphaBlend_HDA_o, opa),
							TVP_DoAffineLoop_ARGS);
					}
				}
				break;

			case bmAddAlphaOnAddAlpha:
				// additive alpha blending, with consideration of destination additive alpha
				if(opa == 255)
					TVPDoAffineLoop(
						tTVPStretchFunctionObject(TVPStretchAlphaBlend_a),
						tTVPAffineFunctionObject(TVPLinTransAdditiveAlphaBlend_a),
						TVP_DoAffineLoop_ARGS);
				else
					TVPDoAffineLoop(
						tTVPStretchWithOpacityFunctionObject(TVPStretchAlphaBlend_ao, opa),
						tTVPAffineWithOpacityFunctionObject(TVPLinTransAdditiveAlphaBlend_ao, opa),
						TVP_DoAffineLoop_ARGS);
				break;

			case bmAddAlphaOnAlpha:
				// additive alpha on simple alpha
				; // yet not implemented
				break;

			case bmAlphaOnAddAlpha:
				// simple alpha on additive alpha
				if(opa == 255)
					TVPDoAffineLoop(
						tTVPStretchFunctionObject(TVPStretchAlphaBlend_a),
						tTVPAffineFunctionObject(TVPLinTransAlphaBlend_a),
						TVP_DoAffineLoop_ARGS);
				else
					TVPDoAffineLoop(
						tTVPStretchWithOpacityFunctionObject(TVPStretchAlphaBlend_ao, opa),
						tTVPAffineWithOpacityFunctionObject(TVPLinTransAlphaBlend_ao, opa),
						TVP_DoAffineLoop_ARGS);
				break;

			case bmCopyOnAddAlpha:
				// constant ratio alpha blending (assuming source is opaque)
				// with consideration of destination additive alpha
				if(opa == 255)
					TVPDoAffineLoop(
						tTVPStretchFunctionObject(TVPStretchCopyOpaqueImage),
						tTVPAffineFunctionObject(TVPLinTransCopyOpaqueImage),
						TVP_DoAffineLoop_ARGS);
				else
					TVPDoAffineLoop(
						tTVPStretchWithOpacityFunctionObject(TVPStretchConstAlphaBlend_a, opa),
						tTVPAffineWithOpacityFunctionObject(TVPLinTransConstAlphaBlend_a, opa),
						TVP_DoAffineLoop_ARGS);
				break;

			default:
				; // yet not implemented
			}
		}
	}
}

//---------------------------------------------------------------------------
bool AffineBlt(tTVPBaseBitmap *destbmp, tTVPRect destrect, const tTVPBaseBitmap *ref,
		tTVPRect refrect, const tTVPPointD * points_in,
			tTVPBBBltMethod method, tjs_int opa,
			tTVPRect * updaterect,
			bool hda, tTVPBBStretchType mode, bool clear, tjs_uint32 clearcolor)
{
	if(0 == InternalAffineBlt(destbmp, destrect, ref, refrect, points_in, method, opa, updaterect, hda,
		mode, clear, clearcolor))
	{
#if 0
		if(clear)
		{
			if(hda)
				FillColor(destrect, clearcolor, 255);
			else
				Fill(destrect, clearcolor);
			return true;
		}
#endif
		return false;
	}
	return true;
}
//---------------------------------------------------------------------------
bool AffineBlt(tTVPBaseBitmap *destbmp, tTVPRect destrect, const tTVPBaseBitmap *ref,
		tTVPRect refrect, const t2DAffineMatrix & matrix,
			tTVPBBBltMethod method, tjs_int opa,
			tTVPRect * updaterect,
			bool hda, tTVPBBStretchType type, bool clear, tjs_uint32 clearcolor)
{
	// do transformation using 2D affine matrix.
	//  x' =  ax + cy + tx
	//  y' =  bx + dy + ty
	tTVPPointD points[3];
	int rp = refrect.get_width();
	int bp = refrect.get_height();

	// note that a pixel has actually 1 x 1 size, so
	// a pixel at (0,0) covers (-0.5, -0.5) - (0.5, 0.5).

#define CALC_X(x, y) (matrix.a * (x) + matrix.c * (y) + matrix.tx)
#define CALC_Y(x, y) (matrix.b * (x) + matrix.d * (y) + matrix.ty)

	// - upper-left
	points[0].x = CALC_X(-0.5, -0.5);
	points[0].y = CALC_Y(-0.5, -0.5);

	// - upper-right
	points[1].x = CALC_X(rp - 0.5, -0.5);
	points[1].y = CALC_Y(rp - 0.5, -0.5);

/*	// - bottom-right
	points[2].x = CALC_X(rp - 0.5, bp - 0.5);
	points[2].y = CALC_Y(rp - 0.5, bp - 0.5);
*/

	// - bottom-left
	points[2].x = CALC_X(-0.5, bp - 0.5);
	points[2].y = CALC_Y(-0.5, bp - 0.5);

	return AffineBlt(destbmp, destrect, ref, refrect, points, method, opa, updaterect, hda, type, clear, clearcolor);
}
