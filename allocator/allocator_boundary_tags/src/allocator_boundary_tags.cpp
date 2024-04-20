#include "allocator_boundary_tags.h"
#include <in6addr.h>
#include <mutex>
#include <not_implemented.h>
#include <regex>

allocator_boundary_tags::~allocator_boundary_tags()
{

}

allocator_boundary_tags::allocator_boundary_tags(
	allocator_boundary_tags &&other) noexcept: _trusted_memory(other._trusted_memory)
{
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
	allocator_boundary_tags &&other) noexcept
{
    if (this != &other)
    {
	_trusted_memory = other._trusted_memory;
	other._trusted_memory = nullptr;
    }

    return *this;
}

allocator_boundary_tags::allocator_boundary_tags(
	size_t space_size,
	allocator *parent_allocator,
	logger *logger,
	allocator_with_fit_mode::fit_mode allocate_fit_mode)
{

    if (space_size < get_block_meta_size())
    {
	throw std::logic_error("can't initialize allocator instance");
    }

    auto common_size = space_size + get_ancillary_space_size();
    try
    {
	_trusted_memory = parent_allocator == nullptr
			  ? ::operator new(common_size)
			  : parent_allocator->allocate(common_size, 1);
    }
    catch (std::bad_alloc const &ex)
    {
	throw;
    }

    allocator **parent_allocator_space_address = reinterpret_cast<allocator **>(_trusted_memory);
    *parent_allocator_space_address = parent_allocator;

    class logger **logger_space_address = reinterpret_cast<class logger **>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    size_t *space_size_space_address = reinterpret_cast<size_t *>(logger_space_address + 1);
    *space_size_space_address = space_size;

    allocator_with_fit_mode::fit_mode *fit_mode_space_address = reinterpret_cast<allocator_with_fit_mode::fit_mode *>(
	    space_size_space_address + 1);
    *fit_mode_space_address = allocate_fit_mode;

    void **first_block_address_space_address = reinterpret_cast<void **>(fit_mode_space_address + 1);
    *first_block_address_space_address = nullptr;

    std::mutex *_mutex = reinterpret_cast<std::mutex *>(first_block_address_space_address + 1);
    construct(_mutex);

    debug_with_guard("Global META was defined");
}

