# Btrfs Linux File System Simulator - Comprehensive Documentation

## 1. Project Overview & Objectives
This project is a simulation of the **Linux Btrfs (B-tree File System)** written in C for an academic Algorithms & Data Structures (ADSA) / Operating Systems curriculum.

The simulator demonstrates core Btrfs characteristics:
1. **Copy-on-Write (CoW)**: Efficient snapshot-style file duplication without allocating duplicate storage blocks.
2. **RAID Level 1 (Disk Mirroring)**: Simultaneous dual-disk replication across simulated in-memory storage devices (`disk1` and `disk2`).
3. **Data Integrity & Self-Healing**: Block-level checksum verification that automatically detects silent data corruption ("bit rot") and reconstructs corrupted data using the healthy mirror disk.
4. **Optimized for Small Text Files**: Custom small block size ($128\text{ bytes}$) configured for lightweight text storage.
5. **Standard B-Tree Indexing ($O(\log n)$)**: Complete Standard B-Tree ($T = 2$, Order 4) index structure mapping `filename -> inode_id` with dynamic node splitting, borrowing, merging, and full CLRS-compliant deletion.

---

## 2. System Architecture & Component Hierarchy

The system follows a strict modular 5-tier design pattern:

```
+==========================================================================+
|                         TIER 1: USER INTERFACE                           |
|                  CLI Interactive Shell (src/main.c)                      |
+==========================================================================+
      |                                              |
      v                                              v
+-------------------------------+      +-----------------------------------+
| TIER 2: FILE OPERATIONS       |      | TIER 3: FILE UTILITIES            |
| (includes/file_ops.h)         |      | (includes/file_utils.h)           |
| (src/file_ops.c)              |      | (src/file_utils.c)                |
| - fs_create_file()            |      | - fs_copy_file() [Btrfs CoW]      |
| - fs_list_files()             |      | - fs_delete_file() [Safe Delete]  |
|                               |      | - fs_read_file() [Self-Heal Read] |
+-------------------------------+      +-----------------------------------+
      |                                              |
      +----------------------+-----------------------+
                             |
                             v
+==========================================================================+
|                   TIER 4: B-TREE INDEXING ENGINE                         |
|                 (includes/btree.h, src/btree.c)                          |
|                                                                          |
|  - Standard B-Tree (T = 2, Order 4) storing (filename -> inode_id)       |
|  - O(log n) File Lookup, Insert with Split, Full Delete with Merge       |
|  - In-order tree traversal & live ASCII hierarchy visualizer             |
+==========================================================================+
                             |
                             v
+==========================================================================+
|                   TIER 5: STORAGE BRAIN & RAID-1                         |
|                 (includes/fs_core.h, src/fs_core.c)                      |
|                                                                          |
|  - RAM Disks: disk1[1000][128], disk2[1000][128]                         |
|  - Metadata: Inode Table (64 files), Block Allocator (First-Fit)         |
|  - Checksum: Hash-based block integrity table                            |
|  - RAID-1: Dual-write, Mirror-read, Self-Healing, Fault-Injection        |
+==========================================================================+
```

---

## 3. Storage Configuration & Limits

| Constant | Value | Description |
| :--- | :--- | :--- |
| `BLOCK_SIZE` | `128 bytes` | Small block size tailored for small text files |
| `TOTAL_BLOCKS` | `1000 blocks` | Storage capacity per disk ($128\text{ KB}$ per disk) |
| `MAX_FILES` | `64 files` | Capacity of the file system Inode Table |
| `MAX_FILENAME` | `128 chars` | Maximum string length for filenames |
| `MAX_BLOCKS_PER_FILE`| `20 blocks` | Max file capacity ($20 \times 128 = 2560\text{ bytes} \approx 2.5\text{ KB}$) |

---

## 4. Key Data Structures

### `Inode` (File Metadata)
Every file on the simulated filesystem is represented by an Inode:
```c
typedef struct {
    char filename[MAX_FILENAME];              // File name (e.g. "notes.txt")
    int  size;                                // Actual file size in bytes
    int  block_pointers[MAX_BLOCKS_PER_FILE]; // Array of allocated block IDs
    int  block_count;                         // Number of blocks occupied
    int  is_used;                             // 1 = Active, 0 = Free slot
    int  ref_count;                           // Reference count for Copy-on-Write (CoW)
} Inode;
```

---

## 5. Core Mechanisms Explained

