#if !defined(CODE_GOOGLE_COM_P_V8_V8_WEAKCLASSGENERATOR_H_INCLUDED)
#define CODE_GOOGLE_COM_P_V8_V8_WEAKCLASSGENERATOR_H_INCLUDED 1
/*
   Author: Stephan Beal (http://wanderinghorse.net/home/stephan/)

   License: same as v8 (see below)
*/
// Copyright 2009 Stephan Beal. All rights reserved.  Redistribution
// and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <v8.h>

#include <v8/juice/JSClassCreator.h>
#include <string>
#include <stdexcept>
#include <map>
namespace v8 {
namespace juice {

#if !defined(DOXYGEN)
    namespace Detail
    {
	/**
	   An internal helper type for only use by WeakJSClassCreator.

	   Actual must be WeakJSClassCreator::WrappedType.
	*/
	template <typename Actual>
	struct WrapperTypeChecker
	{
	    typedef Actual WrappedType;
	    typedef std::pair<WrappedType *,Persistent<Object> > ObjBindT;
	    typedef std::map<void const *,ObjBindT > OneOfUsT;
	    static OneOfUsT & Map()
	    {
		static OneOfUsT bob;
		return bob;
	    }
	};

    }
#endif // DOXYGEN
    /**
       Any type used with WeakJSClassCreator must provide a
       specialization of this type and provide all of the functions,
       typedefs, and constants described herein.
    */
    template <typename T>
    struct WeakJSClassCreatorOps
    {
	/**
	   This type's associated native type. In specializations,
	   T *may* differ from WrappedType, and WrappedType
	   is the definition that WeakJSClassCreator will use
	   as a parameter type. This has some odd uses, actually,
	   such as matching multiple logical JS types to a single
	   native data type.
	*/
	typedef T WrappedType;

	enum {
	/**
	   Must be defined to the number of internal fields
	   which client code will need for each instance
	   of the wrapped object.

	   WeakJSClassCreator will automatically allocate one
	   additional field and store the underlying native object in
	   the *last* internal field (so clients can work with fields
	   0..N as normal, without having to take too much care to not
	   step on this class' slot).
	*/
	ExtraInternalFieldCount = 0
	};

	/**
	   This will be called in response to calling
	   "new WrappedType(...)" in JS code. It should:

	   - Create a new native object.

	   - If successful, return that object.

	   - On error, return 0 and optionally populate the (string &)
	   argument with an informative error message (e.g. "requires
	   arguments (String,Integer)").

	   If the client is using any "supplemental" garbage collection
	   (e.g. the v8::juice::cleanup API) then they should
	   register the object in this function, and use Dtor()
	   as the registered destruction callback.
	*/
	static WrappedType * Ctor( Arguments const &  /*argv*/,
				   std::string & /*exceptionText*/);

	/**
	   Must "destroy" the given object, though the exact meaning
	   of "destroy", and mechanism of destruction, is
	   type-specific.

	   This function is is responsible for destroying the native
	   object. Most implementations must simply call "delete obj",
	   which is fine for many types, but opaque types and types
	   allocated via something other than 'new' will need a
	   different implementation. A destructor functor for shared,
	   static, or stack-allocated objects should simply do nothing
	   (or maybe clear the object to some empty/default state).

	   If the client is using any "supplemental" garbage collection
	   (e.g. the v8::juice::cleanup API) then they should
	   deregister the object in this function.
	*/
	static void Dtor( WrappedType * obj );

	/**
	   Must return the JS-side class name of this type. It must be
	   a legal JS function name.

	   Design note: this is a static function instead of a static
	   string because this requires much less code from
	   implementors (whereas a string string requires out-of-class
	   initialization).
	*/
	static char const * ClassName();
    };

    /** Required specialization for certain template type passing to work as expected. */
    template <typename T>
    struct WeakJSClassCreatorOps<T const> : WeakJSClassCreatorOps<T> {};
    /** Required specialization for certain template type passing to work as expected. */
    template <typename T>
    struct WeakJSClassCreatorOps<T *> : WeakJSClassCreatorOps<T> {};
    /** Required specialization for certain template type passing to work as expected. */
    template <typename T>
    struct WeakJSClassCreatorOps<T const *> : WeakJSClassCreatorOps<T> {};


