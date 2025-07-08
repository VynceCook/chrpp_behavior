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

#ifndef RUNTIME_CONSTRAINT_STORE_INDEX_HH_
#define RUNTIME_CONSTRAINT_STORE_INDEX_HH_

#include <map>
#include <array>
#include <vector>
#include <memory>
#include <tuple>

// Following includes used for type_name function
#include <type_traits>
#include <typeinfo>
#ifndef _MSC_VER
#   include <cxxabi.h>
#endif

#include <statistics.hh>
#include <chrpp.hh>

#include <bt_list.hh>
#include <third_party/robin_map.h>

/**
 * \defgroup Constraints Constraints management
 * 
 * Constraints are a a key component (like logical variables) of a \e CHR++ program.
 * They are managed thanks to constraint stores.
 */
namespace chr {

	// Forward declaration
	template< typename T > class Constraint_store_index_iterator_full;
	template< typename T > class Constraint_store_index_iterator_hash;

	/**
	 * Namespace that contains usefull types. Easily reachable even without knowing the
	 * constraint store type.
	 */
	namespace LNS {
		/**
		 * Template structure used to manipulate static Index description.
		 * An index of the form Index<1,3,0> means that parameters 1, 3 and 0 of the
		 * constraint must be used to compute the key for storing the constraint in the
		 * corresponding index.
		 * Ns... is a list of integers where each value which corresponds to a parameter
		 * number of the constraint.
		 */
		template < unsigned int... Ns >
		struct Index { 
			static constexpr std::size_t size = sizeof...(Ns);
			/**
			 * Get the Nth value of list ...Ns.
			 */
			template< unsigned int N >
			struct get {
				static constexpr unsigned int value = std::get<N>(std::tuple(Ns...));
			};

			/**
			 * Map and convert each integer of the list ...Ns to the corresponding type of
			 * the corresponding Constraint_t parameter.
			 */
			template < typename Constraint_t >
			struct convert_to_type
			{
				// We shift with +1 because Tup is a constraint where first element is
				// the unique constraint id, the first parameter starts just after.
				using type = std::tuple< typename std::tuple_element<Ns+1,Constraint_t>::type... >;
			};

			/**
			 * Map and convert each integer of the list ...Ns to the corresponding Weak_t
			 * type Weak_t of the corresponding Constraint_t parameter.
			 */
			template < typename Constraint_t >
			struct convert_to_weak_type
			{
				// We shift with +1 because Tup is a constraint where first element is
				// the unique constraint id, the first parameter starts just after.
				using type = std::tuple< typename std::tuple_element<Ns+1,Constraint_t>::type::Weak_t... >;
			};

			/**
			 * Map and convert each integer of the list ...Ns to the corresponding value
			 * of the corresponding Constraint_t parameter.
			 */
			template < typename Constraint_t >
			struct map_to_constraint
			{
				using type = std::tuple< typename std::tuple_element<Ns+1,Constraint_t>::type... >;

				/**
				 * Map and convert each integer of the list ...Ns to the corresponding
				 * element (value) of the corresponding \a c constraint.
				 * @param c The constraint element (tuple)
				 */
				static type build(const Constraint_t& c)
				{
					return std::forward_as_tuple( std::get<Ns+1>(c)... );
				}
			};
		};
		
		/**
		 * Template structure used to manipulate the keys of an index.
		 * Key<> is based on a tuple where each element is the key of the corresponding
		 * constraint parameter used to design the key.
		 */
		template < typename... Ts >
		struct Key {
			static constexpr bool HAS_HASH = (chr::Has_hash<Ts>::value && ...);
			static constexpr bool IS_SCALAR = (std::is_scalar<typename Ts::Value_t>::value && ...);
		    using Key_t = std::tuple<typename Ts::Key_t...>;
			Key_t _k;	///< The k element, each element is a key for the corresponding Ts element
	
			static_assert(HAS_HASH, "the chr class chr::XXHash<> hasn't been specialized for one of the types involved in the key (refer to the CHRPP documentation to learn how to define it)");

