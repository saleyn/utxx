/*
	(c) Sergey Ryazanov (http://home.onego.ru/~ryazanov)

	Fast delegate compatible with C++ Standard.
*/
#ifndef SRUTIL_FLEX_BINDER_INCLUDED
#define SRUTIL_FLEX_BINDER_INCLUDED
#include <util/delegate/event.hpp>
namespace util
{
	template <class TSink>
	class event_flex_binder
	{
	public:
		typedef TSink sink_type;

		event_flex_binder() : holder(0) {}
		~event_flex_binder() {delete holder;}

		template <class TFunctor>
		void bind(const event_source<sink_type>& source, TFunctor const& sink)
		{
			unbind();
			functor_holder_impl<TFunctor>* h = new functor_holder_impl<TFunctor>(sink);
			holder = h;
			impl.bind(source, &h->p);
		}

		void unbind()
		{
			impl.unbind();
			delete holder;
			holder = 0;
		}

	private:
		struct functor_holder
		{
			virtual ~functor_holder() {}
		};

		template <typename TFunctor>
		struct functor_holder_impl : public functor_holder
		{
			typename TSink::template proxy<TFunctor> p;
			functor_holder_impl(TFunctor f) : p(f) {}
		};

		functor_holder* holder;
		event_binder<TSink> impl;
	};
}
#endif// SRUTIL_FLEX_BINDER_INCLUDED
