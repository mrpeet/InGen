import os
import urllib.request
import zipfile
import time

def download_and_extract(url, extract_to, name):
    print(f"Downloading {url}...")
    zip_path = os.path.join(extract_to, f"temp_{name}.zip")
    urllib.request.urlretrieve(url, zip_path)
    print(f"Extracting to {extract_to}...")
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(extract_to)
    
    time.sleep(1)
    try:
        os.remove(zip_path)
    except Exception as e:
        print(f"Could not remove {zip_path}: {e}")
    print(f"Done with {name}!")

extract_dir = "woosh"
checkpoints_dir = os.path.join("woosh", "checkpoints")

models = ["Woosh-DFlow.zip", "Woosh-AE.zip", "Woosh-CLAP.zip"]
base_url = "https://github.com/SonyResearch/woosh-sfx/releases/download/v1.0.0/"

for m in models:
    folder_name = m.replace('.zip', '')
    safetensors_path = os.path.join(checkpoints_dir, folder_name, "weights.safetensors")
    if not os.path.exists(safetensors_path):
        download_and_extract(base_url + m, extract_dir, folder_name)
    else:
        print(f"{folder_name} weights already exist. Skipping.")

print("All models downloaded and extracted.")
