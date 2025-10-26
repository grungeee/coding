from time import sleep

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

# pieces = [knight_w1, knight_w2, bishop_w1, bishop_w2]
# count = 0
# piece = pieces[count]

# while count != len(pieces) -1:
# while count != len(pieces) -2:
#     # TODO: - [ ] this is fucking stupid and i gotta fix it
# knight_w1 = list(list(list(game.items())[0][1].items())[0][1].items())[0] #<====== change here /// loop through the dictionary to get the values





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

# dictionary named board with a key teams that contains another dictionary with white and black

# ------------------
## Pawn (P)
## Rook (R)
## Knight (N)
## Bishop (B)
## Queen (Q)
## King (K)
# ------------------

game = {
    'pieces':{
        'white': {
            'P1': {
                'piece_col_index' : 0,
                'piece_row_index' : 1
                },
            'P2': {
                'piece_col_index' : 1,
                'piece_row_index' : 1
                },
            'P3': {
                'piece_col_index' : 2,
                'piece_row_index' : 1
                },
            'P4': {
                'piece_col_index' : 3,
                'piece_row_index' : 1
                },
            'P5': {
                'piece_col_index' : 4,
                'piece_row_index' : 1
                },
            'P6': {
                'piece_col_index' : 5,
                'piece_row_index' : 1
                },
            'P7': {
                'piece_col_index' : 7,
                'piece_row_index' : 1
                },
            'P8': {
                'piece_col_index' : 8,
                'piece_row_index' : 1
                },
            'R1': {
                'piece_col_index' : 0,
                'piece_row_index' : 0
                },
            'R2': {
                'piece_col_index' : 7,
                'piece_row_index' : 0
                },
            'N1': {
                'piece_col_index' : 1,
                'piece_row_index' : 0
                },
            'N2': {
                'piece_col_index' : 6,
                'piece_row_index' : 0
                },
            'B1': {
                'piece_col_index' : 2,
                'piece_row_index' : 0
                },
            'B2': {
                'piece_col_index' : 5,
                'piece_row_index' : 0
                },
            'Q': {
                'piece_col_index' : 3,
                'piece_row_index' : 0
                },
            'K': {
                'piece_col_index' : 4,
                'piece_row_index' : 0
                },
        },
        'black': {
            'P1': {
                'piece_col_index' : 0,
                'piece_row_index' : 6
                },
            'P2': {
                'piece_col_index' : 1,
                'piece_row_index' : 6
                },
            'P3': {
                'piece_col_index' : 2,
                'piece_row_index' : 6
                },
            'P4': {
                'piece_col_index' : 3,
                'piece_row_index' : 6
                },
            'P5': {
                'piece_col_index' : 4,
                'piece_row_index' : 6
                },
            'P6': {
                'piece_col_index' : 5,
                'piece_row_index' : 6
                },
            'P7': {
                'piece_col_index' : 7,
                'piece_row_index' : 6
                },
            'P8': {
                'piece_col_index' : 8,
                'piece_row_index' : 6
                },
            'R1': {
                'piece_col_index' : 0,
                'piece_row_index' : 0
                },
            'R2': {
                'piece_col_index' : 7,
                'piece_row_index' : 7
                },
            'N1': {
                'piece_col_index' : 1,
                'piece_row_index' : 7
                },
            'N2': {
                'piece_col_index' : 6,
                'piece_row_index' : 7
                },
            'B1': {
                'piece_col_index' : 2,
                'piece_row_index' : 7
                },
            'B2': {
                'piece_col_index' : 5,
                'piece_row_index' : 7
                },
            'Q': {
                'piece_col_index' : 3,
                'piece_row_index' : 7
                },
            'K': {
                'piece_col_index' : 4,
                'piece_row_index' : 7
                },
        }
    }
}


pieces = game['pieces']
white = pieces['white']
black = pieces['black']
N1 = white['N1']
P2_B = black['P2']
N1_col = N1["piece_col_index"]
N1_row = N1["piece_row_index"]
P2_col = P2_B["piece_col_index"]
P2_row = P2_B["piece_row_index"]
# print(N1_col)
# print(N1_row)
# print(P2_col)
# print(P2_row)



# ============================================
# for i in (game['pieces']['white']):
    # print(game['pieces']['white'][i])

# === output ===
# {'piece_col_index': 0, 'piece_row_index': 1}
# {'piece_col_index': 1, 'piece_row_index': 1}
# ...
# ============================================



def board_setup():
# tiles_count_in_row = 8
    #TODO: DEHARDCODE THIS SHIT
    rows_count = 8
    cols_count = 8

# tiles_count_in_row = 8

    w = "⬜"
    b = "⬛"

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
    # print("\n=========================\n")
    # for row_index in range(0, rows_count):
    #     for col_index in range(0, cols_count):
    #         # key = list(board.keys())[col_index] # finds the KV of a dictionary based on its index
    #         # board[key].extend(current_tile) # adds the current tile to the KV of a dictionary based on its index
    # print("  ", 1, 2, 3, 4, 5, 6, 7, 8)

board_setup()


# how to rotate?
# 90 degrees counterclockwise
# top left to bottom left
# old: a: 1 2 3 4 5 6 7 8
# new: 1: a1 b1 c1 d1...
