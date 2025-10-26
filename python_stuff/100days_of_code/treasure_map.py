line1 = ["⬜️","️⬜️","️⬜️"]
line2 = ["⬜️","⬜️","️⬜️"]
line3 = ["⬜️️","⬜️️","⬜️️"]
map = [line1, line2, line3]
print("Hiding your treasure! X marks the spot.")
position = "c2" # Where do you want to put the treasure?

#NOTE: line1 A1, B1, C1
#NOTE: line2 A2, B2, C2
#NOTE: line3 A3, B3, C3


if position[1] == "1":
    if position.upper()[0] == "A":
       line1[0] = 'X'
    elif position.upper()[0] == "B":
       line1[1] = 'X'
    elif position.upper()[0] == "C":
       line1[3] = 'X'
elif position[1] == "2":
    if position.upper()[0] == "A":
       line2[0] = 'X'
    elif position.upper()[0] == "B":
       line2[1] = 'X'
    elif position.upper()[0] == "C":
       line2[2] = 'X'
elif position[1] == "3":
    if position.upper()[0] == "A":
       line3[0] = 'X'
    elif position.upper()[0] == "B":
       line3[1] = 'X'
    elif position.upper()[0] == "C":
       line3[2] = 'X'


print(f"{line1}\n{line2}\n{line3}")

