from morse_code_dictionary import morse_code_dictionary

# config
dictionary = morse_code_dictionary

short_sig = chr(0x25CF) # Printing '●' (U+25CF)
long_sig = chr(0x25AC) # Printing '▬' (U+25AC)

input = "abc".upper()
char = input[0] # FIRS char -> "A"
print(dictionary[char]) # corresponding value[B] to key[A] => "A":[B] -> [B]
# print(dictionary[char])

for char in input:
    len = len(input)
    value = dictionary[char] #[value] => (char == key):[value]
    # OUTPUT (1st itr): ['short', 'long', 'pause']
    # FIX: 'pause' last???
    # Q: do i even need pause?



dict_items = dictionary.items() #tuple_from_dict
# print(type(dict_items))
# print(tuple_from_dict)

for item in dict_items: # dict items iterator
    key = item[0]
    value = item[1]
    # print(key,value)
    break