    /**
       WeakOpsNoop is a helper for use in the early stages of
       implementing WeakJSClassCreatorOps specializations. The idea is
       that one's own WeakJSClassCreatorOps specialization inherits
       this type to get dummy/empty functionality, so it'll at least
       it'll compile before one has to implement the Ctor() and Dtor()
       members. Once the custom implementation has all of the required
       members, there is no need to inherit this class (unless one only
       wants to inherit the WrappedType typedef).

       Because there is no sensible default value for a class' name,
       the ClassName() member must be implemented even when subclassing
       this type.
    */
    template <typename T, int FieldCount = 0>
    struct WeakOpsNoop
    {
	/** See WeakJSClassCreatorOps::WrappedType. */
	typedef T WrappedType;
	/** See WeakJSClassCreatorOps::ExtraInternalFieldCount. */
	enum { ExtraInternalFieldCount = FieldCount };
	/** Triggers a "not yet implemented" exception. */
	static WrappedType * Ctor( Arguments const &  /*argv*/,
				   std::string & exceptionText)
	{
	    exceptionText = "WeakOpsNoop<T>::Ctor() not implemented!";
	    return 0;
	}
	/** Warning - Does Nothing! Reimplement this in subclasses! */
	static void Dtor( WrappedType * obj )
	{
	    return 0;
	}
    };

    /**
       WeakJSClassCreator is a tool to simplify the creation of new
       wrapped classes, including addition of automatic cleanup handling
       via weak-pointer callbacks.

       It is used almost exactly like JSClassGenerator, but adds some
       features (and requirements) for handling JS weak pointers to
       arbitrary types. It combines three forms of deletion for bound
       objects:

       - Via v8's weak pointers, the native will be deleted *if* (BIG
       *if*) the gc is triggered and the pointer is due for deletion.
       An option is provided to support external garbage collection,
       in case calling dtors is an absolute must for your types.

       - If the client calls the static DestroyObject() function of
       this class. This also disposes the underlying weak pointers, so
       they won't point back to the object anymore. This allows us to
       implement member function which disconnect from the underlying native
       object (e.g. File.close()).

       The template argument, WrappedT, is the native type we will
       wrap, non-CVP qualified. It is actually legal to use a type other
       than the real native we will wrap, as long as the WeakJSClassCreatorOps
       specialization (see below) provides us with the real type.

       However, the client must also provide a specialization of WeakJSClassCreatorOps
       for the given type. That specialization must conform to the API defined
       in the base template definition (which is not implemented and cannot be
       instantiated).

       The WeakJSClassCreatorOps specialization is a helper which
       defines the core-most operations needed by this class. See the
       docs for WeakJSClassCreatorOps for how types consistent with
       its interface should behave.

       The wrapper works basically like this:

       When, in JS code, "new MyType(...)" is called, this call arranges
       to have a private constructor called. That contructor does the following:

       - Passes the Arguments array to ClassOpsType::Ctor(). If the
       ctor returns 0, the construction fails and throws and
       exception. If the exceptionText string (2nd parameter to
       ClassOpsType::Ctor()) is non-empty when the ctor returns, the
       private constructor will destroy the returned object (if
       necessary) using ClassOpsType::Dtor() and throw a JS exception
       using the exceptionText string.

       - The argv.This() argument passed to ClassOpsType::Ctor() is
       the "this" object of the "real" constructor.

       - If ClassOpsType::Ctor() succeeds (returns non-0) then the new
       object is bound to a weak pointer and gives the new weak
       pointer to v8. It will also add the native as an External in
       the Nth internal slot, where N is the
       WeakJSClassCreatorOps<WrappedT>::ExtraInternalFieldCount. This
       class reserves one extra slot at the end of the list for storing
       the native, so there will always be at least one slot.

       It's a lot simpler than it sounds and completely frees the caller of
       worry of setting up weak pointer callback handling.

       Example usage:

       First we need a custom WeakJSClassCreatorOps specialization. Here's
       a minimal (but functional) implementation:

       \code
       struct MyTypeOps
       {
           typedef MyClass WrappedType;
	   enum { ExtraInternalFieldCount = 0 };
           static MyType * Ctor( Arguments const & argv, std::string & exceptionText)
	   {
	       return new MyType;
	   }
	   static void Dtor( MyType * obj )
	   {
	       if(obj) delete obj;
	   }
	   static char const * ClassName()
	   {
	       return "MyClass";
	   }
       };
       namespace v8 { namespace juice {
           // Required specialization of WeakJSClassCreatorOps:
           template <>
           WeakJSClassCreatorOps<MyType> : MyTypeOps {};
	   // We could have done it without subclassing, but this is copy/paste code
	   // from a working implementation ;)
       }}
       \endcode

       Now we can use the class creator:

       \code
       ...
       // Add the class to JS:
       typedef WeakJSClassCreator<MyType> WC;
       WC c( "MyType", objectToAddClassTo );
       c.Set(...)
         .Set(...)
	 .Set(...)
	 .Set(...)
	 .Seal(); // must be the last setter call made on this object.
       \endcode

       That's all there is to it. More functionality is provided via
       the JSClassCreator base class, and there are several static
       member functions for "casting" between the JS/Native worlds
       and for destroying instances of the generated class.
    */
    template <typename WrappedT>
    class WeakJSClassCreator : public JSClassCreator//, public WeakJSClassCreatorOps<WrappedT>
    {
    public:
	/**
	   The WeakJSClassCreatorOps specialization used by this
	   class.
	*/
	typedef WeakJSClassCreatorOps<WrappedT> ClassOpsType;

