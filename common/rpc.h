#ifndef RPC_H__
#define RPC_H__

#include <stddef.h>

typedef struct RPCMsgHdr RPCMsgHdr;
typedef struct RPCMsg RPCMsg;
typedef struct RPCSess RPCSess;

struct RPCMsgHdr {
	int type;
	int method;
	size_t size;
};

struct RPCMsg {
	int method;
	void *param;
	size_t param_size;
	void *ret;
	size_t ret_size;
};

struct RPCSess {
	int fd;
	RPCSess *alt;
	void (*dispatch)(RPCSess *, RPCMsg *, void *);
	void *ctxt;
};

#define RPCEncUINT64	0
#define RPCEncUINT32	1
#define RPCEncUINT16	2
#define RPCEncUINT8	3
#define RPCEndBytes	4

#define RPCMsgHdrSize	8

#define RPCTypeRequest	0
#define RPCTypeResponse	1

#define RPC_NP(n)	((0 << 16) | (n))
#define RPC_NPP(n)	((1 << 16) | (n))
#define RPC_NPN(n)	((2 << 16) | (n))

#define RPC_NP_Initialize		RPC_NP(0)
#define RPC_NP_Shutdown			RPC_NP(1)
#define RPC_NP_GetValue			RPC_NP(2)
#define RPC_NP_GetMIMEDescription	RPC_NP(3)
#define RPC_NP_GetPluginVersion		RPC_NP(4)

#define RPC_NPP_New			RPC_NPP(0)
#define RPC_NPP_Destroy			RPC_NPP(1)
#define RPC_NPP_SetWindow		RPC_NPP(2)
#define RPC_NPP_NewStream		RPC_NPP(3)
#define RPC_NPP_DestroyStream		RPC_NPP(4)
#define RPC_NPP_StreamAsFile		RPC_NPP(5)
#define RPC_NPP_WriteReady		RPC_NPP(6)
#define RPC_NPP_Write			RPC_NPP(7)
/* RPC_NPP_Print		RPC_NPP(8) */
#define RPC_NPP_HandleEvent		RPC_NPP(9)
#define RPC_NPP_URLNotify		RPC_NPP(10)
/* RPC_NPP_JavaClass		RPC_NPP(11) */
#define RPC_NPP_GetValue		RPC_NPP(12)
#define RPC_NPP_SetValue		RPC_NPP(13)
/* RPC_NPP_GotFocus		RPC_NPP(14) */
/* RPC_NPP_LostFocus		RPC_NPP(15) */
#define RPC_NPP_URLRedirectNotify	RPC_NPP(16)
#define RPC_NPP_ClearSiteData		RPC_NPP(17)
#define RPC_NPP_GetSitesWithData	RPC_NPP(18)
/* RPC_NPP_DidComposite		RPC_NPP(19) */


#define RPC_NPN_GetURL			RPC_NPN(0)
#define RPC_NPN_PostURL			RPC_NPN(1)
#define RPC_NPN_RequestRead		RPC_NPN(2)
#define RPC_NPN_NewStream		RPC_NPN(3)
#define RPC_NPN_Write			RPC_NPN(4)
#define RPC_NPN_DestroyStream		RPC_NPN(5)
#define RPC_NPN_Status			RPC_NPN(6)
#define RPC_NPN_UserAgent		RPC_NPN(7)
#define RPC_NPN_MemAlloc		RPC_NPN(8)
#define RPC_NPN_MemFree			RPC_NPN(9)
#define RPC_NPN_MemFlush		RPC_NPN(10)
#define RPC_NPN_ReloadPlugins		RPC_NPN(11)
#define RPC_NPN_GetJavaEnv		RPC_NPN(12)
#define RPC_NPN_GetJavaPeer		RPC_NPN(13)
#define RPC_NPN_GetURLNotify		RPC_NPN(14)
#define RPC_NPN_PostURLNotify		RPC_NPN(15)
#define RPC_NPN_GetValue		RPC_NPN(16)
#define RPC_NPN_SetValue		RPC_NPN(17)
#define RPC_NPN_InvalidateRect		RPC_NPN(18)
#define RPC_NPN_InvalidateRegion	RPC_NPN(19)
#define RPC_NPN_ForceRedraw		RPC_NPN(20)
#define RPC_NPN_GetStringIdentifier	RPC_NPN(21)
#define RPC_NPN_GetStringIdentifiers	RPC_NPN(22)
#define RPC_NPN_GetIntIdentifier	RPC_NPN(23)
#define RPC_NPN_IdentifierIsString	RPC_NPN(24)
#define RPC_NPN_UTF8FromIdentifier	RPC_NPN(25)
#define RPC_NPN_IntFromIdentifier	RPC_NPN(26)
#define RPC_NPN_CreateObject		RPC_NPN(27)
#define RPC_NPN_RetainObject		RPC_NPN(28)
#define RPC_NPN_ReleaseObject		RPC_NPN(29)
#define RPC_NPN_Invoke			RPC_NPN(30)
#define RPC_NPN_InvokeDefault		RPC_NPN(31)
#define RPC_NPN_Evaluate		RPC_NPN(32)
#define RPC_NPN_GetProperty		RPC_NPN(33)
#define RPC_NPN_SetProperty		RPC_NPN(34)
#define RPC_NPN_RemoveProperty		RPC_NPN(35)
#define RPC_NPN_HasProperty		RPC_NPN(36)
#define RPC_NPN_HasMethod		RPC_NPN(37)
#define RPC_NPN_ReleaseVariantValue	RPC_NPN(38)
#define RPC_NPN_SetException		RPC_NPN(39)
#define RPC_NPN_PushPopupsEnabledState	RPC_NPN(40)
#define RPC_NPN_PopPopupsEnabledState	RPC_NPN(41)
#define RPC_NPN_Enumerate		RPC_NPN(42)
#define RPC_NPN_PluginThreadAsyncCall	RPC_NPN(43)
#define RPC_NPN_Construct		RPC_NPN(44)
#define RPC_NPN_GetValueForURL		RPC_NPN(45)
#define RPC_NPN_SetValueForURL		RPC_NPN(46)
#define RPC_NPN_GetAuthenticationInfo	RPC_NPN(47)
#define RPC_NPN_ScheduleTimer		RPC_NPN(48)
#define RPC_NPN_UnscheduleTimer		RPC_NPN(49)
#define RPC_NPN_PopUpContextMenu	RPC_NPN(50)
#define RPC_NPN_ConvertPoint		RPC_NPN(51)
#define RPC_NPN_HandleEvent		RPC_NPN(52)
#define RPC_NPN_UnfocusInstance		RPC_NPN(53)
#define RPC_NPN_URLRedirectResponse	RPC_NPN(54)
#define RPC_NPN_InitAsyncSurface	RPC_NPN(55)
#define RPC_NPN_FinalizeAsyncSurface	RPC_NPN(56)
#define RPC_NPN_SetCurrentAsyncSurface	RPC_NPN(57)


extern int rpcmsghdr_encode(RPCMsgHdr *, void *, size_t);
extern int rpcmsghdr_decode(RPCMsgHdr *, const void *, size_t);

extern void rpcsess_init(RPCSess *, int, RPCSess *,
		void (*)(RPCSess *, RPCMsg *, void *), void *);
extern int rpc_invoke(RPCSess *, RPCMsg *);
extern int rpc_handle(RPCSess *);
extern int rpc_return(RPCSess *, RPCMsg *);

#endif
