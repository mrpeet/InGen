"""Clone the Sony Woosh repo (if needed) and download checkpoint weights."""
import os
import subprocess
import sys
import time
import urllib.request
import zipfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
WOOSH_DIR = os.path.join(SCRIPT_DIR, "woosh")
WOOSH_REPO = "https://github.com/SonyResearch/Woosh.git"

# DFlow depends on Woosh-AE and the CLAP text conditioner at runtime.
MODELS = ["Woosh-DFlow.zip", "Woosh-AE.zip", "TextConditionerA.zip", "Woosh-CLAP.zip"]
BASE_URL = "https://github.com/SonyResearch/woosh-sfx/releases/download/v1.0.0/"


def ensure_woosh_repo():
    if os.path.isdir(os.path.join(WOOSH_DIR, ".git")):
        print(f"Woosh repo already present at {WOOSH_DIR}")
        return
    if os.path.isdir(WOOSH_DIR) and os.listdir(WOOSH_DIR):
        print(
            f"Directory {WOOSH_DIR} exists but is not a git clone. "
            "Remove it or clone manually, then re-run this script."
        )
        sys.exit(1)
    print(f"Cloning {WOOSH_REPO} into {WOOSH_DIR}...")
    subprocess.check_call(["git", "clone", WOOSH_REPO, WOOSH_DIR])


def download_and_extract(url: str, extract_to: str, name: str):
    print(f"Downloading {url}...")
    zip_path = os.path.join(extract_to, f"temp_{name}.zip")
    urllib.request.urlretrieve(url, zip_path)
    print(f"Extracting to {extract_to}...")
    with zipfile.ZipFile(zip_path, "r") as zip_ref:
        zip_ref.extractall(extract_to)

    time.sleep(1)
    try:
        os.remove(zip_path)
    except OSError as e:
        print(f"Could not remove {zip_path}: {e}")
    print(f"Done with {name}!")


def main():
    ensure_woosh_repo()
    checkpoints_dir = os.path.join(WOOSH_DIR, "checkpoints")
    os.makedirs(checkpoints_dir, exist_ok=True)

    for archive in MODELS:
        folder_name = archive.replace(".zip", "")
        safetensors_path = os.path.join(checkpoints_dir, folder_name, "weights.safetensors")
        if os.path.exists(safetensors_path):
            print(f"{folder_name} weights already exist. Skipping.")
            continue
        download_and_extract(BASE_URL + archive, WOOSH_DIR, folder_name)

    print("All Woosh models are ready.")


if __name__ == "__main__":
    main()
