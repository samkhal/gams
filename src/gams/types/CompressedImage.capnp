# Generated by ./generate_schemas.py. This file should not be modified by hand.
@0xc8f47df429c5b678;

# Namespace setup
using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("gams::types");

# Capnfile Imports
using import "Header.capnp".Header;

# Type definition
struct CompressedImage {
   header @0: Header;
   data @1: List(UInt8);
   format @2: Text;
   
}