			/**
			 * Check the validity of variables according to the key.
			 * This data structure is used to detect for one variable of the key if
			 * it is compatible with a given value.
			 * The ill-formed cases are:
			 * - if key is a Logical_var_ground and value is a not grounded Logical_var
			 * - if key is a Logical_var_ground and value is a Logical_var_mutable
			 * - if key is a Logical_var_mutable and value is a Logical_var_ground
			 * - if key is a Logical_var_mutable and value is a grounded Logical_var
			 */
			template < typename LVT1, typename LVT2 >
			struct check_validity_for_one_var {
				static bool value(const LVT2&) { return true; }
			};
			template < typename VT >
			struct check_validity_for_one_var< chr::Logical_var_ground<VT>, chr::Logical_var<VT> > {
				static bool value(const chr::Logical_var<VT>& v) { return v.ground(); }
			};
			template < typename VT >
			struct check_validity_for_one_var< chr::Logical_var_ground<VT>, chr::Logical_var_mutable<VT> > {
				static bool value(const chr::Logical_var<VT>&) { return false; }
			};
			template < typename VT >
			struct check_validity_for_one_var< chr::Logical_var_mutable<VT>, chr::Logical_var<VT> > {
				static bool value(const chr::Logical_var<VT>& v) { return !v.ground(); }
			};
			template < typename VT >
			struct check_validity_for_one_var< chr::Logical_var_mutable<VT>, chr::Logical_var_ground<VT> > {
				static bool value(const chr::Logical_var<VT>&) { return false; }
			};

			/**
			 * Check the validity of variables according to the key.
			 * For example, a not grounded Logical_var cannot be used to
			 * build a key on a Logical_var_ground. In this case, the definition
			 * is ill-formed.
			 * @param vars The list of logical variables
			 */
			template < typename... VTs >
			static bool check_validity(const VTs&... vars) {
				static_assert(sizeof...(VTs) == sizeof...(Ts));
				return (check_validity_for_one_var<Ts,VTs>::value(vars) && ...);
			}

			/**
			 * Build a key from a list of logical variables. We assume the each variable
			 * has a type compatible with the type of the key.
			 * @param vars The list of logical variables
			 */
			template < typename... VTs >
			static Key build_from_vars(std::tuple<VTs...> vars) {
				Key_t k(std::move(vars));
				return Key({k});
			}

			/**
			 * Build a key from a constraint and an index (given in template
			 * parameters). Each element of the key is built with the corresponding
			 * element of the constraint according to the index mapping.
			 * @param c The constraint
			 */
			template < typename Constraint_t, typename Index_t >
			static Key build_from_constraint(const Constraint_t& c) {
				using MTC = typename Index_t::template map_to_constraint<Constraint_t>;
				Key_t k = MTC::build(c);
				return Key({k});
			}

			/**
			 * Checks if two keys are equals.
			 * @param o The other key
			 * @return True if the two keys are equal false otherwise
			 */
			bool operator==(const Key<Ts...>& o) const
			{
				return _k == o._k;
			}

			/**
			 * Get the Nth element type of the key.
			 */
			template< unsigned int N >
			struct get {
				using type = typename std::tuple_element<N, std::tuple<Ts...>>::type::Key_t;
			};
	
			/**
			 * Hash data structure used to compute a hash from a key.
			 */
			struct Hash
			{
				/**
				 * Meta-programming loop to update a hash value with all elements
				 * of the key.
				 * @param a The key used to 
				 * @param I... The sequence of integer, one for each element of the key
				 */
				template< size_t... I >
				void update_hash(const Key< Ts... >& a, std::index_sequence<I...>) const
				{
					(chr::XXHash< typename std::tuple_element<I,Key_t>::type >::update(std::get<I>(a._k)), ...);
				}
	
