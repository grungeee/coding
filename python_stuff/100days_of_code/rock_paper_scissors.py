import random

rock = '''
    _______
---'   ____)
      (_____)
      (_____)
      (____)
---.__(___)
'''

paper = '''
    _______
---'   ____)____
          ______)
          _______)
         _______)
---.__________)
'''

scissors = '''
    _______
---'   ____)____
          ______)
       __________)
      (____)
---.__(___)
'''
options = [rock, paper, scissors]

user_choice = int(input("What do you choose? Type 0 for Rock, 1 for Paper or 2 for Scissors.\n"))
cpu_choice = random.randint(0, 2)


# for i in range(0,2):
#     if user_choice == i:
#         options[random.randint(0, 2)]

# 0 1
# 1 2
# 2 0

if user_choice == cpu_choice:
    print(f"It's a draw... You picked: {options[user_choice]} \n CPU picked: {options[cpu_choice]}")
elif user_choice == 2 and cpu_choice != 0:
    print(f'You won! You picked: {options[user_choice]} \n CPU picked: {options[cpu_choice]}')
elif user_choice > cpu_choice and cpu_choice != 0:
   print(f'You won! You picked: {options[user_choice]} \n CPU picked: {options[cpu_choice]}')
else:
    print(f'You lost, You picked: {options[user_choice]} \n CPU picked: {options[cpu_choice]}')
    
        