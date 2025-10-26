from time import sleep

board = {
    'a': [], # 'key = clumun' : [value = row]
    'b': [],
    'c': [],
    'd': [],
    'e': [],
    'f': [],
    'g': [],
    'h': [],
}


def board_setup():
# tiles_count_in_row = 8
    #TODO: DEHARDCODE THIS SHIT
    rows_count = 8
    cols_count = 8

    w = "⬜"
    b = "⬛"

    first_tile = b # the first row is starting with black
    current_tile = ''

    for col_index in range(0, cols_count):
        sleep(0.1)

        # skips the first tile, so it won't change
        if col_index == 0:
            current_tile = first_tile
        current_tile = w if current_tile == b else b # swap the color of the tile every iteration

        # find the key value of a dictionary based on its index
        for row_index in range(0, rows_count):
            current_tile = w if current_tile == b else b # switches the color of the tile every iteration
            key = list(board.keys())[col_index] # finds the KV of a dictionary based on its index
            board[key].extend(current_tile) # adds the current tile to the KV of a dictionary based on its index

        row_strig = "".join(board[list(board.keys())[col_index]])
        # ===============print the field =============
        print(key, row_strig)
    print("  ", 1, 2, 3, 4, 5, 6, 7, 8)

    # === V2.0 ===
    print("\n=========================\n")

    row = ""

    for col_index in range(0, cols_count):
        actuall_col_count = 0
        for row_index in range(0, rows_count):
            key = list(board.keys())[col_index] # finds the KV of a dictionary based on its index
            if row == "":
                row += f"{row_index + 1}: "
            else:
                row += f"{current_tile}" 
        print(row)
        row = ""
                # row += board[list(bard.)]


    # print("  ", 1, 2, 3, 4, 5, 6, 7, 8)

board_setup()


# how to rotate?
# 90 degrees counterclockwise
# top left to bottom left
# old: a: 1 2 3 4 5 6 7 8
# new: 1: a1 b1 c1 d1...
