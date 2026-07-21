import json
import re
import shutil
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from scripts.dissertation_evidence import (
    compute_locked_claims,
    load_csv,
    main,
    write_outputs,
)


ROOT = Path(__file__).resolve().parents[2]


class LockedClaimTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.render = load_csv(ROOT / "results" / "_aggregate.csv")
        cls.update = load_csv(ROOT / "results" / "_aggregate_update.csv")
        cls.claims = compute_locked_claims(cls.render, cls.update)

    def test_s1_s2_matched_range(self):
        c = self.claims["C1"]
        self.assertEqual(c["pair_count"], 135)
        self.assertAlmostEqual(c["ratio_min"], 1.7366, places=4)
        self.assertAlmostEqual(c["ratio_max"], 9.9405, places=4)

    def test_stress_configuration(self):
        c = self.claims["C2"]
        self.assertAlmostEqual(c["s3_cpu_us"], 44.745, places=3)
        self.assertAlmostEqual(c["s3_gpu_us"], 236173.0, places=0)

    def test_scene_composition_timing_ratio(self):
        c = self.claims["C3"]
        self.assertAlmostEqual(c["seed9999_gpu_us"], 600999.0, places=0)
        self.assertAlmostEqual(c["ratio_vs_seed42"], 11.37, places=2)
        self.assertAlmostEqual(c["ratio_vs_seed1337"], 11.36, places=2)

    def test_buffer_update_ratio(self):
        c = self.claims["C4"]
        self.assertAlmostEqual(c["batched_mean_us"], 87.07, places=2)
        self.assertAlmostEqual(c["perchunk_mean_us"], 1898.44, places=2)
        self.assertAlmostEqual(c["ratio"], 21.80, places=2)

    def test_duplicate_key_raises_value_error(self):
        duplicate = dict(self.render[0])
        with self.assertRaises(ValueError):
            compute_locked_claims(self.render + [duplicate], self.update)

    def test_missing_required_row_raises_value_error(self):
        missing = [
            row
            for row in self.render
            if not (
                row["scheme"] == 3
                and row["grid"] == 4096
                and row["chunk"] == 4
                and row["density"] == 50
                and row["seed"] == 42
            )
        ]
        with self.assertRaises(ValueError):
            compute_locked_claims(missing, self.update)

    def test_duplicate_update_key_raises_value_error(self):
        duplicate = dict(self.update[0])
        with self.assertRaises(ValueError):
            compute_locked_claims(self.render, self.update + [duplicate])

    def test_missing_c1_s2_row_raises_value_error(self):
        s1 = next(row for row in self.render if row["scheme"] == 1 and row["grid"] >= 256)
        missing = [
            row
            for row in self.render
            if not (
                row["scheme"] == 2
                and (row["grid"], row["chunk"], row["density"], row["seed"])
                == (s1["grid"], s1["chunk"], s1["density"], s1["seed"])
            )
        ]
        with self.assertRaisesRegex(ValueError, "missing S2 rendering-path row for C1"):
            compute_locked_claims(missing, self.update)

    def test_missing_c3_seed_raises_value_error(self):
        missing = [
            row
            for row in self.render
            if not (
                row["scheme"] == 3
                and row["grid"] == 4096
                and row["chunk"] == 16
                and row["density"] == 80
                and row["seed"] == 9999
            )
        ]
        with self.assertRaisesRegex(ValueError, "C3 S3 rendering-path seed 9999"):
            compute_locked_claims(missing, self.update)

    def test_c1_zero_denominator_raises_contextual_value_error(self):
        rows = [dict(row) for row in self.render]
        s1 = next(row for row in rows if row["scheme"] == 1 and row["grid"] >= 256)
        s2 = next(
            row
            for row in rows
            if row["scheme"] == 2
            and (row["grid"], row["chunk"], row["density"], row["seed"])
            == (s1["grid"], s1["chunk"], s1["density"], s1["seed"])
        )
        s2["cpu_record_avg"] = 0.0
        with self.assertRaisesRegex(ValueError, "C1 denominator"):
            compute_locked_claims(rows, self.update)

    def test_c3_zero_denominator_raises_contextual_value_error(self):
        rows = [dict(row) for row in self.render]
        next(
            row
            for row in rows
            if row["scheme"] == 3
            and row["grid"] == 4096
            and row["chunk"] == 16
            and row["density"] == 80
            and row["seed"] == 42
        )["gpu_exec_avg"] = 0.0
        with self.assertRaisesRegex(ValueError, "C3 denominator"):
            compute_locked_claims(rows, self.update)

    def test_c4_zero_denominator_raises_contextual_value_error(self):
        rows = [dict(row) for row in self.update]
        for row in rows:
            if row["update_size"] == 32 and row["mode"] == "batched":
                row["update_cost_avg"] = 0.0
        with self.assertRaisesRegex(ValueError, "C4 denominator"):
            compute_locked_claims(self.render, rows)

    def test_cli_records_actual_source_files_in_both_artifacts(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            render_path = temp / "custom-render.csv"
            update_path = temp / "custom-update.csv"
            output = temp / "evidence"
            shutil.copyfile(ROOT / "results" / "_aggregate.csv", render_path)
            shutil.copyfile(ROOT / "results" / "_aggregate_update.csv", update_path)
            with patch(
                "sys.argv",
                [
                    "dissertation_evidence.py",
                    "--render",
                    str(render_path),
                    "--update",
                    str(update_path),
                    "--out",
                    str(output),
                ],
            ):
                main()
            document = json.loads((output / "locked-claims.json").read_text(encoding="utf-8"))
            markdown = (output / "locked-claims.md").read_text(encoding="utf-8")
            self.assertEqual(document["source_files"], [str(render_path), str(update_path)])
            self.assertIn(str(render_path), markdown)
            self.assertIn(str(update_path), markdown)

    def test_paired_artifacts_share_generated_timestamp(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = Path(temp_dir)
            write_outputs(self.claims, output)
            document = json.loads((output / "locked-claims.json").read_text(encoding="utf-8"))
            markdown = (output / "locked-claims.md").read_text(encoding="utf-8")
            timestamp = re.search(r"Generated UTC: ([^\r\n]+)", markdown).group(1)
            self.assertEqual(document["generated_at_utc"], timestamp)


if __name__ == "__main__":
    unittest.main()
