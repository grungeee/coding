import keyboard

# while True:
    # plane0 #first plane of the gread 
    # continp = input("^________")
    # if continp == "d":
    #     print("INPUT RECIVED")
    # break

def on_key_event(e):
    key = e.event_type 
    if key == keyboard.KEY_DOWN:
        print(key)

# Hook to key events
keyboard.hook(on_key_event)

try:
    # Keep the program running to capture key events
    keyboard.wait()
except KeyboardInterrupt:
    # Clean up and exit when the user presses Ctrl+C
    keyboard.unhook_all()
    print("Program terminated.")
