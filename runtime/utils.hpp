/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the Apache License, Version 2.0.
 *
 *  Copyright:
 *     2016, Vincent Barichard <Vincent.Barichard@univ-angers.fr>
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 */

#ifndef RUNTIME_UTILS_HH_
#define RUNTIME_UTILS_HH_

#include <utility>
#include <list>
#include <set>
#include <sstream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include <tuple>

#include <shared_obj.hh>

#define XXH_INLINE_ALL
// The XXH_FORCE_MEMORY_ACCESS may be set to 2 to get
// better performances on some achitecture (0 is the default value)
#define XXH_FORCE_MEMORY_ACCESS 2
//#define XXH_FORCE_MEMORY_ACCESS 0
#include <third_party/xxhash.h>

namespace chr {

	/**
	 * Template structure used to detect at compil time if the class T
	 * defines a member named BACKTRACK_SELF_MANAGED. The general case is false.
	 */
	template <typename T, typename U = int>
	struct Backtrack_management
	{
		static constexpr bool self_managed = false;
	};

	/**
	 * decltype((void) T::BACKTRACK_SELF_MANAGED, 0) is decltype(0) (i.e. int) if T::BACKTRACK_SELF_MANAGED
	 * exists. Otherwise, the type is ill formed and the specialization doesn't exist.
	 */
	template < typename T >
	struct Backtrack_management < T, decltype((void) T::BACKTRACK_SELF_MANAGED, 0) >
	{
		static constexpr bool self_managed = T::BACKTRACK_SELF_MANAGED;
	};

	/**
	 * Template structure used to detect at compil time if the class T
	 * defines a member named NEED_DESTROY. The general case is true.
	 */
	template <typename T, typename U = int>
	struct Has_need_destroy
	{
		static constexpr bool value = true;
	};

	/**
	 * decltype((void) T::NEED_DESTROY, 0) is decltype(0) (i.e. int) if T::NEED_DESTROY
	 * exists. Otherwise, the type is ill formed and the specialization doesn't exist.
	 */
	template < typename T >
	struct Has_need_destroy < T, decltype((void) T::NEED_DESTROY, 0) >
	{
		static constexpr bool value = T::NEED_DESTROY;
	};

	/**
	 * Specialization for type tupe< ... >.
	 */
	template < typename... T >
	struct Has_need_destroy< std::tuple< T... > >
	{
		/**
		 * Local recursive templated class to iterate over all elements of
		 * the tuple.
		 */
		template < typename TT0, typename... TTs >
		struct _need_destroy
		{
			static constexpr bool value =
				(!std::is_scalar< TT0 >::value && Has_need_destroy< TT0 >::value)
				|| _need_destroy< TTs... >::value;
		};
	
		/**
		 * Local recursive specialized templated class to stop the recursive
		 * loop over all elements of tuple.
		 */
		template < typename TT >
		struct _need_destroy<TT>
		{
			static constexpr bool value =
				!std::is_scalar< TT >::value && Has_need_destroy< TT >::value;
		};

		static constexpr bool value = _need_destroy< T... >::value;
	};

	/**
	 * @brief Macros for XXHASH algorithm
	 */ 
	typedef XXH32_hash_t CHR_XXHASH_hash_t;
	#define CHR_XXHash(X) XXH32(&X, sizeof(X), 0x23C6EF37UL)

	typedef XXH32_state_t CHR_XXHash_state_t;
	#define CHR_XXHash_createState() XXH32_createState()
	#define CHR_XXHash_freeState(STATE) XXH32_freeState(STATE)

	/**
	 * @brief CHR_XXHash state management
	 *
	 * Class to manage the XXHash state variable used to compute streamed hash.
	 * In order to no reallocate/deallocate this variable, we globalize it in the
	 * following class.
	 */
	template < typename Dummy >
	class XXHash_state_t
	{
	public:
		/**
		 * Initialize: disabled.
		 */
		XXHash_state_t() =delete;

		/**
		 * Copy constructor: disabled.
		 */
		XXHash_state_t(const XXHash_state_t&) =delete;

