# Architecture and Function Guide

This guide describes the current implementation. The rationale column reflects comments in the code or the direct purpose of the implementation; it does not claim unrecorded design history.

## Data Structures

Defined in `include/helpers/structures.h`:

| Type | What it stores | Why it is used |
| --- | --- | --- |
| `Profile` | An array of 256 `HuffmanNode` pointers and an active `size`. | The full array gives each byte value a direct index during profiling. After profiling, the active prefix is reused to hold the min-heap nodes. |
| `HuffmanNode` | A frequency and a leaf flag; a leaf stores a byte, while an internal node stores left and right child pointers. | The union shares storage between leaf data and child pointers, since a leaf does not need children. The flag tells traversals whether to read the byte or descend. |
| `LUT` | Four 64-bit words of code bits and a code length. | The table stores a code by byte value so compression can retrieve it without repeatedly searching the tree. Four words provide the representation used by the current code. |

## Command-Line Programs

### `compress.c`

- `main` checks the argument count, opens the input and output, profiles the input, copies the byte/frequency entries, builds the Huffman tree and LUT, and calls `compress`. The copy preserves the leaf entries and frequencies for archive writing while `build_tree` reuses and mutates the profile as heap storage. It then closes files and frees the allocated structures.
- `print_LUT` prints each populated table entry, showing printable bytes as characters and other bytes in hexadecimal. This is a diagnostic helper; `main` does not call it.

### `extract.c`

- `main` checks the argument count, opens the archive and output, calls `extract`, and closes both file descriptors. Decoding is delegated to the shared core function rather than implemented in the CLI entry point.

## Huffman Core

Implemented in `src/huffman.c` and declared in `include/huffman.h`:

| Function | What it does | Why this approach is used |
| --- | --- | --- |
| `init_profile` | Allocates a profile and creates one zero-frequency leaf node for every byte value. | The code comment specifies initializing the full byte range so profiling can update frequencies by byte index. |
| `profiler` | Reads the input in `BUFFER_SIZE` chunks, updates frequencies, then removes zero-frequency entries. | Chunked reads let profiling work on a buffer at a time; byte-indexed updates avoid searching for a byte's profile entry. |
| `build_tree` | Repeatedly removes the two lowest-frequency nodes, joins them below a parent, and inserts the parent back into the heap. Returns the remaining root. A single-symbol profile returns its leaf. | The min-heap supplies the smallest-frequency nodes needed at each construction step. The single-leaf case avoids trying to combine a node with a second, nonexistent node. |
| `get_code` | Recursively searches for a byte and writes the traversed branch decisions into a character buffer. Left is represented by `1`, right by `0`. | The source marks this as a testing helper; production compression uses `create_LUT` instead. |
| `create_LUT` | Traverses the tree and stores the path bits and length at the entry for each leaf byte. A one-leaf tree is assigned a one-bit code. | Its comment gives the purpose: avoid searching the whole tree repeatedly for each input byte. It represents the path across four 64-bit words, with the first path decision at the least-significant end of each word. |
| `compress` | Writes original size and symbol frequencies to the archive, seeks the input back to the start, encodes each byte through the LUT, packs bits into output bytes, and writes buffered data. | The input is profiled before encoding, so it is read again after seeking. A temporary output buffer collects bytes and is flushed when full, avoiding intermediate files. |
| `extract` | Reads the original size and frequency table, rebuilds the tree, traverses it for each packed data bit, and buffers decoded bytes until the recorded original size is reached. | The archive stores frequencies rather than a serialized tree, so the decoder rebuilds the tree. The original size provides the stopping condition despite padding bits in the last encoded byte. |
| `update_output_buffer` | Declared in `include/huffman.h`, but has no definition in the current source and is not called by either executable. | There is no current implementation rationale to document. |

## Heap Module

Implemented in `src/helpers/heap.c` and declared in `include/helpers/heap.h`. The heap is stored in the `Profile.nodes` array and orders nodes by frequency so tree construction can repeatedly retrieve the least-frequent nodes.

| Function | What it does | Why it is used |
| --- | --- | --- |
| `create_leaf_node` | Allocates a leaf with the given byte, zero frequency, and leaf flag set. | Leaf nodes represent the byte values being profiled or reconstructed from the archive. |
| `create_inner_node` | Allocates a non-leaf node, stores its children, and sets its frequency to the sum of their frequencies. | A parent combines the frequencies and subtrees of the two nodes selected during tree construction. |
| `extract_root` | Removes and returns the heap root, replacing it with the last active entry and sifting down. | The root is the minimum-frequency node, which is the next one needed by `build_tree`. |
| `add_node_to_heap` | Appends a node and sifts it up to restore heap order. | New parent nodes must re-enter the pool of candidates for the next minimum selection. |
| `sift_down` | Moves a root toward the leaves by swapping it with the lower-frequency child where needed. | Restores min-heap order after removing the root. |
| `sift_up` | Moves the newly added node toward the root while its frequency is less than its parent's. | Restores min-heap order after insertion. |

## General Helpers

Implemented in `src/helpers/helpers.c` and declared in `include/helpers/helpers.h`:

| Function | What it does | Why it is used |
| --- | --- | --- |
| `swap` | Exchanges two node pointers in a profile array. | Used by sorting to reorder profile entries by frequency. |
| `kill_spaces` | Sorts the 256 entries, frees zero-frequency leaves, compacts active entries at the front, and sets the active size. | Removes unused symbols so subsequent tree construction handles only symbols present in the input. |
| `update_profile` | Increments the frequency associated with every byte in a buffer. | Keeps the frequency update simple and direct by indexing with the byte value. |
| `sort_profile` | Uses insertion sort to order all 256 entries by frequency. | `kill_spaces` relies on the ordering before compacting the nonzero entries. |
| `tree_height` | Recursively returns the longest edge-count from the root to a leaf. | The header comment describes its purpose as determining the maximum code length. The current compression and extraction paths do not call it. |
| `free_tree` | Recursively frees child subtrees and then each node. | Releases the dynamically allocated tree after compression or extraction. |
| `free_profile` | Frees the active node pointers in a profile, but not the `Profile` object itself. | Frees the separately allocated node entries while allowing the profile object to be stack-allocated, as in `compress.c`. |
| `read_exact` | Reads until the requested number of bytes has been received, retrying interrupted reads and exiting if end-of-file arrives early. | Archive fields have fixed sizes, so callers need the full requested field before decoding can continue. |
| `write_exact` | Continues writing a buffer region until all its bytes have been written, retrying interrupted writes. | POSIX `write` may write fewer bytes than requested; callers use this helper to flush complete buffer regions. |

## Build

The `Makefile` gathers C files under `src/` as shared sources, then links them with `compress.c` and `extract.c` to produce the two executables. It compiles with generated dependency files so changes to included headers can trigger recompilation.
