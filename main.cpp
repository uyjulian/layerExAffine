/////////////////////////////////////////////
//                                         //
//    Copyright (C) 2021-2021 Julian Uy    //
//  https://sites.google.com/site/awertyb  //
//                                         //
//   See details of license at "LICENSE"   //
//                                         //
/////////////////////////////////////////////

#include "ncbind/ncbind.hpp"
#include "LayerBitmapIntf.h"
#include "LayerBitmapAffineBlit.h"
#include <string.h>
#include <stdio.h>

bool GetBltMethodFromOperationModeAndDrawFace(
		tTVPDrawFace drawface,
		tTVPBBBltMethod & result,
		tTVPBlendOperationMode mode)
{
	// resulting corresponding  tTVPBBBltMethod value of mode and current drawface.
	// returns whether the method is known.
	tTVPBBBltMethod met;
	bool met_set = false;
	switch(mode)
	{
	case omPsNormal:			met_set = true; met = bmPsNormal;			break;
	case omPsAdditive:			met_set = true; met = bmPsAdditive;			break;
	case omPsSubtractive:		met_set = true; met = bmPsSubtractive;		break;
	case omPsMultiplicative:	met_set = true; met = bmPsMultiplicative;	break;
	case omPsScreen:			met_set = true; met = bmPsScreen;			break;
	case omPsOverlay:			met_set = true; met = bmPsOverlay;			break;
	case omPsHardLight:			met_set = true; met = bmPsHardLight;		break;
	case omPsSoftLight:			met_set = true; met = bmPsSoftLight;		break;
	case omPsColorDodge:		met_set = true; met = bmPsColorDodge;		break;
	case omPsColorDodge5:		met_set = true; met = bmPsColorDodge5;		break;
	case omPsColorBurn:			met_set = true; met = bmPsColorBurn;		break;
	case omPsLighten:			met_set = true; met = bmPsLighten;			break;
	case omPsDarken:			met_set = true; met = bmPsDarken;			break;
	case omPsDifference:   		met_set = true; met = bmPsDifference;		break;
	case omPsDifference5:   	met_set = true; met = bmPsDifference5;		break;
	case omPsExclusion:			met_set = true; met = bmPsExclusion;		break;
	case omAdditive:			met_set = true; met = bmAdd;				break;
	case omSubtractive:			met_set = true; met = bmSub;				break;
	case omMultiplicative:		met_set = true; met = bmMul;				break;
	case omDodge:				met_set = true; met = bmDodge;				break;
	case omDarken:				met_set = true; met = bmDarken;				break;
	case omLighten:				met_set = true; met = bmLighten;			break;
	case omScreen:				met_set = true; met = bmScreen;				break;
	case omAlpha:
		if(drawface == dfAlpha)
						{	met_set = true; met = bmAlphaOnAlpha; break;		}
		else if(drawface == dfAddAlpha)
						{	met_set = true; met = bmAlphaOnAddAlpha; break;		}
		else if(drawface == dfOpaque)
						{	met_set = true; met = bmAlpha; break;				}
		break;
	case omAddAlpha:
		if(drawface == dfAlpha)
						{	met_set = true; met = bmAddAlphaOnAlpha; break;		}
		else if(drawface == dfAddAlpha)
						{	met_set = true; met = bmAddAlphaOnAddAlpha; break;	}
		else if(drawface == dfOpaque)
						{	met_set = true; met = bmAddAlpha; break;			}
		break;
	case omOpaque:
		if(drawface == dfAlpha)
						{	met_set = true; met = bmCopyOnAlpha; break;			}
		else if(drawface == dfAddAlpha)
						{	met_set = true; met = bmCopyOnAddAlpha; break;		}
		else if(drawface == dfOpaque)
						{	met_set = true; met = bmCopy; break;				}
		break;
	}

	result = met;

	return met_set;
}