		/**
		 * Assignment operator: disabled.
		 */
		XXHash_state_t& operator=(const XXHash_state_t&) =delete;

		/**
		 * @brief The XXHash_state_allocator struct
		 *
		 * Used to automatically create and free ste XXHash state static variable.
		 */
		struct XXHash_state_allocator {
			CHR_XXHash_state_t* _state;

			XXHash_state_allocator() : _state(CHR_XXHash_createState()) { }
			~XXHash_state_allocator() { CHR_XXHash_freeState(_state); }
		};
		static XXHash_state_allocator const _alloc; ///< State for XXHash
	};

	// Initialization of static members
	template< typename T >
	typename XXHash_state_t<T>::XXHash_state_allocator const chr::XXHash_state_t<T>::_alloc;

	#define CHR_XXHash_update(X,SIZE_X) XXH32_update(chr::XXHash_state_t<void>::_alloc._state, X, SIZE_X)
	#define CHR_XXHash_reset() XXH32_reset(chr::XXHash_state_t<void>::_alloc._state, 0x23C6EF37UL)
	#define CHR_XXHash_digest() XXH32_digest(chr::XXHash_state_t<void>::_alloc._state)

	/**
	 * @brief XXHash structure computes xxhash for a given type
	 *
	 * Hash tables require to compute a hash value for any used key.
	 * The XXHash class computes a hash value given the xxhash algorithm
	 * for a given element.
	 * The class must be specialized for any type used in hash tables. 
	 */
	template< typename T > struct XXHash;

	/**
	 * Structure used to detect at compil time if the chr::CHR_XXHash template class
	 * has been specialized for the type "T" (more often a user class).
	 * Indeed, many internal CHRPP data structures use hash.
	 */
	template < typename T >
	struct Has_hash
	{
		static constexpr bool value = std::is_default_constructible< chr::XXHash< T > >::value;
	};

	/**
	 * Explicit specialization for any scalar type
	 */
	#define _CHRXXHASH_DEFINE_TRIVIAL_HASH_(_Tp) \
		template< > \
		struct XXHash< _Tp > \
		{ \
			static void update(_Tp x)\
			{ \
				CHR_XXHash_update(&x, sizeof(x)); \
			} \
		};

	/// Explicit specialization for bool.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(bool)

	/// Explicit specialization for char.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(char)

	/// Explicit specialization for signed char.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(signed char)

	/// Explicit specialization for unsigned char.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(unsigned char)

	/// Explicit specialization for wchar_t.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(wchar_t)

	/// Explicit specialization for char16_t.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(char16_t)

	/// Explicit specialization for char32_t.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(char32_t)

	/// Explicit specialization for short.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(short)

	/// Explicit specialization for int.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(int)

	/// Explicit specialization for long.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(long)

	/// Explicit specialization for long long.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(long long)

	/// Explicit specialization for unsigned short.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(unsigned short)

	/// Explicit specialization for unsigned int.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(unsigned int)

	/// Explicit specialization for unsigned long.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(unsigned long)

	/// Explicit specialization for unsigned long long.
	_CHRXXHASH_DEFINE_TRIVIAL_HASH_(unsigned long long)

	/**
	 * Explicit specialization for pointer.
	 */
	template< typename T >
	struct XXHash< T* >
	{
		static void update(const T* x)
		{
			CHR_XXHash_update(x, sizeof(x)); \
		}
	};

	/**
	 * Explicit specialization for std::pair.
	 */
	template< typename U, typename V >
	struct XXHash< std::pair< U, V > >
	{
		static void update(const std::pair< U, V >& x)
		{
			XXHash<U>::update(x.first);
			XXHash<V>::update(x.second);
		}
	};

	/**
	 * Explicit specialization for std::string.
	 */
	template< >
	struct XXHash< std::string >
	{
		static void update(const std::string& s)
		{
			for (auto&& c : s)
				XXHash<char>::update(c);
		}
	};

	/**
	 * Explicit specialization for std::vector.
	 */
	template< typename T >
	struct XXHash< std::vector< T > >
	{
		static void update(const std::vector< T >& x)
		{
			for (auto&& e : x)
				XXHash<T>::update(e);
		}
	};

