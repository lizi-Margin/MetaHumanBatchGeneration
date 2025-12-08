#!/usr/bin/env python3
import os
import sys
import time
import subprocess
import signal
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
HEARTBEAT_FILE = PROJECT_ROOT / "Saved" / "heartbeat.txt"

STARTUP_TIME = 120
HEARTBEAT_TIMEOUT = 300
HEARTBEAT_CHECK_INTERVAL = 5
editor_exe_candidates = [
    r"I:\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe",
    r"C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe",
]



def find_uproject_file(root_dir):
    uproject_files = list(Path(root_dir).glob("*.uproject"))
    if len(uproject_files) == 0:
        raise FileNotFoundError(f"No .uproject file found in {root_dir}")
    if len(uproject_files) > 1:
        raise RuntimeError(f"Multiple .uproject files found in {root_dir}: {uproject_files}")
    return uproject_files[0]

def find_editor_exe():
    for editor_path in editor_exe_candidates:
        if Path(editor_path).exists():
            return editor_path

    raise FileNotFoundError(f"Unreal Editor not found in any of: {editor_exe_candidates}")

UPROJECT_FILE = find_uproject_file(PROJECT_ROOT)
EDITOR_EXE = find_editor_exe()

class HeartbeatMonitor:
    def __init__(self):
        self.last_heartbeat_value = 0
        self.last_update_time = time.time()
        self.editor_process = None
        self.last_check_time = time.time()

    def read_heartbeat(self):
        try:
            if HEARTBEAT_FILE.exists():
                with open(HEARTBEAT_FILE, 'r') as f:
                    content = f.read().strip()
                    if content.isdigit():
                        return int(content)
        except Exception as e:
            print(f"Error reading heartbeat file: {e}")
        return None

    def start_editor(self):
        print(f"[{self.get_timestamp()}] Starting Unreal Editor...")
        try:
            self.editor_process = subprocess.Popen(
                [EDITOR_EXE, str(UPROJECT_FILE), "-AllowStdOutLogVerbosity"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            print(f"[{self.get_timestamp()}] Editor started with PID {self.editor_process.pid}")
            return True
        except Exception as e:
            print(f"[{self.get_timestamp()}] ERROR: Failed to start editor: {e}")
            return False

    def kill_editor(self):
        if self.editor_process is None:
            print(f"[{self.get_timestamp()}] Editor process not tracked, killing UnrealEditor via command line")
            os.system("taskkill /IM UnrealEditor.exe /F")
        else:
            print(f"[{self.get_timestamp()}] Killing editor process (PID {self.editor_process.pid})...")
            try:
                os.kill(self.editor_process.pid, signal.SIGTERM)
                time.sleep(2)
                if self.editor_process.poll() is None:
                    os.kill(self.editor_process.pid, signal.SIGKILL)
            except Exception as e:
                print(f"[{self.get_timestamp()}] Error killing process: {e}")
                os.system("taskkill /IM UnrealEditor.exe /F")

        self.editor_process = None
        print(f"[{self.get_timestamp()}] Editor killed")

    def monitor(self):
        print(f"[{self.get_timestamp()}] Heartbeat monitor started")
        print(f"[{self.get_timestamp()}] Heartbeat file: {HEARTBEAT_FILE}")
        print(f"[{self.get_timestamp()}] Timeout threshold: {HEARTBEAT_TIMEOUT} seconds")
        print(f"[{self.get_timestamp()}] Check interval: {HEARTBEAT_CHECK_INTERVAL} seconds")

        cycle_count = 0

        while True:
            try:
                current_time = time.time()

                if self.editor_process is not None:
                    if self.editor_process.poll() is not None:
                        print(f"[{self.get_timestamp()}] Editor process has exited")
                        self.editor_process = None
                        time.sleep(2)
                        continue

                current_heartbeat = self.read_heartbeat()
                current_check_time = time.time()

                if current_heartbeat is not None:
                    if current_heartbeat != self.last_heartbeat_value:
                        self.last_heartbeat_value = current_heartbeat
                        self.last_update_time = current_check_time
                        print(f"[{self.get_timestamp()}] Heartbeat updated: {current_heartbeat}")
                    else:
                        time_since_update = current_check_time - self.last_update_time
                        if time_since_update > HEARTBEAT_TIMEOUT:
                            print(f"[{self.get_timestamp()}] CRITICAL: No heartbeat update for {time_since_update:.1f}s (threshold: {HEARTBEAT_TIMEOUT}s)")
                            self.kill_editor()
                            time.sleep(3)
                else:
                    if self.editor_process is not None:
                        time_since_update = current_check_time - self.last_update_time
                        if time_since_update > HEARTBEAT_TIMEOUT:
                            print(f"[{self.get_timestamp()}] CRITICAL: Heartbeat file missing for {time_since_update:.1f}s")
                            self.kill_editor()
                            time.sleep(3)

                if self.editor_process is None or self.editor_process.poll() is not None:
                    if self.start_editor():
                        time.sleep(STARTUP_TIME)
                    self.last_heartbeat_value = 0
                    self.last_update_time = time.time()

                time.sleep(HEARTBEAT_CHECK_INTERVAL)
                cycle_count += 1

            except KeyboardInterrupt:
                print(f"\n[{self.get_timestamp()}] Shutting down...")
                self.kill_editor()
                sys.exit(0)
            except Exception as e:
                print(f"[{self.get_timestamp()}] ERROR: {e}")
                time.sleep(HEARTBEAT_CHECK_INTERVAL)

    @staticmethod
    def get_timestamp():
        return time.strftime("%Y-%m-%d %H:%M:%S")

if __name__ == "__main__":
    monitor = HeartbeatMonitor()
    monitor.monitor()
