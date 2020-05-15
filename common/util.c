#include <stddef.h>

#include "npapi.h"

#include "util.h"


const char *
np_errorstr(NPError code)
{
	switch (code) {
	case NPERR_NO_ERROR:
		return "no error";
	case NPERR_GENERIC_ERROR:
		return "generic error";
	case NPERR_INVALID_INSTANCE_ERROR:
		return "invalid instance error";
	case NPERR_INVALID_FUNCTABLE_ERROR:
		return "invalid functable error";
	case NPERR_MODULE_LOAD_FAILED_ERROR:
		return "module load failed error";
	case NPERR_OUT_OF_MEMORY_ERROR:
		return "out of memory error";
	case NPERR_INVALID_PLUGIN_ERROR:
		return "invalid plugin error";
	case NPERR_INVALID_PLUGIN_DIR_ERROR:
		return "invalid plugin dir error";
	case NPERR_INCOMPATIBLE_VERSION_ERROR:
		return "incompatible version error";
	case NPERR_INVALID_PARAM:
		return "invalid param";
	case NPERR_INVALID_URL:
		return "invalid url";
	case NPERR_FILE_NOT_FOUND:
		return "file not found";
	case NPERR_NO_DATA:
		return "no data";
	case NPERR_STREAM_NOT_SEEKABLE:
		return "stream not seekable";
	case NPERR_TIME_RANGE_NOT_SUPPORTED:
		return "time range not supported";
	case NPERR_MALFORMED_SITE:
		return "malformed site";
	default:
		return "*unknown*";
	}
}

const char *
npn_varstr(NPNVariable v)
{
	switch (v) {
        case NPNVxDisplay:
		return "NPNVxDisplay";
        case NPNVxtAppContext:
		return "NPNVxtAppContext";
        case NPNVnetscapeWindow:
		return "NPNVnetscapeWindow";
        case NPNVjavascriptEnabledBool:
		return "NPNVjavascriptEnabledBool";
        case NPNVasdEnabledBool:
		return "NPNVasdEnabledBool";
        case NPNVisOfflineBool:
		return "NPNVisOfflineBool";
        case NPNVserviceManager:
		return "NPNVserviceManager";
        case NPNVDOMElement:
		return "NPNVDOMElement";
        case NPNVDOMWindow:
		return "NPNVDOMWindow";
	case NPNVToolkit:
		return "NPNVToolkit";
	case NPNVSupportsXEmbedBool:
		return "NPNVSupportsXEmbedBool";
	case NPNVWindowNPObject:
		return "NPNVWindowNPObject";
	case NPNVPluginElementNPObject:
		return "NPNVPluginElementNPObject";
	case NPNVSupportsWindowless:
		return "NPNVSupportsWindowless";
	case NPNVprivateModeBool:
		return "NPNVprivateModeBool";
	case NPNVsupportsAdvancedKeyHandling:
		return "NPNVsupportsAdvancedKeyHandling";
	case NPNVdocumentOrigin:
		return "NPNVdocumentOrigin";
	case NPNVCSSZoomFactor:
		return "NPNVCSSZoomFactor";
	case NPNVpluginDrawingModel:
		return "NPNVpluginDrawingModel";
	case NPNVsupportsAsyncBitmapSurfaceBool:
		return "NPNVsupportsAsyncBitmapSurfaceBool";
	case NPNVmuteAudioBool:
		return "NPNVmuteAudioBool";
	default:
		return "*unknown*";
	}
}

const char *
npp_varstr(NPPVariable v)
{
	switch (v) {
        case NPPVpluginNameString:
		return "NPPVpluginNameString";
	case NPPVpluginDescriptionString:
		return "NPPVpluginDescriptionString";
        case NPPVpluginWindowBool:
		return "NPPVpluginWindowBool";
        case NPPVpluginTransparentBool:
		return "NPPVpluginTransparentBool";
	case NPPVjavaClass:
		return "NPPVjavaClass";
        case NPPVpluginWindowSize:
		return "NPPVpluginWindowSize";
        case NPPVpluginTimerInterval:
		return "NPPVpluginTimerInterval";
        case NPPVpluginScriptableInstance:
		return "NPPVpluginScriptableInstance";
        case NPPVpluginScriptableIID:
		return "NPPVpluginScriptableIID";
        case NPPVjavascriptPushCallerBool:
		return "NPPVjavascriptPushCallerBool";
        case NPPVpluginKeepLibraryInMemory:
		return "NPPVpluginKeepLibraryInMemory";
        case NPPVpluginNeedsXEmbed:
		return "NPPVpluginNeedsXEmbed";
        case NPPVpluginScriptableNPObject:
		return "NPPVpluginScriptableNPObject";
        case NPPVformValue:
		return "NPPVformValue";
        case NPPVpluginUrlRequestsDisplayedBool:
		return "NPPVpluginUrlRequestsDisplayedBool";
        case NPPVpluginWantsAllNetworkStreams:
		return "NPPVpluginWantsAllNetworkStreams";
        case NPPVpluginNativeAccessibleAtkPlugId:
		return "NPPVpluginNativeAccessibleAtkPlugId";
        case NPPVpluginCancelSrcStream:
		return "NPPVpluginCancelSrcStream";
        case NPPVsupportsAdvancedKeyHandling:
		return "NPPVsupportsAdvancedKeyHandling";
        case NPPVpluginUsesDOMForCursorBool:
		return "NPPVpluginUsesDOMForCursorBool";
        case NPPVpluginDrawingModel:
		return "NPPVpluginDrawingModel";
	case NPPVpluginIsPlayingAudio:
		return "NPPVpluginIsPlayingAudio";
	default:
		return "*unknown*";
	}
}
