#pragma once


template<typename Storage>
struct SchemaManager
{
	Storage& storage;

	SchemaManager(Storage& storage) : storage { storage}
	{
		storage.foreign_key(false);
	}
	~SchemaManager()
	{
		storage.foreign_key(true);
	}
private:
	// this section will become private
	template<typename Element>
	auto load_drop()
	{
		std::vector<Element> vec;
		bool exists = storage.table_exists(storage.tablename<Element>());
		if (exists)
		{
			vec = storage.get_all<Element>();
			storage.drop_table(storage.tablename<Element>());
		}
		return vec;
	}
	template<typename Vector>
	void replace(const Vector& vec)
	{
		storage.replace_range(vec.begin(), vec.end());
	}
public:
	template<typename Element>
	void load_drop_sync_replace()
	{
		auto vec = load_drop<Element>();
		storage.sync_schema(true);
		replace( vec);
	}
	void guarded_sync_schema()
	{
		storage.sync_schema(true);
	}

public:
	// Column
	void remove_column()
	{
		guarded_sync_schema();
	}
	void add_column()		// must be nullable or with default value - attempt to load throws exception
	{
		guarded_sync_schema();
	}
	// Foreign Keys
	template<typename Element>
	void remove_fk_constraint()		// change not recognized
	{
		load_drop_sync_replace<Element>();
	}
	template<typename Element>
	void add_fk_constraint()		// change not recognized
	{
		load_drop_sync_replace<Element>();
	}
	// Primary Keys
	void remove_pk_constraint()		// change not recognized
	{
		guarded_sync_schema();
	}
	template<typename Element>
	void add_pk_constraint()		// change not recognized
	{
		load_drop_sync_replace<Element>();
	}
	// Nulls
	template<typename Element>
	void remove_null_constraint()	// change recognized but sync_schema() loses all data
	{
		load_drop_sync_replace<Element>();
	}
	template<typename Element>
	void add_null_constraint()		// change recognized but sync_schema() loses all data
	{
		load_drop_sync_replace<Element>();
	}
	// Default values				
	template<typename Element>
	void remove_default_value()		// change recognized but sync_schema() loses all data
	{
		load_drop_sync_replace<Element>();
	}
	template<typename Element>
	void add_default_value()		// change recognized but sync_schema() loses all data
	{
		load_drop_sync_replace<Element>();
	}
	// generated_always_as
	template<typename Element>
	void remove_generated_always_as()		// change in sync_schema() conflict on column duplicate name
	{
		load_drop_sync_replace<Element>();
	}
	template<typename Element>
	void add_generated_always_as()			// change not recognized, sync_schema() does not loose
	{
		load_drop_sync_replace<Element>();
	}
	// Unique constraint
	template<typename Element>
	void remove_unique_constraint()		// change not recognized
	{
		load_drop_sync_replace<Element>();
	}
	template<typename Element>
	void add_unique_constraint()		// change not recognized
	{
		load_drop_sync_replace<Element>();
	}


	// Verify uniqueness general case
	template<typename Element, typename sort_lambda, typename equality_lambda>
	auto find_duplicate(sort_lambda& sorter, equality_lambda& equality_pred )
	{
		auto vec = storage.get_all<Element>();
		std::sort(vec.begin(), vec.end(), sorter);
		auto it = std::adjacent_find(vec.begin(), vec.end(), equality_pred);
		return std::make_pair(it != vec.end(), it);
	}

	// verify uniqueness column case
	template<typename type, auto type::* key_col>
	auto find_duplicate_in_column()
	{
		auto vec = storage.get_all<type>();
		std::sort(vec.begin(), vec.end(), sort_order<type, key_col>());
		auto it = std::adjacent_find(vec.begin(), vec.end(), sort_equal<type, key_col>());
		return std::make_pair(it != vec.end(), it);
	}

	// Check constraints
	template<typename Element>
	void remove_check_constraint()		// change not recognized
	{
		load_drop_sync_replace<Element>();
	}
	template<typename Element>
	void add_check_constraint()		// change not recognized
	{
		load_drop_sync_replace<Element>();
	}

private:
	template<typename type, auto type::* key_col>
	struct sort_order
	{
		bool operator()(const type& lhs, const type& rhs)
		{
			return lhs.*key_col < rhs.*key_col;
		}
	};

	template<typename type, auto type::* key_col>
	struct sort_equal
	{
		bool operator()(const type& lhs, const type& rhs)
		{
			return lhs.*key_col == rhs.*key_col;
		}
	};
};
