"""Black-box tests for shell.c, driven over a real pipe with small delays
between writes so each line lands in its own read() call (this shell reads
one command per blocking read() with no internal buffering across calls,
so pasting multiple lines in a single write() would get mis-tokenized as
one giant command)."""
import os
import select
import subprocess
import time
import signal
import sys
import shutil

SHELL = os.path.expanduser("~/repos/Custom-Linux-C-Shell/shell")

def drain(proc, timeout=0.4):
    out = b""
    end = time.time() + timeout
    while time.time() < end:
        r, _, _ = select.select([proc.stdout], [], [], 0.05)
        if r:
            chunk = os.read(proc.stdout.fileno(), 65536)
            if not chunk:
                break
            out += chunk
        else:
            if out:
                break
    return out.replace(b"\x00", b"").decode(errors="replace")

def wait_for_sleep_state(pid, timeout=2.0):
    """Poll /proc so a SIGINT is only sent once the shell is actually
    blocked in read() -- otherwise the signal can race ahead of the
    process reaching that syscall and the test becomes flaky."""
    end = time.time() + timeout
    while time.time() < end:
        try:
            with open(f"/proc/{pid}/stat") as f:
                stat = f.read()
            state = stat.split(") ", 1)[1].split(" ", 1)[0]
            if state == "S":
                return True
        except FileNotFoundError:
            return False
        time.sleep(0.01)
    return False

def send(proc, line):
    os.write(proc.stdin.fileno(), (line + "\n").encode())
    time.sleep(0.15)

def start():
    return subprocess.Popen([SHELL], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, cwd="/tmp")

failures = []

def check(name, cond, detail=""):
    status = "PASS" if cond else "FAIL"
    print(f"[{status}] {name} {detail}")
    if not cond:
        failures.append(name)

# --- Test 1: EOF (Ctrl+D) on an empty pipe must not crash ---
p = subprocess.Popen([SHELL], stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                      stderr=subprocess.STDOUT, cwd="/tmp")
rc = p.wait(timeout=5)
check("EOF on stdin exits cleanly (not killed by signal)", rc == 0, f"(exit code {rc})")

# --- Test 2: SIGINT while blocked in read() must not crash the shell,
#     and the shell must still run commands afterwards ---
p = start()
drain(p)  # initial prompt
wait_for_sleep_state(p.pid)
p.send_signal(signal.SIGINT)
time.sleep(0.3)
out = drain(p)
alive = p.poll() is None
check("SIGINT while waiting for input doesn't crash the shell", alive, f"(still running: {alive})")
if alive:
    send(p, "echo still-alive")
    out2 = drain(p)
    check("shell still executes commands after SIGINT", "still-alive" in out2, f"(output: {out2!r})")
    send(p, "exit")
    p.wait(timeout=3)
else:
    p.kill()

# --- Test 3: cd ~/<long path> doesn't corrupt the shell / subsequent commands ---
# A path made of several 200-char segments: long enough in total that
# home_dir + the rest of the path is far bigger than the original short
# "~/..." token's space inside input_buffer, without hitting the
# filesystem's 255-byte-per-component limit.
p = start()
drain(p)
segment = "a" * 200
long_rel = "/".join([segment] * 4)  # 800+ chars once home_dir is prepended
real_dir = os.path.expanduser(f"~/{long_rel}")
os.makedirs(real_dir, exist_ok=True)
send(p, f"cd ~/{long_rel}")
drain(p)
send(p, "pwd")
out2 = drain(p)
check("cd ~/<long path> lands in the right directory", segment in out2, f"(pwd output tail: {out2[-120:]!r})")
send(p, "echo after-cd-ok")
out3 = drain(p)
check("shell still works normally after long ~ expansion", "after-cd-ok" in out3, f"(output: {out3!r})")
send(p, "exit")
p.wait(timeout=3)
shutil.rmtree(os.path.expanduser(f"~/{segment}"), ignore_errors=True)

# --- Test 4: "!!" repeats the actual most recent command, even after
#     history has wrapped past HISTORY_DEPTH (10) entries ---
p = start()
drain(p)
for i in range(12):
    send(p, f"echo marker-{i}")
    drain(p)
send(p, "echo marker-LAST")
drain(p)
send(p, "!!")
out = drain(p)
check("!! repeats the most recent command after >10 commands", "marker-LAST" in out, f"(output tail: {out[-300:]!r})")
send(p, "exit")
p.wait(timeout=3)

# --- Test 5: history labels and "!N" agree with each other, in both the
#     under-10 and over-10 (wrapped) regimes ---
p = start()
drain(p)
send(p, "echo first")
drain(p)
send(p, "echo second")
drain(p)
send(p, "history")
hist_out = drain(p)
check("history shows 1-indexed absolute command numbers", "1\techo first" in hist_out and "2\techo second" in hist_out,
      f"(history output: {hist_out!r})")
send(p, "!1")
out = drain(p)
check("!1 replays the first command by its printed history number", "first" in out, f"(output: {out!r})")
send(p, "exit")
p.wait(timeout=3)

print()
if failures:
    print(f"{len(failures)} FAILURE(S): {failures}")
    sys.exit(1)
else:
    print("All tests passed.")
