/*
	(c) Sergey Ryazanov (http://home.onego.ru/~ryazanov)

	Template file. May be included many times with different predefined macros.
*/
#if SRUTXX_DELEGATE_PARAM_COUNT > 0
#define SRUTXX_DELEGATE_SEPARATOR ,
#else
#define SRUTXX_DELEGATE_SEPARATOR
#endif

// see BOOST_JOIN for explanation
#define SRUTXX_DELEGATE_JOIN_MACRO( X, Y) SRUTXX_DELEGATE_DO_JOIN( X, Y )
#define SRUTXX_DELEGATE_DO_JOIN( X, Y ) SRUTXX_DELEGATE_DO_JOIN2(X,Y)
#define SRUTXX_DELEGATE_DO_JOIN2( X, Y ) X##Y

namespace utxx
{
#ifdef SRUTXX_DELEGATE_PREFERRED_SYNTAX
#define SRUTXX_DELEGATE_CLASS_NAME delegate
#define SRUTXX_DELEGATE_INVOKER_CLASS_NAME delegate_invoker
#else
#define SRUTXX_DELEGATE_CLASS_NAME SRUTXX_DELEGATE_JOIN_MACRO(delegate,SRUTXX_DELEGATE_PARAM_COUNT)
#define SRUTXX_DELEGATE_INVOKER_CLASS_NAME SRUTXX_DELEGATE_JOIN_MACRO(delegate_invoker,SRUTXX_DELEGATE_PARAM_COUNT)
	template <typename R SRUTXX_DELEGATE_SEPARATOR SRUTXX_DELEGATE_TEMPLATE_PARAMS>
	class SRUTXX_DELEGATE_INVOKER_CLASS_NAME;
#endif

