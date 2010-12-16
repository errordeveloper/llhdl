#!/usr/bin/python

import os, collections, string, sys

Primitive = collections.namedtuple('Primitive', 'name attributes inputs outputs')

primitives = [
	Primitive(
		name = "IBUF",
		attributes = [],
		inputs = [(1, "I")],
		outputs = [(1, "O")]
	),
	Primitive(
		name = "OBUF",
		attributes = [],
		inputs = [(1, "I")],
		outputs = [(1, "O")]
	),
	Primitive(
		name = "FDE",
		attributes = [],
		inputs = [(1, "C"), (1, "CE"), (1, "D")],
		outputs = [(1, "Q")]
	),
	Primitive(
		name = "VCC",
		attributes = [],
		inputs = [],
		outputs = [(1, "O")]
	),
	Primitive(
		name = "GND",
		attributes = [],
		inputs = [],
		outputs = [(1, "O")]
	),
	Primitive(
		name = "LUT1",
		attributes = [("INIT", "0")],
		inputs = [(1, "I0")],
		outputs = [(1, "O")]
	),
	Primitive(
		name = "LUT2",
		attributes = [("INIT", "0")],
		inputs = [(1, "I0"), (1, "I1")],
		outputs = [(1, "O")]
	),
	Primitive(
		name = "LUT3",
		attributes = [("INIT", "00")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2")],
		outputs = [(1, "O")]
	),
	Primitive(
		name = "LUT4",
		attributes = [("INIT", "0000")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2"), (1, "I3")],
		outputs = [(1, "O")]
	),
	Primitive(
		name = "LUT5",
		attributes = [("INIT", "00000000")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2"), (1, "I3"), (1, "I4")],
		outputs = [(1, "O")]
	),
	Primitive(
		name = "LUT6",
		attributes = [("INIT", "0000000000000000")],
		inputs = [(1, "I0"), (1, "I1"), (1, "I2"), (1, "I3"), (1, "I4"), (1, "I5")],
		outputs = [(1, "O")]
	),
]

def print_io_list(l):
	for e in l:
		if e[0] > 1:
			for i in range(e[0]):
				print "\t\"%s_%d\"," % (e[1], i)
		else:
			print "\t\"%s\"," % e[1]

def io_count(l):
	c = 0
	for e in l:
		c += e[0]
	return c

def generate_c():
	print "#include <stdlib.h>"
	print "#include <netlist/net.h>"
	print "#include <netlist/xilprims.h>"
	for p in primitives:
		print "\n/* %s */" % p.name
		if p.attributes:
			print "static char *%s_attribute_names[] = {" % p.name
			for attr in p.attributes:
				print "\t\"%s\"," % attr[0]
			print "};"

			print "static char *%s_attribute_defaults[] = {" % p.name
			for attr in p.attributes:
				print "\t\"%s\"," % attr[1]
			print "};"

		if p.inputs:
			print "static char *%s_inputs[] = {" % p.name
			print_io_list(p.inputs)
			print "};"

		if p.outputs:
			print "static char *%s_outputs[] = {" % p.name
			print_io_list(p.outputs)
			print "};"

	print "\n\nstruct netlist_primitive netlist_xilprims[] = {"
	for p in primitives:
		print "\t{"
		print "\t\t.type = NETLIST_PRIMITIVE_INTERNAL,"
		print "\t\t.name = \"%s\"," % p.name
		print "\t\t.attribute_count = %d," % len(p.attributes)
		if p.attributes:
			print "\t\t.attribute_names = %s_attribute_names," % p.name
			print "\t\t.default_attributes = %s_attribute_defaults," % p.name
		else:
			print "\t\t.attribute_names = NULL,"
			print "\t\t.default_attributes = NULL,"
		print "\t\t.inputs = %d," % io_count(p.inputs)
		if p.inputs:
			print "\t\t.input_names = %s_inputs," % p.name
		else:
			print "\t\t.input_names = NULL,"
		print "\t\t.outputs = %d," % io_count(p.outputs)
		if p.outputs:
			print "\t\t.output_names = %s_outputs" % p.name
		else:
			print "\t\t.output_names = NULL"
		print "\t},"
	print "};"

def print_io_enum(prefix, l):
	n = 0
	for e in l:
		if e[0] > 1:
			for i in range(e[0]):
				print "%s%s_%d = %d" % (prefix, e[1], i, n)
				n += 1
		else:
			print "%s%s = %d," % (prefix, e[1], n)
			n += 1

def generate_h():
	print "#ifndef __NETLIST_XILPRIMS_H"
	print "#define __NETLIST_XILPRIMS_H\n"

	i = 0
	print "enum {"
	for p in primitives:
		print "\tNETLIST_XIL_%s = %d," % (p.name, i)
		i += 1
	print "};"

	for p in primitives:
		if p.inputs:
			print "\n/* %s: inputs */" % p.name
			print "enum {"
			print_io_enum(("\tNETLIST_XIL_%s_" % p.name), p.inputs)
			print "};"
		if p.outputs:
			print "/* %s: outputs */" % p.name
			print "enum {"
			print_io_enum(("\tNETLIST_XIL_%s_" % p.name), p.outputs)
			print "};"

	print "\nextern struct netlist_primitive netlist_xilprims[];\n"
	
	print "#endif /* __NETLIST_XILPRIMS_H */"


if len(sys.argv) != 2:
	print "Usage: %s c|h" % os.path.basename(sys.argv[0])
	sys.exit(1)

print "/* Generated automatically by %s. Do not edit manually! */" % os.path.basename(sys.argv[0])
if sys.argv[1] == "c":
	generate_c()
else:
	generate_h()
