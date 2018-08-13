#!/usr/bin/env python

""" 

    File: generate_schemas.py
    Author: Devon Ash
    Description: Translates ROS message formats into Capnproto schemas for use within GAMS"

"""

""" ROS Imports """
import rosgraph
import rostopic
import rosmsg
import roslib
import rosmsg
import rospkg
from genmsg import msg_loader
from genmsg import msgs

""" Python Libraries """
import socket
import sys
import yaml
import argparse
import os
import logging
import itertools
import re
import collections
from string import Template
import binascii
import random
import numpy

""" Global variables and constants """
NO_SUBTYPES = False
GLOBAL_TYPES_LIST = []
GLOBAL_BLACKLIST = []
GLOBAL_WHITELIST = []
GENERATED_TYPE_NAMESPACE = "gams::types"
CAPN_NAMESPACE_INCLUDE   = "using Cxx = import \"/capnp/c++.capnp\";"
CAPN_NAMESPACE_DEF       = "$Cxx.namespace(\"%s\");" % GENERATED_TYPE_NAMESPACE
GEN_COMMENT_NOTIF        = "# Generated by ./generate_schemas.py. This file should not be modified by hand."
VARIABLE_TEMPLATE        = "${NAME} ${IDX} : ${TYPE}"

SCHEMA_TEMPLATE = """${GEN_COMMENT_NOTIF}
@${HEX};

# Namespace setup
${CAPN_NAMESPACE_INCLUDE}
${CAPN_NAMESPACE_DEF}

# Capnfile Imports
${CAPN_IMPORTS}
# Type definition
struct ${TYPENAME} {
   ${VARIABLES}
}
"""

def to_camelcase(s):
    return re.sub(r'(?!^)_([a-zA-Z])', lambda m: m.group(1).upper(), s)

def get_random_hex_id():
    """
    Generates 64 bit hex id for a capn proto msg, then prepends "0x"

    Args:
        None

    Return:
        64 bit hex ID used in Capnproto message formats.

    """
    val = numpy.random.randint(1, numpy.iinfo(numpy.uint64).max, dtype='uint64') | (numpy.uint(1) << numpy.uint(63))
    return "{0:#0{1}x}".format(val, 1)

def resolve_schema(typename, variables, imports, hex_id):
    """
    Populates the schema template with values

    Args:
        typename: The name of the schema/type of the schema
        variables: The string representing the variables in the schema's struct
        imports: The imports brought in for types used

    Return: 
        A template of the capn file
    """

    schema_template = Template(SCHEMA_TEMPLATE)
    schema_mapping = {'TYPENAME': typename, 'VARIABLES': variables, 'CAPN_IMPORTS': imports, 'HEX': hex_id, 'CAPN_NAMESPACE_DEF': CAPN_NAMESPACE_DEF, 'CAPN_NAMESPACE_INCLUDE': CAPN_NAMESPACE_INCLUDE, 'GEN_COMMENT_NOTIF': GEN_COMMENT_NOTIF}
    schema_template = schema_template.safe_substitute(schema_mapping)
    return schema_template

def generate_schema(message_type, pkg_paths, directory_path=""):
    """
    Generates a schema for a given ROS message type.

    Args:
        message_type: The ROS message type in format package/Type
        pkg_paths: ROS Package Paths
        directory_path: Output directory path

    Return:
        None

    """

    # What do we need from a message_type? We need its MsgSpec/MsgContext.
    msg_context = msg_loader.MsgContext()
    msgspec = msg_loader.load_msg_by_type(msg_context, message_type, pkg_paths)
    variables = get_schema_variables_from_msgspec(msgspec) # DONE
    imports = get_schema_imports_from_msgspec(msgspec) # DONE
    hex_id = get_random_hex_id(); # Done
    template = resolve_schema(msgspec.short_name, variables, imports, hex_id)

    outdir = directory_path

    if not os.path.exists(outdir):
        os.mkdir(outdir)

    type_dirname = message_type.split('/')[0].split('_msgs')[0]

    filename = msgspec.short_name + ".capnp"
    filepath = outdir + "/" + type_dirname

    if not os.path.isdir(filepath):
        os.mkdir(filepath)

    filenamepath = filepath + "/" + filename

    with open(filenamepath, 'w') as ostream:
        print "Generating schema (.capnp) file for  %s at %s " % (message_type, filenamepath)
        ostream.write(template)
        ostream.close()

    return

