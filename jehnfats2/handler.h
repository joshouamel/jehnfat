#pragma once
#include <vector>
#include <map>
template<class Key=unsigned __int32,class T=void>
class Handler {
public:
	inline Handler(Key min, Key max,Key null):min(min),max(max),current(min),null(null) {}
	inline Key alloc() {
		return alloc(T());
	}
	inline Key alloc(T a)
	{
		auto first = current;
			for (; v.find(current) != v.end(); ) {
				current++;
				if (current == max) {
					current = min;
				}
				if (first == current) {
					return null;
				}
			}
			v[current] = a;
			auto t=current++;
			if (current == max) {
				current = min;
			}
			return t;
		
	}

	inline bool free(Key hndl)
	{
		if (v.find(hndl) == v.end())return false;
		v.erase(hndl);
		return true;
	}
	std::map<Key, T> v;
	Key min, max, current, null;
private:
};