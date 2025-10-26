def move():
    move_from = [] 
    move_to = []

    move_from = input(f"Select the piece you want to move:\n")
    move_from = move_from.replace(" ", "")
    
    if len(move_from) != 2:
        print(f"⛔ brooo 2 charcters! You got: {len(move_from)}")
        move()
    elif move_from[0].isalpha() == False or move_from[1].isnumeric() == False:
        print("⛔ Please enter a letter for colum and a number for row")
        move()

    # whilce loop repeat until the first charcter is a letter and the second is a number
    while True: 
        move_to = input(f"Select the tile you want to move your piece to:\n")
        move_to = move_to.replace(" ", "")

        if len(move_to) != 2:
            print(f"⛔ brooo 2 charcters! You got: {len(move_to)}")
            move()
        elif move_to[0].isalpha() == False or move_to[1].isnumeric() == False:
            print("⛔ Please enter a letter for colum and a number for row")
            move()
        elif move_from == move_to:
            print("So you want to stay in place?\nAin't no fucking way, try again mfk!\n")
            move()
        break
    return
    print("=======< parced >=======")
    print(f"from {move_from} to {move_to} move_from")
move()
