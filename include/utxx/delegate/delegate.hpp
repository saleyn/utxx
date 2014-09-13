/*
	(c) Sergey Ryazanov (http://home.onego.ru/~ryazanov)

	Fast delegate compatible with C++ Standard.

    TODO: implement using this method:
    http://codereview.stackexchange.com/questions/14730/impossibly-fast-delegate-in-c11
*/
#ifndef _UTXX_DELEGATE_HPP__
#define _UTXX_DELEGATE_HPP__

#if !defined(SRUTXX_DELEGATE_PREFERRED_SYNTAX) && !defined(NO_SRUTXX_DELEGATE_PREFERRED_SYNTAX)
#define SRUTXX_DELEGATE_PREFERRED_SYNTAX
#endif

namespace utxx
{
#ifdef SRUTXX_DELEGATE_PREFERRED_SYNTAX
	template <typename TSignature> class delegate;
	template <typename TSignature> class delegate_invoker;
#endif
}

#ifdef _MSC_VER
#define SRUTXX_DELEGATE_CALLTYPE __fastcall
#else
#define SRUTXX_DELEGATE_CALLTYPE
#endif

#include <utxx/delegate/detail/delegate_list.hpp>

#undef SRUTXX_DELEGATE_CALLTYPE

#endif//_UTXX_DELEGATE_HPP__
