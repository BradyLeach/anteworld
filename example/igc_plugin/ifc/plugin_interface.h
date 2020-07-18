#pragma once

#ifndef _INTERGEN_GENERATED__plugin_interface_H_
#define _INTERGEN_GENERATED__plugin_interface_H_

//@file Interface file for plugin_interface interface generated by intergen
//See LICENSE file for copyright and license information

//host class: ::plugin

#include <comm/commexception.h>
#include <comm/intergen/ifc.h>

class plugin;



////////////////////////////////////////////////////////////////////////////////
///Interface declaration: [namespace::]name, path
class plugin_interface
    : public intergen_interface
{
public:

    // --- interface methods ---

    
    void set_value( int x );

    int get_value() const;

    // --- creators ---

    ///Interface creator
    static iref<plugin_interface> get() {
        return get<plugin_interface>(0);
    }

    template<class T>
    static iref<T> get( T* _subclass_ );

    // --- internal helpers ---

    ///Interface revision hash
    static const int HASHID = 1303242348u;

    ///Interface name (full ns::class string)
    static const coid::tokenhash& IFCNAME() {
        static const coid::tokenhash _name = "plugin_interface"_T;
        return _name;
    }

    int intergen_hash_id() const override final { return HASHID; }

    bool iface_is_derived( int hash ) const override final {
        return hash == HASHID;
    }

    const coid::tokenhash& intergen_interface_name() const override final {
        return IFCNAME();
    }

    static const coid::token& intergen_default_creator_static( backend bck ) {
        static const coid::token _dc("plugin_interface.get@1303242348"_T);
        static const coid::token _djs("plugin_interface@wrapper.js"_T);
        static const coid::token _djsc("plugin_interface@wrapper.jsc"_T);
        static const coid::token _dlua("plugin_interface@wrapper.lua"_T);
        static const coid::token _dnone;

        switch(bck) {
        case backend::cxx: return _dc;
        case backend::js:  return _djs;
        case backend::jsc: return _djsc;
        case backend::lua: return _dlua;
        default: return _dnone;
        }
    }

    template<enum class backend B>
    static void* intergen_wrapper_cache() {
        static void* _cached_wrapper=0;
        if (!_cached_wrapper) {
            const coid::token& tok = intergen_default_creator_static(B);
            _cached_wrapper = coid::interface_register::get_interface_creator(tok);
        }
        return _cached_wrapper;
    }

    void* intergen_wrapper( backend bck ) const override final {
        switch(bck) {
        case backend::js:  return intergen_wrapper_cache<backend::js>();
        case backend::jsc: return intergen_wrapper_cache<backend::jsc>();
        case backend::lua: return intergen_wrapper_cache<backend::lua>();
        default: return 0;
        }
    }

    backend intergen_backend() const override { return backend::cxx; }

    const coid::token& intergen_default_creator( backend bck ) const override final {
        return intergen_default_creator_static(bck);
    }

    ///Client registrator
    template<class C>
    static int register_client()
    {
        static_assert(std::is_base_of<plugin_interface, C>::value, "not a base class");

        typedef intergen_interface* (*fn_client)();
        fn_client cc = []() -> intergen_interface* { return new C; };

        coid::token type = typeid(C).name();
        type.consume("class ");
        type.consume("struct ");

        coid::charstr tmp = "plugin_interface"_T;
        tmp << "@client-1303242348"_T << '.' << type;

        coid::interface_register::register_interface_creator(tmp, cc);
        return 0;
    }

protected:

    bool set_host(policy_intrusive_base*, intergen_interface*, iref<plugin_interface>* pout);
};

////////////////////////////////////////////////////////////////////////////////
template<class T>
inline iref<T> plugin_interface::get( T* _subclass_ )
{
    typedef iref<T> (*fn_creator)(plugin_interface*);

    static fn_creator create = 0;
    static const coid::token ifckey = "plugin_interface.get@1303242348"_T;

    if (!create)
        create = reinterpret_cast<fn_creator>(
            coid::interface_register::get_interface_creator(ifckey));

    if (!create) {
        log_mismatch("get"_T, "plugin_interface.get"_T, "@1303242348"_T);
        return 0;
    }

    return create(_subclass_);
}

#pragma warning(push)
#pragma warning(disable : 4191)

inline void plugin_interface::set_value( int x )
{ return VT_CALL(void,(int),0)(x); }

inline int plugin_interface::get_value() const
{ return VT_CALL(int,() const,1)(); }

#pragma warning(pop)


#endif //_INTERGEN_GENERATED__plugin_interface_H_
