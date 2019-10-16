`paranoid_vector` is a (partially implemented) drop-in replacement for C++11's `std::vector`.
Its purpose is to immediately detect illegal access to vector elements.

The implementation currently focuses on attempts to access memory buffers that the `vector`
had _previously_ used.  E.g., the buffer in which a vector's elements were stored before
the vector upgraded to a larger buffer.
