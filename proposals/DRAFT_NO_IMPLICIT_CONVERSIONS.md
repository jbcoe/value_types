# Against implicit conversions for `indirect`

<!-- markdownlint-disable MD029 -->

ISO/IEC JTC1 SC22 WG21 Programming Language C++

D3902R0

Working Group: Library Evolution

Date: 2025-10-29

_Jonathan Coe \<<jonathanbcoe@gmail.com>\>_

_Antony Peacock \<<ant.peacock@gmail.com>\>_

_Sean Parent \<<sparent@adobe.com>\>_

## Abstract

The national body comment US 77-140 says:

_indirect should convert to T& to simplify the use cases (e.g., returning the object from a function with a return type T&) where indirect appears as a drop-in replacement for T when T may be an incomplete type conditionally. With the proposed change, indirect is closer to reference_wrapper, but carries storage._

The authors of indirect are opposed to this change without significant implementation experience.

## Background

TODO

## Future direction

TODO

## Acknowledgements

Many thanks to Neelofer Banglawala for collating information and preparing this
draft at extremely short notice.

## References

TODO
