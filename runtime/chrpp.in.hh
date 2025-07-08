#line 2 "@CURRENT_PROCESSED_FILE_NAME@"
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

#ifndef RUNTIME_CHRPP_HH_
#define RUNTIME_CHRPP_HH_

#if defined(__CHRPPC_MAJOR__) && \
	(__CHRPPC_MAJOR__ != @PROJECT_VERSION_MAJOR@ || \
		(__CHRPPC_MAJOR__ == @PROJECT_VERSION_MAJOR@ && \
		__CHRPPC_MINOR__ != @PROJECT_VERSION_MINOR@ ) )
#error chrppc version and runtime version mismatch
#endif

#include <cassert>
#include <list>
#include <memory>
#include <unordered_map>
#include <ostream>
#include <sstream>

#include <shared_obj.hh>
#include <bt_list.hh>
#include <trace.hh>
#include <backtrack.hh>
#include <statistics.hh>
#include <utils.hpp>

/**
 * \defgroup Examples CHR examples
 * 
 * \e CHR++ provides some examples based on well-known functions and problems.
 * You can browse them to get a more finer idea of the way of making a \e CHR++
 * program.
 */

/**
 * \defgroup Logical_variables Logical variables
 * 
 * Logical variables are a key component of any \e CHR++ program. There is not
 * only one C++ class, but many ones to efficiently manage different cases.
 * For example, grounded logical variables are managed with a dedicated class.
 */

// Forward declaration
namespace tests {
	namespace constraint_store {
		void index_misc_tests();
	}
	namespace logical_var {
		void backtrack_tests();
	}
}

/**
 * @brief CHR namespace
 * 
 * The chr namespace gathers all classes and functions needed to write a \e CHR++
 * program.
 */
namespace chr {
	/**
	 * @brief EVAL templated class
	 *
	 * The EVAL templated class is used to either forward the ES_CHR
	 * value returned by the evaluated function, either return ES_CHR::SUCCESS
	 * if the evaluated function doesn't return a ES_CHR value.
	 */
	template< typename T >
	struct EVAL
	{
		// The evaluated function doesn't return a ES_CHR type value, so we return
		// ES_CHR::SUCCESS. If the evaluated function returns something, it is forgotten.
		template < typename F >
			static inline ES_CHR eval(F f) { f(); return ES_CHR::SUCCESS; }
	};

	template<>
	struct EVAL< ES_CHR >
	{
		// Forward the returned value of the evaluated function as it returns
		// a ES_CHR value.
		template < typename F >
			static inline ES_CHR eval(F f) { return f(); }
	};

	// Macro to embed a constraint built-in constraint call and forward a
	// ES_CHR::FAILURE if the underlying function returns an ES_CHR::FAILURE.
	#define CHECK_ES(X) { if (chr::EVAL< decltype(X) >::eval([&](){return X;}) == chr::ES_CHR::FAILURE) return chr::ES_CHR::FAILURE; }

	/**
	 * @brief Logical status of a variable
	 */
	enum class Status
	{
		NOT_GROUND = 0,	//!< Not Grounded variable
		GROUND,			//!< Grounded variable
		MUTABLE			//!< Mutable variable
	};

	// Forward declaration
	template< typename T > class Logical_var;
	template< typename T > class Logical_var_mutable;
	template< typename T > class Logical_var_ground;
	template< typename T > struct Grounded_key_t;
	template< typename T > struct Mutable_key_t;
	template< typename T > struct Complex_key_t;

	/**
	 * @brief Constraint call (root class)
	 *
	 * The constraint store manages constraint that
	 * are still alive. The constraint store is composed
	 * of set of Constraint_call. The Constraint_call class
	 * is the root class and must be inherited in each CHR
	 * problem class.
	 * \ingroup Constraints
	 */
	class Constraint_call
	{
	public:
		/**
		 * Initialize a constraint call.
		 */
		Constraint_call() : _id(0)
		// 0 is a reserved id which can never be used by any CHR constraint
		{ }

		/**
		 * Initialize for a constraint id.
		 * @param id The id of the constraint that will be stored
		 */
		Constraint_call(unsigned long int id) : _id(id)
		{
			// 0 is a reserved id
			assert(id != 0);
		}

		/**
		 * Return the id of the constraint.
		 * @return The id of the constraint
		 */
		unsigned long int id() const { return _id; }

		/**
		 * Return true if the variable involved in the Constraint_call is grounded.
		 * As this class must be inherited, this function is only relevant in subclasses.
		 * It is there only for compilation purposes (see unit tests)
		 * @return false
		 */
		bool ground(unsigned int) { return false; }

		static const unsigned int arity = 0;	///< Dummy static constraint (needed for compilation with Constraint_call (see unit tests))
	protected:
		unsigned long int _id;	///< The constraint id
	};

	/**
	 * @brief Logical_var_imp_observer_index (root class)
	 *
	 * The callback class is a base class which must be inherited for
	 * each index update observer of a Logical_var_imp.
	 * Callback elements are stored in wake up list in the Logical_var_imp
	 * and wake up all objects when the Logical_var_imp is updated.
	 * \ingroup Logical_variables
	 */
	class Logical_var_imp_observer_index
	{
	public:
		/**
		 * Constructor.
		 */
		Logical_var_imp_observer_index() : _ref_use_count(0) { }

		/**
		 * Virtual destructor.
		 */
		virtual ~Logical_var_imp_observer_index() {}

		/**
		 * Run the callback code when the Logical_var_imp has been updated.
		 * @param old_key The old key value (has to be an address) used before update
		 * @return False is the callback can be removed, true otherwise
		 */
		virtual bool update_index(const void* old_key) = 0;

		// Member hereafter deal with shared and weak references of a Logical_var_imp_observer_index
		unsigned int _ref_use_count;		///< Count of shared references
	};

	/**
	 * @brief Logical_var_imp_constraint callback (root class)
	 *
	 * The constraint callback class is a base class which must be
	 * inherited for each constraint observer of a Logical_var_imp.
	 * Callback elements are stored in wake up list in the Logical_var_imp
	 * and wake up all objects when the Logical_var_imp is updated.
	 * \ingroup Logical_variables
	 */
	class Logical_var_imp_observer_constraint
	{
		template < typename T > friend class Logical_var_imp;
	public:
		/**
		 * Initialize a logical var imp observer.
		 */
		Logical_var_imp_observer_constraint() : _ref_use_count(0) { }

		/**
		 * Virtual destructor.
		 */
		virtual ~Logical_var_imp_observer_constraint() {}

		/**
		 * Run the following callback when the Logical_var_imp has been updated.
		 * @return 0 if the callback can be removed from the wake up list, 1 if it succeeded and 2 if a failure has been raised
		 */
		virtual unsigned char run() = 0;

		// Member hereafter deal with shared and weak references of a Logical_var_imp_observer_constraint
		unsigned int _ref_use_count;		///< Count of shared references
	};

	/**
	 * @brief Logical variable implementation
	 *
	 * The logical variable implementation class is where the variable
	 * is physically stored. It is connected to the constraints in which
	 * the variable is involved (with wake-up queue). When the variable
	 * is updated, all constraints of wake-up queue are awaken.
	 * \ingroup Logical_variables
	 */
	template< typename T >
	class Logical_var_imp : public Backtrack_observer
	{
	public:
		friend class Logical_var<T>;
		friend class Logical_var_ground<T>;
		friend class Logical_var_mutable<T>;
		friend class Logical_var<T>;
		template< typename U > friend class Backtrack_t;
		template< typename U > friend struct Schedule_constraint_callback;
		template< typename U, class... Args  > friend Shared_obj<U> chr::make_shared(Args&&...);

