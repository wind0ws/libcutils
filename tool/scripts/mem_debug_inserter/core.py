"""Core logic: detect/insert `#include "mem/mem_debug.h"` as the first include.

Pure functions over file contents — no argument parsing, no stdout. The CLI
layer (cli.py) drives these and renders the report.
"""
import re
from pathlib import Path

INCLUDE_LINE = '#include "mem/mem_debug.h"'
DEFAULT_EXTS = ('c', 'cpp', 'cc', 'cxx')
DEFAULT_EXCLUDE_DIRS = {'.git', 'build', 'out', 'cmake-build-debug',
                        'cmake-build-release', 'third_party', 'external',
                        'node_modules', '.vs', 'Debug', 'Release'}

RE_INCLUDE = re.compile(r'^#\s*include\b')
RE_IF = re.compile(r'^#\s*(?:if|ifdef|ifndef)\b')
RE_ENDIF = re.compile(r'^#\s*endif\b')
RE_MEMDEBUG = re.compile(r'#\s*include\s*[<"]\s*(?:mem/)?mem_debug\.h\s*[">]')
RE_SKIP = re.compile(r'lcu-mem-debug:\s*skip', re.I)


def read_source(path):
    """Return (lines, newline, has_bom) preserving the file's formatting."""
    raw = Path(path).read_bytes()
    has_bom = raw.startswith(b'\xef\xbb\xbf')
    if has_bom:
        raw = raw[3:]
    text = raw.decode('utf-8')
    crlf = text.count('\r\n')
    lf = text.count('\n') - crlf
    newline = '\r\n' if crlf >= lf else '\n'
    lines = text.split('\n')
    lines = [ln[:-1] if ln.endswith('\r') else ln for ln in lines]
    return lines, newline, has_bom


def write_source(path, lines, newline, has_bom):
    data = newline.join(lines).encode('utf-8')
    if has_bom:
        data = b'\xef\xbb\xbf' + data
    Path(path).write_bytes(data)


def strip_comments(lines):
    """Yield (idx, code_only) with // and /* */ comments blanked, so directive
    detection never trips on commented-out #include lines."""
    in_block = False
    out = []
    for idx, line in enumerate(lines):
        s, res, i = line, [], 0
        while i < len(s):
            if in_block:
                end = s.find('*/', i)
                if end == -1:
                    i = len(s)
                else:
                    in_block = False
                    i = end + 2
            else:
                blk = s.find('/*', i)
                lin = s.find('//', i)
                if lin != -1 and (blk == -1 or lin < blk):
                    res.append(s[i:lin])
                    i = len(s)
                elif blk != -1:
                    res.append(s[i:blk])
                    i = blk + 2
                    in_block = True
                else:
                    res.append(s[i:])
                    i = len(s)
        out.append((idx, ''.join(res)))
    return out


def scan_includes(lines):
    """Return (includes, skip). Each include record: {idx, depth, top_if,
    is_memdebug}. depth = #if nesting; top_if = idx of the outermost open #if
    (None at top level)."""
    code = strip_comments(lines)
    includes, depth, top_if = [], 0, None
    skip = any(RE_SKIP.search(ln) for ln in lines[:40])
    for idx, c in code:
        stripped = c.strip()
        if RE_IF.match(stripped):
            if depth == 0:
                top_if = idx
            depth += 1
        elif RE_ENDIF.match(stripped):
            depth = max(0, depth - 1)
            if depth == 0:
                top_if = None
        elif RE_INCLUDE.match(stripped):
            includes.append({
                'idx': idx,
                'depth': depth,
                'top_if': top_if if depth > 0 else None,
                'is_memdebug': bool(RE_MEMDEBUG.search(lines[idx])),
            })
    return includes, skip


def _anchor_idx(includes, skip_idx):
    """Line to insert before so mem_debug.h becomes the top-level first include.
    If the first real include is nested in an #if, anchor before that outermost
    #if. Returns (idx, warn_if_nested)."""
    for r in includes:
        if r['idx'] == skip_idx:
            continue
        if r['depth'] > 0 and r['top_if'] is not None:
            return r['top_if'], True
        return r['idx'], False
    return skip_idx, False


def analyze(path):
    """Return a dict describing the proposed action for one file:
    status in {OK, INSERT, MOVE, SKIP, NONE} plus anchor/remove indices."""
    lines, newline, has_bom = read_source(path)
    includes, skip = scan_includes(lines)
    info = {'path': Path(path), 'lines': lines, 'newline': newline,
            'has_bom': has_bom, 'status': 'OK', 'detail': '',
            'anchor': None, 'remove': None}

    if skip:
        info['status'] = 'SKIP'
        info['detail'] = 'lcu-mem-debug: skip marker present'
        return info
    if not includes:
        info['status'] = 'NONE'
        info['detail'] = 'no #include found — nothing to anchor to'
        return info

    md = [r for r in includes if r['is_memdebug']]
    first = includes[0]

    if md:
        m = md[0]
        if m['idx'] == first['idx'] and m['depth'] == 0:
            info['detail'] = 'mem_debug.h already first include'
            return info
        anchor, warn = _anchor_idx(includes, skip_idx=m['idx'])
        info['status'] = 'MOVE'
        info['remove'] = m['idx']
        info['anchor'] = anchor
        info['detail'] = ('mem_debug.h present but not first include (line %d)'
                          ' -> relocate to line %d' % (m['idx'] + 1, anchor + 1))
        if warn:
            info['detail'] += '  [WARN: first include sits inside #if block]'
        return info

    anchor, warn = _anchor_idx(includes, skip_idx=None)
    info['status'] = 'INSERT'
    info['anchor'] = anchor
    info['detail'] = 'insert mem_debug.h before line %d' % (anchor + 1)
    if warn:
        info['detail'] += '  [WARN: first include sits inside #if block]'
    return info


def apply_change(info, backup=False):
    """Mutate the file on disk according to a prior analyze() result."""
    lines = list(info['lines'])
    remove, anchor = info['remove'], info['anchor']
    if remove is not None:
        del lines[remove]
        if remove < anchor:
            anchor -= 1
    lines.insert(anchor, INCLUDE_LINE)
    if backup:
        bak = info['path'].with_suffix(info['path'].suffix + '.bak')
        write_source(bak, info['lines'], info['newline'], info['has_bom'])
    write_source(info['path'], lines, info['newline'], info['has_bom'])


def iter_files(paths, exts=DEFAULT_EXTS, exclude_dirs=DEFAULT_EXCLUDE_DIRS):
    """Yield source files under the given paths matching exts, skipping
    excluded directory names."""
    suffixes = {'.' + e.lower().lstrip('.') for e in exts}
    for p in paths:
        p = Path(p)
        if p.is_file():
            if p.suffix.lower() in suffixes:
                yield p
            continue
        for f in sorted(p.rglob('*')):
            if f.is_file() and f.suffix.lower() in suffixes:
                if any(part in exclude_dirs for part in f.parts):
                    continue
                yield f