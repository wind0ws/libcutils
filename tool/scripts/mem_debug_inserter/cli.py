"""Command-line interface: argument parsing and report rendering."""
import argparse
import sys
from pathlib import Path

from . import core


def main(argv=None):
    """Entry point: parse args, run analysis, optionally apply changes, print report."""
    ap = argparse.ArgumentParser(
        prog='mem_debug_inserter',
        description=(
            'Ensure `#include "mem/mem_debug.h"` is the FIRST include in C/C++ '
            'source files, so LCU memory checking takes effect. Default: dry-run '
            'report. Use --apply to write changes.'
        ),
        epilog=(
            'Per-file opt-out: add a comment containing "lcu-mem-debug: skip" '
            '(e.g. for files like allocation_tracker.c that must NOT include it).'
        ),
    )
    ap.add_argument('paths', nargs='*', default=['.'], metavar='PATH',
                    help='files or directories to scan (default: current directory)')
    ap.add_argument('--apply', action='store_true',
                    help='perform edits (default: dry-run report only)')
    ap.add_argument('--ext', default=','.join(core.DEFAULT_EXTS), metavar='EXTS',
                    help='comma-separated source extensions (default: c,cpp,cc,cxx)')
    ap.add_argument('--exclude', default='', metavar='DIRS',
                    help='extra comma-separated directory names to skip '
                         '(build, .git, Debug, Release, etc. already excluded)')
    ap.add_argument('--backup', action='store_true',
                    help='write a .bak copy before editing (with --apply)')
    ap.add_argument('--quiet', action='store_true',
                    help='only print files that need changes')
    args = ap.parse_args(argv)

    paths = args.paths or ['.']
    exts = [e.strip() for e in args.ext.split(',') if e.strip()]
    exclude = set(core.DEFAULT_EXCLUDE_DIRS)
    exclude |= {e.strip() for e in args.exclude.split(',') if e.strip()}

    counts = {}
    changed = []

    for f in core.iter_files(paths, exts, exclude):
        info = core.analyze(f)
        st = info['status']
        counts[st] = counts.get(st, 0) + 1
        actionable = st in ('INSERT', 'MOVE')
        if actionable:
            changed.append(info)
        if not args.quiet or actionable:
            print('[%-6s] %s' % (st, info['path']))
            if info['detail'] and (actionable or st in ('SKIP', 'NONE')):
                print('         %s' % info['detail'])

    print('\n--- summary ---')
    for st in ('OK', 'INSERT', 'MOVE', 'SKIP', 'NONE'):
        if st in counts:
            print('  %-6s : %d' % (st, counts[st]))

    if args.apply:
        for info in changed:
            core.apply_change(info, args.backup)
        print('\nApplied %d change(s).' % len(changed))
    else:
        if changed:
            print('\nDry-run. Re-run with --apply to write %d change(s).'
                  % len(changed))
        else:
            print('\nDry-run. No changes needed.')
    return 0


if __name__ == '__main__':
    sys.exit(main())