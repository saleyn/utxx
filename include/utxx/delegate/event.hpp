/*
    (c) Sergey Ryazanov (http://home.onego.ru/~ryazanov)

    Fast delegate compatible with C++ Standard.
*/
#ifndef SRUTXX_EVENT_INCLUDED
#define SRUTXX_EVENT_INCLUDED
#include <boost/iterator/iterator_concepts.hpp>
#include <list>

namespace utxx
{
    template <typename TSink> class event_source;
    template <typename TSink> class event_binder;

    template <typename TSink> class event_binder
    {
    public:
        typedef TSink sink_type;
        typedef event_source<sink_type> event_source_type;

        event_binder() : sink(sink_type()) {next = prev = this;}
        ~event_binder() { unbind(); }

        void bind(const event_source<sink_type>& source, sink_type sink);

        void unbind()
        {
            prev->next = next;
            next->prev = prev;
            next = prev = this;
        }

    private:
        event_binder* prev;
        event_binder* next;
        sink_type sink;
        friend class event_source<sink_type>;

        void attach_after(event_binder* that)
        {
            next = that->next;
            next->prev = this;
            that->next = this;
            prev = that;
        }

        event_binder(event_binder const&);
        event_binder const& operator=(event_binder const&);
    };

    template <typename TSink> class event_source
    {
    public:
        typedef TSink sink_type;
        typedef event_binder<sink_type> binder_type;

        template <class TInvoker>
        void unsafe_emit(TInvoker const& invoker)
        {
            binder_type* current = list_head.next;

            while(current != &list_head) {
                binder_type* next = current->next;
                if (current->sink)
                    invoker(current->sink);
                current = next;
            }
        }

        template <class TInvoker>
        void emit(TInvoker const& invoker, bool use_bookmark)
        {
            if (use_bookmark) {
                binder_type* current = list_head.next;

                while (current != &list_head) {
                    if (current->sink) {
                        event_binder<sink_type> bookmark;
                        bookmark.attach_after(current);
                        // *current may be excluded from list, but bookmark will 
                        // always be valid
                        invoker(current->sink); 
                        current = bookmark.next;
                    } else
                        current = current->next;
                }
                return;
            }

            unsafe_emit(invoker);
        }

        /// Syntactic sugar for <emit()>
        template <class TInvoker>
        void operator() (TInvoker const& invoker) { unsafe_emit(invoker); }

        /// Syntactic sugar for <emit()>
        template <class TInvoker>
        void operator() (TInvoker const& invoker, bool use_bookmark) {
            emit(invoker, use_bookmark);
        }

    private:
        mutable binder_type list_head;
        friend class event_binder<sink_type>;
    };

    /// This class is analogous to boost::signal2 but it is not
    /// thread safe.  The sinks must be created and destroyed
    /// synchronously with the instance of this class.
    template <typename TSink> class signal
    {
        struct wrapper {
            int id;
            TSink sink;

            wrapper(int a_id, TSink a_sink) : id(a_id), sink(a_sink) {}
            void operator==(const wrapper& a_rhs) const { return id == a_rhs.id; }
        };

        typedef std::list<wrapper> sink_list_type;
        typedef typename std::list<wrapper>::iterator sink_list_iter;

    public:
        typedef TSink sink_type;

        signal() : m_count(0) {}

        /// Connect an event sink to this signal.
        ///
        /// \code
        /// typedef utxx::delegate<void ()> delegate_t;
        /// struct Test { void operator() {} };
        /// void ttt() {}
        /// utxx::signal<delegate_t> sig;
        /// Test t;
        /// sig.connect(delegate_t::from_method<Test, &Test::operator()>(&t));
        /// sig.connect(delegate_t::from_function<&ttt>());
        /// \endcode
        int connect(sink_type a_sink) {
            m_sinks.push_back(wrapper(++m_count, a_sink));
            return m_count;
        }

        /// Disconnect an event sink from this signal
        void disconnect(int a_id) {
            for (sink_list_iter it=m_sinks.begin(); it != m_sinks.end();)
                if (it->id == a_id)
                    it = m_sinks.erase(it);
                else
                    ++it;
        }

        /// Notify all event sinks
        template <class TInvoker>
        void emit(TInvoker const& invoker)
        {
            for (sink_list_iter it=m_sinks.begin(), e=m_sinks.end(); it != e; ++it)
                invoker(it->sink);
        }

        /// Syntactic sugar for <emit()>
        template <class TInvoker>
        void operator() (TInvoker const& invoker) { emit(invoker); }

    private:
        int m_count;
        sink_list_type m_sinks;
    };

    template <typename TSink>
    void event_binder<TSink>::bind(const event_source<sink_type>& source, sink_type sink)
    {
        unbind();
        attach_after(&source.list_head);
        this->sink = sink;
    }

}
#endif// SRUTXX_EVENT_INCLUDED
