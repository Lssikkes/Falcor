#pragma once

#define EXPAND(x) x
#define PP_NARGS(...) \
    EXPAND(_xPP_NARGS_IMPL(__VA_ARGS__,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0))
#define _xPP_NARGS_IMPL(x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,N,...) N

#define CONCATENATE(a, ...) P_CAT(a, __VA_ARGS__)
#define P_CAT(a, ...) a ## __VA_ARGS__

#define STRINGIFY(arg)  #arg

#define STRINGIFY_LIST_0() ""
#define STRINGIFY_LIST_1(x) STRINGIFY(x)
#define STRINGIFY_LIST_2(x, ...) STRINGIFY(x) "," EXPAND(STRINGIFY_LIST_1(__VA_ARGS__))
#define STRINGIFY_LIST_3(x, ...) STRINGIFY(x) "," EXPAND(STRINGIFY_LIST_2(__VA_ARGS__))
#define STRINGIFY_LIST_4(x, ...) STRINGIFY(x) "," EXPAND(STRINGIFY_LIST_3(__VA_ARGS__))
#define STRINGIFY_LIST_5(x, ...) STRINGIFY(x) "," EXPAND(STRINGIFY_LIST_4(__VA_ARGS__))
#define STRINGIFY_LIST_6(x, ...) STRINGIFY(x) "," EXPAND(STRINGIFY_LIST_5(__VA_ARGS__))
#define STRINGIFY_LIST_7(x, ...) STRINGIFY(x) "," EXPAND(STRINGIFY_LIST_6(__VA_ARGS__))
#define STRINGIFY_LIST_8(x, ...) STRINGIFY(x) "," EXPAND(STRINGIFY_LIST_7(__VA_ARGS__))

#define STRINGIFY_LIST_(N, ...) CONCATENATE(STRINGIFY_LIST_, N)(__VA_ARGS__)
#define STRINGIFY_LIST(...) STRINGIFY_LIST_(PP_NARGS(__VA_ARGS__), __VA_ARGS__)

#define MAKE_EXPANDER(NExpander, NExpanderFn, NArg) \
inline void NExpander(NArg args)																																						{ NExpanderFn(args, TNull(), TNull(), TNull(), TNull(), TNull(), TNull(), TNull(), TNull(), 0); } \
template <class T> inline void NExpander(NArg args, T& v)																																{ NExpanderFn(args, v, TNull(), TNull(), TNull(), TNull(), TNull(), TNull(), TNull(), 1); } \
template <class T, class T2> inline void NExpander(NArg args, T& v, T2& v2)																												{ NExpanderFn(args, v, v2, TNull(), TNull(), TNull(), TNull(), TNull(), TNull(), 2); } \
template <class T, class T2, class T3> inline void NExpander(NArg args, T& v, T2& v2, T3& v3)																							{ NExpanderFn(args, v, v2, v3, TNull(), TNull(), TNull(), TNull(), TNull(), 3); } \
template <class T, class T2, class T3, class T4> inline void NExpander(NArg args, T& v, T2& v2, T3& v3, T4& v4)																			{ NExpanderFn(args, v, v2, v3, v4, TNull(), TNull(), TNull(), TNull(), 4); } \
template <class T, class T2, class T3, class T4, class T5> inline void NExpander(NArg args, T& v, T2& v2, T3& v3, T4& v4, T5& v5)														{ NExpanderFn(args, v, v2, v3, v4, v5, TNull(), TNull(), TNull(), 5); } \
template <class T, class T2, class T3, class T4, class T5, class T6> inline void NExpander(NArg args, T& v, T2& v2, T3& v3, T4& v4, T5& v5, T6& v6)										{ NExpanderFn(args, v, v2, v3, v4, v5, v6, TNull(), TNull(), 6); } \
template <class T, class T2, class T3, class T4, class T5, class T6, class T7> inline void NExpander(NArg args, T& v, T2& v2, T3& v3, T4& v4, T5& v5, T6& v6, T7& v7)					{ NExpanderFn(args, v, v2, v3, v4, v5, v6, v7, TNull(), 7); } \
template <class T, class T2, class T3, class T4, class T5, class T6, class T7, class T8> inline void NExpander(NArg args, T& v, T2& v2, T3& v3, T4& v4, T5& v5, T6& v6, T7& v7, T8& v8)	{ NExpanderFn(args, v, v2, v3, v4, v5, v6, v7, v8, 8); } 


#include "d3dx12affinity_macros.h"

struct LogArgs
{
	const char* funcName;
	const char* funcArgs;
};

class TNull {};
template <class T, class T2, class T3, class T4, class T5, class T6, class T7, class T8> inline void __ALog(const LogArgs& args, T& v, T2& v2, T3& v3, T4& v4, T5& v5, T6& v6, T7& v7, T8& v8, int nargs)
{
	D3DX12AFFINITY_LOG("%s(%s)", args.funcName, args.funcArgs)
		D3DX12AFFINITY_LOG("%s(%s): %s", args.funcName, args.funcArgs)
}

MAKE_EXPANDER(__Log, __ALog, LogArgs);

#define D3DX12_AFFINITY_CALL0(x)			{ static LogArgs v={#x, "" }; __Log(v); }
#define D3DX12_AFFINITY_CALL(x, ...)		{ static LogArgs v={#x,  STRINGIFY_LIST(__VA_ARGS__) }; __Log(v, __VA_ARGS__); }