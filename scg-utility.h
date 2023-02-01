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

		class __array_helper {
		public:

			__array_helper(vector<data_type> &ar, array_size start) : ar(ar), start(start) {

			}

			data_type& operator [] (array_size Pos) {
				return ar[start + Pos];
			}

		private:
			vector<data_type> &ar;
			array_size start;
		};

		array_2d(array_size Size1D, array_size Size2D) {
			this->Size1D.fdata = Size1D;
			this->Size2D.fdata = Size2D;
			//ar = new data_type[Size1D * Size2D];
			ar.resize(Size1D * Size2D);
		}

		void Release() {
			ar.resize(0);
			Size1D = Size2D = 0;
		}

		__array_helper operator[] (array_size Pos) {
			//return ar + (Pos*Size2D);
			return __array_helper(this->ar, Pos*Size2D);
		}

		void FillWith(data_type Data) {
			for (size_t i = 0; i < Size1D * Size2D; i++) {
				ar[i] = Data;
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
		vector<data_type> ar;
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

		void RunEvent(event_arg_type data) {
			for (auto &i : event_map) {
				i.second(data);
			}
		}

	private:
		map<event_symbol, event_caller> event_map;
		event_symbol cid = 0;

	};

}