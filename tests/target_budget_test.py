# SPDX-License-Identifier: GPL-3.0-only
"""Regression cases for target backward joins versus actual loops."""
import tempfile
from pathlib import Path
import unittest
from target_budget import cost, functions


class TargetLoopTests(unittest.TestCase):
    def test_backward_exit_join_is_not_a_loop(self):
        # Given: two branches join an earlier return block.
        code = [(0, "if (r0 == 0) goto 6 <f : 6 >"),
                (2, "r0 = 0"), (4, "{pc} = [sp++]"),
                (6, "r0 = 1"), (8, "goto -6 <f : 2 >")]
        # When: the static checker scores the function.
        result = cost(code)
        # Then: this acyclic function contributes no loop work.
        self.assertEqual(result["cost"], 0)

    def test_real_loop_keeps_its_weight(self):
        code = [(0, "r0 = 0"), (2, "r0 += 1"),
                (4, "if (r0 < r1) goto -2 <f : 2 >"),
                (6, "{pc} = [sp++]")]
        result = cost(code)
        self.assertEqual(result["cost"], 2)

    def test_nested_loop_keeps_divide_weight(self):
        code = [(0, "r0 = 0"), (2, "r1 = 0"),
                (4, "r2 = r2 / r3"),
                (6, "if (r1 < r3) goto -2 <f : 4 >"),
                (8, "if (r0 < r3) goto -6 <f : 2 >"),
                (10, "{pc} = [sp++]")]
        result = cost(code)
        self.assertEqual(result["cost"], 42)

    def test_rotated_loop_counts_forward_backedge(self):
        # Entry skips the update; the real dominance backedge is fallthrough.
        code = [(0, "goto 4 <f : 4 >"), (2, "r0 += 1"),
                (4, "r1 += r0"),
                (6, "if (r0 < r2) goto -4 <f : 2 >"),
                (8, "{pc} = [sp++]")]
        result = cost(code)
        self.assertEqual(result["cost"], 3)

    def test_unknown_switch_keeps_conservative_score(self):
        code = [(0, "r0 = 0"), (2, "tbb [r0]"),
                (4, "r0 += 1"), (6, "goto -4 <f : 2 >")]
        result = cost(code)
        self.assertEqual(result["cost"], 3)

    def test_irreducible_cycle_keeps_conservative_score(self):
        code = [(0, "if (r0 == 0) goto 4 <f : 4 >"),
                (2, "goto 2 <f : 4 >"),
                (4, "if (r0 != 0) goto -2 <f : 2 >"),
                (6, "{pc} = [sp++]")]
        result = cost(code)
        self.assertEqual(result["cost"], 2)

    def test_switch_table_preserves_every_successor(self):
        text = ("slice_render:\n"
                " 0: 00 01  tbb [r0]\n.GJTIS0:\n"
                " 2: 02     < 2 : 0x2 >\n"
                " 3: 04     < 4 : 0x4 >\n"
                " 6: 00 01  {pc} = [sp++]\n"
                " 8: 00 01  r0 = 0\n"
                " a: 00 01  r0 += 1\n"
                " c: 00 01  if (r0 < r1) goto -2 <f : a >\n"
                " e: 00 01  {pc} = [sp++]\n")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "switch.dis"
            path.write_text(text)
            code = functions(path)["slice_render"]
        # A real loop is reachable only through the non-first table entry.
        self.assertEqual(cost(code)["cost"], 2)

    def test_self_loop_is_counted(self):
        code = [(0, "if (r0 != 0) goto 0 <f : 0 >"), (2, "{pc} = [sp++]")]
        self.assertEqual(cost(code)["cost"], 1)

    def test_multiple_latches_share_one_loop(self):
        code = [(0, "r0 = 0"), (2, "r0 += 1"),
                (4, "if (r0 < r1) goto -2 <f : 2 >"),
                (6, "r1 += 1"), (8, "if (r1 < r2) goto -6 <f : 2 >"),
                (10, "{pc} = [sp++]")]
        self.assertEqual(cost(code)["cost"], 4)

    def test_missing_local_target_keeps_conservative_score(self):
        code = [(0, "r0 = 0"), (2, "r1 += 1"),
                (4, "if (r1 < r2) goto -3 <f : 1 >"),
                (6, "{pc} = [sp++]")]
        self.assertEqual(cost(code)["cost"], 2)

    def test_unsupported_halfword_switch_keeps_conservative_score(self):
        code = [(0, "r0 = 0"), (2, "tbh [r0]"),
                (4, "r0 += 1"), (6, "goto -4 <f : 2 >")]
        self.assertEqual(cost(code)["cost"], 3)


if __name__ == "__main__":
    unittest.main()