### A. RAID Level 1 Mirroring
* **Write (`raid1_write_block`)**: Writes identical data to both `disk1` and `disk2`, calculating and saving the checksum in `block_checksums[block_num]`.
* **Read (`raid1_read_block`)**: First reads `disk1`. If valid, returns data immediately. If `disk1` is offline or corrupted, falls back to `disk2`.

### B. Btrfs Silent Corruption Detection & Self-Healing
1. A read operation computes `checksum(disk1[block])`.
2. If `checksum != expected_checksum`, corruption is flagged (`[BTRFS ALERT] Data corruption detected on Disk 1`).
3. Core reads the mirror copy from `disk2`.
4. If `disk2` is healthy, it executes **Self-Healing**:
   ```c
   memcpy(disk1[block_num], disk2[block_num], BLOCK_SIZE);
   self_heal_count++;
   ```
5. Returns healthy data to the user without failing the read operation.

### C. Copy-on-Write (CoW)
When `fcopy source.txt dest.txt` is executed:
* The system does **not** allocate duplicate data blocks on disk.
* `dest.txt` points directly to the existing block IDs of `source.txt`.
* The `ref_count` of both Inodes is incremented.
* **Safe Deletion**: Deleting one file decrements the reference count. Shared data blocks are **only freed** when the last referencing file is deleted.

---

## 6. Complete CLI Command Reference

### Standard User Mode (`btrfs> `)

| Command | Syntax | Description |
| :--- | :--- | :--- |
| `fcreate` | `fcreate <filename>` | Prompts for `content: ` and writes file to RAID-1 storage |
| `fcopy` | `fcopy <src> <dest>` | Duplicates file using Btrfs Copy-on-Write (CoW) |
| `fread` | `fread <filename>` | Reads and displays file content with auto self-healing |
| `fdel` | `fdel <filename>` | Safely deletes a file (handles shared CoW blocks) |
| `show-all` | `show-all` | Lists all files, sizes in bytes, and block counts |
| `dev-m` | `dev-m` | Prompts for password (`1234`) to enter Developer Mode |
| `help` | `help` | Shows available commands |
| `exit` | `exit` or `quit` | Terminates the simulator shell |

### Developer Mode (`btrfs(dev)> `)
*Unlocked by typing `dev-m` and entering password `1234`.*

| Command | Syntax | Description |
| :--- | :--- | :--- |
| `show-alldev` | `show-alldev` | Displays extended file table with **RefCnt** and **Allocated Block IDs** (e.g. `[0, 1]`) |
| `btree-show` | `btree-show` | Displays live ASCII hierarchy of the B-Tree (Internal/Leaf nodes, keys, and inode IDs) |
| `fdamage` | `fdamage <disk_id> <block>` | Injects byte corruption into a block to test Self-Healing |
| `fdisk` | `fdisk <disk_id> <1\|0>` | Manually sets Disk 1 or 2 Online (`1`) or Offline (`0`) |
| `fstatus` | `fstatus` | Displays RAID-1 disk status and cumulative self-heal count |
| `dev-exit` | `dev-exit` | Returns to normal user mode |

---

## 7. B-Tree Data Structure & Algorithm (ADSA Focus)

The file index uses a **Standard B-Tree** of minimum degree $T = 2$ (Order 4, 2-3-4 tree variant):

* **Key Capacity**: Every internal/leaf node contains between $T - 1 = 1$ and $2T - 1 = 3$ keys.
* **Child Pointers**: Every non-leaf node with $k$ keys contains exactly $k + 1$ children.
* **Search Complexity**: Searching for a file by name executes in $O(\log n)$ string comparisons instead of linear $O(n)$ scanning.
* **Insertion with Proactive Splitting**: When inserting into a full node ($3$ keys), the node splits around the median key ($T-1$), elevating the median to the parent.
* **CLRS-Compliant Deletion**:
  1. **Leaf Deletion**: If the key is in a leaf with $> T-1$ keys, it is directly removed.
  2. **Internal Node Replacement**: Replaced by its in-order predecessor or successor.
  3. **Borrowing & Merging**: If a child node has fewer than $T$ keys, it borrows from an immediate sibling or merges with a sibling, ensuring all B-Tree properties remain invariant.
* **Live ASCII Visualization**: Developers can inspect the active tree balancing using the `btree-show` command.

---

## 8. Compilation & Execution

```bash
# Clean previous builds
make clean

# Compile with strict warnings
make build

# Run the simulator
make run
```