	/**
	   The actual native type wrapped by this class, which may
	   be different from the WrappedT template parameter!
	*/
	typedef typename ClassOpsType::WrappedType WrappedType;

    private:
	enum Internal {
	/** The internal field number where we store the wrapped object. */
	NativeFieldIndex = ClassOpsType::ExtraInternalFieldCount,
	/** The number of internal fields. */
	FieldCount = NativeFieldIndex + 1
	};
    public:

	/**
	   Starts the setup of a new class. It will be populated into
	   the target object when Seal() is called.  See the
	   JSClassCreator ctor for more information on that process.
	*/
	explicit WeakJSClassCreator( Handle<Object> target)
	    : JSClassCreator( ClassOpsType::ClassName(),
			      target,
			      ctor_proxy,
			      static_cast<int>(FieldCount) )
	{
	}

	/**
	   Identical to the 1-arg ctor except that it does not take a
	   target object. The implications of this are:

	   - The Target() member will return an empty handle.
	   - The caller must add this class to his chosen target manually,
	   as demonstrated here:

	   \code
	   target->Set( String::New("ClassName"), myCreator.Seal() );
	   \endcode

	   That will "close off" the class creation process and add the new class
	   to the target object.
	*/
	WeakJSClassCreator()
	    : JSClassCreator( ClassOpsType::ClassName(),
			      ctor_proxy,
			      static_cast<int>(FieldCount) )
	{
	}

	/**
	   Returns the bound native for the given handle. The handle
	   is assumed to have been created using this class'
	   mechanisms. If it was not, this routine will return 0.

	   Ownership of the returned object remains unchanged.

	   Results are technically undefined if the given handle has
	   not created by this class, but this function makes every
	   effort to ensure that it doesn't do anything too stupid.
	*/
	static WrappedType * GetSelf( Local<Object> h )
	{
	    if( h.IsEmpty() || (h->InternalFieldCount() != (FieldCount)) ) return 0;
	    Local<Value> lv( h->GetInternalField(NativeFieldIndex) );
	    if( lv.IsEmpty() || !lv->IsExternal() ) return 0;
	    Local<External> ex( External::Cast(*lv) );
	    TypeCheckIter it = typeCheck().find( ex->Value() );
	    return ( typeCheck().end() == it )
		? 0
		: (*it).second.first;
	}

#if 1
	/** Reimplemented to DO NOTHING, as the number is defined
	    by the WeakJSClassCreatorOps specialization. When changing
	    it here, we lose the ability to know where the object
	    is in the list (since we store it at the end).
	*/
 	virtual WeakJSClassCreator & SetInternalFieldCount( int n )
 	{
	    //JSClassCreator::SetInternalFieldCount(n+1);
	    // gcc 4.3.2 won't allow this syntax: :-?
 	    //this->JSClassCreator::SetInteralFieldCount( n + 1 );
	    return *this;
 	}
#endif

