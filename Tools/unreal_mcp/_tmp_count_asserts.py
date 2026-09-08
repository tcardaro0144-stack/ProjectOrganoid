import json
import re

path = r"C:\Users\tomca\.cursor\projects\c-Users-tomca-Documents-Unreal-Projects-ProjectOrganoid\agent-tools\89d127c1-8028-4d03-b86a-b5a1380f7dee.txt"
text = open(path, encoding="utf-8").read()
chunks = text.split('"headline": ')
for chunk in chunks[1:]:
    if "SUMMARY" in chunk:
        chunk = chunk.split("SUMMARY")[0]
    head = chunk.split("\n", 1)[0]
    passed = len(re.findall(r'"passed": true', chunk))
    failed = len(re.findall(r'"passed": false', chunk))
    run = re.search(r'"run_id": "([^"]+)"', chunk)
    print(head, "pass", passed, "fail", failed, "run", run.group(1) if run else "?")