	template <typename R SRUTXX_DELEGATE_SEPARATOR SRUTXX_DELEGATE_TEMPLATE_PARAMS>
#ifdef SRUTXX_DELEGATE_PREFERRED_SYNTAX
	class SRUTXX_DELEGATE_CLASS_NAME<R (SRUTXX_DELEGATE_TEMPLATE_ARGS)>
#else
	class SRUTXX_DELEGATE_CLASS_NAME
#endif
	{
	public:
		typedef R return_type;
#ifdef SRUTXX_DELEGATE_PREFERRED_SYNTAX
		//typedef return_type (SRUTXX_DELEGATE_CALLTYPE *signature_type)(SRUTXX_DELEGATE_TEMPLATE_ARGS);
		//typedef SRUTXX_DELEGATE_INVOKER_CLASS_NAME<signature_type> invoker_type;
		typedef SRUTXX_DELEGATE_INVOKER_CLASS_NAME<return_type(SRUTXX_DELEGATE_TEMPLATE_ARGS)> invoker_type;
#else
		typedef SRUTXX_DELEGATE_INVOKER_CLASS_NAME<R SRUTXX_DELEGATE_SEPARATOR SRUTXX_DELEGATE_TEMPLATE_ARGS> invoker_type;
#endif
		template <typename TFunctor> class proxy
		{
			TFunctor f;

		public:
			explicit proxy(TFunctor f) : f(f) {}
			SRUTXX_DELEGATE_CLASS_NAME operator&() {return from_method<proxy, &proxy::invoke>(this);}

		private:
			return_type invoke(SRUTXX_DELEGATE_PARAMS) {f(SRUTXX_DELEGATE_ARGS);}
		};

		SRUTXX_DELEGATE_CLASS_NAME()
			: object_ptr(0)
			, stub_ptr(0)
		{}

		template <return_type (*TMethod)(SRUTXX_DELEGATE_TEMPLATE_ARGS)>
		static SRUTXX_DELEGATE_CLASS_NAME from_function()
		{
			return from_stub(0, &function_stub<TMethod>);
		}

		template <class T, return_type (T::*TMethod)(SRUTXX_DELEGATE_TEMPLATE_ARGS)>
		static SRUTXX_DELEGATE_CLASS_NAME from_method(T* object_ptr)
		{
			return from_stub(object_ptr, &method_stub<T, TMethod>);
		}

		template <class T, return_type (T::*TMethod)(SRUTXX_DELEGATE_TEMPLATE_ARGS) const>
		static SRUTXX_DELEGATE_CLASS_NAME from_const_method(T const* object_ptr)
		{
			return from_stub(const_cast<T*>(object_ptr), &const_method_stub<T, TMethod>);
		}

		return_type operator()(SRUTXX_DELEGATE_PARAMS) const
		{
			return (*stub_ptr)(object_ptr SRUTXX_DELEGATE_SEPARATOR SRUTXX_DELEGATE_ARGS);
		}

		operator bool () const
		{
			return stub_ptr != 0;
		}

		bool operator!() const
		{
			return !(operator bool());
		}

	private:
		
		typedef return_type (SRUTXX_DELEGATE_CALLTYPE *stub_type)(void* object_ptr SRUTXX_DELEGATE_SEPARATOR SRUTXX_DELEGATE_PARAMS);

		void* object_ptr;
		stub_type stub_ptr;

		static SRUTXX_DELEGATE_CLASS_NAME from_stub(void* object_ptr, stub_type stub_ptr)
		{
			SRUTXX_DELEGATE_CLASS_NAME d;
			d.object_ptr = object_ptr;
			d.stub_ptr = stub_ptr;
			return d;
		}

		template <return_type (*TMethod)(SRUTXX_DELEGATE_TEMPLATE_ARGS)>
		static return_type SRUTXX_DELEGATE_CALLTYPE function_stub(void* SRUTXX_DELEGATE_SEPARATOR SRUTXX_DELEGATE_PARAMS)
		{
			return (TMethod)(SRUTXX_DELEGATE_ARGS);
		}

		template <class T, return_type (T::*TMethod)(SRUTXX_DELEGATE_TEMPLATE_ARGS)>
		static return_type SRUTXX_DELEGATE_CALLTYPE method_stub(void* object_ptr SRUTXX_DELEGATE_SEPARATOR SRUTXX_DELEGATE_PARAMS)
		{
			T* p = static_cast<T*>(object_ptr);
			return (p->*TMethod)(SRUTXX_DELEGATE_ARGS);
		}

		template <class T, return_type (T::*TMethod)(SRUTXX_DELEGATE_TEMPLATE_ARGS) const>
		static return_type SRUTXX_DELEGATE_CALLTYPE const_method_stub(void* object_ptr SRUTXX_DELEGATE_SEPARATOR SRUTXX_DELEGATE_PARAMS)
		{
			T const* p = static_cast<T*>(object_ptr);
			return (p->*TMethod)(SRUTXX_DELEGATE_ARGS);
		}
	};

	template <typename R SRUTXX_DELEGATE_SEPARATOR SRUTXX_DELEGATE_TEMPLATE_PARAMS>
#ifdef SRUTXX_DELEGATE_PREFERRED_SYNTAX
	class SRUTXX_DELEGATE_INVOKER_CLASS_NAME<R (SRUTXX_DELEGATE_TEMPLATE_ARGS)>
#else
	class SRUTXX_DELEGATE_INVOKER_CLASS_NAME
#endif
	{
		SRUTXX_DELEGATE_INVOKER_DATA

	public:
		SRUTXX_DELEGATE_INVOKER_CLASS_NAME(SRUTXX_DELEGATE_PARAMS)
#if SRUTXX_DELEGATE_PARAM_COUNT > 0
			:
#endif
			SRUTXX_DELEGATE_INVOKER_INITIALIZATION_LIST
		{
		}

		template <class TDelegate>
		R operator()(TDelegate d) const
		{
			return d(SRUTXX_DELEGATE_ARGS);
		}
	};
}

#undef SRUTXX_DELEGATE_CLASS_NAME
#undef SRUTXX_DELEGATE_SEPARATOR
#undef SRUTXX_DELEGATE_JOIN_MACRO
#undef SRUTXX_DELEGATE_DO_JOIN
#undef SRUTXX_DELEGATE_DO_JOIN2