	/**
	   Like GetSelf(), but takes a Handle to a value. This can be
	   used for checking/converting arguments other than the
	   current This object (e.g., maybe the second argument to a
	   certain WrappedType member function is a different
	   WrappedType object).

	   This can be used in place of GetSelf(), but it does more
	   work than GetSelf() has to.
	*/
        static WrappedType * GetNative( Handle<Value> h )
        {
            if( h.IsEmpty() || ! h->IsObject() ) return 0;
            Local<Object> obj( Object::Cast(*h) );
	    return GetSelf( obj );
        }

	/**
	   If jo "seems" to have been created via this class' mechanism,
	   then the behaviour is as-if the weak pointer callback has been
	   triggered. That will delete the bound native and dispose/clear the
	   persistent handle.

	   The reason this function exists is so that native implementations
	   of member functions can implement functions which logically destroy
	   the JS object. For example:

	   \code
	   var os = new OutStream(...);
	   ...
	   os.close();
	   \endcode

	   That code "should" destroy the underlying device and
	   disconnect the JS reference to that object, and it can use
	   this function to do so.

	   Returns true if it passes a native object to the
	   destructor, else false.
	*/
	static bool DestroyObject( Local<Object> jo )
	{
	    WrappedType * t = GetSelf(jo); // sanity check
	    if( ! t ) return false;
	    Persistent<Object> p = Persistent<Object>::New( jo ); // is this correct? Seems to work.
	    weak_callback( p, t );
	    return true;
	}

	/**
	   Like the other DestroyObject() overload, but takes a handle
	   to a generic value. If h is not an Object reference, false
	   is returned, otherwise the result of calling the other overload
	   is returned.
	*/
        static bool DestroyObject( Handle<Value> h )
        {
            return ( h.IsEmpty() || ! h->IsObject() )
		? false
		: DestroyObject( Local<Object>(Object::Cast(*h)) );
        }

	/**
	   If obj was created via these bindings then this function
	   returns obj's JS object, otherwise it returns a
	   default-constructed (empty) handle.
	*/
	static Handle<Object> GetJSObject( WrappedType * obj )
	{
	    TypeCheckIter it = typeCheck().find(obj);
	    if( typeCheck().end() == it ) return Handle<Object>();
	    else return (*it).second.second;
	}

        // Without this, gcc isn't seeing the two-arg overload!
        using JSClassCreator::NewInstance;

	/**
	   Similar to the two-argument form of NewInstance(), but on
	   success it assigns obj to the new native object.
	*/
	Local<Object> NewInstance( int argc, Handle<Value> argv[], WrappedType * & obj )
	{
	    Local<Object> ji = JSClassCreator::NewInstance( argc, argv );
	    if( ! ji.IsEmpty() && ji->IsObject() )
	    {
		obj = GetNative( ji );
	    }
	    return ji;
	}


    private:
	typedef Detail::WrapperTypeChecker<WrappedType> TypeCheck;
	typedef typename TypeCheck::ObjBindT ObjBindT;
	typedef typename TypeCheck::OneOfUsT OneOfUsT;
	typedef typename OneOfUsT::iterator TypeCheckIter;
	static OneOfUsT & typeCheck()
	{
	    // We must use a map instance defined OUTSIDE of this class
	    // for this binding to work properly. More correctly,
	    // the instance of the map must depend only on WrappedType.
	    return TypeCheck::Map();
	}

