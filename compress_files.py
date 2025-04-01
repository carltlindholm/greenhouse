import os
import gzip
import shutil

SRC_DIR = "data_src"
DEST_DIR = "data"

def compress_file(src_path, dest_path):
    """Compress a file using gzip and save it in the destination directory."""
    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
    with open(src_path, "rb") as f_in, gzip.open(dest_path, "wb") as f_out:
        shutil.copyfileobj(f_in, f_out)
    print(f"Compressed: {src_path} → {dest_path}")

def compress_spiffs_files():
    """Compress files from 'data_src/' into 'data/'."""
    if not os.path.exists(SRC_DIR):
        print(f"No '{SRC_DIR}/' folder found, skipping compression.")
        return

    for root, _, files in os.walk(SRC_DIR):
        for file in files:
            if file.endswith((".html", ".css", ".js")) and not file.endswith(".gz"):
                src_path = os.path.join(root, file)
                relative_path = os.path.relpath(src_path, SRC_DIR)
                dest_path = os.path.join(DEST_DIR, relative_path + ".gz")
                compress_file(src_path, dest_path)

# Run compression before uploading SPIFFS
compress_spiffs_files()
