import math

# _______________
U = 400
Ustr = 0
I = 0
Istr = 0
R = 0
Rstr = 0
Pst = 0
Pdr = 1200
PF = 1 #geht das?
# _______________

stern = False
dreieck = False

# change inputs to input("Stern: S oder Dreieck: D?")
inputs = "d".upper()
if inputs == 'S':
    stern = True
if inputs == 'D':
    stern = True


if stern == True:
    # I = Istr 
    if U == 0:
        U = Ustr * math.sqrt(3)
    elif Ustr == 0:
        Ustr = U / math.sqrt(3)
    elif I + U != 0:
        P = I * U * math.sqrt(3) #cosPhi in GRAD (nicht RAD)

if dreieck == True:
    Ustr = U
    I = Pdr / (U * math.sqrt(3)) #Idr
    Istr = I / math.sqrt(3)
    Rstr = Ustr / Istr
    P = I * U * math.sqrt(3) #cosPhi in GRAD (nicht RAD)
    # if U == 0:
    #     U = Ustr * math.sqrt(3)
    # elif Ustr == 0:
    #     Ustr = U / math.sqrt(3)
    # elif I + U != 0:
    #     P = I * U * math.sqrt(3) #cosPhi in GRAD (nicht RAD)

print('P: ', P) 
print('U: ', U) 
print('Ustr: ', Ustr) 
print('I: ', I) 
print('Istr: ', Istr) 