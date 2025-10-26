#!/usr/bin/env python3
import os, sys, json, time, webbrowser, requests
from urllib.parse import urlencode, urlparse, parse_qs

# ====== CONFIG ======
# CLIENT_ID     = os.getenv("TICKTICK_CLIENT_ID",     "YOUR_CLIENT_ID")
CLIENT_ID     = "L0Au2iWqsMgP24b20p"
# CLIENT_SECRET = os.getenv("TICKTICK_CLIENT_SECRET", "YOUR_CLIENT_SECRET")
CLIENT_SECRET = "I%Rmk6V%56)pa2#2mjD6%m6cnZU3oW#R"
REDIRECT_URI  = "http://localhost:8080/callback"
SCOPES        = "tasks:read tasks:write"

HOST = "https://ticktick.com"
API  = "https://api.ticktick.com/open/v1"
AUTH_URL  = f"{HOST}/oauth/authorize"
TOKEN_URL = f"{HOST}/oauth/token"

TOKEN_FILE = "ticktick_tokens.json"

# ====== HELPERS ======
def build_auth_url() -> str:
    params = {
        "client_id": CLIENT_ID,
        "redirect_uri": REDIRECT_URI,
        "response_type": "code",
        "scope": SCOPES,
    }
    return f"{AUTH_URL}?{urlencode(params)}"

def extract_code_from_redirect(redirected_url: str) -> str:
    q = parse_qs(urlparse(redirected_url).query)
    code = q.get("code", [None])[0]
    if not code:
        raise SystemExit("No 'code' in pasted URL.")
    return code

def exchange_code_for_token(code: str) -> dict:
    r = requests.post(
        TOKEN_URL,
        data={
            "grant_type": "authorization_code",
            "code": code,
            "redirect_uri": REDIRECT_URI,
        },
        auth=(CLIENT_ID, CLIENT_SECRET),
        timeout=30,
    )
    if r.status_code != 200:
        raise SystemExit(f"Token exchange failed: {r.status_code} {r.text}")
    return r.json()

def refresh_token(refresh: str) -> dict:
    r = requests.post(
        TOKEN_URL,
        data={
            "grant_type": "refresh_token",
            "refresh_token": refresh,
        },
        auth=(CLIENT_ID, CLIENT_SECRET),
        timeout=30,
    )
    if r.status_code != 200:
        raise SystemExit(f"Refresh failed: {r.status_code} {r.text}")
    return r.json()

def save_tokens(data: dict):
    with open(TOKEN_FILE, "w") as f:
        json.dump(data, f, indent=2)

def load_tokens() -> dict:
    if os.path.exists(TOKEN_FILE):
        with open(TOKEN_FILE) as f:
            return json.load(f)
    return {}

def api_get(path: str, token: str):
    r = requests.get(f"{API}{path}", headers={"Authorization": f"Bearer {token}"}, timeout=30)
    r.raise_for_status()
    return r.json()

def api_post(path: str, token: str, body: dict):
    r = requests.post(
        f"{API}{path}",
        headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"},
        json=body, timeout=30
    )
    r.raise_for_status()
    return r.json()

# ====== MAIN ======
if __name__ == "__main__":
    if "YOUR_CLIENT_ID" in CLIENT_ID:
        print("Fill CLIENT_ID/CLIENT_SECRET.")
        sys.exit(1)

    tokens = load_tokens()

    if not tokens:
        # First-time auth
        url = build_auth_url()
        print("Opening browser for first-time auth...\n", url)
        try:
            webbrowser.open(url)
        except Exception:
            pass
        redirected = input("Paste redirected URL:\n> ").strip()
        code = extract_code_from_redirect(redirected)
        tokens = exchange_code_for_token(code)
        tokens["expires_at"] = time.time() + tokens["expires_in"]
        save_tokens(tokens)
    else:
        # Check expiry
        if time.time() > tokens.get("expires_at", 0) - 60:
            print("Refreshing access token...")
            tokens = refresh_token(tokens["refresh_token"])
            tokens["expires_at"] = time.time() + tokens["expires_in"]
            save_tokens(tokens)

    access_token = tokens["access_token"]

    # Test: get user info
    me = api_get("/user/info", access_token)
    print("You are:", me)

    # Test: create a task
    # created = api_post("/task", access_token, {
    #     "title": "No more browser flow 🎉",
    #     "priority": 3
    # })
    # print("Created:", created)

    # =====================================
def get_lists(token: str):
    """Return all lists (projects) for the current user."""
    return api_get("/project", token)
#
# def get_tasks_in_list(token: str, project_id: str):
#     """Return all tasks inside a given list (by project_id)."""
#     return api_get(f"/project/{project_id}/tasks", token)
# # 1) Get all lists
# lists = get_lists(access_token)
# print("\nLists:")
# for p in lists:
#     print(f"- {p['name']} ({p['id']})")
#
# # 2) Pick a list (example: the first one)
# first_list_id = lists[0]['id']
# print(f"\nfirst_list_id: {first_list_id}")
#
# # 3) Get tasks in that list
# tasks = get_tasks_in_list(access_token, first_list_id)
# print(f"\nTasks in '{lists[0]['name']}':")
# for t in tasks:
#     print(f"- {t['title']}  (done? {t['status'] == 2})")

# ==== test 2 ====
def get_all_tasks(token: str):
    """
    Returns ALL active tasks grouped by project from the state/batch API.
    This includes every list, so you'll need to filter by projectId.
    """
    r = requests.post(
        f"{API}/batch/task",
        headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"},
        json={"status": 0},  # 0=incomplete; use 2 for completed
        timeout=30
    )
    r.raise_for_status()
    return r.json()  # dict keyed by projectId

def get_tasks_in_list(token: str, project_id: str, status: int = 0):
    all_tasks = get_all_tasks(token)
    return all_tasks.get(project_id, [])

lists = get_lists(access_token)
print("\nLists:")
for p in lists:
    print(f"- {p['name']} ({p['id']})")

first_list_id = lists[0]['id']
tasks = get_tasks_in_list(access_token, first_list_id, status=0)
print(f"\nTasks in '{lists[0]['name']}':")
for t in tasks:
    print(f"- {t['title']} (done? {t.get('status') == 2})")
