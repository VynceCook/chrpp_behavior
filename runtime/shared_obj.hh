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

#ifndef RUNTIME_SHARED_OBJ_HH_
#define RUNTIME_SHARED_OBJ_HH_

#include <utility>
#include <memory>
#include <cassert>

// Forward declaration
namespace tests {
	namespace bt_list {
		void backtrack_tests();
		void backtrack_persistent_tests();
		void replace_backtrack_tests();
	}
	namespace bt_interval {
		void backtrack_tests();
	}
}

namespace chr {

	/**
	 * Template structure used to get at compil time the Allocator_t type
	 * defined in a class T. The general case is std::allocator< T >.
	 */
	template <typename T, typename U = int>
	struct Get_allocator_t
	{
		typedef std::allocator< T > type;
	};

	/**
	 * Specialization for U = int.
	 * decltype((void) T::Allocator_t, 0) is decltype(0) (i.e. int) if T::Allocator_t
	 * exists. Otherwise, the type is ill formed and the specialization doesn't exist.
	 */
	template < typename T >
	struct Get_allocator_t < T, decltype((void) T::Allocator_t, 0) >
	{
		typedef typename T::Allocator_t type;
	};

	// Forward declarations
	template < typename T >	class Shared_obj;
	template < typename T >	class Weak_obj;
	template < typename T >	class Shared_x_obj;
	template< typename T, typename TupIndexes, bool ENABLE_BACKTRACK > class Constraint_store_index;
	template < typename T >	class Backtrack_t;
	template < typename T > class Logical_var_imp;

	/**
	 * Build a Shared_obj which refers a new created shared T object.
	 * It allocates memory and calls the constructor of the class T.
	 * @param args The parameter to give to the constructor of T class
	 * @return A Shared_obj for the newly shared object
	 */
	template< class T, class... Args >
	inline chr::Shared_obj<T> make_shared(Args&&... args)
	{
		typename Get_allocator_t<T>::type a;
		T* o = a.allocate(1);
		::new (static_cast<void*>(o)) T(std::forward<Args>(args)...);
		o->_ref_use_count = 1;
		o->_ref_weak_count = 0;
		return chr::Shared_obj<T>(true,o);
	}

	/**
	 * @brief Shared_obj class manages a reference on a shared object.
	 *
	 * The class manages shared objects. A shared objet is shared by many Shared_obj
	 * elements. When the last Shared_obj, the shared object is destructed and deallocated
	 * if there is no more weak reference that points at it.
	 */
	template < typename T >
	class Shared_obj
	{
		friend void tests::bt_list::backtrack_tests();
		friend void tests::bt_list::backtrack_persistent_tests();
		friend void tests::bt_list::replace_backtrack_tests();
		friend void tests::bt_interval::backtrack_tests();
		template< typename U, class... Args  > friend Shared_obj<U> chr::make_shared(Args&&...);
		friend class Weak_obj<T>;
	public:
		using type = T;	///< Shortcut to type T

		/**
		 * Construct an empty Shared_obj
		 */
		Shared_obj() : _ptr(nullptr) { }

		/**
		 * Construct a Shared_obj from an existing allocated object.
		 */
		Shared_obj(T* ptr) : _ptr(ptr)
		{
			assert(ptr != nullptr);
			++_ptr->_ref_use_count;
		}

		/**
		 * Construct Shared_obj from a Weak_obj. They both
		 * refer at the same element.
		 */
		Shared_obj(const Weak_obj<T>& o) : _ptr(o._ptr) { assert(!o.expired()); ++_ptr->_ref_use_count; }

		/**
		 * Copy constructor. A new reference is created.
		 * @param o The shared object to use
		 */
		Shared_obj(const Shared_obj& o) : _ptr(o._ptr) { if (_ptr != nullptr) ++_ptr->_ref_use_count; }

		/**
		 * Move constructor.
		 * @param o The shared object to use
		 */
		Shared_obj(Shared_obj&& o) : _ptr(o._ptr) { o._ptr = nullptr; }