		// Friend for Unit tests
		friend void tests::logical_var::backtrack_tests();
		friend void tests::constraint_store::index_misc_tests();

		/**
		 * Copy constructor: deleted.
		 * There is a private copy constructor which also modified \a o.
		 * @param o The source variable
		 */
		Logical_var_imp(const Logical_var_imp< T >& o) =delete;

		/**
		 * Destroy.
		 */
		virtual ~Logical_var_imp();

		/**
		 * Deleted copy constructor.
		 * @param o The initial value
		 */
		Logical_var_imp< T >& operator=(Logical_var_imp< T >& o) =delete;

		/// \name Access
		//@{
		/**
		 * Access to the value of the variable. If the variable is not
		 * grounded, the behavior is undefined. The returned reference is
		 * constant, it cannot be used to update the value of the variable.
		 * @return A constant reference to the value
		 */
		const T& operator*() const { assert( ra()._status != Status::NOT_GROUND ); return ra()._e; }
		//@}

		/// \name Tests
		//@{
		/**
		 * Check if the variable is grounded.
		 * @return True if it is a ground variable, false otherwise
		 */
		bool ground() const { return (ra()._status == Status::GROUND); }

		/**
		 * Check if the variable is mutable.
		 * @return True if it is a mutable variable, false otherwise
		 */
		bool is_mutable() const { return (ra()._status == Status::MUTABLE); }

		/**
		 * Returns the ground status of the variable (NOT_GROUNDED, GROUNDED or MUTABLE).
		 * Useful for internal functions not for normal user.
		 * @return The ground status (0, 1 or 2)
		 */
		Status status() const { return ra()._status; }
		//@}

		/**
		 * Return the address of the root ancestor of the variable.
		 * It may serve as a unique id for this equivalence class
		 * @return The address of the variable (the root ancestor)
		 */
		const void* address() const;

	private:
		// Logical_var_imp main members
		bool _backtrack_scheduled;																			///< Flag set to true if logical variable registered for backtrack management

		T _e;																								///< The value of the logical variable
		Status _status;																						///< To know if the variable is grounded, not grounded or mutable
		Depth_t _backtrack_depth;																			///< The depth which is relevant to this logical variable (snapshot)
		chr::Bt_list< chr::Shared_x_obj< Logical_var_imp_observer_index >, true, false, chr::Statistics::VARIABLE > _wake_up_index_updates;			///< The list of index update callbacks to wake up when the variable is updated
		chr::Bt_list< chr::Shared_x_obj< chr::Logical_var_imp_observer_constraint >, true, false, chr::Statistics::VARIABLE  > _wake_up_constraints;	///< The list of low priority callbacks (constraints wake up) to wake up when the variable is updated
		chr::Shared_obj< Logical_var_imp< T > > _ancestor;													///< Pointer to the first equivalent Logical_var_imp
		unsigned int _equiv_class_size;																		///< Size of equivalence class where it belongs

		/**
		 * @brief The Snapshot_t struct
		 *
		 * Record the members of the variable imp in order to restore them on backtrack.
		 */
		struct Snapshot_t
		{
			T _e;																								///< The value of the logical variable
			Status _status;																						///< To know if the variable is grounded, not grounded or mutable
			Depth_t _backtrack_depth;																			///< The depth which is relevant to this logical variable (snapshot)
			chr::Shared_obj< Logical_var_imp<T> > _ancestor;													///< Pointer to the first equivalent Logical_var_imp
			unsigned int _equiv_class_size;																		///< Size of equivalence class where it belongs
			std::unique_ptr< Snapshot_t > _previous_snapshot;													///< Previous (recorded) snapshot of this variable imp

			/**
			 * Create a snapshot of a Logical_var_imp
			 * @param v The logical_var_imp to use for
			 */
			Snapshot_t(Logical_var_imp& v);

			/**
			 * Create a snapshot of a Logical_var_imp but without duplicating the v._e member.
			 * Useful when _e manages itself the backtrack management and doesn't need to be
			 * cloned.
			 * @param v The logical_var_imp to use for
			 * @param dummy Dummy parameter to differentiate from the other constructor
			 */
			Snapshot_t(Logical_var_imp& v, bool dummy);

			/**
			 * Copy constructor (delete)
			 */
			Snapshot_t(const Snapshot_t&) =delete;

#if defined(ENABLE_STATISTICS) && defined(ENABLE_MEMORY_STATISTICS)
			/**
			 * Destructor
			 */
			~Snapshot_t() {	Statistics::dec_memory< Statistics::VARIABLE >(sizeof *this); }
#endif
		};

		std::unique_ptr< Snapshot_t > _previous_snapshot;														///< Previous (recorded) snapshot of this variable imp

		/// \name Construct
		//@{
		/**
		 * Initialize a logical var imp.
		 * @param status To know between Status::NOT_GROUND and Status::MUTABLE
		 */
		Logical_var_imp(Status status)
			: Backtrack_observer(), _backtrack_scheduled(false),
			  _status(status), _backtrack_depth(Backtrack::depth()), _equiv_class_size(1)
		{
			assert((status == Status::NOT_GROUND) || (status == Status::MUTABLE));
			Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
		}

		/**
		 * Initialize with value.
		 * @param e The initial value
		 * @param status To know between Status::GROUND and Status::MUTABLE
		 */
		Logical_var_imp(const T& e, Status status)
			: Backtrack_observer(), _backtrack_scheduled(false),
			  _e(e), _status(status), _backtrack_depth(Backtrack::depth()), _equiv_class_size(1)
		{
			assert((status == Status::GROUND) || (status == Status::MUTABLE));
			Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
		}

		/**
		 * Initialize with moved value.
		 * @param e The initial value
		 * @param status To know between Status::GROUND and Status::MUTABLE
		 */
		Logical_var_imp(T&& e, Status status)
			: Backtrack_observer(), _backtrack_scheduled(false),
			  _e(std::move(e)), _status(status), _backtrack_depth(Backtrack::depth()), _equiv_class_size(1)
		{
			assert((status == Status::GROUND) || (status == Status::MUTABLE));
			Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
		}
		//@}

		/// \name Tests
		//@{
		/**
		 * Check if the logical variable is equal to value \a value. If the variable is grounded,
		 * it checks the equality of the value. Otherwise, it return false.
		 * @param value The value to test
		 * @return True if the variable is equal to value \a value, false otherwise
		 */
		bool operator==(const T& value) const
		{
			auto& this_ra = ra();
			return ((this_ra._status == Status::GROUND) && (this_ra._e == value));
		}

		/**
		 * Check if the logical variable is equal to value \a value. If the variable is grounded,
		 * it checks the equality of the value. Otherwise, it return false.
		 * @param value The value to test
		 * @return True if the variable is equal to value \a value, false otherwise
		 */
		bool operator==(T&& value) const
		{
			auto& this_ra = ra();
			return ((this_ra._status == Status::GROUND) && (this_ra._e == value));
		}

