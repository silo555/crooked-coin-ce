"""Package a reviewed Git commit and validated calculator build; no private inputs."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile

from verify_8xp import read_icon_asm, verify

ROOT = Path(__file__).resolve().parents[1]
VERSION = "v0.1.0"


def main():
    status = subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True)
    if status.strip():
        raise SystemExit("Commit the reviewed source before packaging; working tree is not clean.")
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    binary = ROOT / "bin/CROOKED.8xp"
    report = verify(binary.read_bytes(), icon=read_icon_asm(ROOT / "obj/icon.s"))
    output = ROOT / "dist" / VERSION
    output.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(binary, output / "CROOKED.8xp")
    calculator = output / f"CrookedCoinCE-{VERSION}-calculator.zip"
    inputs = [ROOT / name for name in ("README.md", "LICENSE", "CREDITS.md", "CHANGELOG.md")]
    inputs += sorted((ROOT / "docs").rglob("*"))
    inputs += sorted((ROOT / "assets").glob("*.png"))
    inputs += sorted((ROOT / "tests/fixtures").glob("*.json"))
    with zipfile.ZipFile(calculator, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.write(binary, "CrookedCoinCE/CROOKED.8xp")
        for path in inputs:
            if path.is_file():
                archive.write(path, "CrookedCoinCE/" + path.relative_to(ROOT).as_posix())
        archive.writestr("CrookedCoinCE/BUILD.txt", f"Crooked Coin CE {VERSION}\nSource commit: {commit}\nNative binary SHA-256: {report['sha256']}\nPreview: no physical calculator or CEmu execution verified.\nLibraries: https://github.com/CE-Programming/libraries/releases/tag/v15.0\n")
    source = output / f"CrookedCoinCE-{VERSION}-source.zip"
    subprocess.run(["git", "archive", "--format=zip", "--prefix=CrookedCoinCE-source/", "HEAD", "-o", str(source)], cwd=ROOT, check=True)
    report.update(version=VERSION, source_commit=commit, calculator_execution="unverified", cemu_execution="unverified")
    (output / "BUILD.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    files = [output / "CROOKED.8xp", calculator, source, output / "BUILD.json"]
    manifest = "".join(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n" for p in files)
    (output / "SHA256SUMS.txt").write_text(manifest, encoding="ascii")
    for path in (calculator, source):
        with zipfile.ZipFile(path) as archive:
            if archive.testzip() is not None:
                raise SystemExit(f"ZIP integrity failure: {path.name}")
    print(json.dumps(report, indent=2))
    print(f"Verified calculator/source packages and SHA-256 manifest in {output}")


if __name__ == "__main__":
    main()
