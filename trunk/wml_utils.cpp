#include "wml_utils.hpp"

namespace wml {

std::vector<const_node_ptr> child_nodes(const const_node_ptr& ptr,
                                        const std::string& element)
{
	std::vector<const_node_ptr> res;
	if(!ptr) {
		return res;
	}

	wml::node::const_child_range range = ptr->get_child_range(element);
	while(range.first != range.second) {
		res.push_back(range.first->second);
		++range.first;
	}

	return res;
}

std::vector<node_ptr> child_nodes(const node_ptr& ptr,
                                  const std::string& element)
{
	std::vector<node_ptr> res;
	if(!ptr) {
		return res;
	}

	wml::node::child_range range = ptr->get_child_range(element);
	while(range.first != range.second) {
		res.push_back(range.first->second);
		++range.first;
	}

	return res;
}

}
