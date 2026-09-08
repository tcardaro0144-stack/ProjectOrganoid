import json
import sys
import time

sys.path.insert(0, "Tools/unreal_mcp")
from server import call_unreal


def run(test_id: str) -> bool:
    r = call_unreal("run_playtest", {"test_id": test_id})
    print("START", test_id, json.dumps(r))
    if not r.get("ok"):
        return False
    run_id = r["data"]["run_id"]
    for i in range(100):
        time.sleep(3)
        st = call_unreal("get_playtest_status", {"run_id": run_id})
        data = st.get("data") or {}
        state = str(data.get("state") or "").lower()
        stage = data.get("current_stage")
        print(f"  {test_id} poll {i} state={state} stage={stage}")
        if state in ("pass", "fail", "blocked", "error", "aborted"):
            res = call_unreal("get_playtest_result", {"run_id": run_id})
            d = res.get("data") or {}
            asserts = d.get("assertions") or []
            passed = sum(1 for a in asserts if a.get("passed"))
            print(
                f"RESULT {test_id} state={d.get('state')} {passed}/{len(asserts)} reason={d.get('failure_reason')}"
            )
            for a in asserts:
                if not a.get("passed"):
                    print(" FAIL", a.get("id"), a.get("expected"), a.get("actual"))
            return str(d.get("state") or "").lower() == "pass"
    print("TIMEOUT", test_id)
    return False


if __name__ == "__main__":
    tests = sys.argv[1:] or ["CheckpointHealth_Functional"]
    ok = True
    for test_id in tests:
        if not run(test_id):
            ok = False
    raise SystemExit(0 if ok else 1)
