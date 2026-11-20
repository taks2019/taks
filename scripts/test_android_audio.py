"""Observe actual Android audio worker start, background stop and resume."""
import subprocess,time,json
from pathlib import Path
PACKAGE="hu.taks.world.wonderwerk"
def adb(*args):return subprocess.check_output(["adb",*args],text=True)
def threads():
    pid=adb("shell","pidof",PACKAGE).strip()
    assert pid,"Game process died"
    return adb("shell","ps","-T","-p",pid)
def count():return sum("TAKS-procedural" in row for row in threads().splitlines())
def expect_workers(expected):
    for _ in range(32):
        if count()==expected:return
        time.sleep(.25)
    Path("ui-evidence").mkdir(exist_ok=True)
    Path("ui-evidence/audio-activity.txt").write_text(adb("shell","dumpsys","activity","activities"))
    Path("ui-evidence/audio-logcat.txt").write_text(adb("logcat","-d","-s","TAKSAudio:I","AudioTrack:E","AndroidRuntime:E"))
    raise AssertionError(f"Expected {expected} audio worker(s): "+threads())
adb("shell","am","start","-n",PACKAGE+"/hu.taks.world.MainActivity")
time.sleep(2)
expect_workers(1)
adb("shell","input","keyevent","KEYCODE_HOME")
time.sleep(2)
expect_workers(0)
adb("shell","am","start","-n",PACKAGE+"/hu.taks.world.MainActivity")
time.sleep(2)
expect_workers(1)
Path("ui-evidence").mkdir(exist_ok=True)
Path("ui-evidence/audio-flinger.txt").write_text(adb("shell","dumpsys","media.audio_flinger"))
Path("ui-evidence/audio-lifecycle.json").write_text(json.dumps({"foreground_workers":1,"background_workers":0,"resumed_workers":1,"physical_listening_test":False},indent=2))
print("PASS actual Android PCM worker foreground/background/resume; not an audible quality test")
