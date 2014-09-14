// vim:ts=2:et:sw=2
//----------------------------------------------------------------------------
/// \file  registrar.hpp
//----------------------------------------------------------------------------
/// \brief A class/instance registrar for supporting "inversion of control"
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#ifndef _UTXX_REGISTRAR_HPP_
#define _UTXX_REGISTRAR_HPP_

#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utxx/error.hpp>
#include <utxx/typeinfo.hpp>
#include <utxx/lock.hpp>
#include <boost/functional/hash.hpp>

struct B;
namespace utxx {
    struct instance {
        instance(const std::string& a_class) : m_class(a_class) {}
        instance(const std::string& a_class, const std::string& a_instance)
            : m_class(a_class), m_instance(a_instance)
        {}

        bool operator==(const instance& a) const
        { return m_class == a.m_class && m_instance == a.m_instance; }

        std::string m_class;
        std::string m_instance;
    };
} // namespace utxx

namespace std {
    template <> struct hash<utxx::instance> {
        size_t operator()(const utxx::instance& k) const {
            using boost::hash_value;
            using boost::hash_combine;

            size_t seed = 0;
            hash_combine(seed, hash_value(k.m_class));
            hash_combine(seed, hash_value(k.m_instance));
            return seed;
        }
    };
} // namespace std


namespace utxx {

struct empty {};

/// Registrar capable of creating instances of classes by type name
/// at run-time.
template <class Mutex, class ReflectionInfo = empty>
struct basic_typed_registrar
{
    template <class T>
    using pointer    = std::shared_ptr<T>;
    template <class T>
    using unique     = std::unique_ptr<T>;
    using string     = std::string;
    template <bool  A, class B> using enable_if = std::enable_if<A,B>;
    template <class A, class B> using is_same   = std::is_same<A,B>;
private:

    struct iwrap {
        virtual ~iwrap() {} // So that we can use dynamic_cast to wrap<T>
    };

    template <class T>
    struct wrap : public iwrap {
        pointer<T> m_content;
    public:
        wrap(const pointer<T>& a_content) : m_content(a_content) {}

        pointer<T>&       content()       { return m_content; }
        const pointer<T>& content() const { return m_content; }
    };
    using instance_map    = std::unordered_map<instance, pointer<iwrap>>;

    struct class_info {
        std::function<void* ()> m_ctor;
        ReflectionInfo          m_info;

        class_info()                  = default;
        class_info(const class_info&) = default;
        class_info(class_info&&)      = default;

        class_info(const std::function<void* ()>& a_ctor,
                   const ReflectionInfo& a_info)
            : m_ctor(a_ctor), m_info(a_info)
        {}
    };
    using class_info_map  = std::unordered_map<string,   class_info>;
    //using instance_map    = std::unordered_map<instance, pointer<BaseT>>;

    mutable Mutex   m_mutex;
    class_info_map  m_reflection;
    instance_map    m_instances;

    string descr(const instance& a_name) {
        return (a_name.m_instance.empty()
                ? "singleton" : "instance '" + a_name.m_instance + "'")
             + "of class '" + a_name.m_class + "'";
    }

    template <class T, typename... CtorArgs>
    void do_reg_class(const ReflectionInfo& a_info, CtorArgs&&... args) {
        auto type = type_to_string<T>();

        std::lock_guard<Mutex> guard(m_mutex);

        if (m_reflection.find(type) != m_reflection.end())
            throw utxx::badarg_error
                ("basic_registrar: class '" + type + "' is alredy registered!");

        auto ctor = [&args...]() -> T* {
            return new T(std::forward<CtorArgs>(args)...);
        };

        m_reflection.emplace(type, class_info(ctor, a_info));
    }

    template <class T, class Lambda>
    pointer<T> do_get(instance&& a_nm, Lambda a_ctor, bool a_use_ctor, bool a_register) {
        std::lock_guard<Mutex> guard(m_mutex);
        // Is the instance registered?

        auto it = m_instances.find(a_nm);
        if (it != m_instances.end()) {
            // The instance is registered, so unwrap the pointer to it, and return:
            //wrap<BaseT>* res = dynamic_cast<wrap<BaseT>*>(p.get());
            pointer<T> res = std::dynamic_pointer_cast<T>(it->second);
            if (!res)
                throw utxx::badarg_error
                    ("basic_registrar: type '", a_nm.m_class, "' is not compatible "
                     "with the instance type '", it->first.m_class,
                      "' registered with the registrar!");
            return res;
        }

        unique<T> p;

        if (a_use_ctor)
            p.reset(static_cast<T*>(a_ctor()));
        else {
            // See if there's a construction method registered for class T:
            auto cit = m_reflection.find(a_nm.m_class);
            if (cit == m_reflection.end())
                throw utxx::badarg_error
                    ("basic_registrar: class '", a_nm.m_class, "' must be "
                     "previously registered using reg_class<T> call!");
            // Construct an instance of class T:
            p.reset(static_cast<T*>((cit->second.m_ctor)()));
        }

        assert(p);

        pointer<T> res(static_cast<T*>(p.get()));

        if (!res)
            throw utxx::badarg_error
                ("basic_registrar: type '", a_nm.m_class, "' is not compatible "
                    "with the instance type '", it->first.m_class,
                    "' registered with the registrar!");

        if (a_register) {
            m_instances.emplace(a_nm, static_cast<pointer<iwrap>>(pointer<wrap<T>>(new wrap<T>(res))));
        }

        p.release();

        return res;
    }

public:
    /// Register a class T with the registrar.
    /// The registrar will be able to create instances of T dynamically
    /// using a constructor in the form:
    ///     \code T(DepArgs..., CtorArgs...) \endcode
    /// @tparam T        is the class to register.
    /// @tparam DepArgs  are the dependency injections that would be
    ///                  automatically created or looked up upon creation of
    ///                  an instance of T.
    /// @param  args     arguments lazily passed to the constructor of T.
    template <class T, typename... CtorArgs>
    void reg_class(const ReflectionInfo& a_info, CtorArgs&&... args) {
        do_reg_class<T>(a_info, std::forward<CtorArgs>(args)...);
    }

