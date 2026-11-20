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
adb("shell","am","start","-n",PACKAGE+"/hu.taks.world.MainActivity")
time.sleep(2)
assert count()==1,"Expected exactly one active synthesis worker: "+threads()
adb("shell","input","keyevent","KEYCODE_HOME")
time.sleep(2)
assert count()==0,"Audio worker survived background pause: "+threads()
adb("shell","am","start","-n",PACKAGE+"/hu.taks.world.MainActivity")
time.sleep(2)
assert count()==1,"Audio did not resume exactly once: "+threads()
Path("ui-evidence").mkdir(exist_ok=True)
Path("ui-evidence/audio-flinger.txt").write_text(adb("shell","dumpsys","media.audio_flinger"))
Path("ui-evidence/audio-lifecycle.json").write_text(json.dumps({"foreground_workers":1,"background_workers":0,"resumed_workers":1,"physical_listening_test":False},indent=2))
print("PASS actual Android PCM worker foreground/background/resume; not an audible quality test")