		/**
		 * Check if the logical variable and \a o are equals. If the variable are grounded,
		 * it checks the equality of the value. Otherwise, it test that the variables
		 * are in the same equivalence class.
		 * @param o The other logical variable
		 * @return True if the variable is equal to \a o, false otherwise
		 */
		bool operator==(const Logical_var_imp< T >& o) const
		{
			auto& this_ra = ra();
			auto& o_ra = o.ra();
			return ((&this_ra == &o_ra) ||
					((this_ra._status == Status::GROUND) && (o_ra._status == Status::GROUND) && (this_ra._e == o_ra._e)));
		}
		//@}

		/// \name Modifiers
		//@{
		/**
		 * Try to match *this with \a value.
		 * The equivalence class of *this will be equal to \a value.
		 * @param value The value to match
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(const T& value);

		/**
		 * Try to match the equivalence class of *this with
		 * that of \a o.
		 * The const variant is set as deleted as the \a o logical variable
		 * may be updated when processing.
		 * @param o The variable to be equivalent with
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(Logical_var_imp< T >&);
		//@}

		/**
		 * Build a string representation of the variable.
		 * @return A string
		 */
		std::string to_string() const;

		/**
		 * Wakes up all constraints (only constraints no other callbacks) which involve this
		 * Logical_var_imp object.
		 * If the variable is not mutable, the behaviour is undefined.
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable();

		/**
		 * Update and set the value of the variable to \a value. It wakes up all
		 * constraints (only constraints no other callbacks) which involve this
		 * Logical_var_imp object.
		 * If the variable is not mutable, the behaviour is undefined.
		 * @param value The new value
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable(const T& value);

		/**
		 * Update and set the value of the variable to \a value. It wakes up all
		 * constraints (only constraints no other callbacks) which involve this
		 * Logical_var_imp object.
		 * If the variable is not mutable, the behaviour is undefined.
		 * @param value The new value
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable(std::function< void (T&) > f);

		/**
		 * Apply the \a f function to the underlying value of the variable. It wakes up all
		 * constraints (only constraints no other callbacks) which involve this
		 * Logical_var_mutable object ONLY IF the \a flag parameter is TRUE.
		 * If the variable is not mutable, the behaviour is undefined.
		 * @param flag True if update_mutable has to wake up the involved constraints, false otherwise
		 * @param f The function to apply and check
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable(const bool& flag, std::function< void (T&) > f);

		/**
		 * Add a new constraint callback to the (low priority) list of constraint callbacks to wake up when the variable is updated.
		 * @param callback The constraint callback object to run
		 */
		void schedule(chr::Shared_x_obj< Logical_var_imp_observer_constraint >& callback) { ra().ra_schedule(callback); }

		/**
		 * Add a new constraint callback to the (low priority) list of constraint callbacks to wake up when the variable is updated.
		 * @param callback The constraint callback object to run
		 */
		void schedule(chr::Shared_x_obj< Logical_var_imp_observer_constraint >&& callback) { ra().ra_schedule( std::move(callback) ); }

		/**
		 * Add a new index update callback to the (high priority) list of callbacks to wake up when the variable is updated.
		 * The index update callbacks are all called before constraint callbacks.
		 * @param callback The callback object to run
		 */
		void schedule(chr::Shared_x_obj< Logical_var_imp_observer_index >&& callback) { ra().ra_schedule( std::move(callback) ); }

		/**
		 * ra: root_ancestor. Search and return the root ancestor of this
		 * variable. The there is no ancestor, the function return *this.
		 * This the const variant of the method.
		 * @return A constant reference to the root ancestor variable
		 */
		const Logical_var_imp< T >& ra() const;

		/**
		 * ra: root_ancestor. Search and return the root ancestor of this
		 * variable. The there is no ancestor, the function return *this.
		 * @return A reference to the root ancestor variable
		 */
		Logical_var_imp< T >& ra();

		/**
		 * Add a new callback to the constraint callback list to wake up when the variable is updated.
		 * This function must be called on root ancestor.
		 * @param callback The callback object to run
		 */
		void ra_schedule(const chr::Shared_x_obj< Logical_var_imp_observer_constraint >& callback);

		/**
		 * Add a new callback to the constraint callback list to wake up when the variable is updated.
		 * This function must be called on root ancestor.
		 * @param callback The callback object to run
		 */
		void ra_schedule(chr::Shared_x_obj< Logical_var_imp_observer_constraint >&& callback);

		/**
		 * Add a new index update callback to the (high priority) list of callbacks to wake up when the variable is updated.
		 * The index update callbacks are all called before constraint callbacks.
		 * An index update callback can be called only once! It will be deleted after and not forward
		 * to the new root when equivalence class is updated.
		 * This function must be called on root ancestor.
		 * @param callback The callback object to run
		 */
		void ra_schedule(const chr::Shared_x_obj< Logical_var_imp_observer_index >& callback);

		/**
		 * Add a new index update callback to the (high priority) list of callbacks to wake up when the variable is updated.
		 * The index update callbacks are all called before constraint callbacks.
		 * An index update callback can be called only once! It will be deleted after and not forward
		 * to the new root when equivalence class is updated.
		 * This function must be called on root ancestor.
		 * @param callback The callback object to run
		 */
		void ra_schedule(chr::Shared_x_obj< Logical_var_imp_observer_index >&& callback);

		/**
		 * Wake up the index update callback objects of the wake-up queue.
		 * @param old_key The old key value (has to be an address) used before update
		 */
		void wake_up_index_update_callbacks(const void* old_key);

		/**
		 * Wake up the constraint callback objects (low priority objects).
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		ES_CHR wake_up_constraint_callbacks();

		/**
		 * Wake up the constraint callback objects (low priority objects) for this logical var and the \a o logical
		 * var. It merges the callback of both logical variable to this.
		 * @param o The other logical var imp to wake up
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		ES_CHR wake_up_constraint_callbacks(Logical_var_imp<T>& o);

		/**
		 * Create a snapshot of current Logical_var_imp and link it in the list of previous snapshots.
		 */
		void create_rollback_point();

		/**
		 * Rewind to the snapshot of the variable stored at \a depth.
		 * All cloned encountered along the path will be forgotten.
		 * @param previous_depth The previous depth before call to back_to function
		 * @param new_depth The new depth after call to back_to function
		 * @return False if the callback can be removed from the wake up list, true otherwise
		 */
		bool rewind(Depth_t previous_depth, Depth_t new_depth) override;
	};

	/**
	 * @brief Grounded logical variable
	 *
	 * The Logical_var_ground class is a wrapper to a grounded logical variable.
	 * As a grounded logical variable is given by its underlying value, this class
	 * is just a wrapper to underlying value. But it has all member functions needed
	 * to interact with any logical variable.
	 * \ingroup Logical_variables
	 */
	template< typename T >
	class Logical_var_ground
	{
		template< typename U > friend struct Schedule_constraint_callback;
		friend class Logical_var< T >;
		friend class Logical_var_mutable< T >;
	public:
		/// Check if we need to call destructor
		static constexpr bool NEED_DESTROY = !std::is_scalar<T>::value && chr::Has_need_destroy<T>::value;
		static constexpr bool LV_GROUND = true; ///< Say that this is a Logical_var_ground