		/**
		 * Overload of assignment operator. A new reference is created.
		 * @param o The shared object to use
		 * @return A new Shared_obj which shares the object as \a o
		 */
		Shared_obj< T >& operator=(const Shared_obj< T >& o)
		{
			internal_release();
			_ptr = o._ptr;
			if (_ptr != nullptr) ++_ptr->_ref_use_count;
			return *this;
		}

		/**
		 * Overload of assignment operator. No new reference is created.
		 * @param o The shared object to use
		 * @return A new Shared_obj which owns \a o
		 */
		Shared_obj< T >& operator=(Shared_obj< T >&& o)
		{
			internal_release();
			_ptr = o._ptr;
			o._ptr = nullptr;
			return *this;
		}

		/**
		 * Overload of assignment operator for pointer type.
		 * It is used to create a Shared_obj from a *naked* shared object.
		 * A new reference is created.
		 * @param o The shared object to use
		 * @return A new Shared_obj which shares the object as \a o
		 */
		Shared_obj< T >& operator=(T* ptr)
		{
			internal_release();
			_ptr = ptr;
			if (_ptr != nullptr) ++_ptr->_ref_use_count;
			return *this;
		}

		/**
		 * Destructor.
		 */
		~Shared_obj() { internal_release(); _ptr = nullptr; }

		/**
		 * Release ownership of object and free ressources if needed.
		 */
		void release() { internal_release(); _ptr = nullptr; }

		/**
		 * Overload of bool operator.
		 * @return True if Shared_obj refers to a real one, false otherwise
		 */
		operator bool() const noexcept { return _ptr != nullptr; }

		/**
		 * Overload of operator*().
		 * @return A reference to the underlying shared object
		 */
		T& operator*() { return *_ptr; }

		/**
		 * Overload of operator*() const.
		 * @return A reference to the underlying shared object
		 */
		const T& operator*() const { return *_ptr; }

		/**
		 * Overload of operator->().
		 * @return A pointer to the underlying shared object
		 */
		T* operator->() { return _ptr; }

		/**
		 * Overload of operator->() const.
		 * @return A pointer to the underlying shared object
		 */
		const T* operator->() const { return _ptr; }

		/**
		 * Function used to get the address of the underlying shared object.
		 * @return A pointer to the underlying shared object
		 */
		T* get() { return _ptr; }

		/**
		 * Function used to get the address of the underlying shared object.
		 * @return A const pointer to the underlying shared object
		 */
		const T* get() const { return _ptr; }

	private:
		T* _ptr;	///< The reference to the shared object

		/**
		 * Overload of constructor for pointer type.
		 * It is used wrap *naked* shared object to a Shared_obj.
		 * No new reference is created!!
		 * Only used by chr::make_shared()
		 * @param _ Dummy parameter
		 * @param o The shared object to use
		 */
		explicit Shared_obj(bool, T* ptr) : _ptr(ptr) { }

		/**
		 * Release a Shared_obj. If there is no more reference
		 * on the object shared object, it is destroyed.
		 * @param o The shared object to use
		 */
		void internal_release()
		{
			if (_ptr == nullptr) return;
			assert(_ptr->_ref_use_count > 0);
			if (_ptr->_ref_use_count == 1)
			{
				_ptr->~T(); // Call destructor
				_ptr->_ref_use_count = 0;
				if (_ptr->_ref_weak_count == 0)
				{
					typename Get_allocator_t<T>::type a;
					a.deallocate(_ptr,1);
				}
			} else {
				--_ptr->_ref_use_count;
			}
		}

	};

	/**
	 * @brief Weak_obj class manages a reference on a *weak* shared object.
	 *
	 * The class manages shared objects. A shared objet is shared by many Shared_obj
	 * elements. When the last Shared_obj, the shared object is destructed and deallocated
	 * if there is no more weak reference that points at it. A Weak_obj doesn't increase
	 * the number of shared references on the shared object.
	 */
	template < typename T >
	class Weak_obj
	{
		template< typename U, typename TupIndexes, bool ENABLE_BACKTRACK > friend class Constraint_store_index;
		template < typename U > friend class Backtrack_t;
		template < typename U > friend class Logical_var_imp;
		friend class Shared_obj<T>;
	public:
		using type = T;	///< Shortcut to type T

