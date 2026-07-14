#!/usr/bin/env python3

# Copyright 2024 Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: crheepto_gen.py
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
        prog="crheepto_gen.py",
        description="Generate crheepto HDL files based on the provided configuration.",
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
        "--tpl-sv", "-s", type=str, metavar="SV", help="SystemVerilog template filename"
    )
    parser.add_argument(
        "--tpl-c", "-C", type=str, metavar="C_SOURCE", help="C template filename"
    )
    parser.add_argument(
        "--corev_pulp", nargs="?", type=bool, help="CORE-V PULP extension"
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

    # Get configuration parameters
    # ----------------------------
    # CORE-V-MINI-MCU configuration
    cpu_features = cfg["cpu_features"]
    if args.corev_pulp != None:
        cpu_features["corev_pulp"] = args.corev_pulp

    # Bus configuration
    if "ext_xbar_nmasters" not in cfg:
        print("No external crossbar masters defined")
        xbar_nmasters = 0
    else:
        xbar_nmasters = int(cfg["ext_xbar_masters"])

    if "ext_xbar_slaves" not in cfg:
        print("No external crossbar slaves defined")
        xbar_nslaves = 0
    else:
        xbar_nslaves = int(len(cfg["ext_xbar_slaves"]))

    # Memory configuration
    slaves = []
    if xbar_nslaves > 0:
        idx = 0
        for a_slave, slave_config in cfg["ext_xbar_slaves"].items():
            slaves.append(
                {
                    "name": CamelCase(a_slave),
                    "SCREAMING_NAME": SCREAMING_SNAKE_CASE(a_slave),
                    "idx": idx,
                    "offset": string2int(slave_config["offset"]),
                    "size": string2int(slave_config["length"]),
                    "end_address": string2int(slave_config["offset"])
                    + string2int(slave_config["length"]),
                }
            )
            idx += 1

    if "ext_periph" not in cfg:
        print("No external peripherals defined")
        periph_nslaves = 0
    else:
        periph_nslaves = len(cfg["ext_periph"])

    # Memory configuration
    peripherals = []
    if periph_nslaves > 0:
        idx = 0
        for a_peripheral, peripheral_config in cfg["ext_periph"].items():
            peripherals.append(
                {
                    "name": CamelCase(a_peripheral),
                    "SCREAMING_NAME": SCREAMING_SNAKE_CASE(a_peripheral),
                    "idx": idx,
                    "offset": string2int(peripheral_config["offset"]),
                    "size": string2int(peripheral_config["length"]),
                    "end_address": string2int(peripheral_config["offset"])
                    + string2int(peripheral_config["length"]),
                }
            )
            idx += 1

    # AO SPC configuration
    if "ao_spc_num" not in cfg:
        print("No AO SPC defined")
        ao_spc_num = 0
    else:
        ao_spc_num = int(cfg["ao_spc_num"])

    # External interrupts
    if "external_interrupts" not in cfg:
        print("No EXT interrupts defined")
        ext_interrupts = 0
    else:
        ext_interrupts = int(cfg["external_interrupts"])

    # Explicit arguments
    kwargs = {
        "cpu_corev_pulp": int(cpu_features["corev_pulp"]),
        "cpu_corev_xif": int(cpu_features["corev_xif"]),
        "cpu_fpu": int(cpu_features["fpu"]),
        "cpu_riscv_zfinx": int(cpu_features["riscv_zfinx"]),
        "xbar_nmasters": xbar_nmasters,
        "xbar_nslaves": xbar_nslaves,
        "periph_nslaves": periph_nslaves,
        "ao_spc_num": ao_spc_num,
        "slaves": slaves,
        "peripherals": peripherals,
        "ext_interrupts": ext_interrupts,
    }

    # Generate SystemVerilog package
    if args.tpl_sv is not None:
        write_template(args.tpl_sv, args.outdir, **kwargs)

    # Generate C header
    if args.tpl_c is not None:
        write_template(args.tpl_c, args.outdir, **kwargs)


if __name__ == "__main__":
    main()
