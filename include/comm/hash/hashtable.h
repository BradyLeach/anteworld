/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is COID/comm module.
 *
 * The Initial Developer of the Original Code is
 * PosAm.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Brano Kemen
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#ifndef __COID_COMM_HASHTABLE__HEADER_FILE__
#define __COID_COMM_HASHTABLE__HEADER_FILE__


#include "../namespace.h"
#include "../dynarray.h"
#include "../metastream/metastream.h"
#include "../bitrange.h"
#include <algorithm>

#include "hashfunc.h"

#if !defined(_MSC_VER) || _MSC_VER >= 1800
#include "../alloc/slotalloc.h"
#endif

COID_NAMESPACE_BEGIN


////////////////////////////////////////////////////////////////////////////////
template <class T>
struct _Select_Itself{
    typedef const T&    ret_type;

    ret_type operator()(const T& t) const { return t; }
};

template <class PAIR, class LOOKUP>
struct _Select_pair1st
{
    typedef const LOOKUP&  ret_type;

    ret_type operator() (const PAIR& __x) const  { return __x.first; }
};

////////////////////////////////////////////////////////////////////////////////
template<class T>
struct AllocStd {
    T* alloc() { return new T; }
    T* alloc_uninit() { return (T*)::dlmalloc(sizeof(T)); }
    void free(T* p) { delete p; }

    uints index(const T* p) const { return (uints)p; }
    T* pointer(uints id) const { return (T*)id; }

    ints reserve(uints count) { return 0; }
};

#if !defined(_MSC_VER) || _MSC_VER >= 1800

template<class T>
struct AllocSlot {
    T* alloc() { return _slots.add(); }
    T* alloc_uninit() { return _slots.add_uninit(); }
    void free(T* p) { _slots.del(p); }

    uints index(const T* p) const { return _slots.get_item_id(p); }
    T* pointer(uints id) const { return (T*)_slots.get_item(id); }

    ints reserve(uints count) { return _slots.reserve(count); }

private:
    slotalloc_base<T> _slots;
};

#endif

////////////////////////////////////////////////////////////////////////////////
///Base class for hash containers
//@param VAL value type stored at hashtable nodes
//@param HASHFUNC hash functor, should define type of the key as key_type
//@param EQFUNC equality functor
//@param GETKEYFUNC key extractor from VAL
template <class VAL, class HASHFUNC, class EQFUNC, class GETKEYFUNC, template<class> class ALLOC=AllocStd>
class hashtable
{
public:

    ///Type used for lookups is deduced from the hash template
    typedef typename HASHFUNC::key_type         LOOKUP;

    struct Node
    {
        VAL     _val;
        Node*   _next;

        Node() : _next(0) {}

        COIDNEWDELETE_NOTRACK;
    };

    //@return value index
    uints get_value_index(const VAL* v) const {
        return _ALLOC.index((Node*)v);
    }

    //@return object by index
    const VAL* get_value( uints id ) const {
        return (const VAL*)_ALLOC.pointer(id);
    }

private:
    dynarray<Node*> _table;
    uints _nelem;

    typedef hashtable<VAL,HASHFUNC,EQFUNC,GETKEYFUNC,ALLOC>  _Self;

protected:

    void add( Node** n, const VAL& v )
    {
        Node* r = _ALLOC.alloc();
        r->_next = *n;
        r->_val = v;
        *n = r;
    }

    HASHFUNC    _HASHFUNC;
    EQFUNC      _EQFUNC;
    GETKEYFUNC  _GETKEYFUNC;
    ALLOC<Node> _ALLOC;

    uints bucket( const LOOKUP& v ) const              { return _HASHFUNC(v) % _table.size(); }
    uints bucketn( const LOOKUP& v, uints n ) const    { return _HASHFUNC(v) % n; }


public:

    const HASHFUNC& hash_func() const   { return _HASHFUNC; }
    HASHFUNC& hash_func()               { return _HASHFUNC; }

    const EQFUNC& equal_func() const    { return _EQFUNC; }
    EQFUNC& equal_func()                { return _EQFUNC; }


    class Ptr : public std::iterator<std::forward_iterator_tag, VAL>
    {
        Node* _p;
        const _Self* _ht;

    public:

        typedef LOOKUP                  key_type;
        typedef VAL                     value_type;
        typedef HASHFUNC                hasher;
        typedef EQFUNC                  key_equal;

