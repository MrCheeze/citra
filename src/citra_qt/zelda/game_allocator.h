#pragma once

#include <cstring>
#include "citra_qt/zelda/game_types.h"
#include "common/common_types.h"

namespace zelda {

namespace game {

namespace nn::os {
// Note that this is inaccurate
struct CriticalSection {
    u32 x0;
    u32 x4;
    s32 lock_count;
};
static_assert(sizeof(CriticalSection) == 0xC);
} // namespace nn::os

class Allocator;

/// Allocator memory block.
///
/// * Blocks are always aligned to a 16-byte boundary.
///
/// * Large blocks are allocated from the start of the root block
///   and the heap grows in the direction of high memory addresses.
///
/// * Small blocks are allocated from the end of the root block
///   and the heap grows in the direction of low memory addresses.
///
/// * Blocks can optionally be reference counted and given a user-specified ID. This is typically
///   used for files and some actor resources.
struct AllocatorBlock {
    enum Flag : u8 {
        /// Is reference counted
        Flag_IsRefCounted = 1 << 0,
        /// Prevent this block from being reused even when ref count is 0
        Flag_PreventReuse = 1 << 1,
        /// Free this block immediately when ref count reaches 0
        Flag_FreeBlockOnRefCountZero = 1 << 3,
    };

    static Ptr<AllocatorBlock> FromData(Ptr<void> ptr) {
        return Ptr<AllocatorBlock>{ptr.addr - u32(sizeof(AllocatorBlock))};
    }

    static Ptr<void> Data(Ptr<AllocatorBlock> self) {
        return Ptr<void>{self.addr + u32(sizeof(AllocatorBlock))};
    }

    bool IsFree() const {
        return size > 0;
    }

    /// 'S.Moriya'
    u64 magic;
    /// Result of nn::svc::GetSystemTick() when this block was allocated.
    u64 alloc_ticks;

    /// Previous block. Null for root block. This is a circular list starting at root_block.
    Ptr<AllocatorBlock> prev;
    /// Next block. Null for root block. This is a circular list starting at root_block.
    Ptr<AllocatorBlock> next;

    /// Next free block to try for small allocations. This is a circular list starting at
    /// dummy_block.
    Ptr<AllocatorBlock> next_free_s;
    /// Next free block to try for large allocations. This is a circular list starting at
    /// dummy_block.
    Ptr<AllocatorBlock> next_free_l;

    /// Next reference counted block. This is a circular list.
    Ptr<AllocatorBlock> refcounted_next;
    /// Previous reference counted block. This is a circular list.
    Ptr<AllocatorBlock> refcounted_prev;

    /// Arbitrary, user-specified value to identify reference counted blocks.
    u32 id;
    /// Reference count. Always 0 or 1 for non-reference counted blocks.
    u16 ref_count;
    Flag flags;
    /// Size in bytes. If positive: free size. If negative: allocation size (including header size).
    int size;
    /// Usually source code file path + line number, but can also be a file path or nullptr.
    /// This is almost always a dangling pointer for reference counted blocks because FileEntity
    /// sets the name using a member char array even though FileEntities are not reference counted.
    /// Whoops!
    Ptr<const char> name;

    u32 field_38;
    u32 field_3C;

    bool operator==(const AllocatorBlock& rhs) const {
        return std::memcmp(this, &rhs, sizeof(rhs)) == 0;
    }

    bool operator!=(const AllocatorBlock& rhs) const {
        return !(*this == rhs);
    }
};
static_assert(sizeof(AllocatorBlock) == 0x40);

class Allocator {
public:
    static Allocator& Instance() {
        return *Ptr<Allocator>{0x72F7F0};
    }

    Ptr<void> Alloc(u32 size, Ptr<const char> name);
    Ptr<void> AllocRefCounted(u32 id, u32 size);

    void Free(Ptr<void> ptr);

    void CopyDebugInfo();

    /// Only next_free_s and next_free_l should be used.
    AllocatorBlock dummy_block;

    Ptr<AllocatorBlock> root_block;
    Ptr<void> root_block_end;
    u32 average_alloc_size;
    // Normally 0x800 bytes.
    u32 small_block_threshold;
    u32 size;
    u32 block_count;
    Ptr<nn::os::CriticalSection> crit_section;

private:
    Ptr<AllocatorBlock> GetDummyBlock() {
        return Ptr<AllocatorBlock>{0x72F7F0 + u32(offsetof(Allocator, dummy_block))};
    }

    Ptr<void> AllocSmall(u32 size);
    Ptr<void> AllocLarge(u32 size);

    void DoFree(Ptr<void> ptr);
    bool FreeUnusedRefCountedBlock(u32 required_size);

    void UpdateFreeLists(Ptr<AllocatorBlock> block);
    Ptr<AllocatorBlock> TryToMergeBlock(Ptr<AllocatorBlock> block);
};
static_assert(sizeof(Allocator) == 0x60);

} // namespace game

} // namespace zelda
