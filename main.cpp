/////////////////////////////////////////////
//                                         //
//    Copyright (C) 2021-2021 Julian Uy    //
//  https://sites.google.com/site/awertyb  //
//                                         //
//   See details of license at "LICENSE"   //
//                                         //
/////////////////////////////////////////////

#include "ncbind/ncbind.hpp"
#include "tvpgl.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapUtility.h"
#include "LayerBitmapAffineBlit.h"
#include <string.h>
#include <stdio.h>

static void PreRegistCallback()
{
	TVPCreateTable();
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object)
		{
			layer_valclosure.DeleteMember(TJS_IGNOREPROP, TJS_W("affineCopy"), 0, NULL);
			layer_valclosure.DeleteMember(TJS_IGNOREPROP, TJS_W("operateAffine"), 0, NULL);
		}
	}
}

NCB_PRE_REGIST_CALLBACK(PreRegistCallback);

class LayerLayerExAffine
{
public:
	static bool fillRect_passthrough(
		tTJSVariantClosure bmpobject_clo,
		tTVPRect destrect,
		tjs_uint32 color,
		bool HoldAlpha
		)
	{
		{
			{
				tTJSVariant args[5];
				args[0] = destrect.left;
				args[1] = destrect.top;
				args[2] = destrect.get_width();
				args[3] = destrect.get_height();
				args[4] = (tjs_int64)(color & 0xffffffff);
				tTJSVariant *pargs[5] = {args +0, args +1, args +2, args +3, args +4};
				{
					iTJSDispatch2 *global = TVPGetScriptDispatch();
					if (global)
					{
						tTJSVariant layer_val;
						static ttstr Layer_name(TJS_W("Layer"));
						global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
						tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
						if (layer_valclosure.Object)
						{
							tTJSVariant fillRect_val;
							static ttstr fillRect_name(TJS_W("fillRect"));
							layer_valclosure.PropGet(0, fillRect_name.c_str(), fillRect_name.GetHint(), &fillRect_val, NULL);
							if (fillRect_val.Type() == tvtObject)
							{
								tTJSVariant rresult;
								tTJSVariantClosure fillRect_valclosure = fillRect_val.AsObjectClosureNoAddRef();
								if (fillRect_valclosure.Object)
								{
									tTVPDrawFace OldFace = dfOpaque;
									tTVPDrawFace NewFace = dfOpaque;
									bool OldHoldAlpha = false;
									bool NewHoldAlpha = HoldAlpha;
									GetLayerInformationFromLayerObject(bmpobject_clo, &OldFace, NULL, NULL, &OldHoldAlpha, NULL, NULL, NULL);
									SetLayerInformationOnLayerObject(bmpobject_clo, &NewFace, NULL, NULL, &NewHoldAlpha, NULL, NULL, NULL);
									fillRect_valclosure.FuncCall(0, NULL, 0, &rresult, 5, pargs, bmpobject_clo.Object);
									SetLayerInformationOnLayerObject(bmpobject_clo, &OldFace, NULL, NULL, &OldHoldAlpha, NULL, NULL, NULL);
									return true;
								}
							}
						}
					}
				}
			}
		}

		return false;
	}

	static tjs_error TJS_INTF_METHOD affineCopy(
		tTJSVariant	*result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis)
	{
		if (numparams < 12)
		{
			return TJS_E_BADPARAMCOUNT;
		}

		if(objthis == NULL) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTJSVariant bmpobject = tTJSVariant(objthis, objthis);
		tTJSVariantClosure bmpobject_clo = bmpobject.AsObjectClosureNoAddRef();

		tTJSVariantClosure srcbmpobject_clo = param[0]->AsObjectClosureNoAddRef();

		tTVPRect srcrect(*param[1], *param[2], *param[3], *param[4]);
		srcrect.right += srcrect.left;
		srcrect.bottom += srcrect.top;

		tTVPBBStretchType type = stNearest;

		if(numparams >= 13 && param[12]->Type() != tvtVoid)
			type = (tTVPBBStretchType)(tjs_int)*param[12];

		bool clear = false;

		if(numparams >= 14 && param[13]->Type() != tvtVoid)
			clear = 0!=(tjs_int)*param[13];

		bool is_affine_matrix = param[5]->operator bool();
		// affine matrix mode
		t2DAffineMatrix mat;
		mat.a = *param[6];
		mat.b = *param[7];
		mat.c = *param[8];
		mat.d = *param[9];
		mat.tx = *param[10];
		mat.ty = *param[11];
		// points mode
		tTVPPointD points[3];
		points[0].x = *param[6];
		points[0].y = *param[7];
		points[1].x = *param[8];
		points[1].y = *param[9];
		points[2].x = *param[10];
		points[2].y = *param[11];

		// Now get information (this will independ the bitmap)
		tjs_int bmpwidth = 0;
		tjs_int bmpheight = 0;
		tjs_int bmppitch = 0;
		tjs_uint8* bmpdata = NULL;
		GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
		if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);

