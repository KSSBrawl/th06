import argparse
from pathlib import Path
import textwrap
import sys
import subprocess
import os

from configure import BuildType, configure
from winhelpers import run_windows_program

SCRIPTS_DIR = Path(__file__).parent


def find_diff(path1, path2):
    offset = 0
    with open(path1, "rb") as file1, open(path2, "rb") as file2:
        while True:
            page1 = file1.read(0x1000)
            if not page1:
                return None
            page2 = file2.read(0x1000)
            if page1 != page2:
                for i, (byte1, byte2) in enumerate(zip(page1, page2)):
                    if byte1 != byte2:
                        return (offset + i, byte1, byte2)
            offset += 0x1000


def build(build_type, comdat_permute_enable, verbose=False, jobs=1, target=None):
    ninja_args = []
    if verbose:
        ninja_args += ["-v"]

    if jobs != 0:
        ninja_args += ["-j" + str(jobs)]

    if target is not None:
        ninja_args += [target]
    elif build_type == BuildType.TESTS:
        ninja_args += ["build/th06-tests.exe"]
    elif build_type == BuildType.OBJDIFFBUILD:
        ninja_args += ["objdiff"]
    else:
        ninja_args += ["build/th06.exe"]

    # best yet: 201
    comdat_permute = 0
    best_match = comdat_permute
    best_match_dist = 0

    while True:
        if comdat_permute_enable:
            print("Building comdat attempt " + str(comdat_permute), file=sys.stderr)
        configure(build_type, comdat_permute)

        # Then, run the build. We use run_windows_program to automatically go through
        # wine if running on linux/macos. scripts/th06run.bat will setup PATH and other
        # environment variables for the MSVC toolchain to work before calling ninja.
        run_windows_program(
            [str(SCRIPTS_DIR / "th06run.bat"), "ninja"] + ninja_args,
            cwd=str(SCRIPTS_DIR.parent),
        )

        # Ninja is pretty hard to work with so this is the only (janky)
        # working solution. If you can think of a better one, PRs welcome.
        if build_type == BuildType.BINARY_MATCHBUILD:
            if os.path.isfile("build/th06.exe"):
                run_windows_program(
                    [
                        sys.executable,
                        str(SCRIPTS_DIR / "patch_timestamp.py"),
                        "build/th06.exe",
                        "1038721275",  # 2002-12-01 06:41:15
                    ]
                )
            diff = find_diff("resources/th06.exe", "build/th06.exe")
            if diff == None:
                print("Binary matches!", file=sys.stderr)
            else:
                print(
                    "Diff at byte "
                    + hex(diff[0])
                    + ": "
                    + hex(diff[1])
                    + " "
                    + hex(diff[2]),
                    file=sys.stderr,
                )
                if comdat_permute_enable:
                    if diff[0] > best_match_dist:
                        best_match_dist = diff[0]
                        best_match = comdat_permute
                    comdat_permute += 1
                    if comdat_permute != 1000:
                        continue
                    print(
                        "Giving up, best match was "
                        + str(best_match)
                        + " at "
                        + hex(best_match_dist),
                        file=sys.stderr,
                    )
        break


def main():
    parser = argparse.ArgumentParser(
        "th06-build", formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        "--build-type",
        choices=[
            "normal",
            "diffbuild",
            "tests",
            "objdiffbuild",
            "binary_matchbuild",
            "binary_matchbuild_comdat",
        ],
        default="normal",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=1,
        help=textwrap.dedent("""
            Number of jobs to run in parallel. Set to 0 to run one job per CPU core. Defaults to 1.
            Note that parallel builds may not work when running through wine.
            See https://github.com/happyhavoc/th06/issues/79 for more information."""),
    )
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("--object-name", required=False)
    parser.add_argument(
        "target",
        nargs="?",
        help=textwrap.dedent("""
        Ninja target to build. Default depends on the build type:
          - Normal and diff builds will build th06.exe
          - Test builds will build th06-tests.exe
          - objdiff builds will build all the object files necessary for objdiff.
    """),
    )
    args = parser.parse_args()
    target = None
    comdat_permute = False

    # First, create the build.ninja file that will be used to build.
    if args.build_type == "normal":
        build_type = BuildType.NORMAL
    elif args.build_type == "diffbuild":
        build_type = BuildType.DIFFBUILD
    elif args.build_type == "tests":
        build_type = BuildType.TESTS
    elif args.build_type == "objdiffbuild":
        build_type = BuildType.OBJDIFFBUILD
    elif args.build_type == "binary_matchbuild":
        build_type = BuildType.BINARY_MATCHBUILD
    elif args.build_type == "binary_matchbuild_comdat":
        build_type = BuildType.BINARY_MATCHBUILD
        comdat_permute = True

    if args.object_name is not None:
        object_name = Path(args.object_name).name
        target = "build/objdiff/reimpl/" + object_name
    elif args.target is not None:
        target = args.target

    build(build_type, comdat_permute, args.verbose, args.jobs, target=target)


if __name__ == "__main__":
    main()