	/**
	 * Explicit specialization for std::list.
	 */
	template< typename T >
	struct XXHash< std::list< T > >
	{
		static void update(const std::list< T >& x)
		{
			for (auto&& e : x)
				XXHash<T>::update(e);
		}
	};

	/**
	 * Explicit specialization for std::set.
	 */
	template< typename T >
	struct XXHash< std::set< T > >
	{
		static void update(const std::set< T >& x)
		{
			for (auto&& e : x)
				XXHash<T>::update(e);
		}
	};

	/**
	 * Explicit specialization for std::unordered_set.
	 */
	template< typename T >
	struct XXHash< std::unordered_set< T > >
	{
		static void update(const std::unordered_set< T >& x)
		{
			for (auto&& e : x)
				XXHash<T>::update(e);
		}
	};

	template < typename T > struct Grounded_key_t; // Forward declaration
	/**
	 * Explicit XXHash specialization for chr::Grounded_key_t
	 */
	template< typename T >
	struct XXHash< chr::Grounded_key_t< T > >
	{
		static void update(const chr::Grounded_key_t< T >& x)
		{
			XXHash<T>::update(x._value);
		}
	};

	template < typename T > struct Mutable_key_t; // Forward declaration
	/**
	 * Explicit XXHash specialization for chr::Mutable_key_t
	 */
	template< typename T >
	struct XXHash< chr::Mutable_key_t< T > >
	{
		static void update(const chr::Mutable_key_t< T >& x)
		{
			XXHash<void *>::update(x._address);
		}
	};

	template < typename T > struct Complex_key_t; // Forward declaration
	/**
	 * Explicit XXHash specialization for chr::Complex_key_t
	 */
	template< typename T >
	struct XXHash< chr::Complex_key_t< T > >
	{
		static void update(const chr::Complex_key_t< T >& x)
		{
			if (x._address == nullptr)
				XXHash<T>::update(x._value);
			else
				XXHash<void *>::update(x._address);
		}
	};

	template < typename T > class Logical_var_ground; // Forward declaration
	/**
	 * Explicit XXHash specialization for chr::Logical_var_ground.
	 */
	template< typename T >
	struct XXHash< chr::Logical_var_ground< T > >
	{
		static void update(const chr::Logical_var_ground< T >& x)
		{
			XXHash< chr::Complex_key_t< T > >::update(*x);
		}
	};

	template < typename T > class Logical_var_mutable; // Forward declaration
	/**
	 * Explicit XXHash specialization for chr::Logical_var_mutable.
	 */
	template< typename T >
	struct XXHash< chr::Logical_var_mutable< T > >
	{
		static void update(const chr::Logical_var_mutable< T >& x)
		{
			XXHash< chr::Complex_key_t< T > >::update(x.address());
		}
	};

	template < typename T > class Logical_var; // Forward declaration
	/**
	 * Explicit XXHash specialization for chr::Logical_var.
	 */
	template< typename T >
	struct XXHash< chr::Logical_var< T > >
	{
		static void update(const chr::Logical_var< T >& x)
		{
			if (x.ground())
				XXHash< chr::Complex_key_t< T > >::update(*x);
			else
				XXHash< chr::Complex_key_t< T > >::update(x.address());
		}
	};

	/**
	 * @brief Type wrapper
	 *
	 * The Type_wrapper class wrap a type or class to Type_wrapper< T >.
	 * It is usefull to encapsulate other type in order to rename it.
	 * This new type name can be filtered in traits class of CHRPP engine.
	 * A classical use may be to define a container of Logical_var which
	 * behaves differently than the usual way.
	 */
	template < typename T >
	class Type_wrapper {
	public:
        typedef T Type;	///< Type of encapsulated element

		/// \name Construct
		//@{
		/**
		 * Initialize.
		 */
		Type_wrapper() {}

		/**
		 * Construct from element.
		 * @param elt Initial value
		 */
        Type_wrapper(Type elt) : _elt(std::move(elt)) { }