				/**
				 * Compute a hash value for key \a key.
				 * @param a The key used to 
				 */
				chr::CHR_XXHASH_hash_t operator()(const Key< Ts... >& a) const
				{
					if constexpr (IS_SCALAR)
						return CHR_XXHash(a);
					else
					{
						CHR_XXHash_reset();
						update_hash(a, std::make_index_sequence<sizeof...(Ts)>());
						return CHR_XXHash_digest();
					}
				}
			};
		};

		
		/**
		 * Template structure which represents a callback for an update of any
		 * logical variable involved in the key.
		 * Constraint_store_t is the type of constraint store to remember. N_INDEX is
		 * the index number in the list of index of Constraint_store_t. N_UPDATED_VAR
		 * is the variable number of the key to consider (the variable watched for
		 * any update).
		 */
		template < typename Constraint_store_t, unsigned int N_INDEX, unsigned int N_UPDATED_VAR >
		struct IndexCallback : public chr::Logical_var_imp_observer_index {
			// Shortcuts to usefull types
			using Constraint_t = typename Constraint_store_t::Constraint_t;
			using Index = typename std::tuple_element<N_INDEX,typename Constraint_store_t::TupleIndexes>::type;
			using MTC = typename Index::template map_to_constraint<Constraint_t>;

			using Key_t = typename std::tuple_element<N_INDEX,typename Constraint_store_t::TupleKey_t>::type;
			using Index_weak_t = typename Index::template convert_to_weak_type<Constraint_t>::type;
			Index_weak_t _vars;							///< The list of logical variables involved in the key
			chr::Weak_obj< Constraint_store_t > _cs;	///< The constraint store used to store the constraint

			/**
			 * Build an index callback for the constraint \a c.
			 * @param c The constraint from which to draw the variables.
			 * @param cs The constraint store to remember
			 */
			IndexCallback(const Constraint_t& c, chr::Weak_obj<Constraint_store_t> cs)
				: _vars( MTC::build(c) ),
					_cs( std::move(cs) )
			{}

			/**
			 * Meta-programming loop which checks if any of the involved variable is
			 * expired.
			 * @param vars The list of logical variables
			 * @param I... The sequence of integer, one for each element of the key
			 * @return True if any variable involved in the key is expired, false otherwise
			 */
			template <size_t... I>
			bool expired(Index_weak_t vars, std::index_sequence<I...>) {
				return
					([&]{
						using Var_t = typename std::tuple_element<I,Index_weak_t>::type;
						if constexpr (!Var_t::LV_GROUND) {
							return std::get<I>(vars).expired();
						} else
							return false;
					}() || ...);
			}
			
			/**
			 * Update (callback) an index where a key must be updated because of the
			 * update of a logical variable. This function is only triggered for the
			 * update of NOT grounded variable where the key is built with memory
			 * variable addresses.
			 * @param old_key The old value of the key element which has been updated
			 * @return False if any element of the key is expired (the callback is not longer valid), true otherwise
			 */
			bool update_index(const void* old_key) {
				if ( _cs.expired() ||
					expired(_vars, std::make_index_sequence<std::tuple_size<Index_weak_t>::value>()) ) return false;

				Key_t to_key(Key_t::build_from_vars(_vars));
				Key_t from_key(to_key);
				using Updated_Key_t = typename Key_t::template get<N_UPDATED_VAR>::type;
				std::get<N_UPDATED_VAR>(from_key._k) = Updated_Key_t(old_key);
				_cs->template update_index<N_INDEX>(from_key, to_key);
				return true;
			}
		};
	};

	/**
	 * @brief Constraint store management with index
	 *
	 * A constraint store deals with a specific constraint type T.
	 * The store is a set of constraints (order is not ensured because of indexes splice).
	 * This constraint store uses indexes to speed up the match of constraints
	 * when a specific variable is involved.
	 * The list of indexes TupleIndexes is a tuple where each element is an Index<> structure.
	 * \ingroup Constraints
	 */
	template< typename T, typename TupleIndexes0, bool ENABLE_BACKTRACK0 = true >
	class Constraint_store_index : public Backtrack_observer
	{
	public:
		// Forward and friends classes
		typedef class Constraint_store_index_iterator_full< Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 > > iterator;
		typedef class Constraint_store_index_iterator_hash< Constraint_store_index< T, TupleIndexes0, ENABLE_BACKTRACK0 > > iterator_hash;
	private:
		friend class Constraint_store_index_iterator_full< Constraint_store_index<T,TupleIndexes0,ENABLE_BACKTRACK0> >;
		friend class Constraint_store_index_iterator_hash< Constraint_store_index<T,TupleIndexes0,ENABLE_BACKTRACK0> >;
		friend void tests::constraint_store::index_misc_tests();

