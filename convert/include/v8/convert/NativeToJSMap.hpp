#if ! defined(V8_CONVERT_NATIVE_JS_MAPPER_HPP_INCLUDED)
#define V8_CONVERT_NATIVE_JS_MAPPER_HPP_INCLUDED

#include "detail/convert_core.hpp"
namespace v8 { namespace convert {
    /**
       A helper class to assist in the "two-way-binding" of
       natives to JS objects. This class holds native-to-JS
       binding information.
       
       In the general case, a native-to-JS conversion is only
       needed at the framework-level if bound/converted
       functions/methods will _return_ bound native
       pointers/references. If they only return "core" types (numbers
       and strings, basically), or explicitly return v8-supported
       types (e.g. v8::Handle<v8::Value>) then no native-to-JS
       conversion is typically needed.
    */
    template <typename T>
    struct NativeToJSMap
    {
    private:
        typedef TypeInfo<T> TI;
        typedef typename TI::Type Type;
        /**
           The native type to bind to.
        */
        typedef typename TI::NativeHandle NativeHandle;
        /** The type for holding the JS 'this' object. */
        typedef v8::Persistent<v8::Object> JSObjHandle;
        //typedef v8::Handle<v8::Object> JSObjHandle; // Hmmm.
        typedef std::pair<NativeHandle,JSObjHandle> ObjBindT;
        typedef std::map<void const *, ObjBindT> OneOfUsT;
        /** Maps (void const *) to ObjBindT. */
        static OneOfUsT & Map()
        {
            static OneOfUsT bob;
            return bob;
        }
    public:
        /** Maps obj as a lookup key for jself. Returns false if !obj,
         else true. */
        static bool Insert( JSObjHandle const & jself,
                            NativeHandle obj )
        {
            return obj
                ? (Map().insert( std::make_pair( obj, std::make_pair( obj, jself ) ) ),true)
                : 0;
        }

        /**
           Removes any mapping of the given key. Returns the
           mapped native, or 0 if none is found.
        */
        static NativeHandle Remove( void const * key )
        {
            typedef typename OneOfUsT::iterator Iterator;
            OneOfUsT & map( Map() );
            Iterator it = map.find( key );
            if( map.end() == it )
            {
                return 0;
            }
            else
            {
                NativeHandle victim = (*it).second.first;
                map.erase(it);
                return victim;
            }
        }

        /**
           Returns the native associated (via Insert())
           with key, or 0 if none is found.
        */
        static NativeHandle GetNative( void const * key )
        {
            if( ! key ) return 0;
            else
            {
                typename OneOfUsT::iterator it = Map().find(key);
                return (Map().end() == it)
                    ? 0
                    : (*it).second.first;
            }
        }

        /**
           Returns the JS object associated with key, or
           an empty handle if !key or no object is found.
        */
        static v8::Handle<v8::Object> GetJSObject( void const * key )
        {
            if( ! key ) return Handle<Object>();
            typename OneOfUsT::const_iterator it = Map().find(key);
            if( Map().end() == it ) return v8::Handle<v8::Object>();
            else return (*it).second.second;
        }
        
        /**
            A base NativeToJS<T> implementation for classes which use NativeToJSMap<T>
            to hold their native-to-JS bindings. To be used like this:
            
            @code
            // must be in the v8::convert namespace!
            template <>
            struct NativeToJS<MyType> : NativeToJSMap<MyType>::NativeToJSImpl {};
            @endcode
        */
        struct NativeToJSImpl
        {
            v8::Handle<v8::Value> operator()( T const * n ) const
            {
                typedef NativeToJSMap<T> BM;
                v8::Handle<v8::Value> const & rc( BM::GetJSObject(n) );
                if( rc.IsEmpty() ) return v8::Null();
                else return rc;
            }
            v8::Handle<v8::Value> operator()( T const & n ) const
            {
                return this->operator()( &n );
            }
        };
    };

}} // namespaces

#endif /* include guard */
