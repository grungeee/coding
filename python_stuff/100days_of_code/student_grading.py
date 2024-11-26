# You have access to a database of student_scores in the format of a dictionary. The keys in student_scores are the names of the students and the values are their exam scores.

# Write a program that converts their scores to grades. By the end of your program, you should have a new dictionary called student_grades that should contain student names for keys and their grades for values.

# The final version of the student_grades dictionary will be checked.

# DO NOT modify lines 1-7 to change the existing student_scores dictionary.
# DO NOT write any print statements.

# This is the scoring criteria:
#     Scores 91 - 100: Grade = "Outstanding"
#     Scores 81 - 90: Grade = "Exceeds Expectations"
#     Scores 71 - 80: Grade = "Acceptable"
#     Scores 70 or lower: Grade = "Fail"

# Expected Output

# '{'Harry': 'Exceeds Expectations', 'Ron': 'Acceptable', 'Hermione': 'Outstanding', 'Draco': 'Acceptable', 'Neville': 'Fail'}'





student_scores = {
	"Harry": 81,
	"Ron": 78,
	"Hermione": 99, 
	"Draco": 74,
	"Neville": 62,
	# "Nikita": '',
}

# TODO-1: Create an empty dictionary called student_grades.

# TODO-2: Write your code below to add the grades to student_grades.👇

# This is the scoring criteria:
#     Scores 91 - 100: Grade = "Outstanding"
#     Scores 81 - 90: Grade = "Exceeds Expectations"
#     Scores 71 - 80: Grade = "Acceptable"
#     Scores 70 or lower: Grade = "Fail"
student_grades = {}

for item in student_scores.items():
    name = item[0] #key
    score = item[1] #value
    if score <= 70:
        student_grades[name] = 'Fail'
    elif score <= 80:
        student_grades[name] = 'Acceptable'
    elif score <= 90:
        student_grades[name] = 'Exceeds Expectations'
    else:
        student_grades[name] = 'Outstanding'

print(student_grades)


# for item in student_scores.items():
# 	key = item[0]
# 	value = item[1] 

# 	name = key
# 	score = value #int
# 	if score != '':
# 		print("type of item", type(item[1]))
# 		print("type of score", type(score))
# 		if score <= 100 and score >= 91:
# 			student_grades[name] = 'Outstanding'
# 		elif score <= 90 and score >= 81:
# 			student_grades[name] = 'Exceeds Expectations'
# 		elif score <= 80 and score >= 71:
# 			student_grades[name] = 'Acceptable'
# 		elif score <= 70:
# 			student_grades[name] = 'Fail'
# 	else:
# 		student_grades[name] = 'NO GRADE'

# print(student_grades)
