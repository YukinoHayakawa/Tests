#include <string>
#include <vector>
#include <algorithm>
#include <random>

#include <fmt/format.h>

#include <Usagi/Utility/Allocator/PoolAllocator.hpp>

using namespace usagi;

int main(int argc, char *argv[])
{
    PoolAllocator<std::string> alloc;
    std::vector<std::size_t> indices;

    for(int i = 0; i < 100; ++i)
    {
        indices.push_back(i);
        alloc.allocate(fmt::format("a{}", i));
    }

    for(int i = 0; i < 100; ++i)
    {
        fmt::print("{}\n", alloc.block(i));
    }

    std::shuffle(
        indices.begin(), indices.end(),
        std::mt19937(std::random_device()())
    );

    for(int i = 0; i < 50; ++i)
    {
        alloc.deallocate(indices[i]);
    }

    for(int i = 0; i < 50; ++i)
    {
        alloc.allocate(fmt::format("b{}", i));
    }

    for(int i = 0; i < 100; ++i)
    {
        fmt::print("{}\n", alloc.block(i));
    }

    assert(alloc.numBlocks() == 100);

    return 0;
}
