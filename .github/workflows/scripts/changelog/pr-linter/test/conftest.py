import os
import sys

# Determine the absolute path to the pr_linter root (which contains "src")
pr_linter_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if pr_linter_root not in sys.path:
    sys.path.insert(0, pr_linter_root)
    
pr_linter_src = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "src"))
if pr_linter_src not in sys.path:
    sys.path.insert(0, pr_linter_src)