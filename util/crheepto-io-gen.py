#!/usr/bin/env python3

# Copyright 2024 Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: crheepto-gen.py
# Author: Michele Caon, Luigi Giuffrida
# Date: 03/20/2024
# Description: Generate crheepto HDL files based on configuration.

# Based on occamygen.py from ETH Zurich (https://github.com/pulp-platform/snitch/blob/master/util/occamygen.py)

import argparse
import pathlib
import re
import sys
import logging
import math

import hjson
from jsonref import JsonRef
from mako.template import Template

# Compile a regex to trim trailing whitespaces on lines
re_trailws = re.compile(r"[ \t\r]+$", re.MULTILINE)

def string2int(hex_json_string):
    return (hex_json_string.split('x')[1]).split(',')[0]

def CamelCase(input_string):
    # Split the input string by non-alphanumeric characters (e.g., space, hyphen, underscore)
    words = re.split(r'[^a-zA-Z0-9]+', input_string)
    
    # Capitalize the first letter of each word except the first word
    # Join all words together to form a CamelCase string
    camel_case = words[0].capitalize() + ''.join(word.capitalize() for word in words[1:])
    
    return camel_case

def SCREAMING_SNAKE_CASE(input_string):
    # Replace non-alphanumeric characters with underscores and handle camelCase and PascalCase
    words = re.sub(r'([a-z])([A-Z])', r'\1_\2', input_string)  # Insert underscores between camelCase words
    words = re.sub(r'[^a-zA-Z0-9]+', '_', words)               # Replace non-alphanumerics with underscores
    
    # Convert the entire string to uppercase
    screaming_snake_case = words.upper()
    
    # Remove any leading or trailing underscores
    screaming_snake_case = screaming_snake_case.strip('_')
    
    return screaming_snake_case