		/**
		 * Build a Weak_obj object from an existing Shared_obj.
		 * @param o The Shared_obj object to use
		 * @return A Weak_obj that refer to the same shared object.
		 */
		Weak_obj(const Shared_obj<T>& o) : _ptr(o._ptr)
		{
			assert(o._ptr != nullptr);
			assert(o->_ref_use_count > 0);
			++_ptr->_ref_weak_count;
		}

		/**
		 * Copy constructor. A new *weak* reference is created.
		 * @param o The Weak_obj to use
		 */
		Weak_obj(const Weak_obj& o) : _ptr(o._ptr) { assert(o._ptr != nullptr); ++_ptr->_ref_weak_count; }

		/**
		 * Move constructor.
		 * @param o The Weak_obj to use
		 */
		Weak_obj(Weak_obj&& o) : _ptr(o._ptr) { o._ptr = nullptr; }

		/**
		 * Overload of assignment operator. A new *weak* reference is created.
		 * @param o The *weak* shared object to use
		 * @return A new Weak_obj which shares the object as \a o
		 */
		Weak_obj< T >& operator=(const Weak_obj< T >& o)
		{
			assert(o._ptr != nullptr);
			internal_release();
			_ptr = o._ptr;
			++_ptr->_ref_weak_count;
			return *this;
		}

		/**
		 * Overload of assignment operator. No new reference is created.
		 * @param o The weak object to use
		 * @return A new Weak_obj which owns \a o
		 */
		Weak_obj< T >& operator=(Weak_obj< T >&& o)
		{
			assert(o._ptr != nullptr);
			internal_release();
			_ptr = o._ptr;
			o._ptr = nullptr;
			return *this;
		}

		/**
		 * Overload of constructor for pointer type.
		 * It is used to create a Weak_obj from a *naked* shared object.
		 * A new *weak* reference is created.
		 * @param o The shared object to use
		 */
		explicit Weak_obj(T* ptr) : _ptr(ptr)
		{
			assert(ptr != nullptr);
			assert(ptr->_ref_use_count > 0);
			++_ptr->_ref_weak_count;
		}

		/**
		 * Destructor.
		 */
		~Weak_obj() { internal_release(); _ptr = nullptr ; }

		/**
		 * Release ownership of object and free ressources if needed.
		 */
		void release() { internal_release(); _ptr = nullptr; }

		/**
		 * Overload of bool operator.
		 * @return True if Weak_obj refers to a real one, false otherwise
		 */
		operator bool() const noexcept { return _ptr != nullptr; }

		/**
		 * Overload of operator*().
		 * @return A reference to the underlying shared object
		 */
		T& operator*() { return *_ptr; }

		/**
		 * Overload of operator*() const.
		 * @return A reference to the underlying shared object
		 */
		const T& operator*() const { return *_ptr; }

		/**
		 * Overload of operator->().
		 * @return A pointer to the underlying shared object
		 */
		T* operator->() { return _ptr; }

		/**
		 * Overload of operator->() const.
		 * @return A pointer to the underlying shared object
		 */
		const T* operator->() const { return _ptr; }

		/**
		 * Function used to get the address of the underlying shared object.
		 * @return A pointer to the underlying shared object
		 */
		T* get() { return _ptr; }

		/**
		 * Function used to get the address of the underlying shared object.
		 * @return A const pointer to the underlying shared object
		 */
		const T* get() const { return _ptr; }

		/**
		 * Check if the shared object is still available.
		 * @return False if the underlying object is still reachable true otherwise
		 */
		bool expired() const
		{
			assert(_ptr != nullptr);
			return (_ptr->_ref_use_count == 0);
		}

	private:
		T* _ptr;	///< The reference to the shared object

		/**
		 * Release the Weak_obj to a shared T object. If there is no more reference
		 * on the shared object, it is destroyed.
		 * @param o The shared object to use
		 */
		void internal_release()
		{
			if (_ptr == nullptr) return;
			assert(_ptr->_ref_weak_count > 0);
			if (--_ptr->_ref_weak_count == 0)
			{
				if (_ptr->_ref_use_count == 0)
				{
					typename Get_allocator_t<T>::type a;
					a.deallocate(_ptr,1);
				}
			}
		}
	};