		tjs_int srcbmpwidth = 0;
		tjs_int srcbmpheight = 0;
		tjs_int srcbmppitch = 0;
		tjs_uint8* srcbmpdata = NULL;
		GetBitmapInformationFromObject(srcbmpobject_clo, false, &srcbmpwidth, &srcbmpheight, &srcbmppitch, &srcbmpdata);
		if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap srcbmpinfo(srcbmpwidth, srcbmpheight, srcbmppitch, srcbmpdata);

		tTVPDrawFace Face = dfAuto; // (outward) current drawing layer face
		tTVPLayerType DisplayType = ltOpaque; // actual Type
		tTVPRect ClipRect;
		bool HoldAlpha = false;
		tjs_int ImageLeft = 0;
		tjs_int ImageTop = 0;
		tjs_uint32 NeutralColor = 0xffffffff;

		GetLayerInformationFromLayerObject(bmpobject_clo, &Face, &DisplayType, &ClipRect, &HoldAlpha, &ImageLeft, &ImageTop, &NeutralColor);

		tTVPDrawFace DrawFace = GetDrawFace(Face, DisplayType); // (actual) current drawing layer face

		tTVPRect ur;
		bool updated = false;
		if (is_affine_matrix)
		{
			switch(DrawFace)
			{
			case dfAlpha:
			case dfAddAlpha:
			  {
				if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
				if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));
				updated = AffineBlt(&bmpinfo, ClipRect, &srcbmpinfo, srcrect, mat,
					bmCopy, 255, &ur, false, type, clear, NeutralColor);
				break;
			  }

