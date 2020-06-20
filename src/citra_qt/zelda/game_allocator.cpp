// This is an accurate reimplementation of the memory allocator in MM3D.

#include <cstdio>
#include <cstring>

#include "citra_qt/zelda/game_allocator.h"
#include "citra_qt/zelda/game_types.h"
#include "common/alignment.h"

namespace zelda::game {

Ptr<void> Allocator::Alloc(u32 size, Ptr<const char> name) {
    // nn::os::ScopedLock<nn::os::CriticalSection>
    Ptr<void> ptr = nullptr;
    while (true) {
        ptr = small_block_threshold > size ? AllocSmall(size) : AllocLarge(size);
        if (ptr)
            break;
        if (!FreeUnusedRefCountedBlock(size))
            return ptr;
    }

    std::memset(ptr.get(), 0xCC, size);

    auto block = AllocatorBlock::FromData(ptr);
    block->next_free_s = nullptr;
    block->next_free_l = nullptr;
    block->name = nullptr; // yes, this is useless
    block->name = name;
    block->magic = 0x12345678ABCDEF86; // should be 'S.Moriya'
    block->alloc_ticks = -1;           // should be nn::os::GetSystemTick()

    if (average_alloc_size)
        average_alloc_size = (average_alloc_size + size) / 2;
    else
        average_alloc_size = size;

    return ptr;
}

Ptr<void> Allocator::AllocRefCounted(u32 id, u32 size) {
    // nn::os::ScopedLock<nn::os::CriticalSection>
    Ptr<void> ptr = nullptr;
    while (true) {
        ptr = AllocLarge(size);
        if (ptr)
            break;
        if (!FreeUnusedRefCountedBlock(size))
            return ptr;
    }

    auto block = AllocatorBlock::FromData(ptr);
    block->id = id;
    block->flags = AllocatorBlock::Flag::Flag_IsRefCounted;
    block->name = nullptr;
    block->alloc_ticks = -1; // should be nn::os::GetSystemTick()

    if (average_alloc_size)
        average_alloc_size = (average_alloc_size + size) / 2;
    else
        average_alloc_size = size;

    // Update the linked list of reference counted blocks.
    dummy_block.refcounted_next->refcounted_prev = block;
    block->refcounted_next = dummy_block.refcounted_next;
    block->refcounted_prev = GetDummyBlock();
    dummy_block.refcounted_next = block;

    return ptr;
}

void Allocator::Free(Ptr<void> ptr) {
    if (!ptr)
        return;

    // nn::os::ScopedLock<nn::os::CriticalSection>

    auto block = AllocatorBlock::FromData(ptr);
    if (block->flags & AllocatorBlock::Flag_IsRefCounted) {
        if (block->ref_count != 0) {
            --block->ref_count;

            if (block->ref_count == 0 &&
                block->flags & AllocatorBlock::Flag_FreeBlockOnRefCountZero) {
                block->refcounted_next->refcounted_prev = block->refcounted_prev;
                block->refcounted_prev->refcounted_next = block->refcounted_next;
                block->refcounted_next = nullptr;
                block->refcounted_prev = nullptr;
                DoFree(ptr);
            }
        }
    } else {
        DoFree(ptr);
    }
}

Ptr<void> Allocator::AllocSmall(u32 size) {
    auto block = dummy_block.next_free_s;
    const int alloc_size = Common::AlignUp(size + sizeof(AllocatorBlock), 16);

    // Find a block that has enough free space.
    while (true) {
        if (block->size - alloc_size >= 0)
            break;

        block = block->next_free_s;
        if (block == GetDummyBlock())
            return nullptr;
    }

    const int new_free_size = block->size - alloc_size;

    if (block->size == alloc_size || new_free_size <= int(sizeof(AllocatorBlock))) {
        // No space to make a new block.
        block->next_free_s->next_free_l = block->next_free_l;
        block->next_free_l->next_free_s = block->next_free_s;
        block->id = 0;
        block->flags = AllocatorBlock::Flag(0);
        block->ref_count = 1;
        block->size = -block->size;
        return AllocatorBlock::Data(block);
    }

    // Split the free block.
    Ptr<AllocatorBlock> new_block{block.addr + new_free_size};
    new_block->id = 0;
    new_block->flags = AllocatorBlock::Flag(0);
    new_block->ref_count = 1;
    new_block->prev = block;
    new_block->size = -alloc_size;
    block->size = new_free_size;

    block->next->prev = new_block;
    new_block->next = block->next;
    block->next = new_block;

    ++block_count;
    return AllocatorBlock::Data(new_block);
}

Ptr<void> Allocator::AllocLarge(u32 size) {
    auto block = dummy_block.next_free_l;
    const int alloc_size = Common::AlignUp(size + sizeof(AllocatorBlock), 16);

    // Find a block that has enough free space.
    while (true) {
        if (block->size - alloc_size >= 0)
            break;

        block = block->next_free_l;
        if (block == GetDummyBlock())
            return nullptr;
    }

    const int new_free_size = block->size - alloc_size;

    if (block->size == alloc_size || new_free_size <= int(sizeof(AllocatorBlock))) {
        // No space to make a new block.
        block->next_free_s->next_free_l = block->next_free_l;
        block->next_free_l->next_free_s = block->next_free_s;
        block->id = 0;
        block->flags = AllocatorBlock::Flag(0);
        block->ref_count = 1;
        block->size = -block->size;
        return AllocatorBlock::Data(block);
    }

    // Split the free block.
    Ptr<AllocatorBlock> new_block{block.addr + alloc_size};

    block->next_free_s->next_free_l = block->next_free_l;
    block->next_free_l->next_free_s = block->next_free_s;
    block->id = 0;
    block->flags = AllocatorBlock::Flag(0);
    block->ref_count = 1;
    block->size = -alloc_size;

    new_block->id = 0;
    new_block->flags = AllocatorBlock::Flag(0);
    new_block->ref_count = 1;
    new_block->prev = block;
    new_block->size = new_free_size;

    block->next->prev = new_block;
    new_block->next = block->next;
    block->next = new_block;

    UpdateFreeLists(new_block);

    ++block_count;
    return AllocatorBlock::Data(block);
}

void Allocator::DoFree(Ptr<void> ptr) {
    auto block = AllocatorBlock::FromData(ptr);

    // Clear block memory.
    const int free_size = block->size + sizeof(AllocatorBlock);
    if (-free_size > 0) {
        std::memset(ptr.get(), 0xDE, -free_size);
    }

    block->name = nullptr;
    block->size = -block->size;

    UpdateFreeLists(block);
    TryToMergeBlock(block);
}

bool Allocator::FreeUnusedRefCountedBlock(u32 required_size) {
    auto block = dummy_block.refcounted_prev;
    const int alloc_size = required_size + sizeof(AllocatorBlock);
    while (true) {
        if (block == GetDummyBlock())
            return false;

        if (!(block->flags & AllocatorBlock::Flag_PreventReuse) && block->size < 0 &&
            block->ref_count == 0) {
            // Erase this block from the list.
            block->refcounted_next->refcounted_prev = block->refcounted_prev;
            block->refcounted_prev->refcounted_next = block->refcounted_next;
            block->refcounted_next = nullptr;
            block->refcounted_prev = nullptr;

            // Clear block memory.
            const int free_size = block->size + sizeof(AllocatorBlock);
            if (-free_size > 0) {
                std::memset(AllocatorBlock::Data(block).get(), 0xDE, -free_size);
            }

            block->name = nullptr;
            block->size = -block->size;

            UpdateFreeLists(block);
            if (TryToMergeBlock(block)->size >= alloc_size)
                return true;
        }

        block = block->refcounted_prev;
    }
}

void Allocator::UpdateFreeLists(Ptr<AllocatorBlock> block) {
    auto free_block = block->prev;
    while (free_block->size <= 0)
        free_block = free_block->prev;

    if (free_block == block) {
        dummy_block.next_free_s->next_free_l = block;
        block->next_free_l = GetDummyBlock();
        block->next_free_s = dummy_block.next_free_s;
        dummy_block.next_free_s = block;
    } else {
        free_block->next_free_l->next_free_s = block;
        block->next_free_s = free_block;
        block->next_free_l = free_block->next_free_l;
        free_block->next_free_l = block;
    }
}

Ptr<AllocatorBlock> Allocator::TryToMergeBlock(Ptr<AllocatorBlock> block) {
    const auto merge_into = [this](Ptr<AllocatorBlock> target, Ptr<AllocatorBlock> other) {
        target->size += other->size;

        other->next_free_s->next_free_l = other->next_free_l;
        other->next_free_l->next_free_s = other->next_free_s;

        other->prev->next = other->next;
        other->next->prev = other->prev;
        --block_count;
    };

    auto next = block->next;
    if (next->size > 0 && block.addr + block->size == next.addr)
        merge_into(block, next);

    auto prev = block->prev;
    if (prev->size > 0 && block.addr - prev->size == prev.addr)
        merge_into(prev, block);

    return block;
}

} // namespace zelda::game