CAPN_ROS_TYPE_MAPPING = {
'int8':'Int8', 
'uint8':'UInt8',
'int16':'Int16',
'uint16':'UInt16',
'int32':'Int32',
'uint32':'UInt32',
'int64':'Int64',
'uint64':'UInt64',
'float32':'Float32',
'float64':'Float64',
'string':'Text',
'bool':'Bool',
'char':'Int8',
'byte':'Int8',
'time':'Int64',
'duration':'Int64'
}

CAPN_ROS_ARRAY_TYPE_MAPPING = {
'float64':'Float64',
'float32':'Float32',
'uint16':'UInt16',
'uint8':'UInt8',
'uint32':'UInt32',
'uint64':'UInt64',
'string':'Text',
}

def get_schema_imports_from_msgspec(msgspec):
    """
    Returns a string of inputs in CAPN format given a msgspec

    Args:
        msgspec: Msgspec object of a ROS message

    Return:
        String of newline separated import statements

    """
    output_string = ""
    msgspec_set = set(msgspec.types)
    for t in msgspec_set:
        # Check if the doctored string (correct type) is not in BUILTIN. This can be fixed up..
        t = get_base_type(t)

        if t not in msgs.BUILTIN_TYPES:
            output_string = output_string + "using import \"" + t + ".capnp\"." + t + ";\n"
        elif ("string" in t) and (t in msgs.BUILTIN_TYPES):
            #Handle the "String.h" case since we are imitating the ROS API for now, can be changed later to be the madara String type
            output_string = output_string + ""
    
    return output_string

def slice_index(x):
    """
    Helper function to slice at a certain index

    Args:
        x: string to slice

    Return:
        Index

    """

    i = 0
    for c in x:
        if c.isalpha():
            i = i + 1
            return i
        i = i + 1

def upper_first(string):
    """
    Helper function that uppercases first letter of string to help with C++ syntax generation

    Args:
        string 

    Return:
        same string but first letter is uppercased
    """

    i = slice_index(string)
    return string[:i].upper() + string[i:]

def get_variable_type_declaration(t, name=""):
    """
    Gets a variable type declaration for a struct in a schema file from the type and name.

    Args:
        t: Type of variable in string form
        name: Name of variable in string form

    Return:
        FullStatement() which composes the statement in schema form.
    """

    FullStatement = collections.namedtuple('FullStatement', ['type', 'name'])

    type_statement = ""
    base_type = get_base_type(t)
    #NOTE: Capitalize String, since we are treating String as a class. ROSMSG writes it as string, this is a special case.
    if is_string_type(base_type):
        t = upper_first(base_type)

    #NOTE: Order matters here.
    if is_dynamic_array_type(t):
        return handle_dynamic_array(t, name)
    elif is_static_array_type(t):
        return handle_static_array(t, name)
    elif is_builtin_type(base_type):
        type_statement = map_rosmsg_builtin_type_to_capn(base_type)
    else:
        # Else it is a generated type
        type_statement = t.split('/')[1]

    return FullStatement(type_statement, name)

def get_base_type(t):
    """
    Given a dirty typestring of format package/Type it returns the base type Type

    Args:
        t: Dirty type in string format

    Return:
        Base type

    """
    if '/' in t:
        t = t.split('/')[1]

    if is_dynamic_array_type(t):
        t = t.split('[]')[0]

    if is_static_array_type(t):
        t = t[:t.find('[')]

    return t

def is_builtin_type(t):
    """
    Checks if a type is builtin in ROS

    Args:
        t: Name of type in string format

    Return:
        True or false
    """

    return t in msgs.BUILTIN_TYPES

def is_string_type(t):
    """
    Checks if type is of String type

    Args:
       t: Type in string form

    Return:
       True or false

    """

    return ("string" in t) or ("String" in t)

def is_static_array_type(t):
    """
    Checks if type is static array using regex.

    Args:
       t: Type in string form

    Return:
       True or false

    """

    match = re.search("\[.*?\]", t)
    return match

