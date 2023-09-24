#pragma once
#include <exception>
#include <functional>
#if defined(_WIN32)
#include <Windows.h>
#endif
#include <map>
#include <vector>
using namespace std;

namespace scg {

	using console_pos = unsigned int;
	using console_size = unsigned int;
	using array_size = unsigned int;
	using key_id = int;
	using symboller = unsigned long long;

#if defined(_WIN32)
	using win_handle = HANDLE;
	using win_uint = DWORD;
	using win_bool = BOOL;
#else
	using win_handle = void*;
	using win_uint = unsigned long;
	using win_bool = int;
#endif
	using pixel_color = int;

#if _DEBUG

	void noper() {
		// No operation for this function.
		1 + 1;
	}

#endif

	struct coords {

		// e.g. coords c = nullptr; For functions' arguments.
		coords(nullptr_t t) {

		}

		coords(console_pos x = 0, console_pos y = 0) : x(x), y(y) {

		}

		console_pos x, y;
	};

	coords operator + (coords x, coords y) {
		return coords(x.x + y.x, x.y + y.y);
	}

#if defined(_WIN32)
	class scg_exception : public exception {
	public:
		using exception::exception;
	};
#else
	class scg_exception : public exception {
	public:
		using exception::exception;
		scg_exception() {

		}
		scg_exception(string scg_info) : scg_info(scg_info) {

		}
		virtual const char* what() {
			return scg_info.c_str();
		}
	private:
		string scg_info;
	};
#endif

	template <typename data_type, typename field_type = data_type>
	class property {
	public:

		using my_getter = function<data_type(field_type&)>;
		using my_setter = function<void(data_type, field_type&)>;

		property(my_getter Get, my_setter Set) : getter(Get), setter(Set) {

		}

		void operator = (data_type TargetData) {
			setter(TargetData, fdata);
			//return (*this);
		}

		operator data_type() {
			return GetValue();
		}

		// For some reason operator data_type() can't be used properly.
		data_type GetValue() {
			return getter(fdata);
		}

		field_type fdata;

	private:
		my_getter getter;
		my_setter setter;
	};

	template <typename data_type>
	class array_2d {
	public:

#if __cplusplus >= 201700L
		static_assert(is_default_constructible<data_type>::value);
		static_assert(is_copy_constructible<data_type>::value);
#endif

		using __internal_data = vector<data_type>;
#if _DEBUG
		// Will check array's out-of-range exception
		class __array_helper {
		public:
			__array_helper(__internal_data &ind, array_size Size2D) : Size2D(Size2D), ind(ind) {

			}
			data_type& operator [] (array_size Pos) {
				if (Pos >= Size2D) {
					throw scg_exception("Debug error: Out of range");
				}
				return ind.at(Pos);
			}
		private:
			__internal_data &ind;
			array_size Size2D;
		};
#else
		using __array_helper = __internal_data & ;
#endif
		using array_internal = __array_helper;

		// Will only allocate new parts ...
		void Allocate(array_size Size1D, array_size Size2D) {
			array_size Origin1D = this->Size1D.fdata, Origin2D = this->Size2D.fdata;
			this->Size1D.fdata = Size1D;
			this->Size2D.fdata = Size2D;
			/*
			This map shows origin area and extra area.

			 +----------+-----+
			 | Origin   | E1  |
			 |          |     |
			 +----------+-----+
			 |   E2     | E3  |
			 +----------+-----+
			*/
			// E1
			for (array_size i = 0; i < Origin1D; i++) {
				for (array_size j = Origin2D; j < max(Size2D, Origin2D); j++) {
					ar[i].push_back(data_type());
				}
			}

			// E2+E3
			for (array_size i = Origin1D; i < max(Size1D, Origin1D); i++) {
				ar.push_back(vector<data_type>());
				for (array_size j = 0; j < max(Size2D, Origin2D); j++) {
					ar[i].push_back(data_type());
				}
			}
		}

		array_2d(array_size Size1D, array_size Size2D) {
			Allocate(Size1D, Size2D);
			/*
			// For debug propose only, test if a big size can satisfy it
			for (array_size i = 0; i < 100; i++) {
				ar.push_back(__internal_data());
				for (array_size j = 0; j < 100; j++)
					ar[i].push_back(data_type());
			}
			*/
		}

		array_2d(const array_2d &other) {
			this->ar = other.ar;
			this->Size1D.fdata = other.Size1D.fdata;
			this->Size2D.fdata = other.Size2D.fdata;
		}

		// Not recommended
		void Release() {
			ar.clear();
			ar.shrink_to_fit();
			Size1D = 0;
			Size2D = 0;
		}

		__array_helper operator[] (array_size Pos) {
			//return ar + (Pos*Size2D);
			if (Pos >= Size1D || Pos < 0) {
				throw scg_exception("Position out of range");
			}
#ifdef _DEBUG
			return __array_helper(ar.at(Pos), Size2D);
#else
			return ar.at(Pos);
#endif
		}

		void FillWith(const data_type &Data) {
			for (array_size i = 0; i < Size1D; i++) {
				for (array_size j = 0; j < Size2D; j++) {
					ar[i][j] = Data;	// ?
				}
			}
		}

		property<array_size> Size1D = property<array_size>([](array_size &field) -> array_size {
			return field;
		}, [](array_size set, array_size &field) {
			throw scg_exception("Cannot set this property!");
		}), Size2D = property<array_size>([](array_size &field) -> array_size {
			return field;
		}, [](array_size set, array_size &field) {
			throw scg_exception("Cannot set this property!");
		});

	private:
		vector<__internal_data> ar;	// ar[1D][2D]
		//data_type *ar;
	};


	class event_args {
	public:
		event_args() {

		}
	};

	template <typename event_arg_type = event_args>
	class event {

	public:

		using event_symbol = symboller;
		using event_caller = function<void(event_arg_type)>;

		event() {

		}

		event_symbol operator += (event_caller CallerToAdd) {
			cid++;
			event_map[cid] = CallerToAdd;
			return cid;
		}

		bool operator -= (event_symbol CallerToRemove) {
			if (!event_map.count(CallerToRemove)) return false;
			event_map.erase(CallerToRemove);
			return true;
		}

		void RunEvent(event_arg_type data) const {
			for (auto &i : event_map) {
				i.second(data);
			}
		}

		event(const event &other) : event_map(other.event_map), cid(other.cid) {

		}

	private:
		map<event_symbol, event_caller> event_map;
		event_symbol cid = 0;

	};

}