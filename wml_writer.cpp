#include "wml_node.hpp"
#include "wml_writer.hpp"

namespace wml
{

void write(const wml::const_node_ptr& node, std::string& res)
{
	std::string indent;
	write(node,res,indent);
}

void write(const wml::const_node_ptr& node, std::string& res,
           std::string& indent)
{
	res += indent + "[" + node->name() + "]\n";
	indent.push_back('\t');
	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		res += indent + i->first + "=\"" + i->second + "\"\n";
	}
	for(wml::node::const_all_child_iterator i = node->begin_children();
	    i != node->end_children(); ++i) {
		write(*i, res, indent);
	}
	indent.resize(indent.size()-1);
	res += indent + "[/" + node->name() + "]\n";
}

}