#@input: Takes a rosmsg type
#@return: A string mapping to the madara type. If there is no mapping, it returns false.
def map_rosmsg_builtin_type_to_capn(rosmsg_type):
    """
    Checks if the type is inside the mapping then returns its mapping.

    Args:
        rosmsg_type: ROS Message type

    Return:
        The schema type
    """

    if rosmsg_type in CAPN_ROS_TYPE_MAPPING:
        return CAPN_ROS_TYPE_MAPPING[rosmsg_type]
    return False

def handle_static_array(t, name):
    """
    Generates a FullStatement() tuple given a static array type t 

    Args:
        t: Type of variable in string form (should be static array type)
        name: Name of variable

    Return:
        FullStatement() tuple of the resolved type and name
    """

    if '/' in t:
        t = t.split('/')[1]

    num_eles = t[t.find("[")+1:t.find("]")]
    atomic_type = t[:t.find("[")]
    #TODO Handle types, if its primitive/generated/string etc.
    if atomic_type in msgs.BUILTIN_TYPES:
       atomic_type = CAPN_ROS_ARRAY_TYPE_MAPPING[atomic_type]
    else:
       atomic_type = atomic_type

    FullStatement = collections.namedtuple('FullStatement', ['type', 'name'])
    
    #vector_string = "std::vector<%s> %s(%s);"
    type_statement = "List(%s)" % atomic_type
    #name_statement = "%s = %s(%s)" % (name, type_statement, num_eles)

    return FullStatement(type_statement, name)

def is_dynamic_array_type(t):
    """
    Determines if a type is a dynamic array (ROS Message types)

    Args:
        t: type in string form

    Return:
        True or false
    """

    if t.endswith('[]'):
        return True
    return False


def handle_dynamic_array(t, name):
    """
    Handles a dynamic array type and generates a variable statement from it     

    Args:
        t: Type in string form
        name: Name of type

    Return:
        FullStatement() tuple composed of the type_statement and the name_statement
    """

    base_type = get_base_type(t)
    if is_builtin_type(base_type):
        base_type = map_rosmsg_builtin_type_to_capn(base_type)
    else:
        base_type = base_type

    FullStatement = collections.namedtuple('FullStatement', ['type', 'name'])
    name_statement = "%s" % name
    type_statement = "List(%s)" % base_type

    return FullStatement(type_statement, name_statement)

    
def get_schema_variables_from_msgspec(msgspec):
    """
    Returns a string composed of Capnproto schema variable statements.
    
    Args:
        msgspec: ROS Msgspec object defining a ROS msg in Python form

    Return:
        String of newline separated variable declarations.
    """

    declarations = {}
    for name, t in itertools.izip(msgspec.names, msgspec.types):
        full_statement = get_variable_type_declaration(t, name)
        declarations[full_statement.name] = full_statement.type

    output_string = ""
    for count, (name, type_decl) in enumerate(declarations.iteritems()):
        if len(name) == 1:
            name = name.lower()
        else:
            name = to_camelcase(name)
        output_string = output_string + name + " @" + str(count) + ": " + type_decl + ";\n   "

    return output_string
        
def generate_schemas(types_list, output_directory=""):
    """
    Generates MADARA working types from a list of ROS message types.

    Args:
        types_list [str]: List of types
        blacklist [str]: List of packages/types to ignore
        whitelist [str]: List of packages/types to allow
        output_directory str: Directory to output .h and .cpp files

    Returns:
        templates: A list of strings which are the contents of the .h and .cpp files to write to the system.
    """

    templates = []
    pkg_paths = get_rospkg_paths()
    for rosmsg_type in types_list:
        generate_schema(rosmsg_type, pkg_paths, directory_path=output_directory)

    return templates

def generate_schemas_from_msg(msg, output_directory=""):
    """
    Generates schema(s) from a fully qualified type e.g nav_msgs/Odometry

    Args:
        msg: String name of the .msg file or type. E.g sensor_msgs/PointCloud.msg, sensor_msgs/PointCloud is acceptable too.
        output_directory: Self explanatory.
    
    Return:
        List of templates (contents to be written to file)
   
    """

    t = msg.split('.msg')[0]

    GLOBAL_TYPES_LIST.append(t)

    load_all_subtypes(GLOBAL_TYPES_LIST)

    return generate_schemas(GLOBAL_TYPES_LIST, output_directory)