[[nodiscard]] void *allocator_boundary_tags::allocate(
	size_t value_size,
	size_t values_count)
{
    debug_with_guard("void * allocate started");
    std::mutex *_mutex = get_mutex();
    std::lock_guard<std::mutex> lock(*_mutex);

    auto requested_size = value_size * values_count;

    auto requested_block_size = requested_size + get_block_meta_size();

    auto fit_mode = get_fit_mode();

    auto global_size = get_global_size();

    void *prev_block = get_first_block();

    if (prev_block == nullptr && requested_size <= global_size - get_block_meta_size())
    {
	void *block_pointer = get_memory_begining();

	*reinterpret_cast<size_t *>(block_pointer) = requested_size;
	*reinterpret_cast<void **>(reinterpret_cast<u_char *>(block_pointer) + sizeof(size_t)) = nullptr;
	*reinterpret_cast<void **>(reinterpret_cast<u_char *>(block_pointer) + sizeof(size_t) +
				   sizeof(void *)) = nullptr;
	*reinterpret_cast<allocator **>(reinterpret_cast<u_char *>(block_pointer) + sizeof(size_t) + sizeof(void *) +
					sizeof(void *)) = this;
	set_first_block(block_pointer);

	debug_with_guard("block meta was defined; \nALLOCATED block size: " +
			 std::to_string(*reinterpret_cast<size_t *>(block_pointer))
			 + "\nallocated_BLOCK_size = " + std::to_string(requested_block_size));

	return (reinterpret_cast<u_char *>(block_pointer) + get_block_meta_size());
    } else if (prev_block == nullptr && requested_size > global_size - get_block_meta_size())
    {
	error_with_guard("can't allocate [ " + std::to_string(requested_size) + " ], max available size: " +
			 std::to_string(global_size - get_block_meta_size()));
	throw std::bad_alloc();
    }

    void *current_block = get_next_block_adress(prev_block);

    void *target = nullptr;

    void *prev_to_target = nullptr;

    void *next_to_target = nullptr;

    size_t target_size;

    size_t size_for_best = 10e9;

    size_t size_for_worst = 1;

    void *pointer_free_block;

    if (prev_block != get_memory_begining())
    {
	pointer_free_block = get_memory_begining();
	size_t diff_size = reinterpret_cast<u_char *>(prev_block) -
			   reinterpret_cast<u_char *>(get_memory_begining());
	size_for_best = diff_size;
	size_for_worst = diff_size;
	if (fit_mode == allocator_with_fit_mode::fit_mode::first_fit &&
	    requested_block_size <= diff_size)
	{
	    *reinterpret_cast<size_t *>(pointer_free_block) = requested_size;
	    set_prev_block_adress(pointer_free_block, nullptr);
	    set_next_block_adress(pointer_free_block, prev_block);
	    set_first_block(pointer_free_block);
	    *reinterpret_cast<allocator **>(reinterpret_cast<u_char *>(pointer_free_block) + sizeof(size_t) +
					    sizeof(void *) + sizeof(void *)) = this;

	    return (reinterpret_cast<u_char *>(pointer_free_block) +
		    get_block_meta_size());
	} else if (requested_size <= diff_size)
	{
	    size_for_best = diff_size;
	    size_for_worst = diff_size;
	    target = pointer_free_block;
	    prev_to_target = nullptr;
	    next_to_target = prev_block;
	}
    }

    size_t size_free_block;

    while (prev_block != nullptr)
    {
	if (current_block != nullptr && (reinterpret_cast<u_char *>(current_block) ==
					 reinterpret_cast<u_char *>(prev_block) + get_block_meta_size() +
					 get_block_size(prev_block)))
	{
	    prev_block = current_block;
	    current_block = get_next_block_adress(current_block);
	    continue;
	}

	if (current_block != nullptr)
	{
	    size_free_block = reinterpret_cast<u_char *>(current_block) - reinterpret_cast<u_char *>(prev_block) +
			      get_block_meta_size() + get_block_size(prev_block);
	} else
	{
	    if ((reinterpret_cast<u_char *>(prev_block) + get_block_meta_size() + get_block_size(prev_block)) +
		requested_block_size > reinterpret_cast<u_char *>(get_memory_begining()) + global_size)
	    {
		error_with_guard("bad_alloc; last block too small babe :(");
		throw std::bad_alloc();
	    }
	    size_free_block = reinterpret_cast<u_char *>(get_memory_begining()) + global_size -
			      ((reinterpret_cast<u_char *>(prev_block) + get_block_meta_size() +
				get_block_size(prev_block)));
	}

	pointer_free_block = reinterpret_cast<void *>(reinterpret_cast<u_char *>(prev_block) + get_block_meta_size() +
						      get_block_size(prev_block));

	if (fit_mode == allocator_with_fit_mode::fit_mode::first_fit && requested_block_size <= size_free_block)
	{
	    debug_with_guard("iteration; fit_mode: first-fit");
	    target = pointer_free_block;
	    prev_to_target = prev_block;
	    next_to_target = current_block;
	    target_size = size_free_block;
	    break;
	} else if (fit_mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
		   requested_block_size + get_block_meta_size() <= size_free_block)
	{
	    debug_with_guard("iteration; fit_mode: best-fit");
	    if (size_for_best > size_free_block)
	    {
		size_for_best = size_free_block;
		target = pointer_free_block;
		next_to_target = current_block;
		prev_to_target = prev_block;
		target_size = size_free_block;
	    }
	} else if (fit_mode == allocator_with_fit_mode::fit_mode::the_worst_fit &&
		   requested_block_size + get_block_meta_size() <= size_free_block)
	{
	    debug_with_guard("iteration; fit_mode: worst-fit");
	    if (size_for_worst < size_free_block)
	    {
		size_for_worst = size_free_block;
		target = pointer_free_block;
		next_to_target = current_block;
		prev_to_target = prev_block;
		target_size = size_free_block;
	    }
	}

	prev_block = current_block;
	current_block = get_next_block_adress(current_block);
    }

    if (target == nullptr)
    {
	error_with_guard("can't find block!!");
	throw std::bad_alloc();
    }

    *reinterpret_cast<size_t *>(target) = requested_size;
    *reinterpret_cast<void **>(reinterpret_cast<u_char *>(target) + sizeof(size_t)) = prev_block;
    *reinterpret_cast<void **>(reinterpret_cast<u_char *>(target) + sizeof(size_t) + sizeof(void *)) = current_block;
    *reinterpret_cast<allocator **>(reinterpret_cast<u_char *>(target) + sizeof(size_t) + 2 * sizeof(void *)) = this;

    if (next_to_target == nullptr)
    {
	set_next_block_adress(prev_to_target, target);
	set_prev_block_adress(target, prev_block);
    }
    else
    {
	set_prev_block_adress(next_to_target, target);
	set_next_block_adress(prev_to_target, target);
	set_prev_block_adress(target, prev_to_target);
	set_next_block_adress(target, next_to_target);
    }

    debug_with_guard("size SETTED: [ " + std::to_string(*reinterpret_cast<size_t *>(target)) + " ]");
    return (reinterpret_cast<u_char *>(target) + get_block_meta_size());
}