			case dfOpaque:
			  {
				if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
				if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));
				updated = AffineBlt(&bmpinfo, ClipRect, &srcbmpinfo, srcrect, mat,
					bmCopy, 255, &ur, HoldAlpha, type, clear, NeutralColor);
				break;
			  }

			default:
				TVPThrowExceptionMessage(TJS_W("Not drawble face type %1"), TJS_W("affineCopy"));
			}
		}
		else
		{
			switch(DrawFace)
			{
			case dfAlpha:
			case dfAddAlpha:
			  {
				if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
				if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));
				updated = AffineBlt(&bmpinfo, ClipRect, &srcbmpinfo, srcrect, points,
					bmCopy, 255, &ur, false, type, clear, NeutralColor);
				break;
			  }

			case dfOpaque:
			  {
				if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
				if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));
				updated = AffineBlt(&bmpinfo, ClipRect, &srcbmpinfo, srcrect, points,
					bmCopy, 255, &ur, HoldAlpha, type, clear, NeutralColor);
				break;
			  }

			default:
				TVPThrowExceptionMessage(TJS_W("Not drawble face type %1"), TJS_W("affineCopy"));
			}
		}
		if (updated)
		{
			UpdateLayerWithLayerObject(bmpobject_clo, &ur, &ImageLeft, &ImageTop);
		}
		else if (clear)
		{
			fillRect_passthrough(bmpobject_clo, ClipRect, NeutralColor, HoldAlpha);
		}
		return TJS_S_OK;
	}

	static tjs_error TJS_INTF_METHOD operateAffine(
		tTJSVariant	*result,
		tjs_int numparams,
		tTJSVariant **param,
		iTJSDispatch2 *objthis)
	{
		if (numparams < 12)
		{
			return TJS_E_BADPARAMCOUNT;
		}

		if(objthis == NULL) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTJSVariant bmpobject = tTJSVariant(objthis, objthis);
		tTJSVariantClosure bmpobject_clo = bmpobject.AsObjectClosureNoAddRef();

		tTJSVariantClosure srcbmpobject_clo = param[0]->AsObjectClosureNoAddRef();

		tTVPRect srcrect(*param[1], *param[2], *param[3], *param[4]);
		srcrect.right += srcrect.left;
		srcrect.bottom += srcrect.top;

		tjs_int opa = 255;
		tTVPBBStretchType type = stNearest;

		if(numparams >= 14 && param[13]->Type() != tvtVoid)
			opa = (tjs_int)*param[13];
		if(numparams >= 15 && param[14]->Type() != tvtVoid)
			type = (tTVPBBStretchType)(tjs_int)*param[14];
		if(numparams >= 16 && param[15]->Type() != tvtVoid)
		{
			TVPAddLog(TVPFormatMessage(TJS_W("Warring : Method %1 %2th parameter had is ignore. Hold destination alpha parameter is now deprecated."),
				TJS_W("Layer.operateAffine"), TJS_W("16")));
		}

		tTVPBlendOperationMode mode;
		if(numparams >= 13 && param[12]->Type() != tvtVoid)
			mode = (tTVPBlendOperationMode)(tjs_int)(*param[12]);
		else
			mode = omAuto;

		bool is_affine_matrix = param[5]->operator bool();
		// affine matrix mode
		t2DAffineMatrix mat;
		mat.a = *param[6];
		mat.b = *param[7];
		mat.c = *param[8];
		mat.d = *param[9];
		mat.tx = *param[10];
		mat.ty = *param[11];
		// points mode
		tTVPPointD points[3];
		points[0].x = *param[6];
		points[0].y = *param[7];
		points[1].x = *param[8];
		points[1].y = *param[9];
		points[2].x = *param[10];
		points[2].y = *param[11];

		// Now get information (this will independ the bitmap)
		tjs_int bmpwidth = 0;
		tjs_int bmpheight = 0;
		tjs_int bmppitch = 0;
		tjs_uint8* bmpdata = NULL;
		GetBitmapInformationFromObject(bmpobject_clo, true, &bmpwidth, &bmpheight, &bmppitch, &bmpdata);
		if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap bmpinfo(bmpwidth, bmpheight, bmppitch, bmpdata);

		tjs_int srcbmpwidth = 0;
		tjs_int srcbmpheight = 0;
		tjs_int srcbmppitch = 0;
		tjs_uint8* srcbmpdata = NULL;
		GetBitmapInformationFromObject(srcbmpobject_clo, false, &srcbmpwidth, &srcbmpheight, &srcbmppitch, &srcbmpdata);
		if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Specify Layer or Bitmap class object"));
		tTVPBaseBitmap srcbmpinfo(srcbmpwidth, srcbmpheight, srcbmppitch, srcbmpdata);

		tTVPDrawFace Face = dfAuto; // (outward) current drawing layer face
		tTVPLayerType DisplayType = ltOpaque; // actual Type
		tTVPRect ClipRect;
		bool HoldAlpha = false;
		tjs_int ImageLeft = 0;
		tjs_int ImageTop = 0;

		GetLayerInformationFromLayerObject(bmpobject_clo, &Face, &DisplayType, &ClipRect, &HoldAlpha, &ImageLeft, &ImageTop, NULL);

		// get correct blend mode if the mode is omAuto
		if(mode == omAuto) mode = GetOperationModeFromType(DisplayType);

		tTVPDrawFace DrawFace = GetDrawFace(Face, DisplayType); // (actual) current drawing layer face

		// convert tTVPBlendOperationMode to tTVPBBBltMethod
		tTVPBBBltMethod met;
		if(!GetBltMethodFromOperationModeAndDrawFace(DrawFace, met, mode))
		{
			// unknown blt mode
			TVPThrowExceptionMessage(TJS_W("Not drawble face type %1"), TJS_W("operateAffine"));
		}

		if(!bmpdata) TVPThrowExceptionMessage(TJS_W("Not drawable layer type"));
		if(!srcbmpdata) TVPThrowExceptionMessage(TJS_W("Source layer has no image"));

		bool updated = false;
		tTVPRect ur;
		if (is_affine_matrix)
		{
			updated = AffineBlt(&bmpinfo, ClipRect, &srcbmpinfo, srcrect, mat, met, opa, &ur, HoldAlpha, type);
		}
		else
		{
			updated = AffineBlt(&bmpinfo, ClipRect, &srcbmpinfo, srcrect, points, met, opa, &ur, HoldAlpha, type);
		}

		if (updated)
		{
			UpdateLayerWithLayerObject(bmpobject_clo, &ur, &ImageLeft, &ImageTop);
		}
		return TJS_S_OK;
	}
};

NCB_ATTACH_CLASS(LayerLayerExAffine, Layer)
{
	RawCallback("affineCopy", &Class::affineCopy, 0);
	RawCallback("operateAffine", &Class::operateAffine, 0);
};
