# Generated by ./generate_schemas.py. This file should not be modified by hand.
@0xf5799e90a79fe0e8;

# Namespace setup
using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("gams::types");

# Capnfile Imports
using import "TransformStamped.capnp".TransformStamped;

# Type definition
struct TFMessage {
   transforms @0: List(TransformStamped);
   
}