        typedef size_t                  size_type;
        typedef ptrdiff_t               difference_type;
        typedef VAL*                    pointer;
        typedef const VAL*              const_pointer;
        typedef VAL&                    reference;
        typedef const VAL&              const_reference;

        const Node* _get_node() const   { return _p; }
        const _Self* _get_ht() const    { return _ht; }

        Ptr( Node* p, const _Self& ht ) : _p(p), _ht(&ht) {}
        Ptr() : _p(0), _ht(0) {}

        bool operator == (const Ptr& p) const   { return _p == p._p; }
        bool operator != (const Ptr& p) const   { return _p != p._p; }

        reference operator*()           { return _p->_val; }
        pointer operator ->()           { return &_p->_val; }

        Ptr& operator++()
        {
            Node* n = _p->_next;
            if(!n)
                n = _ht->get_next(_p);
            _p = n;
            return *this;
        }
        
        inline Ptr operator++(int)
        {
            Ptr tmp = *this;
            ++*this;
            return tmp;
        } 
    };

    class CPtr : public std::iterator<std::forward_iterator_tag, VAL>
    {
        const Node* _p;
        const _Self*  _ht;

    public:

        typedef LOOKUP                  key_type;
        typedef VAL                     value_type;
        typedef HASHFUNC                hasher;
        typedef EQFUNC                  key_equal;

        typedef size_t                  size_type;
        typedef ptrdiff_t               difference_type;
        typedef VAL*                    pointer;
        typedef const VAL*              const_pointer;
        typedef VAL&                    reference;
        typedef const VAL&              const_reference;

        CPtr( const Node* p, const _Self& ht ) : _p(p), _ht(&ht) {}
        CPtr() : _p(0), _ht(0) {}

        CPtr( const Ptr& p )
        {
            _p = p._get_node();
            _ht = p._get_ht();
        }

        CPtr& operator = (const CPtr& p)
        {
            _p = p._p;
            _ht = p._ht;
            return *this;
        }

        CPtr& operator = (const Ptr& p)
        {
            _p = p._get_node();
            _ht = p._get_ht();
            return *this;
        }

        bool operator == (const CPtr& p) const  { return _p == p._p; }
        bool operator != (const CPtr& p) const  { return _p != p._p; }

        const_reference operator*() const       { return _p->_val; }
        const_pointer operator ->()             { return &_p->_val; }

        CPtr& operator++()
        {
            const Node* n = _p->_next;
            if(!n)
                n = _ht->get_next(_p);
            _p = n;
            return *this;
        }
        
        inline CPtr operator++(int)
        {
            CPtr tmp = *this;
            ++*this;
            return tmp;
        } 
    };

    typedef Ptr                         iterator;
    typedef CPtr                        const_iterator;

    typedef std::pair<CPtr, CPtr>       range_const_iterator;
    typedef std::pair<Ptr, Ptr>         range_iterator;

    struct hashtable_binstream_container : public binstream_containerT<VAL>
    {
        typedef typename binstream_containerT<VAL>::fnc_stream    fnc_stream;

        virtual const void* extract( uints n )
        {
            DASSERT( _begin != _end );
            const VAL* p = &(*_begin);
            ++_begin;
            return p;
        }

        virtual void* insert( uints n )
        {
            Node* p = *_newnode.add() = _ht._ALLOC.alloc();
            return &p->_val;
        }

        virtual bool is_continuous() const { return false; }

        virtual uints count() const { return _ht.size(); }


        hashtable_binstream_container( const _Self& ht )
            : _ht((_Self&)ht)
        {
            _begin = _ht.begin();
            _end = _ht.end();
        }

        hashtable_binstream_container( _Self& ht )
            : _ht(ht)
        {
            _begin = _ht.begin();
            _end = _ht.end();
        }

        hashtable_binstream_container( const _Self& ht, fnc_stream fout, fnc_stream fin )
            : binstream_containerT<VAL>(fout, fin), _ht((_Self&)ht)
        {
            _begin = _ht.begin();
            _end = _ht.end();
        }

        hashtable_binstream_container( _Self& ht, fnc_stream fout, fnc_stream fin )
            : binstream_containerT<VAL>(fout, fin), _ht(ht)
        {
            _begin = _ht.begin();
            _end = _ht.end();
        }

