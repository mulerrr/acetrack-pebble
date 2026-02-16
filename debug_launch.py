import subprocess
import time

def recreate_crash():
    # Install again to trigger launch?
    subprocess.run("export PATH=\"$HOME/.local/bin:$PATH\" && pebble install --emulator basalt", shell=True)

if __name__ == "__main__":
    recreate_crash()
