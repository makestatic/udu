#!/usr/bin/env python3
import os
import shutil
import subprocess
import tarfile
import zipfile
from pathlib import Path
import tempfile

def run(cmd, check=True):
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=check)

def build_release():
    run(["make", "release"])

def package_binaries():
    repo = os.environ["GITHUB_REPOSITORY"]
    workspace = Path(os.environ["GITHUB_WORKSPACE"])
    name = repo.split("/")[-1]
    build_dir = workspace / "build"
    build_dir.mkdir(parents=True, exist_ok=True)

    for dir_path in build_dir.glob("*"):
        if not dir_path.is_dir():
            continue

        base_name = dir_path.name
        folder_name = f"{name}-{base_name}"

        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp) / folder_name
            folder.mkdir(parents=True)

            exe = dir_path / "udu.exe"
            nix = dir_path / "udu"

            if exe.exists():
                archive_name = build_dir / f"{folder_name}.zip"
                shutil.copy(exe, folder)
                for extra in ["LICENSE", "README.md"]:
                    if Path(extra).exists():
                        shutil.copy(extra, folder)
                with zipfile.ZipFile(archive_name, "w", zipfile.ZIP_DEFLATED) as zf:
                    for f in folder.iterdir():
                        zf.write(f, arcname=f"{folder_name}/{f.name}")
                print(f"Created Windows archive: {archive_name}")

            elif nix.exists():
                archive_name = build_dir / f"{folder_name}.tar.gz"
                shutil.copy(nix, folder)
                for extra in ["LICENSE", "README.md"]:
                    if Path(extra).exists():
                        shutil.copy(extra, folder)
                with tarfile.open(archive_name, "w:gz") as tf:
                    tf.add(folder, arcname=folder_name)
                print(f"Created archive: {archive_name}")
            else:
                print(f"Skipping {dir_path}: no binary found")

def create_release():
    repo = os.environ["GITHUB_REPOSITORY"]
    tag = os.environ.get("GITHUB_REF_NAME") or os.environ.get("tag")

    build_dir = Path("build")
    archives = list(build_dir.glob("*.tar.gz")) + list(build_dir.glob("*.zip"))
    if not archives:
        print("No archives to upload")
        return

    cmd = [
        "gh", "release", "create", tag,
        f"--repo={repo}",
        f"--title={repo.split('/')[-1]} {tag.lstrip('v')}",
        "--generate-notes",
    ] + [str(a) for a in archives]

    run(cmd, check=False)

if __name__ == "__main__":
    build_release()
    package_binaries()
    create_release()