		typedef T Value_t;							///< Type of encapsulated variable
		typedef Grounded_key_t<T> Key_t;			///< Type of index if variable involved
		typedef chr::Logical_var_ground<T> Weak_t;	///< Weak type

		/**
		 * Initialize empty: not valid, only for aggregate data structures initialization.
		 */
		Logical_var_ground()
		{
			Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
		}

		/**
		 * Copy constructor.
		 * @param var The initial variable
		 */
		Logical_var_ground(const Logical_var_ground< T >& var) : _value(var._value)
		{
			Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
		}

		/**
		 * Move constructor.
		 * @param var The initial variable
		 */
		Logical_var_ground(Logical_var_ground< T >&& var) : _value( std::move(var._value) )
		{
			Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
		}

		/**
		 * Initialize with value.
		 * @param value The initial \a value
		 */
		Logical_var_ground(T value) : _value( std::move(value) )
		{
			Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
		}

		/**
		 * Initialize with logical variable.
		 * @param var The initial variable
		 */
		Logical_var_ground(const Logical_var< T >& var) : _value(*var)
		{
			assert(var.ground());
			Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
		}

		/**
		 * Special case to initialize the variable without var_imp.
		 * Only the arity is relevant to distinguish from other constructors.
		 * This constructor is useful for Logical_var, it is just defined here
		 * for convenience.
		 */
		Logical_var_ground(bool,bool)
		{
			Statistics::inc_memory< Statistics::VARIABLE >(sizeof *this);
		}

		/**
		 * Destroy.
		 */
		~Logical_var_ground()
		{
			Statistics::dec_memory< Statistics::VARIABLE >(sizeof *this);
		}

		/// \name Tests
		//@{
		/**
		 * Check if the variable is grounded.
		 * @return True, because this is a grounded variable
		 */
		bool ground() const { return true; }

		/**
		 * Check if the variable is mutable.
		 * @return False, because this is a grounded variable
		 */
		bool is_mutable() const { return false; }

		/**
		 * Returns the ground status of the variable (NOT_GROUNDED, GROUNDED or MUTABLE).
		 * Useful for internal functions not for normal user.
		 * @return 1 as this is a grounded variable
		 */
		Status status() const { return Status::GROUND; }

		/**
		 * Check if the grounded variable is equal to a Logical_var \a o. As this is grounded,
		 * it checks the equality of the Logical_var \a o to the value of this Logical_var_ground.
		 * @param o The value to test
		 * @return True if the variables are equal, false otherwise
		 */
		bool operator==(const Logical_var< T >& o) const {
			return (*(o._var_imp) == _value);
		}

		/**
		 * Check if the grounded variable is equal to a Logical_var_mutable \a o.
		 * A mutable variable cannot be equal to a grounded one. The test will always fail.
		 * @param o The value to test
		 * @return Always false
		 */
		bool operator==(const Logical_var_mutable< T >&) const {
			return false;
		}

		/**
		 * Check if the grounded variable is equal to a Logical_var_ground \a o. As both
		 * variables are grounded, it checks the equality of the values.
		 * @param o The value to test
		 * @return True if the variables are equal, false otherwise
		 */
		bool operator==(const Logical_var_ground< T >& o) const { return (_value == o._value); }

		/**
		 * Check if the grounded variable is equal to value \a value. As the variable is grounded,
		 * it checks the equality of the value.
		 * @param value The value to test
		 * @return True if the variable is equal to value \a value, false otherwise
		 */
		bool operator==(const T& value) const { return (_value == value); }

		/**
		 * Check if the grounded variable is equal to value \a value. As the variable is grounded,
		 * it checks the equality of the value.
		 * @param value The value to test
		 * @return True if the variable is equal to value \a value, false otherwise
		 */
		bool operator==(T&& value) const { return (_value == value); }

		/**
		 * Check if the logical variable is different from to other Logical_var \a o. As this is grounded,
		 * it checks the difference of the Logical_var \a o from the value of this Logical_var_ground.
		 * @param o The value to test
		 * @return True if the variables are not equal, false otherwise
		 */
		bool operator!=(const Logical_var< T >& o) const { return (!(*(o._var_imp) == _value)); }

		/**
		 * Check if the logical variable is different from the other Logical_var_ground \a o. As both
		 * variables are grounded, it checks the difference of the values.
		 * @param o The value to test
		 * @return True if the variables are not equal, false otherwise
		 */
		bool operator!=(const Logical_var_ground< T >& o) const { return (_value != o._value); }

		/**
		 * Check if the grounded variable and the Logical_var_mutable \a o are not equals.
		 * A mutable variable cannot be syntactic equal to a grounded one. The test will
		 * always succeed.
		 * @param o The value to test
		 * @return Always true
		 */
		bool operator!=(const Logical_var_mutable< T >&) const { return true; }

		/**
		 * Check if the logical variable is different from the value \a value. As the variable is grounded,
		 * it checks the difference from the value.
		 * @param value The value to test
		 * @return True if the variable is not equal to value \a value, false otherwise
		 */
		bool operator!=(const T& value) const { return (_value != value); }

		/**
		 * Check if the logical variable is different from the value \a value. As the variable is grounded,
		 * it checks the difference from the value.
		 * @param value The value to test
		 * @return True if the variable is not equal to value \a value, false otherwise
		 */
		bool operator!=(T&& value) const { return (_value != value); }
		//@}

		/// \name Access
		//@{
		/**
		 * Assignment operator changes the underlying value to the one of \a o.
		 * @param o The variable to take value from
		 * @return A reference to this
		 */
		Logical_var_ground< T >& operator=(const Logical_var_ground< T >& o) =default;

		/**
		 * Assignment operator changes the underlying value to the one of \a o.
		 * Used in core engine, not relevant for user.
		 * @param o The variable to take value from
		 * @return A reference to this
		 */
		void force_assign(const Logical_var_ground< T >& o)
		{
			_value = o._value;
		}

		/**
		 * Access to the value of the variable when needed. If the variable is not
		 * grounded, the behavior is undefined. The returned reference is
		 * constant, it cannot be used to update the value of the variable.
		 * @return A constant reference to the value
		 */
		const T& operator*() const { return _value; }

		/**
		 * Cast the logical variable to its inside value type object by
		 * returning the inside value.
		 * If the variable is not grounded, the behavior is undefined.
		 * @return A (copy) of value object
		 */
		operator T() const { return _value; }
		//@}

		/// \name Modifiers
		//@{
		/**
		 * Try to match this with \a value.
		 * As *this is grounded it returns true if *this
		 * is equal to value, false otherwise
		 * @param value The value to match
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(const T& value)
		{
			return (_value == value)?chr::success():chr::failure();
		}

		/**
		 * Try to match the equivalence class of *this with
		 * that of \a o.
		 * @param o The variable to be equivalent with
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(const Logical_var_ground< T >& o)
		{
			return (_value == o._value)?chr::success():chr::failure();
		}

		/**
		 * Try to match the equivalence class of *this with
		 * that of the logical_var \a o.
		 * If the two classes match, then they are merged. Logical_var \a o may
		 * also be updated when classes are merged together.
		 * @param o The variable to be equivalent with
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(Logical_var< T >& o)
		{
			return (o %= _value);
		}
		//@}

		/**
		 * Build a string representation of the variable.
		 * @return A string
		 */
		std::string to_string() const { return TIW::to_string(_value); }

