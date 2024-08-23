import os
import re
from pathlib import Path

def collect_headers(header_dirs: list[Path]) -> dict[str, Path]:
    headers = {}
    for header_dir in header_dirs:
        for path in header_dir.iterdir():
            if not path.is_file():
                continue
            filename = path.name
            if filename.lower().endswith('.h') or filename.lower().endswith('.hpp'):
                if filename in headers:
                    raise ValueError(f"Duplicate header file: {filename}")
                headers[filename] = path
    return headers


_include_line_re = re.compile(r'^\s*[#]\s*include\s*([<"])([^>"]+)([>"])\s*$')


def handle_header_line(headers: dict[str, Path], header_includes: set[str], header_stdincludes: set[str], line: str):
    m = _include_line_re.match(line)
    if not m or m.group(1) != m.group(3).replace(">", "<"):
        raise ValueError(f"Invalid #include line: '{line}'")
    is_system = (m.group(1) == '<')
    header_name = m.group(2)
    if is_system:
        header_stdincludes.add(header_name)
    else:
        if header_name not in headers:
            raise ValueError(f"Unknown header: {header_name} included in line '{line}'")
        header_includes.add(header_name)


def collect_includes(headers: dict[str, Path]) -> tuple[dict[str, list[str]], dict[str, list[str]]]:
    header_includes: dict[str, list[str]] = {}
    stdincludes: dict[str, list[str]] = {}
    for header in headers:
        this_includes = set()
        this_stdincludes = set()
        with open(headers[header], 'r') as f:
            lines = f.readlines()
            for line_ in lines:
                line = line_.strip()
                if line.startswith('#include'):
                    handle_header_line(headers, this_includes, this_stdincludes, line)
        header_includes[header] = list(this_includes)
        stdincludes[header] = list(this_stdincludes)
    return header_includes, stdincludes


def guard_name(output_path: Path):
    name = "_".join(output_path.parts[-2:]).upper()
    for forbidden in ('-', ' ', '/', '+', '.'):
        name = name.replace(forbidden, '_')
    return f'{name}_INCLUDED_'

def write_output_header(all_headers: dict[str, Path], header_order: list[str], 
                        stdincludes: set[str], output_header: Path):
    guard = guard_name(output_header)
    with output_header.open('w') as f:
        f.write(f'#ifndef {guard}\n')
        f.write(f'#define {guard}\n\n')
        f.write('/// DO NOT EDIT THIS AUTO-GENERATED FILE\n\n')
        f.write('/// Standard library includes\n')
        for header in stdincludes:
            f.write(f'#include <{header}>\n')
        f.write('\n/// Project headers concatenated into a single header\n')
        for header in header_order:
            f.write(f'/// Original header: #include "{header}"\n')
            path = all_headers[header]
            with path.open('r') as header_file:
                lines = header_file.readlines()
                for line in lines:
                    if line.strip().startswith('#include'):
                        continue
                    f.write(line)
            f.write(f"/// End original header: '{header}'\n\n")
        f.write(f'#endif // {guard}\n')

def make_single_header(header_dirs, output_header):
    all_headers = collect_headers(header_dirs)
    header_includes, stdincludes = collect_includes(all_headers)
    header_dependencies = {header: set(includes) for header, includes in header_includes.items()}
    header_order = []
    while header_dependencies:
        found = False
        header_names = list(header_dependencies.keys())
        for header in header_names:
            dependencies = header_dependencies[header]
            if not dependencies:
                header_order.append(header)
                del header_dependencies[header]
                for deps in header_dependencies.values():
                    deps.discard(header)
                found = True
        if not found:
            raise ValueError("Circular dependency detected")
    stdinclude_order = set()
    for header in header_order:
        stdinclude_order.update(stdincludes[header])
    os.makedirs(Path(output_header).parent, exist_ok=True)
    write_output_header(all_headers, header_order, stdinclude_order, output_header)


if __name__ == "__main__":
    this_path = Path(__file__).parent
    header_dirs = [this_path / 'include' / 'standalone-propagator']
    make_single_header(header_dirs, this_path / 'single_header' / 'standalone-propagator' / 'standalone-propagator.h')
