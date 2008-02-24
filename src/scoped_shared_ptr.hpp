#ifndef SCOPED_SHARED_PTR_HPP_INCLUDED
#define SCOPED_SHARED_PTR_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace util {

template<typename T>
class scoped_shared_ptr
{
	boost::shared_ptr<T>& pointer_;
    boost::shared_ptr<T> cache_;

	//prohibited operations
	scoped_shared_ptr(const scoped_shared_ptr&);
	scoped_shared_ptr& operator=(const scoped_shared_ptr&);
public:
	typedef boost::shared_ptr<T> pointer_type;
    
    /**
     * Constructor
     *
     * @ param ptr This is the shared ptr to be managed
     * @ param value This is the scoped value for the pointer
     */
	scoped_shared_ptr(pointer_type& ptr, T* value)
        : pointer_(ptr), cache_(ptr) {
        pointer_.reset(value);
    }
    
    /**
     * The destructor is the main point in this class. It resets the
     * shared pointer to the state it found it in.
     */
	~scoped_shared_ptr()
	{
        pointer_ = cache_;
	}
    
    /**
     * This operator makes sure you can access and use the 
     * scoped_shared_ptr just like you were using the shared_ptr itself.
     *
     * @ret a reference to the wrapped shared_ptr
     */
	operator pointer_type&() const { return pointer_; }
    
    /**
     * This function provides explicit access to the resource. Its behaviour
     * is identical to operator pointer_type&()
     *
     * @ret a reference to the wrapped shared_ptr
     */
	pointer_type& get() const { return pointer_; }
    
	/**
     * This function provides convenient direct access to the -> operator
     * of the underlying shared_ptr.
     */
	pointer_type& operator->() const { return pointer_; }
};

}

#endif
