"""Cliff-generator reuse/refusal checks; no Unreal rendering claim.

Author: Angelis Pseftis.
"""
import importlib.util
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import patch


class Material:
    def __init__(self):
        self.outputs = {
            "base": object(),
            "metallic": Scalar("Metallic", .02),
            "roughness": Scalar("Roughness", .90),
        }


class Scalar:
    def __init__(self, name, value):
        self.properties = {"parameter_name": name, "default_value": value}

    def get_editor_property(self, name):
        return self.properties[name]


class CliffMaterialReuseTests(unittest.TestCase):
    def setUp(self):
        self.material = Material()
        fake_unreal = types.SimpleNamespace(
            Material=Material,
            MaterialExpressionScalarParameter=Scalar,
            MaterialProperty=types.SimpleNamespace(
                MP_BASE_COLOR="base", MP_METALLIC="metallic",
                MP_ROUGHNESS="roughness", MP_EMISSIVE_COLOR="emissive"),
            MaterialEditingLibrary=types.SimpleNamespace(
                get_material_property_input_node=lambda material, prop: material.outputs.get(prop)),
            EditorAssetLibrary=types.SimpleNamespace(
                does_asset_exist=lambda path: True,
                load_asset=lambda path: self.material,
                get_metadata_tag=lambda material, key: "cliff-surface-3d-basalt-v4"),
        )
        # The fake exposes reads only. A valid exact-revision reuse must not
        # save, rewrite or recompile the material to pass these checks.
        with patch.dict(sys.modules, {"unreal": fake_unreal}):
            spec = importlib.util.spec_from_file_location(
                "cliff_material_validation", Path(__file__).parents[2] / "Scripts/echoes_cliff_material.py")
            self.generator = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(self.generator)

    def test_valid_current_asset_is_reused_without_mutation(self):
        self.assertIs(self.generator.create_cliff_material(), self.material)

    def test_current_tag_cannot_hide_missing_graph_outputs(self):
        for prop in ("base", "metallic", "roughness"):
            with self.subTest(output=prop):
                self.material = Material()
                del self.material.outputs[prop]
                with self.assertRaisesRegex(RuntimeError, "lost required graph output"):
                    self.generator.create_cliff_material()

    def test_current_tag_cannot_hide_an_emissive_connection(self):
        self.material.outputs["emissive"] = object()
        with self.assertRaisesRegex(RuntimeError, "must not contain an emissive path"):
            self.generator.create_cliff_material()

    def test_registered_scalar_identity_and_values_are_checked(self):
        for value in (.1, float("nan"), float("inf")):
            with self.subTest(roughness=value):
                self.material.outputs["roughness"] = Scalar("Roughness", value)
                with self.assertRaisesRegex(RuntimeError, "invalid registered Roughness"):
                    self.generator.create_cliff_material()
        self.material = Material()
        self.material.outputs["metallic"] = Scalar("OtherParameter", .02)
        with self.assertRaisesRegex(RuntimeError, "invalid registered Metallic"):
            self.generator.create_cliff_material()
        self.material.outputs["metallic"] = object()
        with self.assertRaisesRegex(RuntimeError, "invalid registered Metallic"):
            self.generator.create_cliff_material()

    def test_rebuilt_output_must_remain_bound_to_the_created_node(self):
        expected = dict(self.material.outputs)
        self.material.outputs["base"] = object()
        with self.assertRaisesRegex(RuntimeError, "lost required graph output"):
            self.generator.validate_cliff_material(self.material, expected)


if __name__ == "__main__":
    unittest.main()
