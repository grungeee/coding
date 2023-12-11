from hangman_words import word_list
from hangman_art import logo
from hangman_art import stages
import random
import os
import time

# os.system('cls' if os.name == 'nt' else 'clear')

# dunno how to get anything out of the function so this shit is gonna wait
def game_init():
    print(f"\n \n{logo} \n \n")
    time.sleep(3)
game_init()

# random Index of a word in the list
randomID = random.randint(0,len(word_list) - 1)

# Random Index of a word in the list
word_to_guess = word_list[randomID]

list_to_guess = list(word_to_guess.upper())

# Random Letter 
letters_display= []
for l in list_to_guess:
    letters_display.append('_')
    

#stages[6] # 7 TRIES
lives = len(stages) - 1

while lives != 0:
    os.system('clear') #clears the screan

    guess = input(f"Guess what motherfucker!\n{' '.join(letters_display)}\n").upper()
    # print(' '.join(letters_display))
    letters_count = list_to_guess.count(guess) #n times of x letter in a list

    if letters_count != 0:
        for n in range(0, len(list_to_guess)):
            if list_to_guess[n].upper() == guess:
                letters_display[n] = guess
    elif letters_count == 0:
        lives += -1
        print("suck on diz nuts")
        if (lives == 0):
            print(stages[lives])
            print(f"{word_to_guess} was the word, you moron")
            break

    print(stages[lives])
    print(' '.join(letters_display))



# ids.append(list_to_guess.index(l))
# if letters_count == 1:
#     for l in list_to_guess:
#         letters_display[(list_to_guess.index(guess))] = guess
        # print(stages[6])
        # print('HELL YEAH')