def generate_schemas_from_all(output_directory=""):
    """
    Generates schemas from all ROS message types found in your ROS package paths

    Args:
        output_directory: Directory to output .h and .cpp files

    Returns:
        schemas: A list of strings which are the contents of the .h and .cpp files to write to the system.
    """

    templates = []

    pkg_paths = get_rospkg_paths()
    for pkg, pkg_path in pkg_paths.iteritems():
        if (pkg in GLOBAL_WHITELIST) or (pkg not in GLOBAL_BLACKLIST):
            types = rosmsg._list_types(pkg_path[0], "msg", ".msg")
            print types
            for t in types:
                if (t in GLOBAL_WHITELIST) or (t not in GLOBAL_BLACKLIST):
                    full_type = pkg + "/" + t
                    GLOBAL_TYPES_LIST.append(full_type)

    load_all_subtypes(GLOBAL_TYPES_LIST)

    print "Total number of types (recursively) used in system: %d" % len(GLOBAL_TYPES_LIST)
    return generate_schemas(GLOBAL_TYPES_LIST, output_directory)

def generate_schemas_from_rosbag(rosbag_path, output_directory=""):
    """
    Generates MADARA working types from all ROS message types used in a ROS bag

    Args:
        rosbag_path: Full path to a rosbag on the system
        output_directory: Directory to output .h and .cpp files

    Returns:
        templates: A list of strings which are the contents of the .capn files
    """

    templates = []

    import rosbag

    if not os.path.isfile(rosbag_path):
        raise Exception("File not found %s" % rosbag_path)    

    pkg_paths = get_rospkg_paths()
    bag = rosbag.Bag(rosbag_path)
    generator = bag.get_type_and_topic_info()

    for type_, topic in generator[0].iteritems():
        print type_
        GLOBAL_TYPES_LIST.append(type_)

    load_all_subtypes(GLOBAL_TYPES_LIST)
    print "Total number of types (recursively) used in system: %d" % len(GLOBAL_TYPES_LIST)

    templates = generate_schemas(GLOBAL_TYPES_LIST, output_directory)

    return templates

def generate_schemas_from_live(output_directory=""):
    """
    Generates schemas from a running ROS system by looking at the ROS master for types.
 
    Args:
        output_directory: Directory to write the schemas to.

    Return:
        A list of templates which are populated to make strings of file contents.
    """

    templates = []

    live_message_types = get_live_message_types()

    for t in live_message_types:
        GLOBAL_TYPES_LIST.append(t)

    load_all_subtypes(GLOBAL_TYPES_LIST)
    print "Total number of types (recursively) used in system: %d" % len(GLOBAL_TYPES_LIST)
    
    # Now that we have all the types used in this specific ROS system, generate classes for them.
    pkg_paths = get_rospkg_paths()
        
    templates = generate_schemas(GLOBAL_TYPES_LIST, output_directory)
            
    return templates

def get_subtypes(msg_type, msg_paths, pkg_paths):
    """
    Given a ROS type, get all of its subtypes and populate it within the GLOBAL_TYPES_LIST. Note you shouldn't use
    this function outside of this file. :)

    Args:
        msg_type: Type to recurse into
        msg_paths: Path to /msg directories on your system
        pkg_paths: Path to ROS packages on the system

    Return:
        None
    """

    # We don't want to recurse into atomic types.

    pkg = msg_type.split('/')[0]
    m = get_base_type(msg_type)
    qual_type = pkg + "/" + m

    if m in msgs.BUILTIN_TYPES:
        #print "MSG_TYPE: %s is BUILTIN TYPE. IGNORING" % m
        return 1
    elif qual_type in GLOBAL_TYPES_LIST:
        #print "MSG_TYPE: %s ALREADY LOADED. IGNORING." % qual_type
        return 1
    elif qual_type is None:
        #print "MSG_TYPE: None"
        return 1


    GLOBAL_TYPES_LIST.append(qual_type)

    msg_context = msg_loader.MsgContext()
    msgspec = msg_loader.load_msg_by_type(msg_context, qual_type, pkg_paths)
    
    print "Adding type %s to processing list" % qual_type

    if msgspec.types is not None:
        if len(msgspec.types):
            for msg_type in msgspec.types:
                #if msg_type.endswith(']'):
                #    msg_type = msg_type.split('[')[0]
                pkg = msg_type.split('/')[0]
                m = get_base_type(msg_type)
                if (m in GLOBAL_BLACKLIST) or (pkg in GLOBAL_BLACKLIST):
                    print "WARNING! %s is listed in the global blacklist but is a requirement for %s" % (m, msg_type)
                get_subtypes(msg_type, msg_paths, pkg_paths)

