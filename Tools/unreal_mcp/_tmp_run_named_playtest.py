import json
import sys
import time

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

test_id = sys.argv[1] if len(sys.argv) > 1 else "OpeningInvestigation_Functional"
timeout = int(sys.argv[2]) if len(sys.argv) > 2 else 360

started = call_unreal("run_playtest", {"test_id": test_id})
print("START", json.dumps(started, indent=2)[:1500])
run_id = (started.get("data") or {}).get("run_id")
if not run_id:
    sys.exit(1)

deadline = time.time() + timeout
last = ""
while time.time() < deadline:
    time.sleep(5)
    status = call_unreal("get_playtest_status", {"run_id": run_id})
    data = status.get("data") or {}
    line = "%s %s %s" % (data.get("state"), data.get("current_stage"), data.get("elapsed_ms"))
    if line != last:
        print(line)
        last = line
    if data.get("state") in ("pass", "fail", "blocked", "error"):
        result = call_unreal("get_playtest_result", {"run_id": run_id})
        print(json.dumps(result, indent=2))
        sys.exit(0 if data.get("state") == "pass" else 2)

print("TIMEOUT")
print(json.dumps(call_unreal("stop_playtest", {"run_id": run_id}), indent=2)[:800])
sys.exit(3)