void allocator_boundary_tags::deallocate(
	void *at)
{
    std::mutex *_mutex = get_mutex();
    std::lock_guard<std::mutex> lock(*_mutex);

    void *to_deallocate = reinterpret_cast<u_char *>(at) - get_block_meta_size();

    allocator *parent_alc = get_parent_alc(to_deallocate);

    if (parent_alc != this)
    {
	debug_with_guard("can't deallocate POINTER from another INSTANCE!!!");
	throw std::logic_error("can't deallocate POINTER from another INSTANCE!!!");
    }

    void *prev_to_at = get_prev_block_adress(to_deallocate);

    void *next_to_at = get_next_block_adress(to_deallocate);

    if (prev_to_at != nullptr && next_to_at != nullptr)
    {
	set_next_block_adress(prev_to_at, next_to_at);
	set_prev_block_adress(next_to_at, prev_to_at);
    }
    else if (prev_to_at != nullptr && next_to_at == nullptr)
    {
	set_next_block_adress(prev_to_at, nullptr);
    }
    else if (prev_to_at == nullptr && next_to_at != nullptr)
    {
	set_first_block(next_to_at);
	set_prev_block_adress(next_to_at, nullptr);
    }
    else if (prev_to_at == nullptr && next_to_at == nullptr)
    {
	set_first_block(nullptr);
    }

    debug_with_guard("deallocate ENDED( implemented yet)");
}

inline void allocator_boundary_tags::set_fit_mode(
	allocator_with_fit_mode::fit_mode mode)
{
    *reinterpret_cast<fit_mode *>(reinterpret_cast<u_char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t));
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const noexcept
{
    debug_with_guard("get_blocks_info STARTED");

    std::vector<allocator_test_utils::block_info> result;

    void *target_block = get_first_block();

    void *next_block = get_next_block_adress(target_block);

    if (get_first_block() != get_memory_begining())
    {
	size_t block_size = reinterpret_cast<u_char *>(target_block) - reinterpret_cast<u_char *>(get_memory_begining());
	result.push_back(allocator_test_utils::block_info{block_size, false});
	debug_with_guard("FREE; blok size: " + std::to_string(block_size));
    }

    while (target_block != nullptr)
    {
	size_t free_block_size;

	if (next_block != nullptr)
	{
	    free_block_size = reinterpret_cast<u_char *>(next_block) -
			      (reinterpret_cast<u_char *>(target_block) + get_block_meta_size() +
			       get_block_size(target_block));
	} else
	{
	    free_block_size = (reinterpret_cast<u_char *>(get_memory_begining()) + get_global_size())
			      - (reinterpret_cast<u_char *>(target_block) + get_block_size(target_block) +
				 get_block_meta_size());
	}

	result.push_back(allocator_test_utils::block_info{get_block_size(target_block), true});
	debug_with_guard("OCCUPIED; blok size: [" + std::to_string(get_block_size(target_block)) + " ]");

	result.push_back(allocator_test_utils::block_info{free_block_size, false});

	target_block = next_block;

	if (next_block != nullptr)
	{
	    next_block = get_next_block_adress(next_block);
	}
    }

    debug_with_guard("get_blocks_info ENDED");
    return result;
}


