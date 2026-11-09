Technical considerations
### 1. Reading data as bytes for universality of the algorithm
This ensures that we need only a constant array of 256 bytes to store the bytes and their frequencies

### 2. Avoid exploding space complexity with intermediate files
A temporary byte buffer is used when the codes are being used to write output. When buffer is full, 
write to output and empty buffer and so on

### 3. Array-only can work
With array only min heap, we may not need to keep track of left and right children of inner nodes