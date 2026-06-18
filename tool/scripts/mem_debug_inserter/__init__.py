"""mem_debug_inserter — ensure `#include "mem/mem_debug.h"` is first in C/C++ sources.

Usage:
    python -m mem_debug_inserter [paths...] [--apply] [--help]

Default: dry-run report. Use --apply to write changes.
"""
__version__ = '1.0.0'

from .core import analyze, apply_change, iter_files
from .cli import main

__all__ = ['analyze', 'apply_change', 'iter_files', 'main']
