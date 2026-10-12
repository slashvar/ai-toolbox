import unittest

from format_md import find_issues, format_text

TABLE = "| a | bb |\n|---|---|\n| ccc | d |"
ALIGNED = "| a   | bb |\n|-----|----|\n| ccc | d  |"


class FormatTest(unittest.TestCase):
    def test_aligns_table(self):
        self.assertEqual(format_text(TABLE), ALIGNED)

    def test_idempotent(self):
        self.assertEqual(format_text(ALIGNED), ALIGNED)

    def test_fenced_pipes_untouched(self):
        for fence in ("```text", "~~~text", "````text"):
            closer = fence[: len(fence) - 4]
            doc = f"{fence}\n| x | y |\n  | z\n{closer}\n\n{TABLE}"
            want = f"{fence}\n| x | y |\n  | z\n{closer}\n\n{ALIGNED}"
            self.assertEqual(format_text(doc), want, fence)

    def test_short_fence_does_not_close_long_one(self):
        doc = "````text\n```\n| x |\n````"
        self.assertEqual(format_text(doc), doc)

    def test_bare_fence_flagged(self):
        self.assertEqual(
            find_issues("text\n\n```\nx\n```\n"),
            ["line 3: code fence has no language"],
        )

    def test_closing_fence_not_flagged(self):
        self.assertEqual(find_issues("```text\nx\n```\n"), [])

    def test_escaped_pipe_stays_in_cell(self):
        table = "| a | b |\n|---|---|\n| x\\|y | z |"
        want = "| a    | b |\n|------|---|\n| x\\|y | z |"
        self.assertEqual(format_text(table), want)
        self.assertEqual(format_text(want), want)


if __name__ == "__main__":
    unittest.main()
