alphabet = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z']

input = 'deez nuts'

def cipher(string, range):
    cipher_range = range
    ciphered = ''
    for l in string:
        if l == ' ':
            ciphered += ' '
        else:
            alpha_id = alphabet.index(l)
            ciphered += alphabet[alpha_id + cipher_range]
    return ciphered


def decipher(string, range):
    decipher_range = range
    deciphered = ''
    for l in string:
        if l == ' ':
            deciphered += ' '
        else:
            alpha_id = alphabet.index(l)
            deciphered += alphabet[alpha_id - decipher_range]
    return deciphered



print(cipher(input,5))
print(decipher(cipher(input,5),5))
