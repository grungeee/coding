logo = '''
                         ___________
                         \         /
                          )_______(
                          |"""""""|_.-._,.---------.,_.-._
                          |       | | |               | | ''-.
                          |       |_| |_             _| |_..-'
                          |_______| '-' `'---------'` '-'
                          )"""""""(
                         /_________\\
                       .-------------.
                      /_______________\\
'''
## Blind Auction

# # Instructions

# The objective is to write a program that will collect the names and bids of different people. The program should ask for each bidder's name and their bid individually. 

# ```
# Welcome to the secret auction program. 
# What is your name?: Angela
# ```
# ```
# What's your bid?: $123
# ```
# ```
# Are there any other bidders? Type 'yes' or 'no'.
# yes

# ```
# If there are other bidders, the screen should clear, so you can pass your phone to the next person. If there are no more bidders, then the program should display the name of the winner and their winning bid. 

# ```
# The winner is Elon with a bid of $55000000000
# ```

# Use your knowledge of Python dictionaries and loops to solve this challenge. 
import os

auction_log = {
	# name : bid
}

def add_auctioneers(name, bid):
	auction_log[name] = bid
add_auctioneers('Nikita',420)


def game_init():
	os.system('clear') #clears the screan
	print(logo)
	print("Welcome to the secret auction program.")

def bid_start():
	name = input("\nWhat is your name?\n")
	bid = int(input("\nWhat's your bid?\n"))
	add_auctioneers(name, bid)

	os.system('clear') #clears the screan

	more_bidders = input("\nAre there any other bidders?\n Type 'yes' or 'no':\n").lower()
	if more_bidders == 'yes':
		bid_start()
	else:
		os.system('clear') #clears the screan
bid_start()

def auct_winner():
	highest_bidder = ''
	high_bid = 0
	for i in auction_log.items():
		name = i[0]
		bid = i[1]
		if bid > high_bid:
			highest_bidder = name
			high_bid = bid
	return print(f"The winner is {highest_bidder} with a bid of ${high_bid}")
auct_winner()