	private:
		T _value;	///< The underlying value of the grounded logical variable
	};

	/**
	 * @brief Logical mutable variable
	 * The Logical_var_mutable class is a wrapper to a mutable logical variable.
	 * A mutable logical variable may be unified with not grounded variables. It
	 * cannot be unified with any grounded logical variable or with a mutable
	 * variable which has a different value.
	 *
	 * \ingroup Logical_variables
	 */
	template< typename T >
	class Logical_var_mutable
	{
		template< typename U > friend struct Schedule_constraint_callback;
		friend class Logical_var< T >;
		friend class Logical_var_ground< T >;
		// Friend for Unit tests
		friend void tests::logical_var::backtrack_tests();
		friend void tests::constraint_store::index_misc_tests();
	public:
		static constexpr bool LV_GROUND = false;	///< Say that this is not a Logical_var_ground
		typedef T Value_t;							///< Type of encapsulated variable
		typedef Mutable_key_t<T> Key_t;				///< Type of index if variable involved

		/**
		 * @brief The Weak_t struct wrap a weak_ptr to a var_imp
		 *
		 * This struct is used to break cycle with Shared_obj (in wake up list of var_imp)
		 */
		struct Weak_t
		{
			chr::Weak_obj< Logical_var_imp< T > > _var_imp;	///< The pointer to the physical variable
			static constexpr bool LV_GROUND = false;		///< Say that this is not a Logical_var_ground
			/**
			 * @brief Construct a Weak Logical_var_mutable.
			 * @param v The Logical_var_mutable to use
			 */
			Weak_t(const Logical_var_mutable<T>& v) : _var_imp(v._var_imp) { }

			/**
			 * Check if the underlying object is still available.
			 * @return False if the underlying object is still reachable true otherwise
			 */
			bool expired() const { return _var_imp.expired(); }
		};

		/**
		 * Initialize empty
		 */
		Logical_var_mutable()
		{
			_var_imp = chr::make_shared< Logical_var_imp<T> >(Status::MUTABLE);
		}

		/**
		 * Copy constructor.
		 * @param o Source Logical_var_mutable
		 */
		Logical_var_mutable(const Logical_var_mutable< T >& o)
			: _var_imp(o._var_imp)
		{ }

		/**
		 * Move constructor.
		 * @param o Source Logical_var_mutable
		 */
		Logical_var_mutable(Logical_var_mutable< T >&& o)
			: _var_imp( std::move(o._var_imp) )
		{ }

		/**
		 * Build a Logical_var_mutable from a weak ptr Logical_var_mutable.
		 * @param o Source Logical_var_mutable
		 */
		Logical_var_mutable(const Weak_t& o)
			: _var_imp(o._var_imp)
		{ }

		/**
		 * Initialize with const reference rvalue.
		 * @param value The initial value
		 */
		Logical_var_mutable(const T& value)
		{
			_var_imp = chr::make_shared< Logical_var_imp<T> >(value, Status::MUTABLE);
		}

		/**
		 * Initialize with \a value.
		 * @param value The initial value
		 */
		Logical_var_mutable(T&& value)
		{
			_var_imp = chr::make_shared< Logical_var_imp<T> >( std::move(value), Status::MUTABLE );
		}

		/**
		 * Initialize with a logical variable.
		 * @param var The logical to unify with
		 */
		Logical_var_mutable(const Logical_var< T >& var)
			: _var_imp(var._var_imp)
		{
			assert(var.status() == Status::MUTABLE);
		}

		/**
		 * Special case to initialize the variable without var_imp.
		 * Only the arity is relevant to distinguish from other constructors.
		 */
		Logical_var_mutable(bool,bool) { }

		/// \name Access
		//@{
		/**
		 * Access to the value of the variable when needed.
		 * The returned reference is constant, it cannot be used to update
		 * the value of the variable.
		 * @return A constant reference to the value
		 */
		const T& operator*() const { return **_var_imp; }

		/**
		 * Cast the logical variable to its inside value type object by
		 * returning the inside value.
		 * @return A (copy) of value object
		 */
		operator T() const { return **_var_imp; }
		//@}

		/// \name Tests
		//@{
		/**
		 * Check if the variable is grounded.
		 * @return False, because this is a mutable variable (it cannot be grounded).
		 */
		bool ground() const { return false; }

		/**
		 * Check if the variable is mutable.
		 * @return True, because this is a mutable variable.
		 */
		bool is_mutable() const { return true; }

		/**
		 * Returns the ground status of the variable (NOT_GROUNDED, GROUNDED or MUTABLE).
		 * Useful for internal functions not for normal user.
		 * @return The ground status
		 */
		Status status() const { return Status::MUTABLE; }

		/**
		 * Check if the mutable variable is equal to a Logical_var_ground  \a o.
		 * A mutable variable cannot be equal to a grounded one. The test will always fail.
		 * @param o The value to test
		 * @return Always false
		 */
		bool operator==(const Logical_var_ground< T >&) const { return false; }

		/**
		 * Check if the mutable variable and the logical variable \a o are equals.
		 * The logical variable \a o must be mutable, otherwise it answers false.
		 * @param o The logical variable
		 * @return True if the variable is equal to \a o, false otherwise
		 */
		bool operator==(const Logical_var< T >& o) const
		{
			return ((*_var_imp) == (*o._var_imp));
		}

		/**
		 * Check if the mutable variable and \a o are equals. It test that the variables
		 * are in the same equivalence class.
		 * @param o The other logical mutable variable
		 * @return True if the variable is equal to \a o, false otherwise
		 */
		bool operator==(const Logical_var_mutable< T >& o) const
		{
			return ((*_var_imp) == (*o._var_imp));
		}

		/**
		 * Check if the mutable variable and the Logical_var_ground  \a o are not equals.
		 * A mutable variable cannot be equal to a grounded one. The test will always succeed.
		 * @param o The value to test
		 * @return Always true
		 */
		bool operator!=(const Logical_var_ground< T >&) const { return true; }

		/**
		 * Check if the mutable variable and the logical variable \a o are not equals.
		 * The logical variable \a o must be mutable, otherwise it answers false.
		 * @param o The logical variable
		 * @return True if the variable is not equal to \a o, false otherwise
		 */
		bool operator!=(const Logical_var< T >& o) const
		{
			return !((*_var_imp) == (*o._var_imp));
		}

		/**
		 * Check if the mutable variable and \a o are not equals. It test that the variables
		 * are not in the same equivalence class.
		 * @param o The logical mutable variable
		 * @return True if the variable is not equal to \a o, false otherwise
		 */
		bool operator!=(const Logical_var_mutable< T >& o) const
		{
			return !((*_var_imp) == (*o._var_imp));
		}
		//@}

		/// \name Modifiers
		//@{
		/**
		 * Assignment operator changes the underlying variable to another var_imp.
		 * Ownership of \a o is shared with *this. *this releases
		 * its previous ownership without any further more work.
		 * It's a true assignment, not a match operator.
		 * An assert has been added to prevent wrong use of this operator.
		 * Indeed, it is only necessary because of container data structures
		 * like vector, ... For example, chr::Logical_vector needs it.
		 * @param o The variable to share ownership with
		 * @return A reference to this
		 */
		Logical_var_mutable< T >& operator=(const Logical_var_mutable< T >& o)
		{
			_var_imp = o._var_imp;
			return *this;
		}

