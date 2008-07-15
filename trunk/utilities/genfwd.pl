my $namespace = shift @ARGV;
my $class = shift @ARGV;
my $uclass = uc $class;

my $id = "$uclass" . "_FWD_HPP_INCLUDED";

print "#ifndef $id
#define $id

#include <boost/intrusive_ptr.hpp>

namespace $namespace {

class $class;
typedef boost::intrusive_ptr<$class> $class" . "_ptr;
typedef boost::intrusive_ptr<const $class> const_" . $class . "_ptr;

}

#endif
";