tTVPDrawFace GetDrawFace(tTVPDrawFace Face, tTVPLayerType DisplayType)
{
	tTVPDrawFace DrawFace;
	// set DrawFace from Face and Type
	if(Face == dfAuto)
	{
		// DrawFace is chosen automatically from the layer type
		switch(DisplayType)
		{
	//	case ltBinder:
		case ltOpaque:				DrawFace = dfOpaque;			break;
		case ltAlpha:				DrawFace = dfAlpha;				break;
		case ltAdditive:			DrawFace = dfOpaque;			break;
		case ltSubtractive:			DrawFace = dfOpaque;			break;
		case ltMultiplicative:		DrawFace = dfOpaque;			break;
		case ltDodge:				DrawFace = dfOpaque;			break;
		case ltDarken:				DrawFace = dfOpaque;			break;
		case ltLighten:				DrawFace = dfOpaque;			break;
		case ltScreen:				DrawFace = dfOpaque;			break;
		case ltAddAlpha:			DrawFace = dfAddAlpha;			break;
		case ltPsNormal:			DrawFace = dfAlpha;				break;
		case ltPsAdditive:			DrawFace = dfAlpha;				break;
		case ltPsSubtractive:		DrawFace = dfAlpha;				break;
		case ltPsMultiplicative:	DrawFace = dfAlpha;				break;
		case ltPsScreen:			DrawFace = dfAlpha;				break;
		case ltPsOverlay:			DrawFace = dfAlpha;				break;
		case ltPsHardLight:			DrawFace = dfAlpha;				break;
		case ltPsSoftLight:			DrawFace = dfAlpha;				break;
		case ltPsColorDodge:		DrawFace = dfAlpha;				break;
		case ltPsColorDodge5:		DrawFace = dfAlpha;				break;
		case ltPsColorBurn:			DrawFace = dfAlpha;				break;
		case ltPsLighten:			DrawFace = dfAlpha;				break;
		case ltPsDarken:			DrawFace = dfAlpha;				break;
		case ltPsDifference:	 	DrawFace = dfAlpha;				break;
		case ltPsDifference5:	 	DrawFace = dfAlpha;				break;
		case ltPsExclusion:			DrawFace = dfAlpha;				break;
		default:
							DrawFace = dfOpaque;			break;
		}
	}
	else
	{
		DrawFace = Face;
	}
	return DrawFace;
}

tTVPBlendOperationMode GetOperationModeFromType(tTVPLayerType DisplayType)
{
	// returns corresponding blend operation mode from layer type

	switch(DisplayType)
	{
//	case ltBinder:
	case ltOpaque:			return omOpaque;			 
	case ltAlpha:			return omAlpha;
	case ltAdditive:		return omAdditive;
	case ltSubtractive:		return omSubtractive;
	case ltMultiplicative:	return omMultiplicative;	 
	case ltDodge:			return omDodge;
	case ltDarken:			return omDarken;
	case ltLighten:			return omLighten;
	case ltScreen:			return omScreen;
	case ltAddAlpha:		return omAddAlpha;
	case ltPsNormal:		return omPsNormal;
	case ltPsAdditive:		return omPsAdditive;
	case ltPsSubtractive:	return omPsSubtractive;
	case ltPsMultiplicative:return omPsMultiplicative;
	case ltPsScreen:		return omPsScreen;
	case ltPsOverlay:		return omPsOverlay;
	case ltPsHardLight:		return omPsHardLight;
	case ltPsSoftLight:		return omPsSoftLight;
	case ltPsColorDodge:	return omPsColorDodge;
	case ltPsColorDodge5:	return omPsColorDodge5;
	case ltPsColorBurn:		return omPsColorBurn;
	case ltPsLighten:		return omPsLighten;
	case ltPsDarken:		return omPsDarken;
	case ltPsDifference:	return omPsDifference;
	case ltPsDifference5:	return omPsDifference5;
	case ltPsExclusion:		return omPsExclusion;


	default:
							return omOpaque;
	}
}

