/************************************************************************/
/*                                                                      */
/************************************************************************/
#ifndef _REDIS_ZMALLOC_H
#define _REDIS_ZMALLOC_H

#include <stddef.h>

/* Double expansion needed for stringification of macro values. */
#define __xstr(s) __str(s)
#define __str(s) #s

#if defined(USE_TCMALLOC)
#define ZMALLOC_LIB ("tcmalloc-" __xstr(TC_VERSION_MAJOR) "." __xstr(TC_VERSION_MINOR))
#include <google/tcmalloc.h>
#if (TC_VERSION_MAJOR == 1 && TC_VERSION_MINOR >= 6) || (TC_VERSION_MAJOR > 1)
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) tc_malloc_size(p)
#else
#error "Newer version of tcmalloc required"
#endif

#elif defined(USE_JEMALLOC)
#define ZMALLOC_LIB ("jemalloc-" __xstr(JEMALLOC_VERSION_MAJOR) "." __xstr(JEMALLOC_VERSION_MINOR) "." __xstr(JEMALLOC_VERSION_BUGFIX))
#include <jemalloc/jemalloc.h>
#if (JEMALLOC_VERSION_MAJOR == 2 && JEMALLOC_VERSION_MINOR >= 1) || (JEMALLOC_VERSION_MAJOR > 2)
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) je_malloc_usable_size(p)
#else
#error "Newer version of jemalloc required"
#endif

#elif defined(__APPLE__)
#include <malloc/malloc.h>
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) malloc_size(p)
#endif

#ifndef ZMALLOC_LIB
#define ZMALLOC_LIB "libc"
#endif

namespace redis{
	void *zmalloc(size_t size);
	void *zcalloc(size_t size);
	void *zrealloc(void *ptr, size_t size);
	void zfree(void *ptr);
	char *zstrdup(const char *s);
	size_t zmalloc_used_memory(void);
	void zmalloc_enable_thread_safeness(void);
	void zmalloc_set_oom_handler(void (*oom_handler)(size_t));
	float zmalloc_get_fragmentation_ratio(void);
	size_t zmalloc_get_rss(void);
	size_t zmalloc_get_private_dirty(void);
	void zlibc_free(void *ptr);

#ifndef HAVE_MALLOC_SIZE
	size_t zmalloc_size(void *ptr);
#endif

	template<class T>
	class ZAllocator{
	public:
		typedef T value_type;
		typedef value_type* pointer;
		typedef value_type& reference;
		typedef const value_type* const_pointer;
		typedef const value_type& const_reference;

		typedef size_t size_type;
		typedef ptrdiff_t difference_type;

		template<class _other>
		struct rebind
		{	
			typedef ZAllocator<_other> other;
		};

		pointer address(reference val) const
		{
			return (&val);
		}

		const_pointer address(const_reference val) const
		{	
			return (&val);
		}

		ZAllocator()
		{	
		}

		ZAllocator(const ZAllocator<T>&)
		{	
		}

		template<class other>
		ZAllocator(const ZAllocator<other>&)
		{
		}

		template<class other>
		ZAllocator<T>& operator=(const ZAllocator<other>&)
		{
			return (*this);
		}

		void deallocate(pointer ptr, size_type)
		{
			zfree(ptr);
		}

		pointer allocate(size_type count)
		{
			return reinterpret_cast<pointer>(zmalloc(sizeof(T) * count));
		}

		pointer allocate(size_type count, const void  *)
		{
			return allocate(count);
		}

		void construct(pointer ptr, const T& val)
		{	// construct object at ptr with value val
			::new (ptr) T(val);
		}

		void destroy(pointer ptr)
		{	
			zfree(ptr);
		}

		size_t max_size() const
		{
			return (size_t)(-1) / sizeof (T);
		}
	};

	template<class T,class other> inline
		bool operator==(const ZAllocator<T>&, const ZAllocator<other>&)
	{	
		return (true);
	}

	template<class T,class other> inline
		bool operator!=(const ZAllocator<T>&, const ZAllocator<other>&)
	{	
		return (false);
	}
}


#endif // _REDIS_ZMALLOC_H 