		/**
		 * Construct from initialization list.
		 * @param l Initialization list to take values from
		 */
        Type_wrapper(std::initializer_list< typename Type::value_type > l) : _elt(l) { }

		/**
		 * Move constructor.
		 */
        Type_wrapper(Type&&) = delete;

		/**
		 * Copy constructor.
		 */
        Type_wrapper(const Type_wrapper< Type >&) = default;
		//@}

		/// \name Modifiers
		//@{
		/**
		 * Assignement operator overload.
		 */
        Type_wrapper& operator=(const Type_wrapper< Type >&) = default;

		/**
		 * Assignement operator from a element with same type as encapsulated one.
		 * @param e Value
		 */
        Type_wrapper& operator=(const Type& e) { _elt = e; return *this; }
		//@}

		/// \name Test
		//@{
		/**
		 * Check if two element are equal
		 * @param o The other Type_wrapper element
		 * @return True if the two encapsulated elements are equal to \a o, false otherwise
		 */
        bool operator==(const Type_wrapper< Type >& o) const { return (_elt == o._elt); }

		/**
		 * Check if two element are not equal
		 * @param o The other Type_wrapper element
		 * @return True if the two encapsulated elements are not equal to \a o, false otherwise
		 */
        bool operator!=(const Type_wrapper< Type >& o) const { return (_elt != o._elt); }
		//@}

		/// \name Access
		//@{
		/**
		 * Cast operator overload.
		 * @return A reference to this
		 */
        operator Type& () noexcept { return _elt; }

		/**
		 * Const cast operator overload.
		 * @return A constant reference to this
		 */
        operator const Type& () const noexcept { return _elt; }

		/**
		 * Get function which returns a constant reference to this.
		 * @return A constant reference to this
		 */
        const Type& get() const noexcept { return _elt; }

		/**
		 * Get function which returns a reference to this.
		 * @return A reference to this
		 */
        Type& get() noexcept { return _elt; }
		//@}

	private:
        Type _elt;	///< The encapsulated element
	};

    /**
     * @brief Type wrapper
     *
     * Partial specialization for std::pair<U, V> which cannot be instanced
     * from a std::initializer_list.
     */
    template < typename U, typename V >
    class Type_wrapper< std::pair< U, V > > {
    public:
        typedef std::pair< U, V > Type;	///< Type of encapsulated element

        /// \name Construct
        //@{
        /**
         * Initialize.
         */
        Type_wrapper() {}

        /**
         * Construct from element.
         * @param elt Initial value
         */
        Type_wrapper(Type elt) : _elt(std::move(elt)) { }

        /**
         * Construct from a tuple.
         * @param u First element of pair
         * @param v Second element of pair
         */
        Type_wrapper(U u, V v) : _elt(std::make_pair(std::move(u),std::move(v))) { }

        /**
         * Move constructor.
         */
        Type_wrapper(Type&&) = delete;

        /**
         * Copy constructor.
         */
        Type_wrapper(const Type_wrapper< Type >&) = default;
        //@}

        /// \name Modifiers
        //@{
        /**
         * Assignement operator overload.
         */
        Type_wrapper& operator=(const Type_wrapper< Type >&) = default;

        /**
         * Assignement operator from a element with same type as encapsulated one.
         * @param e Value
         */
        Type_wrapper& operator=(const Type& e) { _elt = e; return *this; }
        //@}

        /// \name Test
        //@{
        /**
         * Check if two element are equal
         * @param o The other Type_wrapper element
         * @return True if the two encapsulated elements are equal to \a o, false otherwise
         */
        bool operator==(const Type_wrapper< Type >& o) const { return (_elt == o._elt); }

        /**
         * Check if two element are not equal
         * @param o The other Type_wrapper element
         * @return True if the two encapsulated elements are not equal to \a o, false otherwise
         */
        bool operator!=(const Type_wrapper< Type >& o) const { return (_elt != o._elt); }
        //@}

        /// \name Access
        //@{
        /**
         * Cast operator overload.
         * @return A reference to this
         */
        operator Type& () noexcept { return _elt; }