	/**
	   Unbinds native and destroys it using CleanupFunctor.

	   Must only be called from weak_callback().
	*/
	static void the_cleaner( WrappedType * native )
	{
	    if( native )
	    {
		ClassOpsType::Dtor( native );
	    }
	}

	/**
	   Only called by v8 GC. It removes the object from "supplemental" GC,
	   calls the_cleaner(nobj), and Dipose()es the given object.
	*/
	static void weak_callback(Persistent< Value > pv, void * nobj)
	{
	    //CERR << "weak callback @"<<nobj<<"\n";
	    Local<Object> jobj( Object::Cast(*pv) );

	    if( jobj->InternalFieldCount() != (FieldCount) ) return; // and warn on this, too!
	    Local<Value> lv( jobj->GetInternalField(NativeFieldIndex) );
	    if( lv.IsEmpty() || !lv->IsExternal() ) return; // and this!
	    /**
	       We have to ensure that we have no dangling External in JS space. This
	       is so that functions like IODevice.close() can act safely with the
	       knowledge that member funcs called after that won't get a dangling
	       pointer. Without this, some code will crash in that case.
	    */
	    jobj->SetInternalField(NativeFieldIndex,Null());
	    pv.Dispose();
	    pv.Clear();
#if 0
	    // Not safe (can crash) b/c this might get called after v8 context is gone:
	    WrappedType * native = GetNative(pv);
	    if( ! native ) return; // we need a way to warn if this happens!
	    //the_cleaner( static_cast<WrappedType*>( nobj ) );
	    the_cleaner( native );//static_cast<WrappedType*>( nobj ) );
#endif
#if 0 // this works okay but i hate casts
	    the_cleaner( static_cast<WrappedType*>( nobj ) );
#else
	    TypeCheckIter it = typeCheck().find( nobj );
	    if( typeCheck().end() == it ) // serious error
	    {
		return;
	    }
	    the_cleaner( (*it).second.first );
#endif
	}

	/**
	   "Wraps" a native pointer in a JS weak pointer. Returns the new
	   Persistent weak object, which is a wrapper for the self object.

	   Results are undefined if this function is called from anywhere
	   other than ctor_proxy().
	*/
	static Persistent<Object> wrap_native( Handle<Object> _self, WrappedType * obj )
	{
	    Persistent<Object> self( Persistent<Object>::New(_self) );
	    self.MakeWeak( obj, weak_callback );
	    self->SetInternalField( NativeFieldIndex, External::New(obj) );
	    typeCheck().insert( std::make_pair( obj, std::make_pair( obj, self ) ) );
	    return self;
	}

	/**
	   Constructs a WrappedType by calling NativeConstructor. If it
	   succeeds, it returns wrap_native(), otherwise it throws.

	   Returns argv.This() unless it throws.
	 */
	static Handle<Value> ctor_proxy( const Arguments & argv )
	{
	    if (!argv.IsConstructCall()) 
	    {
		return ThrowException(String::New("Cannot call this constructor as function!"));
	    }
	    std::string err;
	    WrappedType * obj = 0;
	    try
	    {
		obj = ClassOpsType::Ctor( argv, err );
	    }
	    catch(std::exception const & ex)
	    {
		return ThrowException(String::New(ex.what()));
	    }
	    catch(...)
	    {
		return ThrowException(String::New("Native constructor threw an unknown native exception!"));
	    }
	    if( ! err.empty() )
	    {
		if( obj )
		{
		    ClassOpsType::Dtor(obj);
		    obj = 0;
		}
		return ThrowException(String::New(err.c_str(),static_cast<int>(err.size())));
	    }
	    if( ! obj )
	    {
		return ThrowException(String::New("Constructor failed for an unspecified reason!"));
	    }
	    return wrap_native( argv.This(), obj );
	}

    };

}} /* namespaces */

#endif /* CODE_GOOGLE_COM_P_V8_V8_WEAKCLASSGENERATOR_H_INCLUDED */
