
#ifndef __LAYERBITMAPAFFINEBLIT_H__
#define __LAYERBITMAPAFFINEBLIT_H__

//---------------------------------------------------------------------------
// t2DAffineMatrix
//---------------------------------------------------------------------------
struct t2DAffineMatrix
{
	/* structure for subscribing following 2D affine transformation matrix */
	/*
	|                          | a  b  0 |
	| [x', y', 1] =  [x, y, 1] | c  d  0 |
	|                          | tx ty 1 |
	|  thus,
	|
	|  x' =  ax + cy + tx
	|  y' =  bx + dy + ty
	*/

	double a;
	double b;
	double c;
	double d;
	double tx;
	double ty;
};
//---------------------------------------------------------------------------

bool AffineBlt(tTVPBaseBitmap *destbmp, tTVPRect destrect, const tTVPBaseBitmap *ref,
	tTVPRect refrect, const tTVPPointD * points,
		tTVPBBBltMethod method, tjs_int opa,
		tTVPRect * updaterect = NULL,
		bool hda = true, tTVPBBStretchType mode = stNearest, bool clear = false,
			tjs_uint32 clearcolor = 0);

bool AffineBlt(tTVPBaseBitmap *destbmp, tTVPRect destrect, const tTVPBaseBitmap *ref,
	tTVPRect refrect, const t2DAffineMatrix & matrix,
		tTVPBBBltMethod method, tjs_int opa,
		tTVPRect * updaterect = NULL,
		bool hda = true, tTVPBBStretchType mode = stNearest, bool clear = false,
			tjs_uint32 clearcolor = 0);

#endif
