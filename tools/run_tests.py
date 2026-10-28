"""Compile and execute the portable C tests, without a calculator or ROM."""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SUITES = (
    ("game", ["tests/test_game.c", "src/game.c"]),
    ("journal", ["tests/test_journal.c", "src/journal.c"]),
    ("ui", ["tests/test_ui.c", "src/game.c", "src/ui.c", "src/render.c"]),
    ("integration", ["tests/test_integration.c", "src/game.c", "src/ui.c", "src/render.c", "src/journal.c"]),
)


def compiler_command(explicit=None):
    selected = explicit or os.environ.get("CC")
    if selected:
        command = [selected] if Path(selected).is_file() else shlex.split(selected)
    else:
        found = next((shutil.which(name) for name in ("cc", "clang", "gcc", "zig") if shutil.which(name)), None)
        if not found:
            raise SystemExit("No host C compiler found. Set CC to clang, gcc, or a Zig executable.")
        command = [found]
    if Path(command[0]).stem.lower() == "zig" and len(command) == 1:
        command.append("cc")
    return command


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", help="host compiler executable or command (also accepts CC environment variable)")
    parser.add_argument("--sanitize", action="store_true", help="enable AddressSanitizer and UndefinedBehaviorSanitizer")
    parser.add_argument("--suite", choices=[name for name, _ in SUITES], action="append")
    args = parser.parse_args()
    compiler = compiler_command(args.cc)
    flags = ["-std=c99", "-Wall", "-Wextra", "-Werror", "-g", "-O1", "-UNDEBUG", "-Isrc", "-DCC_HOST"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    print("Compiler:", " ".join(compiler), flush=True)
    with tempfile.TemporaryDirectory(prefix="crooked-coin-tests-") as directory:
        for name, sources in SUITES:
            if args.suite and name not in args.suite:
                continue
            executable = Path(directory) / (name + (".exe" if os.name == "nt" else ""))
            missing = [source for source in sources if not (ROOT / source).is_file()]
            if missing:
                raise SystemExit(f"Missing {name} test inputs: {', '.join(missing)}")
            command = compiler + flags + sources + ["-o", str(executable)]
            if name != "journal":
                command += ["-lm"]
            print(f"Building {name} tests", flush=True)
            subprocess.run(command, cwd=ROOT, check=True)
            print(f"Running {name} tests", flush=True)
            subprocess.run([str(executable)], cwd=ROOT, check=True)
    print("All selected host test suites passed.")


if __name__ == "__main__":
    main()
