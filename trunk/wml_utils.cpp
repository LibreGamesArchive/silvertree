#include "wml_utils.hpp"

namespace wml {

node_ptr deep_copy(const_node_ptr ptr)
{
	node_ptr res(new node(ptr->name()));
	for(node::const_attr_iterator i = ptr->begin_attr();
	    i != ptr->end_attr(); ++i) {
		res->set_attr(i->first,i->second);
	}

	for(node::const_all_child_iterator i = ptr->begin_children();
	    i != ptr->end_children(); ++i) {
		res->add_child(deep_copy(*i));
	}

	return res;
}

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