	public:
		// Shortcuts to common types and constants
		using Constraint_t = T;
		using PID_t = typename chr::Bt_list<T>::PID_t;
		static constexpr PID_t END_LIST = chr::Bt_list<T>::END_LIST; // Shortcut to END_LIST
		using TupleIndexes = TupleIndexes0;
		static constexpr bool ENABLE_BACKTRACK = ENABLE_BACKTRACK0;
		static constexpr unsigned int INDEX_COUNT = std::tuple_size_v<TupleIndexes>; ///< Shortcut to template value INDEX_COUNT
		using Sublist_t = chr::Bt_list<PID_t,ENABLE_BACKTRACK,true,chr::Statistics::CONSTRAINT_STORE,std::allocator<PID_t>>;

		/**
		 * Data structure which represents a list of constraints. Each element is
		 * a reference to the constraint in the main store (the pid of the constraint in the _store list).
		 * The current backtrack_depth is also stored to make the store aware
		 * of the time when the index has been created.
		 */
		struct Index_element_t
		{
			chr::Depth_t _backtrack_depth;			///< The backtrack depth used to create this index
			std::unique_ptr< Sublist_t > _sublist;	///< The list of elements (constraints) for this index

			/**
			 * Initialize.
			 * @param backtrack_depth The backtrack depth value that is relevant when the index was created
			 */
			Index_element_t(chr::Depth_t backtrack_depth) : _backtrack_depth(backtrack_depth) { }

			/**
			 * Copy constructor set deleted.
			 */
			Index_element_t(const Index_element_t&) =delete;

			/**
			 * Move constructor.
			 * @param o Index_element_t value to use
			 */
			Index_element_t(Index_element_t&& o) noexcept : _backtrack_depth(o._backtrack_depth), _sublist( std::move(o._sublist) ) { }
		};
	
	private:
		/**
		 * Map and convert each Index<> of a tuple of ...Args to the corresponding
		 * type of tuple of parameter Constraint_t.
		 */
		template < typename... Args > struct index_value_to_type;
		template < typename... Args >
		struct index_value_to_type< std::tuple<Args...> >
		{
		    using type = std::tuple<typename Args::template convert_to_type<Constraint_t>::type...>;
		};

		/**
		 * Map and convert a list of type Ts... to a Key<Ts...>.
		 */
		template < typename... Ts > struct ConvertArgsToKey;
		template < typename... Ts >
		struct ConvertArgsToKey< std::tuple< Ts... > >{ 
		    using type = LNS::Key<Ts...>;
		};

		/**
		 * Map and convert a list (tuple) of index types to a list (tuple) of
		 * key types.
		 */
		template < typename... Ts > struct TupleKey;
		template < typename... Ts >
		struct TupleKey< std::tuple< Ts... > >{ 
		    using type = std::tuple<typename ConvertArgsToKey<Ts>::type...>;
		};

		/**
		 * Map and convert a list (tuple) of index types to a list (tuple) of
		 * robin_map types. Each map instanced form this type part will contain
		 * the index elements.
		 */
		template < typename... Ts > struct TupleMapIndex;
		template < typename... Ts >
		struct TupleMapIndex< std::tuple< Ts... > >{ 
		    using type = std::tuple< 
					tsl::robin_map<
						typename ConvertArgsToKey<Ts>::type,
						Index_element_t,
						typename ConvertArgsToKey<Ts>::type::Hash
					>
			... >;
		};

