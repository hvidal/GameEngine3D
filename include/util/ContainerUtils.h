#ifndef GAMEDEV3D_CONTAINERUTILS_H
#define GAMEDEV3D_CONTAINERUTILS_H

#include <forward_list>


class ContainerUtils {
public:
	template<typename T>
	static std::size_t count(const std::forward_list<T>& list);
};

template<typename T>
std::size_t ContainerUtils::count(const std::forward_list<T>& list) {
	std::size_t count = 0;
	for (auto it = list.begin(); it != list.end(); ++it)
		count++;
	return count;
}

#endif