        ~hashtable_binstream_container()
        {
            for( uints i=0; i<_newnode.size(); ++i )
            {
                Node** ppn = _ht.find_socket( _ht._GETKEYFUNC(_newnode[i]->_val) );
                _newnode[i]->_next = *ppn;
                *ppn = _newnode[i];
            }

            _ht._nelem += _newnode.size();
        }

    protected:
        _Self& _ht;
        const_iterator _begin, _end;
        dynarray<Node*> _newnode;
    };

protected:

    ///Find first node that matches the key, provided hash value is given
    Node* find_node( uint hash, const LOOKUP& k ) const
    {
        uints h = hash % _table.size();
        Node* n = (Node*)_table[h];
        while(n)
        {
            if( _EQFUNC( _GETKEYFUNC(n->_val), k ) )
                return n;
            n = n->_next;
        }
        return 0;
    }

    ///Find first node that matches the key
    Node* find_node( const LOOKUP& k ) const
    {
        uints h = bucket(k);
        Node* n = (Node*)_table[h];
        while(n)
        {
            if( _EQFUNC( _GETKEYFUNC(n->_val), k ) )
                return n;
            n = n->_next;
        }
        return 0;
    }

    ///Find first node that matches the key
    Node* find_node( const LOOKUP& k, uints& slot ) const
    {
        slot = bucket(k);
        Node* n = (Node*)_table[slot];
        while(n)
        {
            if( _EQFUNC( _GETKEYFUNC(n->_val), k ) )
                return n;
            n = n->_next;
        }
        return 0;
    }

    ///Find first node that matches the key
    Node** find_socket( const LOOKUP& k ) const
    {
        uints h = bucket(k);
        Node** n = (Node**)&_table[h];
        while(*n)
        {
            if( _EQFUNC( _GETKEYFUNC((*n)->_val), k ) )
                return n;
            n = &(*n)->_next;
        }
        return n;
    }

    ///Find first node that matches the key
    Node** find_socket( dynarray<Node*>& a, const LOOKUP& k ) const
    {
        uints h = bucketn( k, a.size() );
        Node** n = (Node**)&a[h];
        while(*n)
        {
            if( _EQFUNC( _GETKEYFUNC((*n)->_val), k ) )
                return n;
            n = &(*n)->_next;
        }
        return n;
    }

    ///Delete all records that match the key
    uints del( const LOOKUP& k )
    {
        Node** ppn = find_socket(k);
        Node* n = *ppn;
        if(!n)  return 0;

        uints c=0;
        while( n  &&  _EQFUNC( _GETKEYFUNC(n->_val), k ) )
        {
            Node* t = n->_next;
            _ALLOC.free(n);
            n = t;
            ++c;
        }

        *ppn = n;
        _nelem -= c;
        return c;
    }

