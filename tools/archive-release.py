"""Create a reproducible-layout install ZIP with correct PS5 executable modes."""
import pathlib
import stat
import subprocess
import sys
import zipfile

source = pathlib.Path(sys.argv[1]).resolve(strict=True)
destination = pathlib.Path(sys.argv[2]).resolve()
source_only = len(sys.argv) > 3 and sys.argv[3] == '--source'
if source in destination.parents and not source_only:
    raise ValueError('Archive output must be outside its input directory')
if source_only:
    listed = subprocess.check_output(['git', '-C', str(source), 'ls-files', '--cached', '--others', '--exclude-standard', '-z'])
    paths = sorted({source / name.decode('utf-8') for name in listed.split(b'\0') if name})
else:
    paths = sorted(source.rglob('*'))
with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for path in paths:
        if path == destination:
            raise ValueError('Source archive must be Git-ignored')
        if path.is_symlink():
            raise ValueError(f'Symlink in release input: {path}')
        if not path.is_file():
            continue
        name = path.relative_to(source).as_posix()
        info = zipfile.ZipInfo.from_file(path, arcname=name)
        info.create_system = 3
        executable = path.name == 'eboot.bin' or path.suffix in ('.elf', '.prx')
        info.external_attr = (stat.S_IFREG | (0o755 if executable else 0o644)) << 16
        info.compress_type = zipfile.ZIP_DEFLATED
        with path.open('rb') as reader, archive.open(info, 'w') as writer:
            while data := reader.read(1024 * 1024):
                writer.write(data)