    template <class T, typename... CtorArgs>
    void reg_class(CtorArgs&&... args) {
        do_reg_class<T>(empty(), std::forward<CtorArgs>(args)...);
    }

    /// Check if type T has a constructor registered with the registrar.
    template <class T>
    bool is_class_registered() const {
        return is_class_registered(type_to_string<T>());
    }
    bool is_class_registered(const string& a_type) const {
        std::lock_guard<Mutex> guard(m_mutex);
        return m_reflection.find(a_type) != m_reflection.end();
    }

    /// Check if instance \a a_inst of type T is registered with the registrar.
    template <class T>
    bool is_singleton_registered() const {
        return is_instance_registered(type_to_string<T>(), "");
    }
    bool is_singleton_registered(const string& a_type) const {
        return is_instance_registered(a_type, "");
    }

    /// Check if instance \a a_inst of type T is registered with the registrar.
    template <class T>
    bool is_instance_registered(const string& a_inst) const {
        return is_instance_registered(type_to_string<T>(), a_inst);
    }
    bool is_instance_registered(const string& a_type, const string& a_inst) const {
        std::lock_guard<Mutex> guard(m_mutex);
        instance nm(a_type, a_inst);
        return m_instances.find(nm) != m_instances.end();
    }

    /// Register a unique instance \a a_name of class T with the registrar.
    /// The instance will be constructed using ctor previously registered
    /// via reg_class<T>() call.
    /// @param a_inst instance name unique to the scope of this registrar.
    template <class T>
    pointer<T> get_and_register(const string& a_inst) {
        static const auto ctor = []() -> T* { return nullptr; };
        return do_get<T>(instance(type_to_string<T>(), a_inst), ctor, false, true);
    }

    /// Register a unique instance \a a_inst of class T with the registrar.
    /// @param a_inst instance name unique to the scope of this registrar.
    /// @param a_ctor instance ctor to be used to create the instance.
    template <class T, class CtorLambda>
    typename enable_if<!is_same<CtorLambda, const char*>::value, pointer<T>>::type
    get_and_register(const string& a_inst, CtorLambda a_ctor) {
        return do_get<T>(instance(type_to_string<T>(), a_inst), a_ctor, true, true);
    }

    /// Register a unique instance \a a_name of class T with the registrar.
    /// @param a_type literal type name to be used to create an instance.
    ///               This type must be derived from BaseT class.
    /// @param a_inst instance name unique to the scope of this registrar.
    template <class T>
    pointer<T> get_and_register(const string& a_type, const string& a_inst) {
        static const auto ctor = []() -> T* { return nullptr; };
        return do_get<T>(instance(a_type, a_inst), ctor, false, true);
    }

    /// Register a unique instance \a a_name of class T with the registrar.
    /// @param a_type literal type name to be used to create an instance.
    ///               This type must be derived from BaseT class.
    /// @param a_inst instance name unique to the scope of this registrar.
    /// @param a_ctor instance ctor to be used to create the instance.
    template <class T, class CtorLambda>
    pointer<T> get_and_register(const string& a_type,
                                const string& a_inst, CtorLambda a_ctor) {
        return do_get<T>(instance(a_type, a_inst), a_ctor, true, true);
    }

    /// Get a registered singleton of class T or create one.
    /// The returned signleton is registered with the registrar, so consecutive
    /// calls to get_singleton on the same type will return the same pointer.
    template <class T>
    pointer<T> get_singleton() {
        static const auto ctor = []() -> T* { return nullptr; };
        return do_get<T>(instance(type_to_string<T>(),""), ctor, false, true);
    }

    /// Get a registered singleton of class T or create one.
    /// The returned signleton is registered with the registrar, so consecutive
    /// calls to get_singleton on the same type will return the same pointer.
    /// @param a_type literal type name to be used to create an instance.
    /// @param a_ctor instance ctor to be used to create the instance.
    template <class T, class CtorLambda>
    typename enable_if<!is_same<CtorLambda, const char*>::value, pointer<T>>::type
    get_singleton(CtorLambda a_ctor) {
        return do_get<T>(instance(type_to_string<T>(), ""), a_ctor, true, true);
    }