def load_all_subtypes(rosmsg_types):
    """
    Args:
        rosmsg_types: List of ROS message types

    Return:
        Gets all the subtypes from a list of ROS message types and adds them to the GLOBAL_TYPE_LIST

    """

    if NO_SUBTYPES:
        print "WARNING: --no-subtypes selected. Not generating subtypes"
        return  

    print "Gathering subtypes for %d ROS message types" % len(rosmsg_types)
    msg_paths = get_rosmsg_paths(rosmsg_types)

    pkg_paths = get_rospkg_paths()

    for msgtype in rosmsg_types:
        # Should be iterating this types subtypes.
        msg_context = msg_loader.MsgContext()
        msgspec = msg_loader.load_msg_by_type(msg_context, msgtype, pkg_paths)
        for t in msgspec.types:
            get_subtypes(t, msg_paths, pkg_paths)   

def get_rospkg_paths():
    """
    Gets a map of ROS package paths.

    Return:
        Map of map[package_name] = path
    """

    rospack = rospkg.RosPack()
    found_pkgs = rosmsg.iterate_packages(rospack, '.msg')
    dir_map = {}
    for p, path in found_pkgs:
        # Must put path in array otherwise it shits itself.
        dir_map[p] = [path]
    return dir_map

def get_rosmsg_paths(types):
    """
    Gets a list of ROS message paths as they exist on your system

    Args:
        types: A list of ROS Message types in the format package/Type

    Return:
        A list of ROS message types as a map of map[type] = path
    """

    rospack = rospkg.RosPack()
    found_pkgs = rosmsg.iterate_packages(rospack, '.msg')

    rosmsg_type_path_map = {}
    dir_map = {}
    type_map = {}

    # This associates a type with a package name.
    for full_type in types:
        package_name = full_type.split('/')[0]
        print "Searching package %s ... " % package_name
        base_type = full_type.split('/')[1]
        type_map[base_type] = package_name
        
    # This associates a package name with a msg path
    for p, path in found_pkgs:
        #print "Msg path for package %s found at %s" % (p, path)
        dir_map[p] = path

    for msg_type in types:
        msg_type = msg_type.split('/')[1]
        package_name = type_map[msg_type]
        rosmsg_type_path_map[msg_type] = dir_map[package_name]

    return rosmsg_type_path_map    

