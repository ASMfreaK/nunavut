#
# Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# Copyright (C) 2018-2019  UAVCAN Development Team  <uavcan.org>
# This software is distributed under the terms of the MIT License.
#
import json
from pathlib import Path

import pytest
from nunavut import build_namespace_tree
from nunavut.jinja import DSDLCodeGenerator
from pydsdl import read_namespace
from nunavut import generate_types


@pytest.mark.parametrize("std,expect_uses_variant", [("c++14", False), ("c++17", True)])
def test_ifuses(gen_paths, configurable_language_context_factory, std, expect_uses_variant):  # type: ignore
    """
    Verifies that instance tests are added for pydsdl.SerializableType and
    all of its subclasses.
    """
    config_overrides = {"nunavut.lang.cpp": {"options": {"std": std}}}
    root_namespace_dir = gen_paths.dsdl_dir / Path("denada")
    type_map = read_namespace(str(root_namespace_dir), [])
    language_context = configurable_language_context_factory(config_overrides, "cpp")
    namespace = build_namespace_tree(type_map, root_namespace_dir, gen_paths.out_dir, language_context)
    generator = DSDLCodeGenerator(namespace, templates_dir=gen_paths.templates_dir)
    generator.generate_all(False)

    outfile = gen_paths.find_outfile_in_namespace("denada.serializables", namespace)

    assert outfile is not None

    with open(str(outfile), "r") as json_file:
        json_blob = json.load(json_file)

    assert json_blob is not None
    assert not json_blob["never"]
    assert json_blob["ifuses_std_variant"] is expect_uses_variant
    assert json_blob["ifnuses_std_variant"] is not expect_uses_variant
