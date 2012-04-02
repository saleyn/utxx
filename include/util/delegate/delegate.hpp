/*
	(c) Sergey Ryazanov (http://home.onego.ru/~ryazanov)

	Fast delegate compatible with C++ Standard.
*/
#ifndef SRUTIL_DELEGATE_INCLUDED
#define SRUTIL_DELEGATE_INCLUDED

#if !defined(SRUTIL_DELEGATE_PREFERRED_SYNTAX) && !defined(NO_SRUTIL_DELEGATE_PREFERRED_SYNTAX)
#define SRUTIL_DELEGATE_PREFERRED_SYNTAX
#endif

namespace util
{
#ifdef SRUTIL_DELEGATE_PREFERRED_SYNTAX
	template <typename TSignature> class delegate;
	template <typename TSignature> class delegate_invoker;
#endif
}

#ifdef _MSC_VER
#define SRUTIL_DELEGATE_CALLTYPE __fastcall
#else
#define SRUTIL_DELEGATE_CALLTYPE
#endif

#include <util/delegate/detail/delegate_list.hpp>

#undef SRUTIL_DELEGATE_CALLTYPE

#endif//SRUTIL_DELEGATE_INCLUDED