void GetBitmapInformationFromObject(tTJSVariantClosure clo, bool for_write, tjs_int *bmpwidth, tjs_int *bmpheight, tjs_int *bmppitch, tjs_uint8 **bmpdata)
{
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object && clo.Object)
		{
			tTJSVariant val;
			if (bmpwidth)
			{
				static ttstr imageWidth_name(TJS_W("imageWidth"));
				if (TJS_FAILED(layer_valclosure.PropGet(0, imageWidth_name.c_str(), imageWidth_name.GetHint(), &val, clo.Object)))
				{
					static ttstr width_name(TJS_W("width"));
					clo.PropGet(0, width_name.c_str(), width_name.GetHint(), &val, clo.Object);
				}
				*bmpwidth = (tjs_uint)(tTVInteger)val;
			}
			if (bmpheight)
			{
				static ttstr imageHeight_name(TJS_W("imageHeight"));
				if (TJS_FAILED(layer_valclosure.PropGet(0, imageHeight_name.c_str(), imageHeight_name.GetHint(), &val, clo.Object)))
				{
					static ttstr height(TJS_W("height"));
					clo.PropGet(0, height.c_str(), height.GetHint(), &val, clo.Object);
				}
				*bmpheight = (tjs_uint)(tTVInteger)val;
			}
			if (bmppitch)
			{
				static ttstr mainImageBufferPitch_name(TJS_W("mainImageBufferPitch"));
				if (TJS_FAILED(layer_valclosure.PropGet(0, mainImageBufferPitch_name.c_str(), mainImageBufferPitch_name.GetHint(), &val, clo.Object)))
				{
					static ttstr bufferPitch_name(TJS_W("bufferPitch"));
					clo.PropGet(0, bufferPitch_name.c_str(), bufferPitch_name.GetHint(), &val, clo.Object);
				}
				*bmppitch = (tjs_int)val;
			}
			if (bmpdata)
			{
				if (for_write)
				{
					static ttstr mainImageBufferForWrite_name(TJS_W("mainImageBufferForWrite"));
					if (TJS_FAILED(layer_valclosure.PropGet(0, mainImageBufferForWrite_name.c_str(), mainImageBufferForWrite_name.GetHint(), &val, clo.Object)))
					{
						static ttstr bufferForWrite_name(TJS_W("bufferForWrite"));
						clo.PropGet(0, bufferForWrite_name.c_str(), bufferForWrite_name.GetHint(), &val, clo.Object);
					}
				}
				else
				{
					static ttstr mainImageBuffer_name(TJS_W("mainImageBuffer"));
					if (TJS_FAILED(layer_valclosure.PropGet(0, mainImageBuffer_name.c_str(), mainImageBuffer_name.GetHint(), &val, clo.Object)))
					{
						static ttstr buffer_name(TJS_W("buffer"));
						clo.PropGet(0, buffer_name.c_str(), buffer_name.GetHint(), &val, clo.Object);
					}
				}
				*bmpdata = reinterpret_cast<tjs_uint8*>((tjs_intptr_t)(tjs_int64)val);
			}
		}
	}
}

void GetLayerInformationFromLayerObject(tTJSVariantClosure clo, tTVPDrawFace *Face, tTVPLayerType *DisplayType, tTVPRect *ClipRect, bool *HoldAlpha, tjs_int *ImageLeft, tjs_int *ImageTop, tjs_uint32 *NeutralColor)
{
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object && clo.Object)
		{
			tTJSVariant val;

			if (Face)
			{
				static ttstr face_name(TJS_W("face"));
				layer_valclosure.PropGet(0, face_name.c_str(), face_name.GetHint(), &val, clo.Object);
				*Face = (tTVPDrawFace)(tjs_int)val;
			}
			if (DisplayType)
			{
				static ttstr type_name(TJS_W("type"));
				layer_valclosure.PropGet(0, type_name.c_str(), type_name.GetHint(), &val, clo.Object);
				*DisplayType = (tTVPLayerType)(tjs_int)val;
			}

			if (ClipRect)
			{
				static ttstr clipLeft_name(TJS_W("clipLeft"));
				layer_valclosure.PropGet(0, clipLeft_name.c_str(), clipLeft_name.GetHint(), &val, clo.Object);
				ClipRect->left = (tjs_int)val;
				static ttstr clipTop_name(TJS_W("clipTop"));
				layer_valclosure.PropGet(0, clipTop_name.c_str(), clipTop_name.GetHint(), &val, clo.Object);
				ClipRect->top = (tjs_int)val;
				static ttstr clipWidth_name(TJS_W("clipWidth"));
				layer_valclosure.PropGet(0, clipWidth_name.c_str(), clipWidth_name.GetHint(), &val, clo.Object);
				ClipRect->right = ClipRect->left + ((tjs_int)val);
				static ttstr clipHeight_name(TJS_W("clipHeight"));
				layer_valclosure.PropGet(0, clipHeight_name.c_str(), clipHeight_name.GetHint(), &val, clo.Object);
				ClipRect->bottom = ClipRect->top + ((tjs_int)val);
			}

			if (HoldAlpha)
			{
				static ttstr holdAlpha_name(TJS_W("holdAlpha"));
				layer_valclosure.PropGet(0, holdAlpha_name.c_str(), holdAlpha_name.GetHint(), &val, clo.Object);
				*HoldAlpha = ((tjs_int)val) != 0;
			}

			if (ImageLeft)
			{
				static ttstr imageLeft_name(TJS_W("imageLeft"));
				layer_valclosure.PropGet(0, imageLeft_name.c_str(), imageLeft_name.GetHint(), &val, clo.Object);
				*ImageLeft = (tjs_int)val;
			}
			if (ImageTop)
			{
				static ttstr imageTop_name(TJS_W("imageTop"));
				layer_valclosure.PropGet(0, imageTop_name.c_str(), imageTop_name.GetHint(), &val, clo.Object);
				*ImageTop = (tjs_int)val;
			}

			if (NeutralColor)
			{
				static ttstr neutralColor_name(TJS_W("neutralColor"));
				layer_valclosure.PropGet(0, neutralColor_name.c_str(), neutralColor_name.GetHint(), &val, clo.Object);
				*NeutralColor = ((tjs_int64)val) & 0xffffffff;
			}
		}
	}
}

