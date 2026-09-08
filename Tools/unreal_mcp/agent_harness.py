import json
import urllib.request
import ollama

UNREAL_URL = "http://127.0.0.1:8732/v1/command"

def send_unreal_command(command: str, args: dict = None):
    payload = {
        "command": command,
        "args": args or {},
        "session": {"read_only": False}
    }
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        UNREAL_URL,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST"
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as response:
            return json.loads(response.read().decode("utf-8"))
    except Exception as e:
        return {"ok": False, "error": str(e)}

def run_local_agent(prompt: str):
    response = ollama.chat(
        model='qwen2.5-coder',
        messages=[
            {
                "role": "system", 
                "content": "You are an Unreal Engine 5 development assistant for Project Organoid. You can inspect editor state and make adjustments."
            },
            {"role": "user", "content": prompt}
        ]
    )
    print(response['message']['content'])

if __name__ == "__main__":
    print("Checking Unreal Editor State...")
    state = send_unreal_command("get_editor_state")
    print(json.dumps(state, indent=2))