        /**
         * Const cast operator overload.
         * @return A constant reference to this
         */
        operator const Type& () const noexcept { return _elt; }

        /**
         * Get function which returns a constant reference to this.
         * @return A constant reference to this
         */
        const Type& get() const noexcept { return _elt; }

        /**
         * Get function which returns a reference to this.
         * @return A reference to this
         */
        Type& get() noexcept { return _elt; }
        //@}

    private:
        Type _elt;	///< The encapsulated element
    };

    /**
     * List of Logical variables. All elements of the pair
     * are scheduled so if any of them is updated, the involved constraint
     * will be awaken
     */
    template< typename U, typename V > using Logical_pair =
        chr::Type_wrapper< std::pair< U, V > >;

	/**
     * Logical vector of elements. All elements of the vector
	 * are scheduled so if any of them is updated, the involved constraint
	 * will be awaken
	 */
	template< typename T > using Logical_vector =
        chr::Type_wrapper< std::vector< T > >;

	/**
     * Logical list of elements. All elements of the list
	 * are scheduled so if any of them is updated, the involved constraint
	 * will be awaken
	 */
	template< typename T > using Logical_list =
        chr::Type_wrapper< std::list< T > >;
}

/**
 * \defgroup TIW Type Instruction Wrapper
 * 
 * The TIW functions and classes gather all components used to map C++ types
 * to standard strings.
 */

namespace chr
{
	/**
	 * @brief Type Instruction Wrapper
	 * 
	 * The TIW namespace gathers all classes and functions needed to convert an object
	 * to a string.
	 */
	namespace TIW
	{
		/**
		 * Convert an element \a e to a string.
		 * It statically chooses between the specialization for
		 * fundamental types or the other one.
		 * @param e The object to convert
		 * @return A new string
		 */
		template< typename T >
		std::string to_string(const T& e);

		/**
		 * Convert an element \a e, given by a tuple to a string.
		 * It assumes that \a e is for trace purposes and it will returns
		 * a string built with the concatenation of all parts of \a e.
		 * @param c The message parts to convert
		 * @return A new string
		 */
		template< typename... T >
		std::string trace_to_string(const std::tuple< T... >& e);

		/**
		 * Convert a constraint \a c, given by a tuple to a string.
		 * It assumes that tuple describes a constraint and that its first element
		 * it the constraint id.
		 * @param c The constraint to convert
		 * @return A new string
		 */
		template< typename... T >
		std::string constraint_to_string(const std::tuple< T... >& c);
	}

	/**
	 * Class wrapper to apply same actions on fundamental types
	 * and non fundamental (class and struct instances).
	 * \ingroup TIW
	 */
	template< typename T, bool PRIMITIVE >
	struct Type_instruction_wrapper
	{ };

	/**
	 * Specialization for fundamental types
	 * \ingroup TIW
	 */
	template< typename T >
	struct Type_instruction_wrapper< T, true >
	{
		/**
		 * Convert a fundamental type element \a e to a string.
		 * @param e The element to convert
		 * @return A new string
		 */
		static std::string to_string(T e) { return std::to_string(e); }
	};

	/**
	 * Specialization for fundamental type char
	 * \ingroup TIW
	 */
	template< >
	struct Type_instruction_wrapper< char, true >
	{
		/**
		 * Convert a fundamental type element \a e to a string.
		 * @param e The element to convert
		 * @return A new string
		 */
		static std::string to_string(char e) { return std::string(1,e); }
	};

	/**
	 * Specialization for non fundamental types (class and struct instances)
	 * \ingroup TIW
	 */
	template< typename T>
	struct Type_instruction_wrapper< T, false >
	{
		/**
		 * Template structure used to detect at compil time if the class U
		 * defines a member function named to_string(). The general case is true.
		 */
		template < typename U, typename V = int >
		struct Has_to_string
		{
			static constexpr bool value = false;
		};

