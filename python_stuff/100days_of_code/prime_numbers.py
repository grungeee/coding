import math

def prime_checker(number):
    count = 0
    for n in range(1, number):
        if number % 1 == 0 and number % n == 0:
            count += 1
    if count > 1:
        print("It's not a prime number.")
    else:
        print("It's a prime number.")

# Teacher example
# def prime_checker(number):
#   is_prime = True
#   for i in range(2, number):
#     if number % i == 0:
#       is_prime = False
#   if is_prime:
#     print("It's a prime number.")
#   else:
#     print("It's not a prime number.")  



for i in range(1, 100):
    prime_checker(i)

#if a number % 1 == 0 and number % number == 0
# and number / 2 == 1
# and number / 3 == 1
# and number / 5 == 1
# and number / 7 == 1

# x^y = prime
# 2^2 = 4 not prime
# 
# test_log = (math.log(numbr,2))
# test_root = 6 ** 1/3
# sqrt_log = (math.log(test_sqrt,2))
# print(test_log)
# print(test_sqrt)
# print(sqrt_log)
# print(numbr % test_log)
# print(bool(numbr % test_log != 0 and numbr % test_sqrt != 0))
# print(bool(numbr % test_log != 0))