	/**
	 * @brief Shared_x_obj class manages a reference on a shared exclusive object.
	 *
	 * The class manages shared exclusive objects. A shared exclusive objet is shared by many Shared_x_obj
	 * elements. When the last Shared_obj, the shared object is destructed and deallocated
	 * Weak references are not possible with a Shared_obj_x.
	 * Compare to a Shared_obj, it saves the space of a counter (the counter of weak references).
	 */
	template < typename T >
	class Shared_x_obj
	{
	public:
		using type = T;	///< Shortcut to type T

		/**
		 * Construct a Shared_x_obj from an existing allocated object.
		 */
		Shared_x_obj(T* ptr) : _ptr(ptr)
		{
			assert(ptr != nullptr);
			++_ptr->_ref_use_count;
		}

		/**
		 * Copy constructor. A new reference is created.
		 * @param o The shared exclusive object to use
		 */
		Shared_x_obj(const Shared_x_obj& o) : _ptr(o._ptr) { if (_ptr != nullptr) ++_ptr->_ref_use_count; }

		/**
		 * Move constructor.
		 * @param o The shared object to use
		 */
		Shared_x_obj(Shared_x_obj&& o) : _ptr(o._ptr) { o._ptr = nullptr; }

		/**
		 * Overload of assignment operator. A new reference is created.
		 * @param o The shared exclusive object to use
		 * @return A new Shared_x_obj which shares the object as \a o
		 */
		Shared_x_obj< T >& operator=(const Shared_x_obj< T >& o)
		{
			internal_release();
			_ptr = o._ptr;
			if (_ptr != nullptr) ++_ptr->_ref_use_count;
			return *this;
		}

		/**
		 * Overload of assignment operator. No new reference is created.
		 * @param o The shared exclusive object to use
		 * @return A new Shared_x_obj which owns \a o
		 */
		Shared_x_obj< T >& operator=(Shared_x_obj< T >&& o)
		{
			internal_release();
			_ptr = o._ptr;
			o._ptr = nullptr;
			return *this;
		}

		/**
		 * Destructor.
		 */
		~Shared_x_obj() { internal_release(); _ptr = nullptr; }

		/**
		 * Release ownership of object and free ressources if needed.
		 */
		void release() { internal_release(); _ptr = nullptr; }

		/**
		 * Overload of bool operator.
		 * @return True if Shared_x_obj refers to a real one, false otherwise
		 */
		operator bool() const noexcept { return _ptr != nullptr; }

		/**
		 * Function used to get the address of the underlying shared exclusive object.
		 * @return A pointer to the underlying shared exclusive object
		 */
		T* get() { return _ptr; }

		/**
		 * Function used to get the address of the underlying shared exclusive object.
		 * @return A const pointer to the underlying shared exclusive object
		 */
		const T* get() const { return _ptr; }

		/**
		 * Overload of operator*().
		 * @return A reference to the underlying shared exclusive object
		 */
		T& operator*() { return *_ptr; }

		/**
		 * Overload of operator*() const.
		 * @return A reference to the underlying shared exclusive object
		 */
		const T& operator*() const { return *_ptr; }

		/**
		 * Overload of operator->().
		 * @return A pointer to the underlying shared exclusive object
		 */
		T* operator->() { return _ptr; }

		/**
		 * Overload of operator->() const.
		 * @return A pointer to the underlying shared exclusive object
		 */
		const T* operator->() const { return _ptr; }

	private:
		T* _ptr;	///< The reference to the shared exclusive object

		/**
		 * Release a Shared_x_obj. If there is no more reference
		 * on the object shared object, it is destroyed.
		 * @param o The shared object to use
		 */
		void internal_release()
		{
			if (_ptr == nullptr) return;
			assert(_ptr->_ref_use_count > 0);
			if (_ptr->_ref_use_count == 1)
			{
				_ptr->~T(); // Call destructor
				_ptr->_ref_use_count = 0;
				typename Get_allocator_t<T>::type a;
				a.deallocate(_ptr,1);
			} else {
				--_ptr->_ref_use_count;
			}
		}
	};
}
#endif /* RUNTIME_SHARED_OBJ_HH_ */
