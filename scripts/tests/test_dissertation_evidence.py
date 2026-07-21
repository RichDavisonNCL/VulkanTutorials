import unittest
from pathlib import Path

from scripts.dissertation_evidence import compute_locked_claims, load_csv


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


if __name__ == "__main__":
    unittest.main()