	public:
		// Types built with TupleIndexes (types, keys, maps)
		using TupleIndexes_t = typename index_value_to_type<TupleIndexes>::type;
		using TupleKey_t = typename TupleKey<TupleIndexes_t>::type;
		using TupleMap_t = typename TupleMapIndex<TupleIndexes_t>::type;

		/**
		 * Statistics about an instance of Constraint_store_index.
		 * Useful to verify the good behavior
		 */
		struct Statistics
		{
			size_t size;																///< Number of elements in the store
			size_t nb_pending;															///< Number of pending sublist waiting to be deleted
			std::vector< unsigned long int > constraints;								///< List of constraint id of the store
			std::array< std::map< CHR_XXHASH_hash_t,  std::vector< unsigned long int > >,
						INDEX_COUNT > indexes;											///< Indexes (Key = hash value, Values = constraints (id) that involve the hash
		};

		/**
		 * Initialize.
		 */
		Constraint_store_index(std::string_view label = "")
			:	Backtrack_observer(),
				_backtrack_scheduled(false),
				_backtrack_depth(Backtrack::depth()),
				_label(label)
		{ }

		/**
		 * Copy constructor: deleted.
		 * @param o The source Constraint_store
		 */
		Constraint_store_index(const Constraint_store_index& o) =delete;

		/**
		 * Assignment operator: deleted.
		 * @param o The source Constraint_store
		 * @return A reference to this
		 */
		Constraint_store_index& operator=(const Constraint_store_index& o) =delete;

		/**
		  * Destroy and free ressources.
		  */
		virtual ~Constraint_store_index();

		/**
		 * Returns the label of this constraint store.
		 * @return A string corresponding to the label.
		 */
		std::string_view label() const { return _label; }

		/**
		 * Check is the store is empty.
		 * @return True if the store is empty, false otherwise
		 */
		bool empty() const { return _store.empty(); }

		/**
		 * Return the number of constraints stores in this store
		 * @return The size of this store of constraints
		 */
		std::size_t size() const { return _store.size(); }

		/**
		 * Return the current constraint store backtrack depth.
		 * @return The current constraint store backtrack depth
		 */
		Depth_t depth() const { return _store.depth(); }

		/**
		 * Return the number of constraints stored in the store that
		 * matches the hash computed with \a args.
		 * The template parameter N_INDEX corresponds to the number of the index to
		 * use for the search.
		 * @return The number of constraints stored
		 */
		template < unsigned int N_INDEX, typename ...Args >
		std::size_t size(Args&& ... args) const;

		/**
		 * Check is the store is empty.
		 * @return True if the store is empty, false otherwise
		 */
		const T& get(PID_t pid) const { return _store.get(pid)._c; }

		/**
		 * Build and return an iterator on the first constraint
		 * of the store.
		 * @return An iterator on first element
		 */
		iterator begin();

		/**
		 * Build and return an iterator on the last constraint
		 * of the store. Usefull for the C++ for range loop!
		 * @return An iterator on first element
		 */
		iterator end();

		/**
		 * Build and return an iterator on the first constraint of the store that
		 * matches the hash computed with \a args.
		 * The returned iterator will iterate over all constraints of the store
		 * that match the same hash value.
		 * @param args The n-uple to search for
		 * @return An iterator on first element
		 */
		template < unsigned int N_INDEX, typename ...Args >
		iterator_hash begin(Args&& ... args);

		/**
		 * Add a constraint into the store.
		 * @param e The constraint to add
		 * @return An iterator on the new element
		 */
		iterator add(T e);

		/**
		 * Update the indexes of the store by moving elements from \a from_key
		 * to the new index given by \a to_key.
		 * @param from_key The previous key recorded
		 * @param to_key The new key to use instead of the previous one
		 */
		template < unsigned int N_INDEX >
		void update_index(typename std::tuple_element<N_INDEX,TupleKey_t>::type from_key, typename std::tuple_element<N_INDEX,TupleKey_t>::type to_key);

		/**
		 * Compute and return the statistics about this Constraint_store.
		 * @return A statistics object which gather all data
		 */
		Statistics index_statistics() const;

