#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H

#include <allocator_guardant.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <logger_guardant.h>
#include <typename_holder.h>
#include <mutex>

class allocator_boundary_tags final :
	private allocator_guardant,
	public allocator_test_utils,
	public allocator_with_fit_mode,
	private logger_guardant,
	private typename_holder
{

private:

    void *_trusted_memory;

public:

    ~allocator_boundary_tags() override;

    allocator_boundary_tags(
	    allocator_boundary_tags const &other) = delete;

    allocator_boundary_tags &operator=(
	    allocator_boundary_tags const &other) = delete;

    allocator_boundary_tags(
	    allocator_boundary_tags &&other) noexcept;

    allocator_boundary_tags &operator=(
	    allocator_boundary_tags &&other) noexcept;

public:

    explicit allocator_boundary_tags(
	    size_t space_size,
	    allocator *parent_allocator = nullptr,
	    logger *logger = nullptr,
	    allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

public:

    [[nodiscard]] void *allocate(
	    size_t value_size,
	    size_t values_count) override;

    void deallocate(
	    void *at) override;

public:

    inline void set_fit_mode(
	    allocator_with_fit_mode::fit_mode mode) override;

private:

    inline allocator *get_allocator() const override;

public:

    std::vector<allocator_test_utils::block_info> get_blocks_info() const noexcept override;

private:

    inline logger *get_logger() const override;

    inline size_t get_global_size() const;

    inline fit_mode get_fit_mode() const;

    inline std::mutex *get_mutex() const;

private:

    size_t get_ancillary_space_size() const noexcept;

    inline std::string get_typename() const noexcept override;

    inline void *get_next_block_adress(void *block_adress) const noexcept;

    inline void *get_prev_block_adress(void *block_adress) const noexcept;

    inline size_t get_block_size(void *block_adress) const noexcept;

    inline allocator *get_parent_alc(void *block_adress) const noexcept;

    inline void *get_first_block() const noexcept;

    inline size_t get_block_meta_size() const noexcept;

    size_t get_diff_blocks(void *first_block, void *second_block) const noexcept;

    void set_first_block(void *adress) noexcept;

    void set_prev_block_adress(void *block_adress, void *prev_block_adress) noexcept;

    void set_next_block_adress(void *block_adress, void *next_block_adress) noexcept;

    void *get_memory_begining() const noexcept;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H