void SetLayerInformationOnLayerObject(tTJSVariantClosure clo, tTVPDrawFace *Face, tTVPLayerType *DisplayType, tTVPRect *ClipRect, bool *HoldAlpha, tjs_int *ImageLeft, tjs_int *ImageTop, tjs_uint32 *NeutralColor)
{
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if (global)
	{
		tTJSVariant layer_val;
		static ttstr Layer_name(TJS_W("Layer"));
		global->PropGet(0, Layer_name.c_str(), Layer_name.GetHint(), &layer_val, global);
		tTJSVariantClosure layer_valclosure = layer_val.AsObjectClosureNoAddRef();
		if (layer_valclosure.Object && clo.Object)
		{
			tTJSVariant val;

			if (Face)
			{
				static ttstr face_name(TJS_W("face"));
				val = (tjs_int)*Face;
				layer_valclosure.PropSet(0, face_name.c_str(), face_name.GetHint(), &val, clo.Object);
			}
			if (DisplayType)
			{
				static ttstr type_name(TJS_W("type"));
				val = (tjs_int)*DisplayType;
				layer_valclosure.PropSet(0, type_name.c_str(), type_name.GetHint(), &val, clo.Object);
			}

			if (ClipRect)
			{
				static ttstr clipLeft_name(TJS_W("clipLeft"));
				val = (tjs_int)(ClipRect->left);
				layer_valclosure.PropSet(0, clipLeft_name.c_str(), clipLeft_name.GetHint(), &val, clo.Object);
				static ttstr clipTop_name(TJS_W("clipTop"));
				val = (tjs_int)(ClipRect->top);
				layer_valclosure.PropSet(0, clipTop_name.c_str(), clipTop_name.GetHint(), &val, clo.Object);
				static ttstr clipWidth_name(TJS_W("clipWidth"));
				val = (tjs_int)(ClipRect->right - ClipRect->left);
				layer_valclosure.PropSet(0, clipWidth_name.c_str(), clipWidth_name.GetHint(), &val, clo.Object);
				static ttstr clipHeight_name(TJS_W("clipHeight"));
				val = (tjs_int)(ClipRect->bottom - ClipRect->top);
				layer_valclosure.PropSet(0, clipHeight_name.c_str(), clipHeight_name.GetHint(), &val, clo.Object);
			}

			if (HoldAlpha)
			{
				static ttstr holdAlpha_name(TJS_W("holdAlpha"));
				val = (tjs_int)(*HoldAlpha ? 1 : 0);
				layer_valclosure.PropSet(0, holdAlpha_name.c_str(), holdAlpha_name.GetHint(), &val, clo.Object);
			}

			if (ImageLeft)
			{
				static ttstr imageLeft_name(TJS_W("imageLeft"));
				val = (tjs_int)*ImageLeft;
				layer_valclosure.PropSet(0, imageLeft_name.c_str(), imageLeft_name.GetHint(), &val, clo.Object);
			}
			if (ImageTop)
			{
				static ttstr imageTop_name(TJS_W("imageTop"));
				val = (tjs_int)*ImageTop;
				layer_valclosure.PropSet(0, imageTop_name.c_str(), imageTop_name.GetHint(), &val, clo.Object);
			}

			if (NeutralColor)
			{
				static ttstr neutralColor_name(TJS_W("neutralColor"));
				val = (tjs_int64)*NeutralColor;
				layer_valclosure.PropSet(0, neutralColor_name.c_str(), neutralColor_name.GetHint(), &val, clo.Object);
			}
		}
	}
}

void UpdateLayerWithLayerObject(tTJSVariantClosure clo, tTVPRect *ur, tjs_int *ImageLeft, tjs_int *ImageTop)
{
	tTVPRect update_rect = *ur;
	if (ImageLeft != 0 || ImageTop != 0)
	{
		update_rect.add_offsets(*ImageLeft, *ImageTop);
	}
	if (!update_rect.is_empty())
	{
		tTJSVariant args[4];
		args[0] = (tjs_int)update_rect.left;
		args[1] = (tjs_int)update_rect.top;
		args[2] = (tjs_int)update_rect.get_width();
		args[3] = (tjs_int)update_rect.get_height();
		tTJSVariant *pargs[4] = {args +0, args +1, args +2, args +3};
		if (clo.Object)
		{
			static ttstr update_name(TJS_W("update"));
			clo.FuncCall(0, update_name.c_str(), update_name.GetHint(), NULL, 4, pargs, NULL);
		}
	}
}

static void PreRegistCallback()
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