		/**
		 * decltype((void) std::declval<U>().to_string(), 0) is decltype(0) if T defines a to_string() member
		 * exists. Otherwise, the type is ill formed and the specialization doesn't exist.
		 */
		template < typename U >
		struct Has_to_string < U, decltype((void) std::declval<U>().to_string(),0) >
		{
			static constexpr bool value = true;
		};

		/**
		 * Convert a non fundamental type object \a e to a string.
		 * by calling its .to_string() member function
		 * @param e The object to convert
		 * @return A new string
		 */
		static std::string to_string(const T& e) {
			if constexpr (Has_to_string<T>::value)
				return e.to_string();
			else
				return e;
		}
	};

	/**
	 * Specialization for non fundamental Type_wrapper type
	 * \ingroup TIW
	 */
	template< typename T >
	struct Type_instruction_wrapper< Type_wrapper< T >, false >
	{
		/**
		 * Convert a Type_wrapper object \a o to a string.
		 * @param o The object to convert
		 * @return A new string
		 */
		static std::string to_string(const Type_wrapper< T >& o) {
			return TIW::to_string( o.get() );
		}
	};

	/**
	 * Specialization for fundamental type const void*
	 * \ingroup TIW
	 */
	template< >
	struct Type_instruction_wrapper< const void *, false >
	{
		/**
		 * Convert an address \a a to a string.
		 * @param a The address to convert
		 * @return A new string
		 */
		static std::string to_string(const void* a) {
			std::ostringstream address;
			address << a;
			return address.str();
		}
	};

	/**
	 * Specialization for fundamental type void*
	 * \ingroup TIW
	 */
	template< >
	struct Type_instruction_wrapper< void *, false >
	{
		/**
		 * Convert an address \a a to a string.
		 * @param a The address to convert
		 * @return A new string
		 */
		static std::string to_string(void* a) {
			std::ostringstream address;
			address << a;
			return address.str();
		}
	};

	/**
	 * Specialization for non fundamental type std::shared_ptr<T>
	 * \ingroup TIW
	 */
	template< typename T>
	struct Type_instruction_wrapper< std::unique_ptr< T >, false >
	{
		/**
		 * Convert a std::unique_ptr<> object \a l to a string.
		 * @param p The unique_ptr<> to convert
		 * @return A new string
		 */
		static std::string to_string(const std::unique_ptr< T >& p) {
			return chr::TIW::to_string(*p);
		}
	};

	/**
	 * Specialization for non fundamental type std::shared_ptr<T>
	 * \ingroup TIW
	 */
	template< typename T>
	struct Type_instruction_wrapper< std::shared_ptr< T >, false >
	{
		/**
		 * Convert a std::shared_ptr<> object \a l to a string.
		 * @param p The shared_ptr<> to convert
		 * @return A new string
		 */
		static std::string to_string(const std::shared_ptr< T >& p) {
			return chr::TIW::to_string(*p);
		}
	};

	/**
	 * Specialization for non fundamental type chr::Weak_obj<T>
	 * \ingroup TIW
	 */
	template< typename T>
	struct Type_instruction_wrapper< chr::Weak_obj< T >, false >
	{
		/**
		 * Convert a chr::Weak_obj<> object \a l to a string.
		 * @param p The Weak_obj<> to convert
		 * @return A new string
		 */
		static std::string to_string(const chr::Weak_obj< T >& p) {
			return chr::TIW::to_string(*p);
		}
	};

	/**
	 * Specialization for non fundamental type chr::Shared_obj<T>
	 * \ingroup TIW
	 */
	template< typename T>
	struct Type_instruction_wrapper< chr::Shared_obj< T >, false >
	{
		/**
		 * Convert a chr::Shared_obj<> object \a l to a string.
		 * @param p The Shared_obj<> to convert
		 * @return A new string
		 */
		static std::string to_string(const chr::Shared_obj< T >& p) {
			return chr::TIW::to_string(*p);
		}
	};

