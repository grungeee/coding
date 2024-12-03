from time import sleep


# print(field[-1][2])


# print index of value in a list that is equal to 1
# for row in field:
#     for col in row:
#         if col == 1:
            # print(row.index(col))

# 8
# 7
# 6
# 5
# 4
# 3
# 2
# 1 
#   a b c d e f g h 

# ------------------
## Pawn (P)
## Rook (R)
## Knight (N)
## Bishop (B)
## Queen (Q)
## King (K)
# ------------------


# ============= create a chess board ============

# generic python object

field = [
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0, 0, 0],
        ]



# board
    # rows & columns
        # black & white
            # tiles
                # pieces
                    #starting position
                    # possible moves
                        # calc max possible tile
                            # B: bischop_bt
                                # cur column 2 (C)
                                    #-> min: 0 = 2 - 2 -> 2 steps 
                                    #-> max: 7 = 2 + 5 -> 5 steps
                                
                                
                    # possible attacks
                        # max 4?





# dictionary named board with a key teams that contains another dictionary with white and black
game = {
    'pieces':{
        'white': {
            'knight_N_w1': {
                'N_piece_col_index' : 1,
                'N_piece_row_index' : 0
                },
            'knight_N_w2': {
                'N_piece_col_index' : 6,
                'N_piece_row_index' : 0
                },
        },
        'black': {
            'a': [],
            'b': [],
            'c': [],
            'd': [],
            'e': [],
            'f': [],
            'g': [],
            'h': [],
        }
    }
}


# piece_row_index = knight_w1

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
    rows_count = 8
    cols_count = 8

# tiles_count_in_row = 8

    w = "⬜"
    b = "⬛"

    knight_w1 = "🐴"
    knight_w2 = knight_w1
    knight_b1 = "🐎"
    bishop_w1= "🔵"
    bishop_w2= "🔴"
    bishop = "🔴"



    first_tile = b # the first row is starting with black
    current_tile = ''

    pieces = [knight_w1, knight_w2, bishop_w1, bishop_w2]
    count = 0
    piece = pieces[count]

    # while count != len(pieces) -1:
    # while count != len(pieces) -2:
#     # TODO: - [ ] this is fucking stupid and i gotta fix it
    knight_w1 = list(list(list(game.items())[0][1].items())[0][1].items())[0] #<====== change here /// loop through the dictionary to get the values
    print(knight_w1)
    piece_col_index = list(knight_w1[1].values())[0]
    piece_row_index = list(knight_w1[1].values())[1]
    print("--------")
    print(piece_col_index)
    print(piece_row_index)
    print("--------")


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


            # replaces the value of the item at the specified index with a new value
            if col_index == piece_col_index:
            # replace the item of a list with a value
                board[list(board.keys())[col_index]][piece_row_index] = piece



            # replaces the value of the item at the specified index with a new value at specified index
            # dictionary[list(dictionary.items())[0]][0] = piece

            # dictionary[list(dictionary.items())[0]].append(piece)

        # print(dictionary)

        # turn a list into a string
        row_strig = "".join(board[list(board.keys())[col_index]])
        # ===============print the field =============
        print(key, row_strig)
    print("  ", 1, 2, 3, 4, 5, 6, 7, 8)


board_setup()
    



# knight_w1 = "🐴"
# knight_w2 = knight_w1
# knight_b1 = "🐎"
# bishop_w1= "🔵"
# bishop_w2= "🔴"
# bishop = "🔴"
#
#
# pieces = [knight_w1, knight_w2, bishop_w1, bishop_w2]
# print(len(pieces))