	private:
		bool _backtrack_scheduled;													///< Flag set to true if this constraint store registered for backtrack management
		Depth_t _backtrack_depth;													///< The current backtrack depth for this snapshot
		std::string _label;															///< Label of this constraint store (to print before each constraint)
		Bt_list<T,ENABLE_BACKTRACK,false,chr::Statistics::CONSTRAINT_STORE> _store;	///< The store of constraints.
		TupleMap_t _indexes;														///< The structure of indexes each member corresponds to a specific index.
		std::vector< std::unique_ptr<Sublist_t> > _pending_sl;						///< Sublist waiting for being deleted. They will be deleted when they will be safe

		/**
		 * Remove a constraint from the store (and all indexes).
		 * @param pid The pid of the constraint in the main store
		 */
		void remove(PID_t pid);

		/**
		 * Add a new sublist \a sl to the list of pending sublist that are
		 * waiting to be safe in order to be deleted.
		 * @param sl The sublist to add to pending list
		 */
		void add_to_pending_list(std::unique_ptr<Sublist_t>&& sl);

		/**
		 * Check if a rollback point must be prepared and schedule backtrack observer if needed.
		 */
		void prepare_rollback_point();

		/**
		 * Rewind to the snapshot of the constraint store stored at \a depth.
		 * All snapshots encountered along the path will be forgotten.
		 * @param previous_depth The previous depth before call to back_to function
		 * @param new_depth The new depth after call to back_to function
		 * @return False if the callback can be removed from the wake up list, true otherwise
		 */
		bool rewind(Depth_t previous_depth, Depth_t new_depth) override;

		/**
		 * Template Meta Programming loop to use compil time optimization.
		 * Schedule an index callback when a variable of index N_INDEX is updated.
		 * @param e The constraint used to gather the variables which schedule an update index callback
		 * @param I... The sequence of integer, one for each index
		 */
		template< unsigned int N_INDEX, size_t... I > void schedule_index_callback(const T& e, std::index_sequence<I...>);

		/**
		 * Template Meta Programming loop to use compil time optimization.
		 * @param e The constraint to add
		 * @param pid The pid (slot) of the constraint in main store
		 * @param I... The sequence of integer, one for each index
		 */
		template< size_t... I > void loop_add(const T& e, PID_t pid, std::index_sequence<I...>);

		/**
		 * Template Meta Programming loop to use compil time optimization.
		 * @param e The constraint to remove
		 * @param pid_e The pid (slot) of the constraint to remove in the main store
		 * @param I... The sequence of integer, one for each index
		 */
		template< size_t... I > void loop_remove(const T& e, PID_t pid_e, std::index_sequence<I...>);

		/**
		 * Template Meta Programming loop to use compil time optimization.
		 * @param new_depth The new depth to backtrack
		 * @param I... The sequence of integer, one for each index
		 */
		template< size_t... I > void loop_rewind(Depth_t new_depth, std::index_sequence<I...>);

		/**
		 * Template Meta Programming loop to use compil time optimization.
		 * @param stats The statistics to update
		 * @param I... The sequence of integer, one for each index
		 */
		template< size_t... I > void loop_statistics(Statistics& stats, std::index_sequence<I...>) const;

		/**
		 * Compute and return the statistics in a human readable form. Only for debug purposes.
		 * @return A string which gather statistics
		 */
		std::string str_index_statistics() const;

		/**
		 * Builds and returns a string representation of type TT
		 * @return A string a representation of type
		 */
		template< typename TT > std::string type_name() const;
	};

	/**
	 * @brief Iterator on a constraint store with index
	 *
	 * An iterator of a constraint store takes care of memory
	 * management of the store. It can read through the store
	 * of constraints to browse all living (non killed) constraints.
	 * \ingroup Constraints
	 */
	template< typename Constraint_store_t >
	class Constraint_store_index_iterator_full {
		friend class Constraint_store_index< typename Constraint_store_t::Constraint_t, typename Constraint_store_t::TupleIndexes, Constraint_store_t::ENABLE_BACKTRACK >;
	public:
		/**
		 * Return the current element pointed by the iterator.
		 * @return A const reference to the element pointed
		 */
		const typename Constraint_store_t::Constraint_t& operator*() const {
			return *_it;
		}

