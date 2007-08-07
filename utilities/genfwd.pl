my $namespace = shift @ARGV;
my $class = shift @ARGV;
my $uclass = uc $class;

my $id = "$uclass" . "_FWD_HPP_INCLUDED";

print "#ifndef $id
#define $id

#include <boost/shared_ptr.hpp>

namespace $namespace {

class $class;
typedef boost::shared_ptr<$class> $class" . "_ptr;
typedef boost::shared_ptr<const $class> const_" . $class . "_ptr;

}

#endif
";
