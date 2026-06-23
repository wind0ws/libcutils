"""Allow running: python -m mem_debug_inserter"""
import sys
from .cli import main

if __name__ == '__main__':
    sys.exit(main())