    ///Find first node that matches the key
    Node** get_socket( const Node* n ) const
    {
        if(!n)  return 0;
        uints h = bucket( _GETKEYFUNC(n->_val) );

        Node** pn = (Node**)&_table[h];
        while(*pn)
        {
            if( *pn == n )
                return pn;
            pn = &(*pn)->_next;
        }
        RASSERTX( 1, "invalid argument" );
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////
public:

    binstream& stream_out( binstream& bin ) const
    {
        hashtable_binstream_container bc(*this);
        bin.xwrite_array( bc );

        return bin;
    }

    binstream& stream_in( binstream& bin )
    {
        hashtable_binstream_container bc(*this, UMAXS);
        bin.xread_array( bc );

        return bin;
    }

    metastream& stream_out( metastream& m ) const
    {
        hashtable_binstream_container bc(*this);
        m.write_container(bc);

        return m;
    }

    metastream& stream_in( metastream& m )
    {
        hashtable_binstream_container bc(*this, UMAXS);
        m.read_container(bc);

        return m;
    }


    ///
    Node* get_next( const Node* cn ) const
    {
        if(!cn)  return 0;
        
        uints h = bucket( _GETKEYFUNC(cn->_val) );
        DASSERTX( _table[h] != 0, "probably mixed keys and different hash functions used" );

        return get_nonempty(++h);
    }

    Node* get_nonempty( uints slot ) const
    {
        uints n = _table.size();
        for( ; slot<n; ++slot )
        {
            if( _table[slot] )
                return (Node*)_table[slot];
        }
        return 0;
    }

    size_t size() const          { return _nelem; }
    size_t max_size() const      { return size_t(-1); }
    bool empty() const           { return size() == 0; }

    friend void swap(_Self& a, _Self& b)
    {
        std::swap( a._HASHFUNC, b._HASHFUNC );
        std::swap( a._EQFUNC, b._EQFUNC );
        std::swap( a._GETKEYFUNC, b._GETKEYFUNC );
        std::swap( a._table, b._table );
        std::swap( a._nelem, b._nelem );
    }

    iterator begin()
    {
        uints n = _table.size();
        for( uints i=0; i<n; ++i )
            if( _table[i] )
                return iterator( _table[i], *this );
        return end();
    }

    iterator end() {
        return iterator( (Node*)0, *this );
    }

    const_iterator begin() const
    {
        uints n = _table.size();
        for( uints i=0; i<n; ++i )
            if( _table[i] )
                return const_iterator( _table[i], *this );
        return end();
    }

    const_iterator end() const {
        return const_iterator( (Node*)0, *this );
    }

  
    size_t bucket_count() const {
        return _table.size();
    }

    size_t max_bucket_count() const {
        return size_t(-1);
    }

    size_t elems_in_bucket( size_t k ) const
    {
        Node* n = _table[k];
        size_t r=0;
        for( ; n!=0; n=n->_next )  ++r;
        return r;
    }


    iterator find( const LOOKUP& k )
    {
        Node* n = find_node(k);
        if(n)
            return iterator( n, *this );
        else
            return end();
    }

    const_iterator find( const LOOKUP& k ) const
    {
        const Node* n = find_node(k);
        if(n)
            return const_iterator( n, *this );
        else
            return end();
    }

    size_t count( const LOOKUP& k )
    {
        Node* n = find_node(k);
        if(!n)  return 0;
        
        size_t c=0;
        while( n  &&  _EQFUNC( _GETKEYFUNC(n->_val), k ) )
        {
            ++c;
            n = n->_next;
        }
        
        return c;
    }

    std::pair<iterator, iterator> equal_range( const LOOKUP& k )
    {
        uints slot;
        Node* f = find_node(k,slot);
        if(!f)
            return std::pair<iterator,iterator>( end(), end() );
        
        Node* l = f->_next;
        while( l  &&  _EQFUNC( _GETKEYFUNC(l->_val), k ) )
            l = l->_next;

        if(!l)  l = get_nonempty(++slot);
        return std::pair<iterator,iterator>( iterator(f,*this), iterator(l,*this) );
    }

    std::pair<const_iterator, const_iterator> equal_range( const LOOKUP& k ) const
    {
        uints slot;
        const Node* f = find_node(k,slot);
        if(!f)
            return std::pair<const_iterator,const_iterator>( end(), end() );
        
        const Node* l = f->_next;
        while( l  &&  _EQFUNC( _GETKEYFUNC(l->_val), k ) )
            l = l->_next;

        if(!l)  l = get_nonempty(++slot);
        return std::pair<const_iterator,const_iterator>( const_iterator(f,*this), const_iterator(l,*this) );
    }

    bool erase_value( const LOOKUP& k, VAL* dst ) {
        return this->__erase_value(k, dst);
    }

    size_t erase( const LOOKUP& k ) {
        return del(k);
    }

    void erase( iterator& it )
    {
        Node** pn = get_socket(it._get_node());
        ++it;

        Node* n = *pn;
        *pn = n->_next;
        _ALLOC.free(n);
        --_nelem;
    }

    void erase( iterator f, iterator l )
    {
        Node** pn = get_socket(f._p);
        Node* n = *pn;
        if(!n)  return;
        Node* nl = l._p;
        while( n  &&  n!=nl )
        {
            Node* t = n;
            n = n->_next;
            if(!n)
                n = get_next(t);
            _ALLOC.free(t);
            --_nelem;
        }
    }


    void resize( size_t bucketn )
    {
        uints ts = _table.size();
        if( bucketn > ts )
        {
            uints nb = nearest_high_pow2(bucketn);

            dynarray<Node*> temp;
            temp.need_newc(nb);
            ints offset = _ALLOC.reserve(nb);

            for( uints i=0; i<ts; ++i )
            {
                Node* n = _table[i];
                while(n)
                {
                    Node* t = ptr_byteshift(n->_next, offset);
                    Node** pn = find_socket( temp, _GETKEYFUNC(n->_val) );
                    n->_next = *pn;
                    *pn = n;
                    n = t;
                }
            }

            std::swap(temp, _table );
        }
    }

    void clear()
    {
        for( uints i=0; i<_table.size(); ++i )
        {
            Node* n = _table[i];
            while(n)
            {
                Node* t = n->_next;
                _ALLOC.free(n);
                n = t;
            }
            _table[i] = 0;
        }

        _nelem = 0;
        //_table.need_newc(64);
    }


    std::pair<iterator, bool> insert_unique( const VAL& v )
    {
        adjust(1);

        Node** ppn = __insert_unique(v);
        if(!ppn)
            return std::pair<iterator,bool>( end(), false );
        else
            return std::pair<iterator,bool>( iterator( *ppn, *this ), true );
    }

    iterator insert_equal( const VAL& v )
    {
        adjust(1);
        return iterator( *_copy_insert_equal(v), *this );
    }

    void insert_unique( const VAL* f, const VAL* l )
    {
        size_t n = l - f;
        adjust(n);
        for( ; n>0; --n, ++f )
            _copy_insert_unique(*f);
    }

    void insert_equal( const VAL* f, const VAL* l )
    {
        size_t n = l - f;
        adjust(n);
        for( ; n>0; --n, ++f )
            _copy_insert_equal(*f);
    }

    void insert_unique( const_iterator f, const_iterator l )
    {
        size_t n = 0;
        std::distance( f, l, n );
        adjust(n);

        for( ; n>0; --n, ++f )
            _copy_insert_unique(*f);
    }

    void insert_equal( const_iterator f, const_iterator l )
    {
        size_t n = 0;
        std::distance( f, l, n );
        adjust(n);

        for( ; n>0; --n, ++f )
            _copy_insert_equal(*f);
    }
    
 
    hashtable( uints n, const HASHFUNC& hf, const EQFUNC& eqf, const GETKEYFUNC& gkf, uint reserve=64 )
    :
        _HASHFUNC(hf),
        _EQFUNC(eqf),
        _GETKEYFUNC(gkf)
    {
        _nelem = 0;
        _table.need_newc(reserve);
        _ALLOC.reserve(reserve);
    }
/*
    hashtable( uints n, const HASHFUNC& hf, const EQFUNC& eqf )
    :
        _HASHFUNC(hf),
        _EQFUNC(eqf),
        _GETKEYFUNC( _Select_Itself<VAL>() )
    {
        _table.need_newc(64);
    }
*/
    hashtable( const hashtable& ht )
    :
        _HASHFUNC( ht._HASHFUNC ),
        _EQFUNC( ht._EQFUNC ),
        _GETKEYFUNC( ht._GETKEYFUNC )
    {
        copy_from(ht);
    }

    _Self& operator= (const _Self& ht)
    {
        clear();
        copy_from(ht);
        return *this;
    }

    ~hashtable() {
        clear();
    }

private:
    void copy_from( const _Self& ht )
    {
        uints n = ht._table.size();
        _table.need_newc(n);
        _ALLOC.reserve(n);

        for( uints h=0; h<n; ++h )
        {
            Node** pn = &_table[h];
            const Node* cn = ht._table[h];
            while(cn)
            {
                Node* n = new(_ALLOC.alloc_uninit()) Node(*cn);
                *pn = n;
                pn = &n->_next;
                cn = cn->_next;
            }
            *pn = 0;
        }

        _nelem = ht._nelem;
    }

protected:

    void adjust( uint n ) {
        resize(_nelem + n);
    }

    Node** _insert_unique_slot( const LOOKUP& k )
    {
        Node** ppn = find_socket(k);
        if( *ppn == 0  ||  !_EQFUNC( _GETKEYFUNC((*ppn)->_val), k ) )
        {
            Node* n = _ALLOC.alloc();
            n->_next = *ppn;
            *ppn = n;

            ++_nelem;
            return ppn;
        }

        return 0;
    }

    Node** _insert_unique_slot_uninit( const LOOKUP& k )
    {
        Node** ppn = find_socket(k);
        if( *ppn == 0  ||  !_EQFUNC( _GETKEYFUNC((*ppn)->_val), k ) )
        {
            Node* n = _ALLOC.alloc_uninit();
            n->_next = *ppn;
            *ppn = n;

            ++_nelem;
            return ppn;
        }

        return 0;
    }

    Node** _find_or_insert_slot( const LOOKUP& k, bool* isnew )
    {
        Node** ppn = find_socket(k);
        bool isnew_ = *ppn == 0  ||  !_EQFUNC( _GETKEYFUNC((*ppn)->_val), k );
        if(isnew_)
        {
            Node* n = _ALLOC.alloc();
            n->_next = *ppn;
            *ppn = n;

            ++_nelem;
        }

        if(isnew) *isnew = isnew_;

        return ppn;
    }

    Node** _find_or_insert_slot_uninit( const LOOKUP& k, bool* isnew )
    {
        Node** ppn = find_socket(k);
        bool isnew_ = *ppn == 0  ||  !_EQFUNC( _GETKEYFUNC((*ppn)->_val), k );
        if(isnew_)
        {
            Node* n = _ALLOC.alloc_uninit();
            n->_next = *ppn;
            *ppn = n;

            ++_nelem;
        }

        if(isnew) *isnew = isnew_;

        return ppn;
    }

    Node** _insert_equal_slot( const LOOKUP& k )
    {
        Node** ppn = find_socket(k);

        Node* n = _ALLOC.alloc();
        n->_next = *ppn;
        *ppn = n;

        ++_nelem;
        return ppn;
    }

protected:

    bool __erase_value( const LOOKUP& k, VAL* dst )
    {
        Node** pn = find_socket(k);
        if(!*pn)
            return false;

        Node* n = *pn;
        *pn = n->_next;
        if(dst)
            std::swap(*dst, n->_val);

        _ALLOC.free(n);
        --_nelem;

        return true;
    }

    Node** __insert_unique( VAL&& v )
    {
        typename GETKEYFUNC::ret_type k = _GETKEYFUNC(v);
        Node** ppn = find_socket(k);
        if( *ppn == 0  ||  !_EQFUNC( _GETKEYFUNC((*ppn)->_val), k ) )
        {
            Node* n = _ALLOC.alloc();
            n->_next = *ppn;
            n->_val = std::move(v);

            *ppn = n;
            ++_nelem;
            return ppn;
        }

        return 0;
    }

    Node** __insert_unique( const VAL& v )
    {
        typename GETKEYFUNC::ret_type k = _GETKEYFUNC(v);
        Node** ppn = find_socket(k);
        if( *ppn == 0  ||  !_EQFUNC( _GETKEYFUNC((*ppn)->_val), k ) )
        {
            Node* n = _ALLOC.alloc();
            n->_next = *ppn;
            n->_val = v;

            *ppn = n;
            ++_nelem;
            return ppn;
        }

        return 0;
    }

    Node** __insert_unique__replace( VAL&& v )
    {
        typename GETKEYFUNC::ret_type k = _GETKEYFUNC(v);
        Node* n;
        Node** ppn = find_socket(k);
        if( *ppn != 0  &&  _EQFUNC( _GETKEYFUNC((*ppn)->_val), k ) ) {
            n = *ppn;
        }
        else {
            n = _ALLOC.alloc();
            n->_next = *ppn;
            *ppn = n;
            ++_nelem;
        }

        n->_val = std::move(v);

        return ppn;
    }

    Node** __insert_unique__replace( const VAL& v )
    {
        typename GETKEYFUNC::ret_type k = _GETKEYFUNC(v);
        Node* n;
        Node** ppn = find_socket(k);
        if( *ppn != 0  &&  _EQFUNC( _GETKEYFUNC((*ppn)->_val), k ) ) {
            n = *ppn;
        }
        else {
            n = _ALLOC.alloc();
            n->_next = *ppn;
            *ppn = n;
            ++_nelem;
        }

        n->_val = v;

        return ppn;
    }

    Node** __insert_equal( VAL&& v )
    {
        typename GETKEYFUNC::ret_type k = _GETKEYFUNC(v);
        Node** ppn = find_socket(k);

        Node* n = _ALLOC.alloc();
        n->_next = *ppn;
        n->_val = std::move(v);

        *ppn = n;

        ++_nelem;
        return ppn;
    }

    Node** __insert_equal( const VAL& v )
    {
        typename GETKEYFUNC::ret_type k = _GETKEYFUNC(v);
        Node** ppn = find_socket(k);

        Node* n = _ALLOC.alloc();
        n->_next = *ppn;
        n->_val = v;

        *ppn = n;

        ++_nelem;
        return ppn;
    }
};


COID_NAMESPACE_END

#endif //__COID_COMM_HASHTABLE__HEADER_FILE__
