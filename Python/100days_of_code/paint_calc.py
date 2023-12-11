# an of paint can cover 5 square meters of wall. Given a random height and width of wall, calculate how many cans of paint you'll need to buy.
# Define a function called paint_calc() so the code below works.
# number of cans = (wall height x wall width) ÷ coverage per can.

# e.g. Height = 2, Width = 4, Coverage = 5

# number of cans = (2 \* 4) / 5
#                = 1.6

# But because you can't buy 0.6 of a can of paint, the result should be rounded up to 2 cans.

# 3
# 9

# Example Output
# You'll need 6 cans of paint.



import math
def paint_calc(height, width, cover):
    number_of_cans = int(math.ceil((height * width) / cover))
    print(f"You'll need {number_of_cans} cans of paint.")


test_h = int(3) # Height of wall (m)
test_w = int(9) # Width of wall (m)
coverage = 5
test = paint_calc(height=test_h, width=test_w, cover=coverage)

print(test)