# Generated by ./generate_schemas.py. This file should not be modified by hand.
@0xb133e9f1e3ec1be2;

# Namespace setup
using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("gams::types");

# Capnfile Imports
using import "Header.capnp".Header;

# Type definition
struct RelativeHumidity {
   header @0: Header;
   variance @1: Float64;
   relativeHumidity @2: Float64;
   
}