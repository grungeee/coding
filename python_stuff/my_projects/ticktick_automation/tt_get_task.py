from ticktick.oauth2 import OAuth2
from ticktick.api import TickTickClient

# 1) First run: open auth URL in browser, paste back the code when prompted
oauth = OAuth2(
    client_id="L0Au2iWqsMgP24b20p",
    client_secret="I%Rmk6V%56)pa2#2mjD6%m6cnZU3oW#R",
    redirect_uri="http://localhost:8080/callback",
    scope="tasks:write tasks:read"
)

# 2) Login with your normal TickTick credentials (library handles session + token)
client = TickTickClient(username="sharapanikita@gmail.com", password="Verylongp4ticktick", oauth=oauth)

# --- Retrieve tasks (uncompleted) ---
tasks = client.state["tasks"]      # library keeps a synced local state
print(len(tasks), "open tasks")

# --- Create a task ---
created = client.task.create(
    title="Order 10x M3 T-nuts",
    project_id=client.inbox_id,    # or a specific project id
    content="For 2020 extrusion jig",
    priority=3
)
print("Created:", created["id"], created["title"])
