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


class TNull {};
static TNull ANull;
#define MAKE_EXPANDER(NExpander, NExpanderFn, NArg) \
inline void NExpander(NArg args)																																						{ NExpanderFn(args, ANull, ANull, ANull, ANull, ANull, ANull, ANull, ANull, 0); } \
template <class T> inline void NExpander(NArg args, T& v)																																{ NExpanderFn(args, v, ANull, ANull, ANull, ANull, ANull, ANull, ANull, 1); } \
template <class T, class T2> inline void NExpander(NArg args, T& v, T2& v2)																												{ NExpanderFn(args, v, v2, ANull, ANull, ANull, ANull, ANull, ANull, 2); } \
template <class T, class T2, class T3> inline void NExpander(NArg args, T& v, T2& v2, T3& v3)																							{ NExpanderFn(args, v, v2, v3, ANull, ANull, ANull, ANull, ANull, 3); } \
template <class T, class T2, class T3, class T4> inline void NExpander(NArg args, T& v, T2& v2, T3& v3, T4& v4)																			{ NExpanderFn(args, v, v2, v3, v4, ANull, ANull, ANull, ANull, 4); } \
template <class T, class T2, class T3, class T4, class T5> inline void NExpander(NArg args, T& v, T2& v2, T3& v3, T4& v4, T5& v5)														{ NExpanderFn(args, v, v2, v3, v4, v5, ANull, ANull, ANull, 5); } \
template <class T, class T2, class T3, class T4, class T5, class T6> inline void NExpander(NArg args, T& v, T2& v2, T3& v3, T4& v4, T5& v5, T6& v6)										{ NExpanderFn(args, v, v2, v3, v4, v5, v6, ANull, ANull, 6); } \
template <class T, class T2, class T3, class T4, class T5, class T6, class T7> inline void NExpander(NArg args, T& v, T2& v2, T3& v3, T4& v4, T5& v5, T6& v6, T7& v7)					{ NExpanderFn(args, v, v2, v3, v4, v5, v6, v7, ANull, 7); } \
template <class T, class T2, class T3, class T4, class T5, class T6, class T7, class T8> inline void NExpander(NArg args, T& v, T2& v2, T3& v3, T4& v4, T5& v5, T6& v6, T7& v7, T8& v8)	{ NExpanderFn(args, v, v2, v3, v4, v5, v6, v7, v8, 8); } 


struct LogArgs
{
	void* ctx;
	const char* funcName;
	const char* funcArgs;
};


//template <class T> void __ArgToString(char* out, T data) { sprintf(out, "%s{%d}, ", typeid(T).name(), (int)sizeof(data)); }
template <class T> void __ArgToString(char* out, const T& data) { sprintf(out, "(%s*)0x%llX, ", typeid(T).name(), (unsigned long long)&data); }
template <class T> void __ArgToString(char* out, T* data) { if (data == nullptr) sprintf(out, "null, "); else __ArgToString(out, *data); }
template <class T> void __ArgToString(char* out, T** data) { if (data == nullptr) sprintf(out, "null, "); else __ArgToString(out, *data); }
template <> void __ArgToString<const char>(char* out, const char* data) { sprintf(out, "\"%s\", ", data); }
template <> void __ArgToString<char>(char* out, char* data) { sprintf(out, "\"%s\", ", data); }
template <> void __ArgToString<const void>(char* out, const void* data) { sprintf(out, "0x%llX{?}, ", (unsigned long long)data); }
template <> void __ArgToString<const void*>(char* out, const void** data) { sprintf(out, "0x%llX{??}, ", (unsigned long long)*data); }
template <> void __ArgToString<void>(char* out, void* data) { sprintf(out, "0x%llX{?}, ", (unsigned long long)data); }
template <> void __ArgToString<void*>(char* out, void** data) { sprintf(out, "0x%llX{??}, ", (unsigned long long)*data); }
template <> void __ArgToString<int>(char* out, const int& data) { sprintf(out, "%d, ", data); }
template <> void __ArgToString<unsigned int>(char* out, const unsigned int& data) { sprintf(out, "0x%X, ", data); }
template <> void __ArgToString<unsigned long long>(char* out, const unsigned long long& data) { sprintf(out, "0x%llX, ", data); }
template <> void __ArgToString<long long>(char* out, const long long& data) { sprintf(out, "%lld, ", data); }



template <class T, class T2, class T3, class T4, class T5, class T6, class T7, class T8> inline void __ALog(const LogArgs& args, T& v, T2& v2, T3& v3, T4& v4, T5& v5, T6& v6, T7& v7, T8& v8, int nargs)
{
	char argsData[1024], *pArgsData = argsData;
	*pArgsData = 0;

	if (nargs > 0) { __ArgToString(pArgsData, v); pArgsData += strlen(pArgsData); }
	if (nargs > 1) { __ArgToString(pArgsData, v2); pArgsData += strlen(pArgsData); }
	if (nargs > 2) { __ArgToString(pArgsData, v3); pArgsData += strlen(pArgsData); }
	if (nargs > 3) { __ArgToString(pArgsData, v4); pArgsData += strlen(pArgsData); }
	if (nargs > 4) { __ArgToString(pArgsData, v5); pArgsData += strlen(pArgsData); }
	if (nargs > 5) { __ArgToString(pArgsData, v6); pArgsData += strlen(pArgsData); }
	if (nargs > 6) { __ArgToString(pArgsData, v7); pArgsData += strlen(pArgsData); }
	if (nargs > 7) { __ArgToString(pArgsData, v8); pArgsData += strlen(pArgsData); }

	D3DX12AFFINITY_LOG("[0x%llX] %s(%s) %s", (unsigned long long)args.ctx, args.funcName, args.funcArgs, argsData)
}

MAKE_EXPANDER(__Log, __ALog, LogArgs);

class __CallDeferred {
public:
	template <typename T> __CallDeferred(const T& Call) : call(Call) {}
	~__CallDeferred() { call(); }
protected:
	std::function<void()> call;
};

// Deferred
#define D3DX12_AFFINITY_DOCALL0(x)		     LogArgs __v={this, #x, "" }; __CallDeferred __vDef([&] () { __Log(__v); }); 
#define D3DX12_AFFINITY_DOCALL(x, ...)		 LogArgs __v={this, #x,  STRINGIFY_LIST(__VA_ARGS__) }; __CallDeferred __vDef([&] () { __Log(__v, __VA_ARGS__); }); 

// Non Deferred
//#define D3DX12_AFFINITY_DOCALL0(x)		     LogArgs __v={this, #x, "" }; __Log(__v);  
//#define D3DX12_AFFINITY_DOCALL(x, ...)		 LogArgs __v={this, #x,  STRINGIFY_LIST(__VA_ARGS__) }; __Log(__v, __VA_ARGS__);