	/**
	 * Specialization for non fundamental type chr::Shared_x_obj<T>
	 * \ingroup TIW
	 */
	template< typename T>
	struct Type_instruction_wrapper< chr::Shared_x_obj< T >, false >
	{
		/**
		 * Convert a chr::Shared_x_obj<> object \a l to a string.
		 * @param p The Shared_x_obj<> to convert
		 * @return A new string
		 */
		static std::string to_string(const chr::Shared_x_obj< T >& p) {
			return chr::TIW::to_string(*p);
		}
	};

	/**
	 * Specialization for non fundamental type std::string
	 * \ingroup TIW
	 */
	template< >
	struct Type_instruction_wrapper< std::string, false >
	{
		/**
		 * Convert a std::string object \a s to a string.
		 * @param s The string to convert
		 * @return A new string
		 */
		static std::string to_string(const std::string& s) {
			return s;
		}
	};

	/**
	 * Specialization for non fundamental type std::tuple< T... >
	 * \ingroup TIW
	 */
	template< typename... T >
	struct Type_instruction_wrapper< std::tuple< T... >, false >
	{
		/**
		 * Wrapper to decide if the element of a tuple (corresponding to a trace message)
		 * must be write as a constraint or as a default type.
		 * Default case, we convert as usual.
		 * @param tup The tuple to convert
		 */
		template< typename ArgType >
		static std::string local_to_string(const ArgType& tup)
		{
			return TIW::to_string(tup);
		}

		/**
		 * Wrapper to decide if the element of a tuple (corresponding to a trace message)
		 * must be write as a constraint or as a default type.
		 * For this case, we assume that any nested tuple will be a constraint.
		 * @param tup The tuple to convert
		 */
		template< typename... TT >
		static std::string local_to_string(const std::tuple< TT...>& tup)
		{
			return TIW::constraint_to_string(tup);
		}

		/**
		 * Iterate over all elements of the tuple (assuming that \a tup is composed
		 * of parts of the trace message)
		 * @param tup The tuple to convert
		 * @param I Index sequence of the recursive loop
		 * @return A new string
		 */
		template< typename TupType, size_t... I >
		static std::string trace_loop_to_string(const TupType& tup, std::index_sequence<I...>)
		{
			std::string str;
			((str += local_to_string( std::get<I>(tup) )), ...);
			return  str;
		}

		/**
		 * Convert a std::tuple<> \a s to a string trace message.
		 * @param tup The tuple to convert
		 * @return A new string
		 */
		static std::string trace_to_string(const std::tuple< T... >& tup) {
			return trace_loop_to_string(tup, std::make_index_sequence<sizeof...(T)>());
		}

		/**
		 * Iterate over all elements of the tuple (assuming that \a tup is a constraint definition)
		 * @param tup The tuple to convert
		 * @param I Index sequence of the recursive loop
		 * @return A new string
		 */
		template< typename TupType, size_t... I >
		static std::string loop_constraint_to_string(const TupType& tup, std::index_sequence<I...>)
		{
			std::string str;
			((str += (I == 0? "#" + TIW::to_string( std::get<I>(tup) ) + "(" :
			   (( I == 1? "" : ", ") + TIW::to_string( std::get<I>(tup) )))), ...);
			str += ")";
			return  str;
		}

		/**
		 * Convert a std::tuple<> \a s to a string. It assumes that \a tup is a constraint
		 * and that first tuple element is its id.
		 * @param tup The tuple to convert
		 * @return A new string
		 */
		static std::string constraint_to_string(const std::tuple< T... >& tup) {
			return loop_constraint_to_string(tup, std::make_index_sequence<sizeof...(T)>());
		}

		/**
		 * Iterate over all elements of the tuple
		 * @param tup The tuple to convert
		 * @param I Index sequence of the recursive loop
		 * @return A new string
		 */
		template< typename TupType, size_t... I >
		static std::string loop_to_string(const TupType& tup, std::index_sequence<I...>)
		{
			std::string str;
			str += "(";
			((str += (I == 0? "" : ", ") + TIW::to_string( std::get<I>(tup) )), ...);
			str += ")";
			return  str;
		}

		/**
		 * Convert a std::tuple<> \a s to a string.
		 * @param tup The tuple to convert
		 * @return A new string
		 */
		static std::string to_string(const std::tuple< T... >& tup) {
			return loop_to_string(tup, std::make_index_sequence<sizeof...(T)>());
		}
	};

