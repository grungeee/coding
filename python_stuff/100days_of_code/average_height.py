# Input a Python list of student heights
# Important You should not use the sum() or len() functions in your answer. You should try to replicate their functionality using what you have learnt about for loops.

student_heights = '180 124 165 173 189 169 146'.split()
for n in range(0, len(student_heights)):
  student_heights[n] = int(student_heights[n])
# 🚨 Don't change the code above 👆

# index_student = student_heights.index(height)
# print(index_student)

# total_height = sum(student_heights)
# num_of_students = len(student_heights)
# av_height = round(total_height / num_of_students)

# print(f"total height = {total_height}")
# print(f"number of students = {num_of_students}")
# print(f"average height = {av_height}")
# _____________________________

total_height = 0
num_of_students = 0

for height in student_heights:
  total_height += height
  num_of_students += 1

av_height = round(total_height / num_of_students)

print(f"total height = {total_height}")
print(f"number of students = {num_of_students}")
print(f"average height = {av_height}")