		/**
		 * Assignment operator changes the underlying value to the one of \a o.
		 * Used in core engine, not relevant for user.
		 * @param o The variable to take value from
		 * @return A reference to this
		 */
		void force_assign(const Logical_var_mutable< T >& o)
		{
			_var_imp = o._var_imp;
		}

		/**
		 * Wakes up all constraints (only constraints no other callbacks) which involve this
		 * Logical_var_mutable object.
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable() { return _var_imp->update_mutable(); }

		/**
		 * Update and set the value of the variable to \a value. It wakes up all
		 * constraints (only constraints no other callbacks) which involve this
		 * Logical_var_mutable object.
		 * @param value The new value
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable(const T& value) { return _var_imp->update_mutable(value); }

		/**
		 * Update and set the value of the variable to \a value. It wakes up all
		 * constraints (only constraints no other callbacks) which involve this
		 * Logical_var_mutable object.
		 * @param value The new value
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable(std::function< void (T&) > f) { return _var_imp->update_mutable(f); }

		/**
		 * Apply the \a f function to the underlying value of the variable. It wakes up all
		 * constraints (only constraints no other callbacks) which involve this
		 * Logical_var_mutable object ONLY IF the \a flag parameter is TRUE.
		 * @param flag True if update_mutable has to wake up the involved constraints, false otherwise
		 * @param f The function to apply and check
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable(const bool& flag, std::function< void (T&) > f) { return _var_imp->update_mutable(flag,f); }

		/**
		 * Try to match the equivalence class of *this with
		 * that of \a o.
		 * If the two classes match, then they are merged. Logical_var_mutable \a o may
		 * also be updated when classes are merged together.
		 * @param o The variable to be equivalent with
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(Logical_var_mutable< T >& o)
		{
			return (*_var_imp %= *o._var_imp);
		}

		/**
		 * Try to match the equivalence class of *this with
		 * that of \a o.
		 * If the two classes match, then they are merged. Logical_var \a o may
		 * also be updated when classes are merged together.
		 * @param o The variable to be equivalent with
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(Logical_var< T >& o)
		{
			return (*_var_imp %= *o._var_imp);
		}
		//@}

		/**
		 * Return the address of the root ancestor of the variable.
		 * It may serve as a unique id for this equivalence class
		 * @return The address of the variable (the root ancestor)
		 */
		const void* address() const { return _var_imp->address(); }

		/**
		 * Build a string representation of the variable.
		 * @return A string
		 */
		std::string to_string() const { return _var_imp->to_string(); }

		/**
		 * Add a new constraint callback to the (low priority) list of constraint callbacks to wake up when the variable is updated.
		 * @param callback The constraint callback object to run
		 */
		void schedule(chr::Shared_x_obj< Logical_var_imp_observer_constraint >& callback) { _var_imp->schedule(callback); }

		/**
		 * Add a new constraint callback to the (low priority) list of constraint callbacks to wake up when the variable is updated.
		 * @param callback The constraint callback object to run
		 */
		void schedule(chr::Shared_x_obj< Logical_var_imp_observer_constraint >&& callback) { _var_imp->schedule( std::move(callback) ); }

		/**
		 * Add a new index update callback to the (high priority) list of callbacks to wake up when the variable is updated.
		 * The index update callbacks are all called before constraint callbacks.
		 * @param callback The callback object to run
		 */
		void schedule(chr::Shared_x_obj< Logical_var_imp_observer_index >&& callback) { _var_imp->schedule(std::move(callback)); }

	private:
		chr::Shared_obj< Logical_var_imp< T > > _var_imp;	///< The pointer to the physical variable
	};

	/**
	 * Overload of the << operator for Logical_var.
	 * @param out The output stream
	 * @param var The variable to write to output
	 * @return A reference to the given output stream \a out
	 */
	template < typename T >
	std::ostream & operator<< (std::ostream& out, const Logical_var_mutable< T >& var);

	/**
	 * @brief Logical variable
	 * The Logical_var class is a wrapper to a not initialy grounded logical variable.
	 *
	 * \ingroup Logical_variables
	 */
	template< typename T >
	class Logical_var
	{
		template< typename U > friend struct Schedule_constraint_callback;
		friend class Logical_var_ground< T >;
		friend class Logical_var_mutable< T >;
		// Friend for Unit tests
		friend void tests::logical_var::backtrack_tests();
		friend void tests::constraint_store::index_misc_tests();
	public:
		static constexpr bool LV_GROUND = false;	///< Say that this is not a Logical_var_ground
		typedef T Value_t;							///< Type of encapsulated variable
		typedef Complex_key_t<T> Key_t;				///< Type of index if variable involved

		/**
		 * @brief The Weak_t struct wrap a weak_ptr to a var_imp
		 *
		 * This struct is used to break cycle with Shared_obj (in wake up list of var_imp)
		 */
		struct Weak_t
		{
			chr::Weak_obj< Logical_var_imp< T > > _var_imp;	///< The pointer to the physical variable
			static constexpr bool LV_GROUND = false;		///< Say that this is not a Logical_var_ground
			/**
			 * @brief Construct a Weak Logical_var.
			 * @param v The Logical_var to use
			 */
			Weak_t(const Logical_var<T>& v) : _var_imp(v._var_imp) {}

			/**
			 * Check if the underlying object is still available.
			 * @return False if the underlying object is still reachable true otherwise
			 */
			bool expired() const { return _var_imp.expired(); }
		};

		/**
		 * Initialize.
		 */
		Logical_var()
		{
			_var_imp = chr::make_shared< Logical_var_imp<T> >(Status::NOT_GROUND);
		}

		/**
		 * Copy constructor.
		 * @param o Source Logical_var
		 */
		Logical_var(const Logical_var< T >& o)
			: _var_imp(o._var_imp)
		{ }

		/**
		 * Move constructor.
		 * @param o Source Logical_var
		 */
		Logical_var(Logical_var< T >&& o)
			: _var_imp(std::move(o._var_imp))
		{ }

		/**
		 * Build a Logical_var from a weak ptr Logical_var.
		 * @param o Source Logical_var
		 */
		Logical_var(const Weak_t& o)
			: _var_imp(o._var_imp)
		{ }

		/**
		 * Initialize with const reference rvalue.
		 * @param value The initial value
		 */
		Logical_var(const T& value)
		{
			_var_imp = chr::make_shared< Logical_var_imp<T> >(value, Status::GROUND);
		}

		/**
		 * Initialize with \a value.
		 * @param value The initial value
		 */
		Logical_var(T&& value)
		{
			_var_imp = chr::make_shared< Logical_var_imp<T> >( std::move(value), Status::GROUND );
		}

		/**
		 * Initialize with a grounded logical variable.
		 * @param ground_var The logical grounded variable to take value from
		 */
		Logical_var(const Logical_var_ground< T >& ground_var)
		{
			_var_imp = chr::make_shared< Logical_var_imp<T> >(*ground_var, Status::GROUND);
		}

		/**
		 * Initialize with a mutable logical variable.
		 * @param mutable_var The logical mutable to be equivalent
		 */
		Logical_var(const Logical_var_mutable< T >& mutable_var)
			: _var_imp(mutable_var._var_imp)
		{ }