		/**
		 * Return the string conversion of the underlying constraint of this iterator
		 * @return The string
		 */
		std::string to_string() const {
			return std::string(_store.label()) + chr::TIW::constraint_to_string(*_it);
		}

		/**
		 * Move the iterator to the next element.
		 * @return A reference to the current iterator
		 */
		Constraint_store_index_iterator_full& operator++()
		{
			++_it;
			return *this;
		}

		/**
		 * Tell if the iterator is at the end of the constraint store or not.
		 * Remember, that the "end" corresponds to the end() marked when
		 * this iterator was created.
		 * @return True if there is still element to see, false otherwise
		 */
		bool at_end() const
		{
			return _it.at_end();
		}

		/**
		 * Lock the current underlying iterator.
		 * It helps to deal with unvalid iterator (because of the remove function)
		 */
		void lock()
		{
			_store.prepare_rollback_point();
			_it.lock();
		}

		/**
		 * Unlock the current underlying iterator.
		 * It helps to deal with unvalid iterator (because of the remove function)
		 */
		void unlock()
		{
			_store.prepare_rollback_point();
			_it.unlock();
		}

		/**
		 * Unlock and refresh the frozen underlying iterator.
		 * This function must be called to refresh iterators after
		 * a constraint remove because the underlying iterator
		 * is not valid anymore.
		 */
		void next_and_unlock()
		{
			_store.prepare_rollback_point();
			_it.next_and_unlock();
		}

		/**
		 * Tell if the element pointed by the iterator is alive or not.
		 * @return True if the underlying constraint is alive, false otherwise
		 */
		bool alive() const
		{
			return _it.valid();
		}

		/**
		 * Kill the current iterator.
		 * It only mark for kill, the real removing of element will
		 * be done when no iterator will point at it.
		 */
		void kill()
		{
			_store.remove(_it.pid());
		}

		/**
		 * Check if two iterators are equals.
		 * @return True if the two iterators are equals, false otherwise
		 */
		bool operator==(const Constraint_store_index_iterator_full& o) const
		{
			return _it == o._it;
		}

		/**
		 * Check if two iterators are not equals.
		 * @return True if the two iterators are not equals, false otherwise
		 */
		bool operator!=(const Constraint_store_index_iterator_full& o) const
		{
			return _it != o._it;
		}

		Constraint_store_index_iterator_full(const Constraint_store_index_iterator_full& o)
			: _store(o._store), _it(o._it)
		{}
		void operator=(Constraint_store_index_iterator_full&& o)
		{
			_it = std::move(o._it);
		}
	private:
		Constraint_store_t& _store;	///< Store of constraint
		typename Bt_list< typename Constraint_store_t::Constraint_t, Constraint_store_t::ENABLE_BACKTRACK, false, chr::Statistics::CONSTRAINT_STORE >::iterator _it;	///< The current iterator on the full list of constraints

		/**
		 * Initialize iterator to the beginning of the store.
		 * @param store The store of constraints
		 */
		Constraint_store_index_iterator_full(Constraint_store_t& store)
			: _store(store), _it(store._store.begin())
		{ }

		/**
		 * Initialize to a specific constraint of the store.
		 * @param store The store of constraints
		 * @param current The constraint to start with
		 */
		Constraint_store_index_iterator_full(Constraint_store_t& store, typename Bt_list< typename Constraint_store_t::Constraint_t, Constraint_store_t::ENABLE_BACKTRACK, false, chr::Statistics::CONSTRAINT_STORE >::iterator current)
			: _store(store), _it(current)
		{ }
	};

