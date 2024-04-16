#include </home/mp_os/common/include/not_implemented.h>
#include <typeinfo>
#include "/home/mp_os/allocator/allocator_global_heap/include/allocator_global_heap.h"
#include "/home/mp_os/logger/logger/include/logger_guardant.h"

allocator_global_heap::allocator_global_heap(
    logger *logger) : _logger(logger)
{

}

allocator_global_heap::~allocator_global_heap()
{

}

allocator_global_heap::allocator_global_heap(
    allocator_global_heap &&other) noexcept : _logger(other._logger)
{
    other._logger = nullptr;
}

allocator_global_heap &allocator_global_heap::operator=(
    allocator_global_heap &&other) noexcept 
{
    if (this != &other) {
        _logger = other._logger;
        other._logger == nullptr;
    }

    return *this;
}

[[nodiscard]] void *allocator_global_heap::allocate(
    size_t value_size,
    size_t values_count)
{
    size_t requested_size = value_size * values_count;

    debug_with_guard("requested size: " + std::to_string(requested_size));
    void * pointer = nullptr;

    try {
        pointer = ::operator new(requested_size + sizeof(allocator*) + sizeof(size_t));
    }
    catch (std::bad_alloc const &ex) {
        error_with_guard("Can't allocate memory");
        throw;
    }

    *reinterpret_cast<allocator**>(pointer) = this;
    *reinterpret_cast<size_t *>(reinterpret_cast<unsigned char *>(pointer) + sizeof(allocator *)) = requested_size;
    
    information_with_guard("memory was allocated, requested_size: " + std::to_string(requested_size));
    return (reinterpret_cast<unsigned char *>(pointer) + sizeof(allocator*) + sizeof(size_t));
}

void allocator_global_heap::deallocate(
    void *at)
{
    void* to_del = reinterpret_cast<unsigned char *>(at) - sizeof(size_t) - sizeof(allocator*);
   
    if (*reinterpret_cast<allocator**>(to_del) != this) 
    {
        _logger->error("can't deallocate block from other instance");
        throw std::logic_error("can't deallocate block from other instance");
    }

    size_t block_size = *reinterpret_cast<size_t *>(reinterpret_cast<allocator**>(to_del) + 1);

    std::string result;
    unsigned char * ptr_for_debug = reinterpret_cast<unsigned char *>(at);
    for (int i = 0; i < block_size; ++i) {
        result += std::to_string(ptr_for_debug[i]);
    }

    debug_with_guard("array of bytes: " + result);
    information_with_guard("memory was deallocated");
    ::operator delete(to_del);
}

inline logger *allocator_global_heap::get_logger() const
{
    return _logger;
}

inline std::string allocator_global_heap::get_typename() const noexcept
{
    return typeid(*this).name();
}