def get_live_message_types():
    """
    Gathers a list of all the types used in a running ROS system.

    Return:
        a list of types used in a running ROS system.

    """

    print "Generating ROS Messages from rostopics"
    master = rosgraph.Master('/rostopic')
    rostopic_info = ""
    try:
        state = master.getSystemState()

        pubs, subs, _ = state

        topics_published_to = [item[0] for item in pubs]
        topics_subscribed_to = [item[0] for item in subs]

        unique_topics = list(set(topics_published_to + topics_subscribed_to))
        
        types = []
        for topic in unique_topics:
            topic_type = rostopic._get_topic_type(topic)[0]
            types.append(topic_type)

        output_list = set(types)
        print "Discovered %d types " % len(set(types))
        return output_list
            
    except socket.error:
        raise ROSTopicIOException("Unable to communicate with master!")

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Generates .h and .cpp files given a running ROS system, a ROS message type, or a .msg file. These generated .h and .cpp files will contain variables which are setup to point at static places in the knowledgebase. They are generated with the assumption that you also used the other scripts provided (roslaunch_to_kb, rostopics_to_kb) to generate your knowledgebase. NEW: The types generated now support the KnowledgeRecord<Any> interaction within Madara. This means that if you run this against a running ROS system or a rosbag, you will automatically create a knowledge base with parameters, arguments, and topics that are typed with the ROS message types and MADARA primitive types.")

    parser.add_argument('--live', '-l', action="store_true", help='Passing in --live will generate all .h and .cpp files for all of the message types on a running ROS system. This assumes a master is running.')

    parser.add_argument('--path', '-p', action="store_true", help="Passing in --path will try and load the .msg file and generate a .h and .cpp file based on that message. This will be usable in MADARA/GAMS.")

    parser.add_argument('--msg', '-m', help="Passing in --type will try and load the ROS message type and generate a .h and .cpp file based on that message. The resulting files will be loadable as types for MADARA/GAMS and point to variables in the knowledgebase.")

    parser.add_argument('--all', '-a', action="store_true", help="Generates all ROS message types found in the paths on your system. Anything on your ROS package path list will be generated into a .h and .cpp compatible with GAMS/MADARA.")

    parser.add_argument('--rosbag', '-b', help="Passing in --rosbag will generate all the types that are used within that ROS bag. .h and .cpp files will be used")

    parser.add_argument('--debug', '-dbg', action="store_true", help="Turns on debugging messages")

    parser.add_argument('--whitelist', '-wl', nargs="*", help="List of packages to white list for type generation. White listed packages ARE ALLOWED. If whitelist is enabled, it will also read a file called 'whitelist' from the current directory. in addition to the list passed in.")

    parser.add_argument('--blacklist', '-bl', nargs="*", help="List of packages to black list for type generation. Black listed packages ARE NOT ALLOWED. If blacklist is enabled, it will also read a file called 'whitelist' from the current directory. in addition to the list passed in.")

    parser.add_argument('--output', '-o', help="Output directory for the headers and cpp files", type=str)

    parser.add_argument('--no-subtypes', action="store_true", help="Tells the script to not recursively generate new types from the provided list and only do top level generation")

    args = parser.parse_args()
    
    templates = []

    if args.no_subtypes:
        NO_SUBTYPES = True

    if args.debug:
        logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)

    output_directory = ""
    if args.output:
        output_directory = args.output
    else:
        output_directory = os.getcwd()
        print "No output directory specified. Defaulting to current directory %s" % os.getcwd()

    bag_path = ""

    if args.whitelist:
        GLOBAL_WHITELIST = args.whitelist

    if args.blacklist:
        GLOBAL_BLACKLIST = args.blacklist

    list_to_open = ""
    if os.path.isfile('whitelist'):
       print "Using whitelist file"
       list_to_open = 'whitelist'
    elif os.path.isfile('blacklist'):
        print "Using blacklist file" 
        list_to_open = 'blacklist'

    if not (list_to_open == ""):
        lines = [line.rstrip('\n') for line in open(list_to_open)]
        for line in lines:
            if list_to_open == 'blacklist':
                print "Added " + line + " to the blacklist"
                GLOBAL_BLACKLIST.append(line)
            elif list_to_open == 'whitelist':
                print "Added " + line + " to the whitelist"
                GLOBAL_WHITELIST.append(line)
    else:
        print "No whitelist or blacklist detected. Generating all"

    print "Adding builtin types to the blacklist, doesn't make sense, really."
    for k, v in CAPN_ROS_TYPE_MAPPING.iteritems():
        GLOBAL_BLACKLIST.append(v)
        print "Added " + v + " to the blacklist"

    if args.live:
        print "Attempting to generate templates from a running ROS System"
        templates = generate_schemas_from_live(output_directory)
    elif args.rosbag:
        templates = generate_schemas_from_rosbag(args.rosbag, output_directory)
    elif args.path:
        print "TODO"
        #templates = generate_templates_from_path(args.path)
    elif args.msg:
        templates = generate_schemas_from_msg(args.msg, output_directory)
    elif args.all:
        templates = generate_schemas_from_all(output_directory)
    else:
        parser.print_help()
        sys.exit()

    for template in templates:
        with open(template.schema_filename_, 'w') as ostream:
            ostream.write(template.schema_file_contents_)
            ostream.close()
            print "Succesfully wrote schema contents to %s " % template.schema_filename_

    print "Done. Exiting."
    sys.exit()




