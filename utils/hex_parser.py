def xor_string(input_string):
    xored_chars = []
    for char in input_string:
        xored_value = ord(char) ^ 42
        xored_chars.append('\\x' + format(xored_value, '02x'))
    return ''.join(xored_chars)

# Get input from the user
user_input = input("Enter a string to XOR: ")

# XOR the string and print the result
result = xor_string(user_input)
print("XORed string (hex values):", result)