	/**
	 * Specialization for non fundamental type std::pair<U,V>
	 * \ingroup TIW
	 */
	template< typename U, typename V >
	struct Type_instruction_wrapper< std::pair< U, V >, false >
	{
		/**
		 * Convert a std::pair<> \a s to a string.
		 * @param s The pair to convert
		 * @return A new string
		 */
		static std::string to_string(const std::pair< U, V >& s) {
			std::string str = "< ";
			str += chr::TIW::to_string(s.first) + ", " + chr::TIW::to_string(s.second);
			str += " >";
			return str;
		}
	};

	/**
	 * Specialization for non fundamental type std::list<T>
	 * \ingroup TIW
	 */
	template< typename T>
	struct Type_instruction_wrapper< std::list< T >, false >
	{
		/**
		 * Convert a std::list object \a l to a string.
		 * @param l The list to convert
		 * @return A new string
		 */
		static std::string to_string(const std::list< T >& l) {
			std::string str = "[ ";
			for (auto& e : l)
				str += chr::TIW::to_string(e) + ", ";
			if (!l.empty())
				str.resize(str.length()-2);
			else
				str.resize(str.length()-1);
			str += " ]";
			return str;
		}
	};

	/**
	 * Specialization for non fundamental type std::set<T>
	 * \ingroup TIW
	 */
	template< typename T>
	struct Type_instruction_wrapper< std::set< T >, false >
	{
		/**
		 * Convert a std::set object \a s to a string.
		 * @param s The set to convert
		 * @return A new string
		 */
		static std::string to_string(const std::set< T >& s) {
			std::string str = "{ ";
			for (auto& e : s)
				str += chr::TIW::to_string(e) + ", ";
			if (!s.empty())
				str.resize(str.length()-2);
			else
				str.resize(str.length()-1);
			str += " }";
			return str;
		}
	};

	/**
	 * Specialization for non fundamental type std::unordered_set<T>
	 * \ingroup TIW
	 */
	template< typename T>
	struct Type_instruction_wrapper< std::unordered_set< T >, false >
	{
		/**
		 * Convert a std::unordered_set object \a s to a string.
		 * @param s The set to convert
		 * @return A new string
		 */
		static std::string to_string(const std::unordered_set< T >& s) {
			std::string str = "{ ";
			for (auto& e : s)
				str += chr::TIW::to_string(e) + ", ";
			if (!s.empty())
				str.resize(str.length()-2);
			else
				str.resize(str.length()-1);
			str += " }";
			return str;
		}
	};

	/**
	 * Specialization for non fundamental type std::vector<T>
	 * \ingroup TIW
	 */
	template< typename T>
	struct Type_instruction_wrapper< std::vector< T >, false >
	{
		/**
		 * Convert a std::vector object \a a to a string.
		 * @param a The vector to convert
		 * @return A new string
		 */
		static std::string to_string(const std::vector< T >& a) {
			std::string str = "[ ";
			for (auto& e : a)
				str += chr::TIW::to_string(e) + ", ";
			if (!a.empty())
				str.resize(str.length()-2);
			else
				str.resize(str.length()-1);
			str += " ]";
			return str;
		}
	};

	namespace TIW
	{
		template< typename T >
		std::string to_string(const T& e)
		{
			return (Type_instruction_wrapper< T, std::is_fundamental<T>::value >::to_string(e));
		}

		template< typename... T >
		std::string trace_to_string(const std::tuple<T...>& e)
		{
			return (Type_instruction_wrapper< std::tuple<T...>, std::is_fundamental< std::tuple<T...> >::value >::trace_to_string(e));
		}

		template< typename... T >
		std::string constraint_to_string(const std::tuple<T...>& e)
		{
			return (Type_instruction_wrapper< std::tuple<T...>, std::is_fundamental< std::tuple<T...> >::value >::constraint_to_string(e));
		}
	}
}
#endif  /* RUNTIME_UTILS_HH_ */
