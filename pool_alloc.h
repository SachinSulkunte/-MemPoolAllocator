// Initialize the pool allocator with a set of block sizes appropriate for this application.

// Returns true on success, false on failure.
bool pool_init(const size_t* block_sizes, size_t block_size_count);

// Allocate n bytes.
// Returns pointer to allocate memory on success, NULL on failure.
void* pool_malloc(size_t n);

// Release allocation pointed to by ptr.
void pool_free(void* ptr);