inline allocator *allocator_boundary_tags::get_allocator() const
{
    return *reinterpret_cast<allocator **>(_trusted_memory);
}

inline logger *allocator_boundary_tags::get_logger() const
{
    return *reinterpret_cast<class logger **>(reinterpret_cast<u_char *>(_trusted_memory) + sizeof(allocator *));
}

inline std::string allocator_boundary_tags::get_typename() const noexcept
{
    return "allocator_boundary_tags";
}

size_t allocator_boundary_tags::get_ancillary_space_size() const noexcept
{
    return sizeof(logger *) + sizeof(allocator *) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) +
	   sizeof(std::mutex) + sizeof(void *);
}

size_t allocator_boundary_tags::get_global_size() const
{
    return *reinterpret_cast<size_t *>(reinterpret_cast<u_char *>(_trusted_memory)
				       + sizeof(allocator *) + sizeof(logger *));
}

allocator_with_fit_mode::fit_mode allocator_boundary_tags::get_fit_mode() const
{
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<u_char *>(_trusted_memory) +
      sizeof(allocator *) + sizeof(logger *) + sizeof(size_t));
}

std::mutex *allocator_boundary_tags::get_mutex() const
{
    return reinterpret_cast<std::mutex *>(reinterpret_cast<u_char *>(_trusted_memory)
					  + sizeof(allocator *)
					  + sizeof(logger *) + sizeof(size_t) + sizeof(fit_mode) + sizeof(void *));
}

void *allocator_boundary_tags::get_next_block_adress(void *block_adress) const noexcept
{
    return *reinterpret_cast<void **>(reinterpret_cast<u_char *>(block_adress)
				      + sizeof(size_t) + sizeof(void *));
}

void *allocator_boundary_tags::get_prev_block_adress(void *block_adress) const noexcept
{
    return *reinterpret_cast<void **>(reinterpret_cast<u_char *>(block_adress) + sizeof(size_t));
}

void allocator_boundary_tags::set_next_block_adress(void *block_adress, void *next_block_adress) noexcept
{
    *reinterpret_cast<void **>(reinterpret_cast<u_char *>(block_adress)
			       + sizeof(size_t) + sizeof(void *)) = next_block_adress;
}

void allocator_boundary_tags::set_prev_block_adress(void *block_adress, void *prev_block_adress) noexcept
{
    *reinterpret_cast<void **>(reinterpret_cast<u_char *>(block_adress) + sizeof(size_t)) = prev_block_adress;
}

size_t allocator_boundary_tags::get_block_size(void *block_adress) const noexcept
{
    return *reinterpret_cast<size_t *>(block_adress);
}

size_t allocator_boundary_tags::get_diff_blocks(void *first_block, void *second_block) const noexcept
{
    return get_block_size(first_block);
}

allocator *allocator_boundary_tags::get_parent_alc(void *block_adress) const noexcept
{
    return *reinterpret_cast<allocator **>(reinterpret_cast<u_char *>(block_adress)
					   + sizeof(size_t) + sizeof(void *) + sizeof(void *));
}

void *allocator_boundary_tags::get_first_block() const noexcept
{
    return *reinterpret_cast<void **>(reinterpret_cast<u_char *>(_trusted_memory)
				      + sizeof(allocator *) + sizeof(logger *)
				      + sizeof(size_t) + sizeof(fit_mode));
}


void *allocator_boundary_tags::get_memory_begining() const noexcept
{
    return reinterpret_cast<void *>(reinterpret_cast<u_char *>(_trusted_memory)
				    + sizeof(allocator *) + sizeof(logger *)
				    + sizeof(size_t) + sizeof(fit_mode)
				    + sizeof(void *) + sizeof(std::mutex));
}

void allocator_boundary_tags::set_first_block(void *adress) noexcept
{
    *reinterpret_cast<void **>(reinterpret_cast<u_char *>(_trusted_memory)
			       + sizeof(allocator *) + sizeof(logger *)
			       + sizeof(size_t) + sizeof(fit_mode)) = adress;
}

size_t allocator_boundary_tags::get_block_meta_size() const noexcept
{
    return sizeof(size_t) + 2 * sizeof(void *) + sizeof(allocator *);
}