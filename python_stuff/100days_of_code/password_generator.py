import random
letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z']
numbers = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
symbols = ['!', '#', '$', '%', '&', '(', ')', '*', '+']

print("Welcome to the PyPassword Generator!")
# nr_letters = int(input("How many letters would you like in your password?\n")) 
# nr_symbols = int(input(f"How many symbols would you like?\n"))
# nr_numbers = int(input(f"How many numbers would you like?\n"))
nr_letters = 4
nr_symbols = 4
nr_numbers = 4
#TODO:
#  - need max length 16
#  - need max length for lists


rand_letters = ''
rand_symbols = ''
rand_numbers = ''

# picks random ID from a list in ITS range 
def random_id(list):
    return random.randint(0, len(list) - 1) # returns an int

# picks random [letters/symbols/numbers] based on requested ammount -> adds to var
for l in range(0, nr_letters): 
    rand_letters += str(letters[random_id(letters)])
for s in range(0, nr_symbols):
    rand_symbols += str(symbols[random_id(symbols)])
for n in range(0, nr_numbers):
    rand_numbers += str(numbers[random_id(numbers)])

print(rand_letters)
print(rand_symbols)
print(rand_numbers)
rand_char_sum = list(rand_letters+rand_symbols+rand_numbers) #turns string to list
print(rand_char_sum)
password = random.sample(rand_char_sum, len(rand_char_sum)) #shuffles items in list(len times)
print(password)