	/**
	 * @brief Iterator on a constraint store with index
	 *
	 * An iterator of a constraint store takes care of memory
	 * management of the store. It can read through the store
	 * of constraints to browse all living (non killed) constraints.
	 * \ingroup Constraints
	 */
	template< typename Constraint_store_t >
	class Constraint_store_index_iterator_hash {
		friend class Constraint_store_index< typename Constraint_store_t::Constraint_t, typename Constraint_store_t::TupleIndexes, Constraint_store_t::ENABLE_BACKTRACK >;
	public:
		/**
		 * Return the current element pointed by the iterator.
		 * @return A const reference to the element pointed
		 */
		const typename Constraint_store_t::Constraint_t& operator*() const {
			return _store._store.get(*_it);
		}

		/**
		 * Return the string conversion of the underlying constraint of this iterator
		 * @return The string
		 */
		std::string to_string() const {
			return std::string(_store.label()) + chr::TIW::constraint_to_string(_store._store.get(*_it));
		}

		/**
		 * Move the iterator to the next living element.
		 * @return A reference to the current iterator
		 */
		Constraint_store_index_iterator_hash& operator++()
		{
			++_it;
			return *this;
		}

		/**
		 * Tell if the iterator is at the end of the constraint store or not.
		 * Remember, that the "end" corresponds to the end() marked when
		 * this iterator was created.
		 * @return True if there is still element to see, false otherwise
		 */
		bool at_end() const
		{
			return _it.at_end();
		}

		/**
		 * Lock the current underlying iterator.
		 * It helps to deal with unvalid iterator (because of the remove function)
		 */
		void lock()
		{
			_store.prepare_rollback_point();
			_it.lock();
		}

		/**
		 * Unlock the current underlying iterator.
		 * It helps to deal with unvalid iterator (because of the remove function)
		 */
		void unlock()
		{
			_store.prepare_rollback_point();
			_it.unlock();
		}

		/**
		 * Unlock and refresh the frozen underlying iterator.
		 * This function must be called to refresh iterators after
		 * a constraint remove because the underlying iterator
		 * is not valid anymore.
		 */
		void next_and_unlock()
		{
			_store.prepare_rollback_point();
			_it.next_and_unlock();
		}

		/**
		 * Tell if the element pointed by the iterator is alive or not.
		 * @return True if the underlying constraint is alive, false otherwise
		 */
		bool alive() const
		{
			return _it.valid();
		}

		/**
		 * Kill the current iterator in the main constraint store.
		 * It also removes the current iterator of the indexed list.
		 */
		void kill()
		{
			_store.remove(*_it);
		}

		/**
		 * Check if two iterators are equals.
		 * @return True if the two iterators are equals, false otherwise
		 */
		bool operator==(const Constraint_store_index_iterator_hash& o) const
		{
			return _it == o._it;
		}

		/**
		 * Check if two iterators are not equals.
		 * @return True if the two iterators are not equals, false otherwise
		 */
		bool operator!=(const Constraint_store_index_iterator_hash& o) const
		{
			return _it != o._it;
		}

	private:
		Constraint_store_t& _store;								///< Store of constraint
		typename Constraint_store_t::Sublist_t::iterator _it;	///< The current iterator on the sublist of indexed constraints

		/**
		 * Initialize iterator to a given position of a sublist
		 * @param store The store of constraints
		 * @param it The initial pos of the iterator
		 */
		Constraint_store_index_iterator_hash(Constraint_store_t& store, typename Constraint_store_t::Sublist_t::iterator it)
			: _store(store), _it(it)
		{ }
	};

}

namespace std {
	/**
	 * @brief Overload of swap function to make robin_map happy
	 * @param v1 The first value
	 * @param v2 The second value
	 */
	template< typename U, typename... T >
	inline void swap(std::pair<typename chr::LNS::Key<T...>,U>& v1, std::pair<typename chr::LNS::Key<T...>,U>& v2) noexcept {
		std::swap(v1.first, v2.first);
		std::swap(v1.second._backtrack_depth, v2.second._backtrack_depth);
		std::swap(v1.second._sublist, v2.second._sublist);
	}
}

#include <constraint_store_index.hpp>

#endif /* RUNTIME_CONSTRAINT_STORE_INDEX_HH_ */