		/**
		 * Special case to initialize the variable without var_imp.
		 * Only the arity is relevant to distinguish from other constructors.
		 */
		Logical_var(bool,bool) { }

		/// \name Access
		//@{
		/**
		 * Access to the value of the variable when needed. If the variable is not
		 * grounded (i.e. not grounded or mutable), the behavior is undefined.
		 * The returned reference is constant, it cannot be used to update
		 * the value of the variable.
		 * @return A constant reference to the value
		 */
		const T& operator*() const { return **_var_imp; }

		/**
		 * Cast the logical variable to its inside value type object by
		 * returning the inside value.
		 * If the variable is not grounded (i.e. not grounded or mutable),
		 * the behavior is undefined.
		 * @return A (copy) of value object
		 */
		operator T() const { return **_var_imp; }
		//@}

		/// \name Tests
		//@{
		/**
		 * Check if the variable is grounded.
		 * @return True if it is a ground variable, false otherwise
		 */
		bool ground() const { return _var_imp->ground(); }

		/**
		 * Check if the variable is mutable.
		 * @return True if it is a mutable variable, false otherwise
		 */
		bool is_mutable() const { return _var_imp->is_mutable(); }

		/**
		 * Returns the ground status of the variable (NOT_GROUNDED, grounded or mutable).
		 * Useful for internal functions not for normal user.
		 * @return The ground status
		 */
		Status status() const { return _var_imp->status(); }

		/**
		 * Check if the logical variable is equal to value \a value. If the variable is grounded,
		 * it checks the equality of the value. Otherwise, it returns false.
		 * @param value The value to test
		 * @return True if the variable is equal to value \a value, false otherwise
		 */
		bool operator==(const T& value) const { return ((*_var_imp) == value); }

		/**
		 * Check if the logical variable is equal to value \a value. If the variable is grounded,
		 * it checks the equality of the value. Otherwise, it returns false.
		 * @param value The value to test
		 * @return True if the variable is equal to value \a value, false otherwise
		 */
		bool operator==(T&& value) const { return ((*_var_imp) == value); }

		/**
		 * Check if the logical variable and the ground logical variable \a o are equals.
		 * If the first logical variable is grounded, it checks the equality of the values.
		 * Otherwise, it returns false.
		 * @param o The ground logical variable
		 * @return True if the variable is equal to \a o, false otherwise
		 */
		bool operator==(const Logical_var_ground< T >& o) const
		{
			return ((*_var_imp) == *o);
		}

		/**
		 * Check if the logical variable and the mutable variable \a o are equals.
		 * The logical variable \a o must be mutable, otherwise it answers false.
		 * @param o The logical variable
		 * @return True if the variable is equal to \a o, false otherwise
		 */
		bool operator==(const Logical_var_mutable< T >& o) const
		{
			return ((*_var_imp) == (*o._var_imp));
		}

		/**
		 * Check if the logical variable and \a o are equals. If the variable are grounded,
		 * it checks the equality of the value. Otherwise, it test that the variables
		 * are in the same equivalence class.
		 * @param o The other logical variable
		 * @return True if the variable is equal to \a o, false otherwise
		 */
		bool operator==(const Logical_var< T >& o) const
		{
			return ((*_var_imp) == (*o._var_imp));
		}

		/**
		 * Check if the logical variable is not equal to value \a value. If the variable is grounded,
		 * it checks that it differs from the value. Otherwise, it return true.
		 * @param value The value to test
		 * @return True if the variable is not equal to value \a value, false otherwise
		 */
		bool operator!=(const T& value) const { return !((*_var_imp) == value); }

		/**
		 * Check if the logical variable is not equal to value \a value. If the variable is grounded,
		 * it checks that it differs from the value. Otherwise, it return true.
		 * @param value The value to test
		 * @return True if the variable is not equal to value \a value, false otherwise
		 */
		bool operator!=(T&& value) const { return !((*_var_imp) == value); }

		/**
		 * Check if the logical variable and \a o are not equals. If the variable are grounded,
		 * it checks the equality of the value. Otherwise, it test that the variables
		 * are in different same equivalence classes.
		 * @param o The other logical variable
		 * @return True if the variable is not equal to \a o, false otherwise
		 */
		bool operator!=(const Logical_var< T >& o) const
		{
			return !((*_var_imp) == (*o._var_imp));
		}

		/**
		 * Check if the logical variable and the mutable variable \a o are not equals.
		 * The logical variable \a o must be mutable, otherwise it answers false.
		 * @param o The logical variable
		 * @return True if the variable is not equal to \a o, false otherwise
		 */
		bool operator!=(const Logical_var_mutable< T >& o) const
		{
			return !((*_var_imp) == (*o._var_imp));
		}

		/**
		 * Check if the logical variable and the ground logical variable \a o are not equals.
		 * If the first logical variable is grounded and differs from \a o.
		 * Otherwise, it returns true.
		 * @param o The ground logical variable
		 * @return True if the variable is equal to \a o, false otherwise
		 */
		bool operator!=(const Logical_var_ground< T >& o) const
		{
			return !((*_var_imp) == *o);
		}
		//@}

		/// \name Modifiers
		//@{
		/**
		 * Ownership of \a o is shared with *this. *this releases
		 * Assignment operator changes the underlying variable to another var_imp.
		 * its previous ownership without any further more work.
		 * It's a true assignment, not a match operator.
		 * An assert has been added to prevent wrong use of this operator.
		 * Indeed, it is only necessary because of container data structures
		 * like vector, ... For example, chr::Logical_vector needs it.
		 * @param o The variable to share ownership with
		 * @return A reference to this
		 */
		Logical_var< T >& operator=(const Logical_var< T >& o)
		{
			_var_imp = o._var_imp;
			return *this;
		}

		/**
		 * Assignment operator changes the underlying value to the one of \a o.
		 * Used in core engine, not relevant for user.
		 * @param o The variable to take value from
		 * @return A reference to this
		 */
		void force_assign(const Logical_var< T >& o)
		{
			_var_imp = o._var_imp;
		}

		/**
		 * Wakes up all constraints (only constraints no other callbacks) which involve this
		 * Logical_var_imp object.
		 * If the variable is not mutable, the behaviour is undefined.
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable() { return _var_imp->update_mutable(); }

		/**
		 * Update and set the value of the variable to \a value. It wakes up all
		 * constraints (only constraints no other callbacks) which involve this
		 * Logical_var object.
		 * If the variable is not mutable, the behaviour is undefined.
		 * @param value The new value
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable(const T& value) { return _var_imp->update_mutable(value); }

		/**
		 * Apply the \a f function to the underlying value of the variable. It wakes up all
		 * constraints (only constraints no other callbacks) which involve this
		 * Logical_var object.
		 * If the variable is not mutable, the behaviour is undefined.
		 * @param f The function to apply
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable(std::function< void (T&) > f) { return _var_imp->update_mutable(f); }

		/**
		 * Apply the \a f function to the underlying value of the variable. It wakes up all
		 * constraints (only constraints no other callbacks) which involve this
		 * Logical_var_mutable object ONLY IF the \a flag parameter is TRUE.
		 * If the variable is not mutable, the behaviour is undefined.
		 * @param flag True if update_mutable has to wake up the involved constraints, false otherwise
		 * @param f The function to apply and check
		 * @return FAILURE if a failure has been raised during constaint wake-up, SUCCESS otherwise
		 */
		chr::ES_CHR update_mutable(const bool& flag, std::function< void (T&) > f) { return _var_imp->update_mutable(flag,f); }

