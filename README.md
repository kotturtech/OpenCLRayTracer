# OpenCLRayTracer
OpenCL Ray Tracing Utility Library

This library provide building blocks for GPU cross-hardware ray tracer based on OpenCL platform.
Those building blocks include:

    1. Acceleration Structures - Bounding Volume Hierarchy and Two Level Grid
    2. Basic 3D model loading and handling
    3. Primitive structures for Ray Tracing, such as AABB, Triangle, and their intersection functions with Ray
    4. Basic Camera functions

The library also provides a simplified wrapper for OpenCL API.

Functions of this library are demonstrated in the Demo application and test 3D models. 

The library is build for extensibility - It provides an interface for adding more Acceleration structures,
and gives essential building blocks for algorithms, such as Sorting and Prefix Sum.

The purpose of this library is to be a learning source for Ray Tracing algorithm implementation, and to provide
a base for developers to extend and improve it - So you are encouraged to do so!