def int2hexstr(n, nbits) -> str:
    """
    Converts an integer to a hexadecimal string representation.

    Args:
        n (int): The integer to be converted.
        nbits (int): The number of bits to represent the hexadecimal string.

    Returns:
        str: The hexadecimal string representation of the integer.

    """
    return hex(n)[2:].zfill(nbits // 4).upper()


def write_template(tpl_path, outdir, **kwargs):
    if tpl_path is not None:
        tpl_path = pathlib.Path(tpl_path).absolute()
        if tpl_path.exists():
            tpl = Template(filename=str(tpl_path))
            with open(
                outdir / tpl_path.with_suffix("").name, "w", encoding="utf-8"
            ) as f:
                code = tpl.render_unicode(**kwargs)
                code = re_trailws.sub("", code)
                f.write(code)
        else:
            raise FileNotFoundError(f"Template file {tpl_path} not found")


def main():
    # Parser for command line arguments
    parser = argparse.ArgumentParser(
        prog="crheepto-io-gen.py",
        description="Generate crheepto IO file based on the provided configuration. (crheepto.io.gen in outdir by default)",
    )
    parser.add_argument(
        "--cfg",
        "-c",
        metavar="FILE",
        type=argparse.FileType("r"),
        required=True,
        help="Configuration file in HJSON format",
    )
    parser.add_argument(
        "--outdir",
        "-o",
        metavar="DIR",
        type=pathlib.Path,
        required=True,
        help="Output directory",
    )
    parser.add_argument(
        "--verbose", "-v", action="store_true", help="Increase verbosity"
    )
    args = parser.parse_args()

    # Set verbosity level
    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)

    # Read HJSON configuration file
    with args.cfg as f:
        try:
            cfg = hjson.load(f, use_decimal=True)
            cfg = JsonRef.replace_refs(cfg)
        except ValueError as exc:
            raise SystemExit(sys.exc_info()[1]) from exc

    # Check if the output directory is valid
    if not args.outdir.is_dir():
        exit(f"Output directory {args.outdir} is not a valid path")

    # Create output directory
    args.outdir.mkdir(parents=True, exist_ok=True)

    outfile = args.outdir / "crheepto.io.gen"

    preamble = "( globals\n  version = 3\n io_order = clockwise\n)\n\n"

    margins = "( row_margin\n"

    for margin in cfg["margins"]:
        margins += f"  ( {margin}\n"
        for ring, space in cfg["margins"][margin].items():
            margins += f"    (io_row ring_number = {ring} margin = {space})\n"
        margins += "  )\n"
    margins += ")\n\n"

    iopad = "( iopad\n"

    iopad += "  # N65CHIPCDU2_New\n\n"

    orient = {
        "top": "R180",
        "right": "MY90",
        "bottom": "R0",
        "left": "R90"
    }

    for side, info in cfg["placing"].items():
        iopad += f"  ( {side}\n" \
              "    ( locals\n" \
              "      ring_number = 2\n" \
              "      io_order = clockwise\n" \
              "    )\n" \
             f"    ( inst name=\"{side}_N65CHIPCDU2_New\" cell=\"N65CHIPCDU2_New\" orient={orient[side]} offset=187.8 place_status=placed )\n" \
              "  )\n\n"

    iopad += "  # IO ring corners\n\n"

    for corner, info in cfg["ioring"].items():
        iopad += f"  ( {corner}\n" \
                  "    ( locals\n" \
                  "      ring_number = 4\n" \
                  "    )\n" \
                 f"    ( inst name=\"{info['name']}\" cell=\"{info['cell']}\" place_status=placed )\n" \
                  "  )\n\n"

    iopad += "  # BONDPADS\n\n"

    suffix = {
        "input": "_i",
        "output": "_o",
        "inout": "_io"
    }

    vdd_cnt = cfg["VDD"]
    pads = []
    for pad, att in cfg["pads"].items():
        if att["type"] != "bypass_inout":
            if att["num"] > 1:
                for i in range(att.get("num_offset", 0), att.get("num_offset", 0)+att["num"]):
                    pads.append({
                        "original_name": pad,
                        "name": f"{pad}_{i}",
                        "type": att["type"]
                    })
            else:
                pads.append({
                    "original_name": pad,
                    "name": f"{pad}",
                    "type": att["type"]
                })

    side = 0
    io_vdd = True
    vdd_idx = 1
    orientations = {
        "top": 0,
        "bottom": 180,
        "left": 90,
        "right": 270
    }
    space = {
        "top": 89.0625,
        "right": 58.07,
        "bottom": 89.0625,
        "left": 58.07,
    }
    offset = {
        "top": 341.5,
        "right": 197.5,
        "bottom": 341.5,
        "left": 197.5,
    }
    for place, qty in cfg["placing"].items():
        iopad += f"  ( {place}\n" \
                  "    ( locals\n" \
                  "      ring_number = 3\n" \
                  "      io_order = clockwise\n" \
                 f"      space = {space[place]}\n" \
                  "    )\n"
        for idx, the_pad in enumerate(pads[side:side+qty]):
            if the_pad["type"] != "bypass_inout":
                if idx != 0:
                    iopad += f"    ( inst name=\"u_pad_ring/pad_{the_pad['name']}_i_bondpad\" cell=\"PAD60L\" orientation=R{orientations[place]} )\n"
                else:
                    iopad += f"    ( inst name=\"u_pad_ring/pad_{the_pad['name']}_i_bondpad\" cell=\"PAD60L\" orientation=R{orientations[place]} offset={offset[place]})\n"
                if (idx % 7 == 0):
                    if io_vdd:
                        iopad += f"    ( inst name=\"u_pad_ring/pad_IO_VDD_{vdd_idx}_i_bondpad\" cell=\"PAD60L\" orientation=R{orientations[place]} )\n"
                        iopad += f"    ( inst name=\"u_pad_ring/pad_VSS_{vdd_idx}_i_bondpad\" cell=\"PAD60L\" orientation=R{orientations[place]} )\n"
                        io_vdd = False
                    else:
                        iopad += f"    ( inst name=\"u_pad_ring/pad_VDD_{vdd_idx}_i_bondpad\" cell=\"PAD60L\" orientation=R{orientations[place]} )\n"
                        iopad += f"    ( inst name=\"u_pad_ring/pad_VSS_{vdd_idx}_i_bondpad\" cell=\"PAD60L\" orientation=R{orientations[place]} )\n"
                    vdd_idx += 1

        side += qty
        iopad += "  )\n"
    

    side = 0
    io_vdd = True
    vdd_idx = 1
    space = {
        "top": 89.0625 - 5,
        "right": 58.07 - 5,
        "bottom": 89.0625 - 5,
        "left": 58.07 - 5,
    }
    offset = {
        "top": 268,
        "right": 126-2.5,
        "bottom": 268,
        "left": 126-2.5,
    }

    iopad += "  # IO pads\n\n"

    for place, qty in cfg["placing"].items():
        iopad += f"  ( {place}\n" \
                  "    ( locals\n" \
                  "      ring_number = 4\n" \
                 f"      space = {space[place]}\n" \
                  "      io_order = clockwise\n" \
                  "    )\n"
        for idx, the_pad in enumerate(pads[side:side+qty]):
            if the_pad["type"] != "bypass_inout":
                if idx != 0:
                    iopad += f"    ( inst name=\"u_pad_ring/pad_{the_pad['name']}_i/u_pad_cell_{the_pad['type']}/u_pad_{the_pad['type']}\" place_status=fixed )\n"
                else:
                    iopad += f"    ( inst name=\"u_pad_ring/pad_{the_pad['name']}_i/u_pad_cell_{the_pad['type']}/u_pad_{the_pad['type']}\" place_status=fixed offset={offset[place]} )\n"
                if (idx % 7 == 0):
                    if io_vdd:
                        iopad += f"    ( inst name=\"u_pad_ring/pad_IO_VDD_{vdd_idx}_i/u_pad_cell_supply/u_pad_supply\" cell=\"PVDD2CDG\" place_status=fixed )\n"
                        iopad += f"    ( inst name=\"u_pad_ring/pad_VSS_{vdd_idx}_i/u_pad_cell_supply/u_pad_supply\" cell=\"PVSS3CDG\" place_status=fixed )\n"
                        io_vdd = False
                    else:
                        iopad += f"    ( inst name=\"u_pad_ring/pad_VDD_{vdd_idx}_i/u_pad_cell_supply/u_pad_supply\" cell=\"PVDD1CDG\" place_status=fixed )\n"
                        iopad += f"    ( inst name=\"u_pad_ring/pad_VSS_{vdd_idx}_i/u_pad_cell_supply/u_pad_supply\" cell=\"PVSS3CDG\" place_status=fixed )\n"
                    vdd_idx += 1

        side += qty
        iopad += "  )\n"

    iopad += ")\n"

    with open(outfile, "w", encoding="utf-8") as f:
        f.write(preamble)
        f.write(margins)
        f.write(iopad)


if __name__ == "__main__":
    main()