    /// Get a registered singleton of class T or create one.
    /// The returned signleton is registered with the registrar, so consecutive
    /// calls to get_singleton on the same type will return the same pointer.
    ///
    /// Use this function with causion, only when the base class BaseT of
    /// the class type T is known at run-time.
    /// @param a_type literal type name to be used to create an instance.
    ///               This type must be derived from BaseT class.
    template <class T>
    pointer<T> get_singleton(const string& a_type) {
        static const auto ctor = []() -> T* { return nullptr; };
        return do_get<T>(instance(a_type, ""), ctor, false, true);
    }

    /// Get a registered singleton of class T or create one.
    /// The returned signleton is registered with the registrar, so consecutive
    /// calls to get_singleton on the same type will return the same pointer.
    ///
    /// Use this function with causion, only when the base class BaseT of
    /// the class type T is known at run-time.
    /// @param a_type literal type name to be used to create an instance.
    ///               This type must be derived from BaseT class.
    /// @param a_ctor instance ctor to be used to create the instance.
    template <class T, class CtorLambda>
    typename enable_if<!is_same<CtorLambda, const char*>::value, pointer<T>>::type
    get_singleton(const string& a_type, CtorLambda a_ctor) {
        return do_get<T>(instance(a_type, ""), a_ctor, true, true);
    }

    /// Get or create a named instance of type T by name.
    /// @param a_type is the type of the instance to create.
    ///
    /// Get a registered instance of class T by name \a a_inst or create one if its
    /// constructor was registered with reg_class<T>, but not yet instantiated.
    /// @param a_inst is the name of the instance.
    template <class T>
    pointer<T> get(const string& a_inst) {
        if (a_inst.empty())
            throw utxx::badarg_error
                ("basic_registrar: instance name cannot be empty!");
        static const auto ctor = []() -> T* { return nullptr; };
        return do_get<T>(instance(type_to_string<T>(), a_inst), ctor, false, false);
    }

    /// Get or create a named instance of type T by name using privided \a a_ctor.
    /// @param a_inst is the name of the instance.
    template <class T, class CtorLambda>
    typename enable_if<!is_same<CtorLambda, const char*>::value, pointer<T>>::type
    get(const string& a_inst, CtorLambda a_ctor) {
        if (a_inst.empty())
            throw utxx::badarg_error
                ("basic_registrar: instance name cannot be empty!");
        return do_get<T>(instance(type_to_string<T>(), a_inst), a_ctor, true, false);
    }

    /// Get or create an instance of type \a a_type by name.
    /// @param a_type is the type of the instance to create.
    /// @param a_inst is the name of the instance to create.
    ///
    /// Get a registered instance of class whose type is given by \a a_type,
    /// or create one if its constructor was registered with reg_class<T>,
    /// but not yet instantiated.
    ///
    /// @return shared pointer of type \a a_type dynamically cast to BaseT
    ///         type.
    /// @throw utxx::badarg_error if the function fails to return a valid ptr.
    ///
    /// Use this function with causion, only when the base class BaseT of
    /// the class type T is known at run-time.
    template <class T>
    pointer<T> get(const string& a_type, const string& a_inst) {
        static const auto ctor = []() -> T* { return nullptr; };
        return do_get<T>(instance(a_type, a_inst), ctor, false, false);
    }

    /// Get or create a named instance of type BaseT by name using privided \a a_ctor.
    /// @param a_type is the type of the instance to create.
    /// @param a_inst is the name of the instance to create.
    /// @param a_ctor is the constructor to use when creating the instance.
    ///
    /// Get a registered instance of class whose type is given by \a a_type,
    /// or create one if its constructor was registered with reg_class<T>,
    /// but not yet instantiated.
    ///
    /// @return shared pointer of type \a a_type dynamically cast to BaseT
    ///         type.
    /// @throw utxx::badarg_error if the function fails to return a valid ptr.
    ///
    /// Use this function with causion, only when the base class BaseT of
    /// the class type T is known at run-time.
    template <class T, class CtorLambda>
    pointer<T> get(const string& a_type, const string& a_inst, CtorLambda a_ctor) {
        return do_get<T>(instance(a_type, a_inst), a_ctor, true, false);
    }

    /// Remove a registered instance of type \a a_type from the registrar
    template <class T>
    void erase(const string& a_instance) {
        erase(type_to_string<T>(), a_instance);
    }

    /// Remove a registered instance of type \a a_type from the registrar
    void erase(const string& a_type, const string& a_instance) {
        instance nm(a_type, a_instance);

        std::lock_guard<Mutex> guard(m_mutex);
        auto it = m_instances.find(nm);
        if (it == m_instances.end())
            throw utxx::badarg_error("basic_registrar: cannot erase ",
                descr(nm), " - it is not registered!");
        // If the requested instance is registered, erase it
        m_instances.erase(it);
    }
};

using typed_registrar            = basic_typed_registrar<null_lock>;
using concurrent_typed_registrar = basic_typed_registrar<std::mutex>;

} // namespace utxx

#endif // _UTXX_REGISTRAR_HPP_