		/**
		 * Try to match *this with \a value.
		 * The equivalence class of *this will be equal to \a value.
		 * @param value The value to match
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(const T& value)
		{
			return (*_var_imp %= value);
		}

		/**
		 * Try to match *this with \a ground_var grounded variable.
		 * The equivalence class of *this will be equal to \a ground_var.
		 * @param ground_var The grounded variable to take value from
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(const Logical_var_ground< T >& ground_var)
		{
			return (*_var_imp %= *ground_var);
		}

		/**
		 * Try to match the equivalence class of *this with
		 * the mutable variable of \a o.
		 * If the two classes match, then they are merged. Logical_var_mutable \a o may
		 * also be updated when classes are merged together.
		 * @param o The mutable variable to be equivalent with
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(Logical_var_mutable< T >& o)
		{
			return (*_var_imp %= *o._var_imp);
		}

		/**
		 * Try to match the equivalence class of *this with
		 * that of \a o.
		 * If the two classes match, then they are merged. Logical_var \a o may
		 * also be updated when classes are merged together.
		 * @param o The variable to be equivalent with
		 * @return chr::ES_CHR::SUCCESS if the operator succeeded, chr::ES_CHR::FAILURE otherwise
		 */
		chr::ES_CHR operator%=(Logical_var< T >& o)
		{
			return (*_var_imp %= *o._var_imp);
		}
		//@}

		/**
		 * Return the address of the root ancestor of the variable.
		 * It may serve as a unique id for this equivalence class
		 * @return The address of the variable (the root ancestor)
		 */
		const void* address() const { return _var_imp->address(); }

		/**
		 * Build a string representation of the variable.
		 * @return A string
		 */
		std::string to_string() const { return _var_imp->to_string(); }

		/**
		 * Add a new constraint callback to the (low priority) list of constraint callbacks to wake up when the variable is updated.
		 * @param callback The constraint callback object to run
		 */
		void schedule(chr::Shared_x_obj< Logical_var_imp_observer_constraint >& callback) { _var_imp->schedule(callback); }

		/**
		 * Add a new constraint callback to the (low priority) list of constraint callbacks to wake up when the variable is updated.
		 * @param callback The constraint callback object to run
		 */
		void schedule(chr::Shared_x_obj< Logical_var_imp_observer_constraint >&& callback) { _var_imp->schedule( std::move(callback) ); }

		/**
		 * Add a new index update callback to the (high priority) list of callbacks to wake up when the variable is updated.
		 * The index update callbacks are all called before constraint callbacks.
		 * @param callback The callback object to run
		 */
		void schedule(chr::Shared_x_obj< Logical_var_imp_observer_index >&& callback) { _var_imp->schedule(std::move(callback)); }

	private:
		chr::Shared_obj< Logical_var_imp< T > > _var_imp;	///< The pointer to the physical variable
	};

	/**
	 * Overload of the << operator for Logical_var.
	 * @param out The output stream
	 * @param var The variable to write to output
	 * @return A reference to the given output stream \a out
	 */
	template < typename T >
	std::ostream & operator<< (std::ostream& out, const Logical_var< T >& var);

	/**
	 * Convenient type alias for Constraint_callback
	 */
	using Constraint_callback = chr::Shared_x_obj< chr::Logical_var_imp_observer_constraint >;

	/**
	 * Schedule a constraint callback \a constraint_callback to the list of callbacks of \a variable.
	 * This function relies on another stucture depending on the type T. Default case is to call the
	 * .schedule() member function of the logical variable. But user can redefine its own schedule structure
	 * which allow to make special cases. For example, it allows to create container which schedule callbacks
	 * for its members.
	 * It statically call the right "schedule" data structure.
	 * @param variable The variable to update
	 * @param constraint_callback The constraint callback to add
	 * \ingroup Logical_variables
	 */
	template< typename T >
	void schedule_constraint_callback(T& variable, Constraint_callback& constraint_callback);

	/**
	 * @brief Grounded_key_t
	 *
	 * Key data structure used by Logical_var_ground to build a key.
	 * It uses the value of the Logical_var_ground
	 */
	#pragma pack(push, 1)
	template < typename T >
	struct Grounded_key_t
	{
		T _value;	///< The value is used
		Grounded_key_t() : _value{0} { }
		Grounded_key_t(const T& v) : _value(v) { }
		Grounded_key_t(const Logical_var_ground<T>& v) : _value(*v) { }
		Grounded_key_t(const Logical_var_mutable<T>&) : _value(0) { assert(false); }
		Grounded_key_t(const Logical_var<T>& v) : _value(*v) { assert(v.ground()); }
		bool operator==(const Grounded_key_t& o) const { return _value == o._value; }
	};
	#pragma pack(pop)

	/**
	 * @brief Mutable_key_t
	 *
	 * Key data structure used by Logical_var_mutable to build a key
	 * on the address of the root variable.
	 */
	#pragma pack(push, 1)
	template < typename T >
	struct Mutable_key_t
	{
		const void* _address;	///< The address is used
		Mutable_key_t() : _address(nullptr) { }
		Mutable_key_t(const void* a) : _address(a) { }
		Mutable_key_t(const Logical_var_ground<T>&) : _address(nullptr) { assert(false); }
		Mutable_key_t(const Logical_var_mutable<T>& v) : _address(v.address()) { }
		Mutable_key_t(const Logical_var<T>& v) : _address(v.address()) { assert(!v.ground()); }
		bool operator==(const Mutable_key_t& o) const { return (_address == o._address); }
	};
	#pragma pack(pop)

	/**
	 * @brief Complex_key_t
	 *
	 * Key data structure used by Logical_var_imp to build a key either
	 * on the address of the root variable or its value if the variable
	 * is grounded (used for Logical_var).
	 */
	#pragma pack(push, 1)
	template < typename T >
	struct Complex_key_t
	{
		const void* _address;		///< If not ground, the address is used
		T _value;					///< If ground, the value is used
		Complex_key_t(const void* a) : _address(a) {  if constexpr(std::is_scalar<T>::value) _value = 0; }
		Complex_key_t(const T& v) : _address(nullptr), _value(v) { }
		Complex_key_t(const Logical_var_ground<T>& v) : _address(nullptr), _value(*v) { }
		Complex_key_t(const Logical_var_mutable<T>& v) : _address(v.address()) { if constexpr(std::is_scalar<T>::value) _value = 0; }
		Complex_key_t(const Logical_var<T>& v)
		{
			if (v.ground())
			{
				_address = nullptr;
				_value = *v;
			} else {
				_address = v.address();
				 if constexpr(std::is_scalar<T>::value) _value = 0;
			}
		}
		bool operator==(const Complex_key_t& o) const
		{
			return (_address == o._address) && ((_address != nullptr) || (_value == o._value));
		}
	};
	#pragma pack(pop)
}

#include <logical_var.hpp>

#include <constraint_store.hh>
#include <constraint_store_index.hh>
#include <constraint_stores_iterator.hh>
#include <history.hh>

#endif /* RUNTIME_CHRPP_HH_ */
